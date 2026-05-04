/**
 * Copyright 2026 AntGroup CO., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#include "proxy/proxy_server.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "bolt/pack_stream.h"
#include "bolt/worker_pool.h"
#include "common/exceptions.h"
#include "common/logger.h"
#include "proxy/backend_session.h"

namespace proxy {
namespace {

constexpr const char* kShardKeyParam = "_shard_key_";
constexpr const char* kArgumentError =
    "Neo.ClientError.Statement.ArgumentError";
constexpr const char* kRequestError = "Neo.ClientError.Request.Invalid";
constexpr const char* kNetworkError =
    "Neo.TransientError.Network.CommunicationError";

class ProxyClientError : public std::runtime_error {
 public:
  ProxyClientError(std::string code, std::string message)
      : std::runtime_error(message), code_(std::move(code)) {}

  const std::string& code() const { return code_; }

 private:
  std::string code_;
};

const char* BoltMsgName(bolt::BoltMsg msg) {
  switch (msg) {
    case bolt::BoltMsg::Run:
      return "RUN";
    case bolt::BoltMsg::PullN:
      return "PULL";
    case bolt::BoltMsg::DiscardN:
      return "DISCARD";
    case bolt::BoltMsg::Reset:
      return "RESET";
    case bolt::BoltMsg::Hello:
      return "HELLO";
    case bolt::BoltMsg::Goodbye:
      return "GOODBYE";
    case bolt::BoltMsg::Begin:
      return "BEGIN";
    case bolt::BoltMsg::Commit:
      return "COMMIT";
    case bolt::BoltMsg::Rollback:
      return "ROLLBACK";
    case bolt::BoltMsg::Route:
      return "ROUTE";
    default:
      return "UNKNOWN";
  }
}

struct ProxyMessage {
  bolt::BoltMsg type;
  std::vector<std::any> fields;
};

enum class ProxySessionState { READY = 0, STREAMING, FAILED, DEFUNCT };

class BackendSessionPool;

struct ActiveBackend {
  BackendEndpoint endpoint;
  std::shared_ptr<BoltBackendSession> backend;
};

struct ProxySession {
  ~ProxySession();

  void RequestInterrupt() { remaining_interrupts.fetch_add(1); }

  bool HasInterrupt() const { return remaining_interrupts.load() != 0; }

  bool ConsumeInterrupt() {
    size_t current = remaining_interrupts.load();
    while (current != 0) {
      if (remaining_interrupts.compare_exchange_weak(current, current - 1)) {
        return current == 1;
      }
    }
    return true;
  }

  std::unordered_map<std::string, std::any> hello_meta;
  std::shared_ptr<BackendSessionPool> backend_pool;
  ProxySessionState state = ProxySessionState::READY;
  std::optional<ActiveBackend> active_backend;
  std::mutex backend_mutex;
  std::atomic<size_t> remaining_interrupts = 0;
};

struct ProxySessionContext {
  explicit ProxySessionContext(bolt::BoltWorkerPool::Strand strand)
      : session(std::make_shared<ProxySession>()), strand(std::move(strand)) {}

  std::shared_ptr<ProxySession> session;
  bolt::BoltWorkerPool::Strand strand;
};

const char* ProxySessionStateName(ProxySessionState state) {
  switch (state) {
    case ProxySessionState::READY:
      return "READY";
    case ProxySessionState::STREAMING:
      return "STREAMING";
    case ProxySessionState::FAILED:
      return "FAILED";
    case ProxySessionState::DEFUNCT:
      return "DEFUNCT";
  }
  return "UNKNOWN";
}

bool IsSessionInterrupted(ProxySession* session) {
  return session->HasInterrupt();
}

bool ConsumeSessionInterrupt(ProxySession* session) {
  return session->ConsumeInterrupt();
}

class LeaderCache {
 public:
  std::optional<BackendEndpoint> Get(const std::string& graph_name) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto iter = leaders_.find(graph_name);
    if (iter == leaders_.end()) {
      return std::nullopt;
    }
    return iter->second;
  }

  void Put(const std::string& graph_name, BackendEndpoint endpoint) {
    std::lock_guard<std::mutex> guard(mutex_);
    leaders_[graph_name] = std::move(endpoint);
  }

  void InvalidateIf(const std::string& graph_name,
                    const BackendEndpoint& endpoint) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto iter = leaders_.find(graph_name);
    if (iter != leaders_.end() && iter->second.name == endpoint.name) {
      leaders_.erase(iter);
    }
  }

 private:
  std::mutex mutex_;
  std::unordered_map<std::string, BackendEndpoint> leaders_;
};

class BackendPoolExhausted : public std::runtime_error {
 public:
  explicit BackendPoolExhausted(const std::string& message)
      : std::runtime_error(message) {}
};

class BackendSessionPool {
 public:
  BackendSessionPool(size_t max_connections_per_backend,
                     std::chrono::milliseconds borrow_timeout)
      : max_connections_per_backend_(max_connections_per_backend),
        borrow_timeout_(borrow_timeout) {}

  ~BackendSessionPool() { CloseIdleSessions(); }

  std::shared_ptr<BoltBackendSession> Borrow(
      const BackendEndpoint& endpoint,
      const std::unordered_map<std::string, std::any>& hello_meta) {
    const auto deadline = std::chrono::steady_clock::now() + borrow_timeout_;
    const auto& key = endpoint.name;
    while (true) {
      {
        std::unique_lock<std::mutex> lock(mutex_);
        auto& bucket = buckets_[key];
        if (!bucket.idle.empty()) {
          auto backend = std::move(bucket.idle.front());
          bucket.idle.pop_front();
          return backend;
        }
        if (max_connections_per_backend_ == 0 ||
            bucket.total < max_connections_per_backend_) {
          ++bucket.total;
          lock.unlock();
          try {
            return std::make_shared<BoltBackendSession>(endpoint, hello_meta);
          } catch (...) {
            ReleaseSlot(key);
            throw;
          }
        }
        if (borrow_timeout_.count() == 0 ||
            condition_.wait_until(lock, deadline) == std::cv_status::timeout) {
          throw BackendPoolExhausted(fmt::format(
              "backend connection pool exhausted for {}", endpoint.name));
        }
      }
    }
  }

  void Return(const BackendEndpoint& endpoint,
              std::shared_ptr<BoltBackendSession> backend) {
    if (!backend) {
      return;
    }
    {
      std::lock_guard<std::mutex> guard(mutex_);
      auto iter = buckets_.find(endpoint.name);
      if (iter == buckets_.end()) {
        backend->Close();
        return;
      }
      iter->second.idle.emplace_back(std::move(backend));
    }
    condition_.notify_one();
  }

  void Drop(const BackendEndpoint& endpoint,
            std::shared_ptr<BoltBackendSession> backend) {
    if (backend) {
      backend->Close();
    }
    ReleaseSlot(endpoint.name);
  }

 private:
  struct Bucket {
    std::deque<std::shared_ptr<BoltBackendSession>> idle;
    size_t total = 0;
  };

  void ReleaseSlot(const std::string& key) {
    {
      std::lock_guard<std::mutex> guard(mutex_);
      auto iter = buckets_.find(key);
      if (iter != buckets_.end() && iter->second.total > 0) {
        --iter->second.total;
        if (iter->second.total == 0 && iter->second.idle.empty()) {
          buckets_.erase(iter);
        }
      }
    }
    condition_.notify_one();
  }

  void CloseIdleSessions() {
    std::vector<std::shared_ptr<BoltBackendSession>> idle_sessions;
    {
      std::lock_guard<std::mutex> guard(mutex_);
      for (auto& pair : buckets_) {
        auto& bucket = pair.second;
        while (!bucket.idle.empty()) {
          idle_sessions.emplace_back(std::move(bucket.idle.front()));
          bucket.idle.pop_front();
          if (bucket.total > 0) {
            --bucket.total;
          }
        }
      }
      buckets_.clear();
    }
    for (const auto& backend : idle_sessions) {
      backend->Close();
    }
  }

  size_t max_connections_per_backend_;
  std::chrono::milliseconds borrow_timeout_;
  std::mutex mutex_;
  std::condition_variable condition_;
  std::unordered_map<std::string, Bucket> buckets_;
};

struct ProxyContext {
  ProxyContext(ShardMap shard_map, size_t backend_max_connections_per_backend,
               std::chrono::milliseconds backend_borrow_timeout)
      : shard_map(std::make_shared<ShardMap>(std::move(shard_map))),
        leader_cache(std::make_shared<LeaderCache>()),
        backend_pool(std::make_shared<BackendSessionPool>(
            backend_max_connections_per_backend, backend_borrow_timeout)) {}

  std::shared_ptr<const ShardMap> shard_map;
  std::shared_ptr<LeaderCache> leader_cache;
  std::shared_ptr<BackendSessionPool> backend_pool;
};

std::shared_ptr<ProxySessionContext> GetSessionContext(
    bolt::BoltConnection& conn) {
  auto ctx = conn.GetContextShared();
  if (!ctx) {
    return {};
  }
  return std::static_pointer_cast<ProxySessionContext>(ctx);
}

std::shared_ptr<ProxySession> GetSession(bolt::BoltConnection& conn) {
  auto ctx = GetSessionContext(conn);
  if (!ctx) {
    return {};
  }
  return ctx->session;
}

std::string AnyToShardKey(const std::any& value) {
  if (!value.has_value()) {
    throw ProxyClientError(kArgumentError,
                           "routing parameter _shard_key_ should not be null");
  }
  const auto& type = value.type();
  if (type == typeid(std::string)) {
    auto shard_key = std::any_cast<std::string>(value);
    if (shard_key.empty()) {
      throw ProxyClientError(
          kArgumentError, "routing parameter _shard_key_ should not be empty");
    }
    return shard_key;
  }
  if (type == typeid(int64_t)) {
    return std::to_string(std::any_cast<int64_t>(value));
  }
  if (type == typeid(bool)) {
    return std::any_cast<bool>(value) ? "true" : "false";
  }
  if (type == typeid(double)) {
    return fmt::format("{:.17g}", std::any_cast<double>(value));
  }
  throw ProxyClientError(
      kArgumentError,
      "routing parameter _shard_key_ type should be String, Integer, Bool, or "
      "Float");
}

std::unordered_map<std::string, std::any> CastMapField(const std::any& value,
                                                       const char* field_name) {
  auto* map = std::any_cast<std::unordered_map<std::string, std::any>>(&value);
  if (map == nullptr) {
    throw ProxyClientError(kArgumentError,
                           fmt::format("{} type should be Map", field_name));
  }
  return *map;
}

std::string CastStringField(const std::any& value, const char* field_name) {
  auto* str = std::any_cast<std::string>(&value);
  if (str == nullptr) {
    throw ProxyClientError(kArgumentError,
                           fmt::format("{} type should be String", field_name));
  }
  return *str;
}

int64_t ExtractPullN(const std::vector<std::any>& fields) {
  if (fields.size() != 1) {
    throw ProxyClientError(kRequestError,
                           "PULL/DISCARD fields size should be 1");
  }
  auto metadata = CastMapField(fields[0], "PULL/DISCARD metadata");
  auto iter = metadata.find("n");
  if (iter == metadata.end()) {
    throw ProxyClientError(kRequestError,
                           "PULL/DISCARD metadata should contain n");
  }
  auto* n = std::any_cast<int64_t>(&iter->second);
  if (n == nullptr) {
    throw ProxyClientError(kRequestError,
                           "PULL/DISCARD n type should be Integer");
  }
  if (*n == 0) {
    throw ProxyClientError(kRequestError, "PULL/DISCARD n should not be 0");
  }
  return *n;
}

bool IsExplicitTransactionRequest(bolt::BoltMsg type) {
  return type == bolt::BoltMsg::Begin || type == bolt::BoltMsg::Commit ||
         type == bolt::BoltMsg::Rollback;
}

bool IsSessionRequest(bolt::BoltMsg type) {
  return type == bolt::BoltMsg::Run || type == bolt::BoltMsg::PullN ||
         type == bolt::BoltMsg::DiscardN || type == bolt::BoltMsg::Route ||
         IsExplicitTransactionRequest(type);
}

bool IsResetMessage(bolt::BoltMsg type) { return type == bolt::BoltMsg::Reset; }

void SendFailure(const std::shared_ptr<bolt::BoltConnection>& conn,
                 const std::string& code, const std::string& message) {
  bolt::PackStream ps;
  ps.AppendFailure({{"code", code}, {"message", message}});
  conn->PostResponse(std::move(ps.MutableBuffer()));
}

void SendSuccess(const std::shared_ptr<bolt::BoltConnection>& conn) {
  bolt::PackStream ps;
  ps.AppendSuccess();
  conn->PostResponse(std::move(ps.MutableBuffer()));
}

void SendIgnored(const std::shared_ptr<bolt::BoltConnection>& conn) {
  bolt::PackStream ps;
  ps.AppendIgnored();
  conn->PostResponse(std::move(ps.MutableBuffer()));
}

void CloseProtocolError(const std::shared_ptr<bolt::BoltConnection>& conn,
                        ProxySession* session, bolt::BoltMsg type) {
  LOG_ERROR("unexpected {} in {} proxy session state, close the connection",
            BoltMsgName(type), ProxySessionStateName(session->state));
  session->state = ProxySessionState::DEFUNCT;
  conn->Close();
}

void ForwardMessages(const std::shared_ptr<bolt::BoltConnection>& conn,
                     const std::vector<BackendMessage>& messages) {
  for (const auto& message : messages) {
    conn->PostResponse(message.raw);
  }
}

class ClientResponseBatcher {
 public:
  ClientResponseBatcher(std::shared_ptr<bolt::BoltConnection> conn,
                        ProxySession* session)
      : conn_(std::move(conn)), session_(session) {}

  bool Forward(const BackendMessage& message) {
    if (conn_->has_closed() || IsSessionInterrupted(session_)) {
      return false;
    }
    buffer_.append(message.raw);
    if (buffer_.size() >= kFlushBytes) {
      Flush();
    }
    return !conn_->has_closed() && !IsSessionInterrupted(session_);
  }

  void Flush() {
    if (buffer_.empty() || conn_->has_closed()) {
      buffer_.clear();
      return;
    }
    conn_->PostResponse(std::move(buffer_));
    buffer_.clear();
  }

 private:
  static constexpr size_t kFlushBytes = 64 * 1024;

  std::shared_ptr<bolt::BoltConnection> conn_;
  ProxySession* session_;
  std::string buffer_;
};

BackendMessage LastMessage(const std::vector<BackendMessage>& messages) {
  if (messages.empty()) {
    throw std::runtime_error("backend returned no Bolt message");
  }
  return messages.back();
}

void SetActiveBackend(ProxySession* session, BackendEndpoint endpoint,
                      std::shared_ptr<BoltBackendSession> backend) {
  std::lock_guard<std::mutex> guard(session->backend_mutex);
  session->active_backend = ActiveBackend{.endpoint = std::move(endpoint),
                                          .backend = std::move(backend)};
}

std::shared_ptr<BoltBackendSession> ActiveBackendSession(
    ProxySession* session) {
  std::lock_guard<std::mutex> guard(session->backend_mutex);
  if (!session->active_backend.has_value()) {
    return {};
  }
  return session->active_backend->backend;
}

std::optional<ActiveBackend> TakeActiveBackend(ProxySession* session) {
  std::lock_guard<std::mutex> guard(session->backend_mutex);
  if (!session->active_backend.has_value()) {
    return std::nullopt;
  }
  auto active = std::move(session->active_backend);
  session->active_backend.reset();
  return active;
}

std::optional<ActiveBackend> TakeActiveBackendIf(
    ProxySession* session, const BackendEndpoint& endpoint) {
  if (endpoint.name.empty()) {
    return std::nullopt;
  }
  std::lock_guard<std::mutex> guard(session->backend_mutex);
  if (!session->active_backend.has_value() ||
      session->active_backend->endpoint.name != endpoint.name) {
    return std::nullopt;
  }
  auto active = std::move(session->active_backend);
  session->active_backend.reset();
  return active;
}

std::shared_ptr<BoltBackendSession> BorrowActiveBackend(
    const std::shared_ptr<BackendSessionPool>& pool, ProxySession* session,
    const BackendEndpoint& endpoint) {
  auto backend = pool->Borrow(endpoint, session->hello_meta);
  SetActiveBackend(session, endpoint, backend);
  return backend;
}

void ReturnActiveBackend(const std::shared_ptr<BackendSessionPool>& pool,
                         ProxySession* session) {
  auto active = TakeActiveBackend(session);
  if (active.has_value()) {
    pool->Return(active->endpoint, std::move(active->backend));
  }
}

void DropActiveBackend(const std::shared_ptr<BackendSessionPool>& pool,
                       ProxySession* session) {
  auto active = TakeActiveBackend(session);
  if (active.has_value()) {
    pool->Drop(active->endpoint, std::move(active->backend));
  }
}

void DropActiveBackendIf(const std::shared_ptr<BackendSessionPool>& pool,
                         ProxySession* session,
                         const BackendEndpoint& endpoint) {
  auto active = TakeActiveBackendIf(session, endpoint);
  if (active.has_value()) {
    pool->Drop(active->endpoint, std::move(active->backend));
  }
}

ProxySession::~ProxySession() {
  auto active = TakeActiveBackend(this);
  if (!active.has_value()) {
    return;
  }
  if (backend_pool) {
    backend_pool->Drop(active->endpoint, std::move(active->backend));
  } else if (active->backend) {
    active->backend->Close();
  }
}

void FailSession(const std::shared_ptr<BackendSessionPool>& pool,
                 const std::shared_ptr<bolt::BoltConnection>& conn,
                 ProxySession* session, const std::string& code,
                 const std::string& message) {
  DropActiveBackend(pool, session);
  SendFailure(conn, code, message);
  session->state = ProxySessionState::FAILED;
}

void RequestSessionInterrupt(const std::shared_ptr<ProxySession>& session) {
  session->RequestInterrupt();
  auto backend = ActiveBackendSession(session.get());
  if (backend) {
    backend->Cancel();
  }
}

BackendEndpoint ResolveConfiguredEndpoint(const ShardReplicaGroup& group,
                                          const RaftNodeEndpoint& discovered) {
  for (const auto& replica : group.replicas) {
    if (replica.node_id == discovered.node_id) {
      return replica;
    }
  }

  BackendEndpoint endpoint;
  endpoint.node_id = discovered.node_id;
  endpoint.host = discovered.host;
  endpoint.port = discovered.port;
  endpoint.raft_port = discovered.raft_port;
  endpoint.name = std::to_string(endpoint.node_id) + "@" + endpoint.host + ":" +
                  std::to_string(endpoint.port);
  return endpoint;
}

BackendEndpoint DiscoverLeader(
    const ShardRoute& route,
    const std::unordered_map<std::string, std::any>& hello_meta,
    const std::shared_ptr<BackendSessionPool>& pool) {
  if (route.replica_group == nullptr || route.replica_group->replicas.empty()) {
    throw std::runtime_error(
        fmt::format("proxy shard {} has no raft replicas", route.shard_id));
  }

  std::string last_error;
  bool has_pool_exhaustion = false;
  bool has_non_pool_error = false;
  for (const auto& replica : route.replica_group->replicas) {
    std::shared_ptr<BoltBackendSession> backend;
    try {
      backend = pool->Borrow(replica, hello_meta);
      auto node_infos = backend->FetchRaftNodeInfos(route.graph_name);
      pool->Return(replica, std::move(backend));
      for (const auto& node_info : node_infos) {
        if (!node_info.is_leader) {
          continue;
        }
        auto leader =
            ResolveConfiguredEndpoint(*route.replica_group, node_info);
        LOG_INFO("proxy discovered leader for graph {}: {}:{} node {}",
                 route.graph_name, leader.host, leader.port, leader.node_id);
        return leader;
      }
      last_error = "raft leader is not known";
      has_non_pool_error = true;
    } catch (const BackendPoolExhausted& e) {
      has_pool_exhaustion = true;
      last_error = e.what();
      LOG_WARN("failed to discover leader for graph {} from {}:{}: {}",
               route.graph_name, replica.host, replica.port, e.what());
    } catch (const std::exception& e) {
      if (backend) {
        pool->Drop(replica, std::move(backend));
      }
      has_non_pool_error = true;
      last_error = e.what();
      LOG_WARN("failed to discover leader for graph {} from {}:{}: {}",
               route.graph_name, replica.host, replica.port, e.what());
    }
  }
  if (has_pool_exhaustion && !has_non_pool_error) {
    throw BackendPoolExhausted(
        fmt::format("failed to discover raft leader for graph {}: {}",
                    route.graph_name, last_error));
  }
  throw std::runtime_error(
      fmt::format("failed to discover raft leader for graph {}: {}",
                  route.graph_name, last_error));
}

bool IsNotLeaderFailure(const std::vector<BackendMessage>& messages) {
  if (messages.empty() || messages.back().tag != bolt::BoltMsg::Failure) {
    return false;
  }
  const auto& failure = messages.back();
  if (failure.failure_code == "Neo.ClientError.Cluster.NotALeader" ||
      failure.failure_code ==
          "Neo.ClientError.General.ForbiddenOnReadOnlyDatabase") {
    return true;
  }
  return failure.failure_message.find("not leader") != std::string::npos;
}

void ProcessRun(const std::shared_ptr<ProxyContext>& context,
                const std::shared_ptr<bolt::BoltConnection>& conn,
                ProxySession* session, const std::vector<std::any>& fields) {
  auto& backend_pool = context->backend_pool;
  if (fields.size() < 3) {
    FailSession(backend_pool, conn, session, kArgumentError,
                "RUN requires cypher, parameters, and metadata fields");
    return;
  }
  if (fields.size() != 3) {
    FailSession(backend_pool, conn, session, kArgumentError,
                "RUN fields size should be 3");
    return;
  }
  if (IsSessionInterrupted(session)) {
    return;
  }

  auto cypher = CastStringField(fields[0], "RUN cypher");
  auto params = CastMapField(fields[1], "RUN parameters");
  auto extra = CastMapField(fields[2], "RUN metadata");

  auto shard_key_iter = params.find(kShardKeyParam);
  if (shard_key_iter == params.end()) {
    FailSession(backend_pool, conn, session, kArgumentError,
                "missing required routing parameter _shard_key_");
    return;
  }
  auto shard_key = AnyToShardKey(shard_key_iter->second);
  params.erase(shard_key_iter);

  const auto& shard_map = *context->shard_map;
  auto& leader_cache = context->leader_cache;
  std::string logical_graph = shard_map.logical_graph();
  auto db_iter = extra.find("db");
  if (db_iter != extra.end() && db_iter->second.has_value()) {
    logical_graph = CastStringField(db_iter->second, "RUN metadata.db");
  }

  auto route = shard_map.Route(logical_graph, shard_key);
  extra["db"] = route.graph_name;

  bolt::PackStream ps;
  ps.AppendRun(cypher, params, extra);
  std::vector<BackendMessage> messages;
  BackendEndpoint selected_endpoint;
  std::string last_error;
  for (int attempt = 0; attempt < 2; ++attempt) {
    selected_endpoint = {};
    try {
      if (attempt == 0) {
        auto cached_leader = leader_cache->Get(route.graph_name);
        selected_endpoint =
            cached_leader.has_value()
                ? *cached_leader
                : DiscoverLeader(route, session->hello_meta, backend_pool);
      } else {
        selected_endpoint =
            DiscoverLeader(route, session->hello_meta, backend_pool);
      }

      auto backend =
          BorrowActiveBackend(backend_pool, session, selected_endpoint);
      if (IsSessionInterrupted(session)) {
        throw BackendOperationCancelled("proxy RUN");
      }
      messages = backend->SendAndReadUntilTerminal(ps.ConstBuffer());
      if (!IsNotLeaderFailure(messages)) {
        leader_cache->Put(route.graph_name, selected_endpoint);
        break;
      }

      last_error = messages.back().failure_message;
      DropActiveBackendIf(backend_pool, session, selected_endpoint);
      leader_cache->InvalidateIf(route.graph_name, selected_endpoint);
      LOG_WARN("proxy stale leader for graph {} backend {}:{}: {}",
               route.graph_name, selected_endpoint.host, selected_endpoint.port,
               last_error);
      messages.clear();
    } catch (const BackendPoolExhausted&) {
      DropActiveBackendIf(backend_pool, session, selected_endpoint);
      throw;
    } catch (const BackendOperationCancelled&) {
      DropActiveBackendIf(backend_pool, session, selected_endpoint);
      throw;
    } catch (const std::exception& e) {
      last_error = e.what();
      DropActiveBackendIf(backend_pool, session, selected_endpoint);
      if (!selected_endpoint.name.empty()) {
        leader_cache->InvalidateIf(route.graph_name, selected_endpoint);
      }
      LOG_WARN("proxy backend attempt failed for graph {} backend {}:{}: {}",
               route.graph_name, selected_endpoint.host, selected_endpoint.port,
               e.what());
      messages.clear();
    }
    if (IsSessionInterrupted(session)) {
      throw BackendOperationCancelled("proxy RUN");
    }
  }
  if (messages.empty()) {
    DropActiveBackendIf(backend_pool, session, selected_endpoint);
    if (IsSessionInterrupted(session)) {
      return;
    }
    FailSession(backend_pool, conn, session, kNetworkError,
                fmt::format("failed to route graph {} shard {}: {}",
                            route.graph_name, route.shard_id, last_error));
    return;
  }
  if (IsSessionInterrupted(session)) {
    return;
  }
  auto last = LastMessage(messages);
  ForwardMessages(conn, messages);

  if (last.tag == bolt::BoltMsg::Success) {
    session->state = ProxySessionState::STREAMING;
  } else {
    DropActiveBackendIf(backend_pool, session, selected_endpoint);
    session->state = ProxySessionState::FAILED;
  }
  LOG_DEBUG("proxy routed shard_key [{}] to shard {} graph {} backend {}:{}",
            shard_key, route.shard_id, route.graph_name, selected_endpoint.host,
            selected_endpoint.port);
}

void ProcessPullOrDiscard(const std::shared_ptr<BackendSessionPool>& pool,
                          const std::shared_ptr<bolt::BoltConnection>& conn,
                          ProxySession* session, bolt::BoltMsg type,
                          const std::vector<std::any>& fields) {
  auto backend = ActiveBackendSession(session);
  if (!backend) {
    throw std::runtime_error("active backend session is missing");
  }
  if (IsSessionInterrupted(session)) {
    return;
  }

  int64_t n = ExtractPullN(fields);

  bolt::PackStream ps;
  if (type == bolt::BoltMsg::PullN) {
    ps.AppendPullN(n);
  } else {
    ps.AppendDiscardN(n);
  }
  ClientResponseBatcher batcher(conn, session);
  auto last = backend->SendAndForwardUntilTerminal(
      ps.ConstBuffer(), [&batcher](const BackendMessage& message) {
        return batcher.Forward(message);
      });
  batcher.Flush();

  if (IsSessionInterrupted(session)) {
    return;
  }
  if (last.tag == bolt::BoltMsg::Success && !last.success_has_more) {
    ReturnActiveBackend(pool, session);
    session->state = ProxySessionState::READY;
  } else if (last.tag == bolt::BoltMsg::Success) {
    session->state = ProxySessionState::STREAMING;
  } else if (last.tag != bolt::BoltMsg::Success) {
    DropActiveBackend(pool, session);
    session->state = ProxySessionState::FAILED;
  }
}

bool HandleInterruptedSessionMessage(
    const std::shared_ptr<BackendSessionPool>& pool,
    const std::shared_ptr<bolt::BoltConnection>& conn, ProxySession* session,
    bolt::BoltMsg type) {
  if (!IsSessionInterrupted(session)) {
    return false;
  }
  DropActiveBackend(pool, session);
  if (IsResetMessage(type)) {
    if (ConsumeSessionInterrupt(session)) {
      session->state = ProxySessionState::READY;
      SendSuccess(conn);
    } else {
      SendIgnored(conn);
    }
  } else {
    SendIgnored(conn);
  }
  return true;
}

void ProcessRecoverableState(const std::shared_ptr<bolt::BoltConnection>& conn,
                             ProxySession* session, bolt::BoltMsg type) {
  if (IsSessionRequest(type)) {
    SendIgnored(conn);
  } else {
    CloseProtocolError(conn, session, type);
  }
}

void ProcessReadyState(const std::shared_ptr<ProxyContext>& context,
                       const std::shared_ptr<bolt::BoltConnection>& conn,
                       ProxySession* session, bolt::BoltMsg type,
                       const std::vector<std::any>& fields) {
  if (IsExplicitTransactionRequest(type)) {
    FailSession(context->backend_pool, conn, session, kRequestError,
                "explicit transactions are not supported by lgraph_proxy");
  } else if (type == bolt::BoltMsg::Route) {
    FailSession(context->backend_pool, conn, session, kRequestError,
                "routing is not supported by lgraph_proxy");
  } else if (type == bolt::BoltMsg::Run) {
    ProcessRun(context, conn, session, fields);
  } else {
    CloseProtocolError(conn, session, type);
  }
}

void ProcessStreamingState(const std::shared_ptr<ProxyContext>& context,
                           const std::shared_ptr<bolt::BoltConnection>& conn,
                           ProxySession* session, bolt::BoltMsg type,
                           const std::vector<std::any>& fields) {
  if (type == bolt::BoltMsg::PullN || type == bolt::BoltMsg::DiscardN) {
    ProcessPullOrDiscard(context->backend_pool, conn, session, type, fields);
  } else {
    CloseProtocolError(conn, session, type);
  }
}

void ProcessProxyMessage(const std::shared_ptr<ProxyContext>& context,
                         const std::shared_ptr<bolt::BoltConnection>& conn,
                         ProxySession* session, ProxyMessage message) {
  try {
    if (HandleInterruptedSessionMessage(context->backend_pool, conn, session,
                                        message.type)) {
      return;
    }

    switch (session->state) {
      case ProxySessionState::FAILED:
        ProcessRecoverableState(conn, session, message.type);
        break;
      case ProxySessionState::READY:
        ProcessReadyState(context, conn, session, message.type, message.fields);
        break;
      case ProxySessionState::STREAMING:
        ProcessStreamingState(context, conn, session, message.type,
                              message.fields);
        break;
      case ProxySessionState::DEFUNCT:
        CloseProtocolError(conn, session, message.type);
        break;
    }
  } catch (const BackendOperationCancelled& e) {
    LOG_INFO("proxy message cancelled: {}", e.what());
    if (conn->has_closed()) {
      DropActiveBackend(context->backend_pool, session);
      return;
    }
    if (IsSessionInterrupted(session)) {
      return;
    }
    FailSession(context->backend_pool, conn, session, kNetworkError, e.what());
  } catch (const ProxyClientError& e) {
    LOG_WARN("proxy client message failed: {}", e.what());
    if (!conn->has_closed()) {
      FailSession(context->backend_pool, conn, session, e.code(), e.what());
    }
  } catch (const LgraphException& e) {
    LOG_WARN("proxy client message failed: {}", e.what());
    if (!conn->has_closed()) {
      FailSession(context->backend_pool, conn, session, kArgumentError,
                  e.msg());
    }
  } catch (const std::exception& e) {
    LOG_WARN("proxy message failed: {}", e.what());
    if (!conn->has_closed()) {
      FailSession(context->backend_pool, conn, session, kNetworkError,
                  e.what());
    }
  }
}

bool EnqueueSessionMessage(const std::shared_ptr<ProxyContext>& context,
                           const std::shared_ptr<bolt::BoltWorkerPool>& pool,
                           bolt::BoltConnection& conn,
                           std::shared_ptr<ProxySessionContext> session_context,
                           ProxyMessage message) {
  if (!pool->Post(session_context->strand,
                  [context, conn = conn.shared_from_this(), session_context,
                   message = std::move(message)]() mutable {
                    auto session = session_context->session;
                    if (!conn->has_closed()) {
                      ProcessProxyMessage(context, conn, session.get(),
                                          std::move(message));
                    }
                    if (conn->has_closed()) {
                      RequestSessionInterrupt(session);
                      DropActiveBackend(context->backend_pool, session.get());
                    }
                  })) {
    RequestSessionInterrupt(session_context->session);
    conn.Close();
    return false;
  }
  return true;
}

void HandleHello(const std::shared_ptr<BackendSessionPool>& backend_pool,
                 const std::shared_ptr<bolt::BoltWorkerPool>& worker_pool,
                 bolt::BoltConnection& conn, std::vector<std::any> fields) {
  if (fields.size() != 1) {
    bolt::PackStream ps;
    ps.AppendFailure({{"code", kRequestError},
                      {"message", "HELLO fields size should be 1"}});
    conn.Respond(std::move(ps.MutableBuffer()));
    conn.Close();
    return;
  }
  auto hello_meta = CastMapField(fields[0], "HELLO metadata");
  auto session_context =
      std::make_shared<ProxySessionContext>(worker_pool->MakeStrand());
  auto session = session_context->session;
  session->hello_meta = hello_meta;
  session->backend_pool = backend_pool;
  conn.SetContext(session_context);

  std::unordered_map<std::string, std::any> meta;
  meta["connection_id"] = std::string("proxy") + std::to_string(conn.conn_id());
  meta["server"] = "Neo4j/tugraph-db-proxy";

  auto patch_iter = hello_meta.find("patch_bolt");
  if (patch_iter != hello_meta.end()) {
    auto* patches = std::any_cast<std::vector<std::any>>(&patch_iter->second);
    if (patches != nullptr && patches->size() == 1) {
      auto* patch = std::any_cast<std::string>(&(*patches)[0]);
      if (patch != nullptr && *patch == "utc") {
        meta["patch_bolt"] = std::vector<std::string>{"utc"};
      }
    }
  }

  bolt::PackStream ps;
  ps.AppendSuccess(meta);
  conn.Respond(std::move(ps.MutableBuffer()));
}

std::function<void(bolt::BoltConnection&, bolt::BoltMsg, std::vector<std::any>)>
NewProxyHandler(const ShardMap& shard_map, uint32_t worker_thread_num,
                size_t backend_max_connections_per_backend,
                std::chrono::milliseconds backend_borrow_timeout) {
  auto worker_pool = std::make_shared<bolt::BoltWorkerPool>(
      worker_thread_num, "proxy-worker-", "proxy");
  auto context = std::make_shared<ProxyContext>(
      shard_map, backend_max_connections_per_backend, backend_borrow_timeout);
  return [context, worker_pool](bolt::BoltConnection& conn, bolt::BoltMsg msg,
                                std::vector<std::any> fields) mutable {
    if (msg == bolt::BoltMsg::Hello) {
      auto existing_session = GetSession(conn);
      if (existing_session) {
        LOG_WARN("receive duplicate proxy HELLO, close the connection");
        RequestSessionInterrupt(existing_session);
        conn.Close();
        return;
      }
      try {
        HandleHello(context->backend_pool, worker_pool, conn,
                    std::move(fields));
      } catch (const std::exception& e) {
        bolt::PackStream ps;
        ps.AppendFailure({{"code", kRequestError}, {"message", e.what()}});
        conn.Respond(std::move(ps.MutableBuffer()));
        conn.Close();
      }
      return;
    }

    if (msg == bolt::BoltMsg::Goodbye) {
      auto session = GetSession(conn);
      if (session) {
        RequestSessionInterrupt(session);
      }
      conn.Close();
      return;
    }

    auto session_context = GetSessionContext(conn);
    if (!session_context) {
      LOG_WARN("receive {} before proxy HELLO, close the connection",
               BoltMsgName(msg));
      conn.Close();
      return;
    }
    auto session = session_context->session;

    if (msg == bolt::BoltMsg::Run || msg == bolt::BoltMsg::PullN ||
        msg == bolt::BoltMsg::DiscardN || msg == bolt::BoltMsg::Reset ||
        msg == bolt::BoltMsg::Begin || msg == bolt::BoltMsg::Commit ||
        msg == bolt::BoltMsg::Rollback || msg == bolt::BoltMsg::Route) {
      if (msg == bolt::BoltMsg::Reset) {
        RequestSessionInterrupt(session);
      }
      EnqueueSessionMessage(context, worker_pool, conn,
                            std::move(session_context),
                            {.type = msg, .fields = std::move(fields)});
      return;
    }

    bolt::PackStream ps;
    ps.AppendFailure(
        {{"code", kRequestError},
         {"message", "unsupported Bolt message for lgraph_proxy"}});
    conn.Respond(std::move(ps.MutableBuffer()));
    conn.Close();
  };
}

}  // namespace

ProxyServer::ProxyServer(ProxyServerOptions options)
    : options_(std::move(options)) {}

bool ProxyServer::Start() {
  if (started_.load()) {
    return true;
  }
  auto handler = NewProxyHandler(
      options_.shard_map, options_.worker_thread_num,
      options_.backend_max_connections_per_backend,
      std::chrono::milliseconds(options_.backend_borrow_timeout_ms));
  if (!bolt_server_.Start(options_.listen_port, options_.bolt_io_thread_num,
                          options_.max_connections, std::move(handler))) {
    return false;
  }
  started_.store(true);
  LOG_INFO("lgraph_proxy started on bolt port {}", options_.listen_port);
  return true;
}

void ProxyServer::Stop() {
  bolt_server_.Stop();
  started_.store(false);
}

bool ProxyServer::Started() const {
  return started_.load() && bolt_server_.Started();
}

}  // namespace proxy
