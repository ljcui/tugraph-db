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

#include <pthread.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>

#include "bolt/blocking_queue.h"
#include "bolt/pack_stream.h"
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

class ProxyWorkerPool {
 public:
  explicit ProxyWorkerPool(uint32_t thread_num) {
    if (thread_num == 0) {
      thread_num = 1;
    }
    for (uint32_t i = 0; i < thread_num; ++i) {
      threads_.emplace_back([this, i]() { Run(i); });
    }
  }

  ~ProxyWorkerPool() { Stop(); }

  bool Post(std::function<void()> task) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (stopped_) {
        return false;
      }
      tasks_.push_back(std::move(task));
    }
    condition_.notify_one();
    return true;
  }

  void Stop() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (stopped_) {
        return;
      }
      stopped_ = true;
      tasks_.clear();
    }
    condition_.notify_all();
    for (auto& thread : threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

 private:
  void Run(uint32_t worker_id) {
    std::string name = "proxy-worker-" + std::to_string(worker_id);
    pthread_setname_np(pthread_self(), name.c_str());
    while (true) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]() { return stopped_ || !tasks_.empty(); });
        if (stopped_ && tasks_.empty()) {
          return;
        }
        task = std::move(tasks_.front());
        tasks_.pop_front();
      }
      try {
        task();
      } catch (const std::exception& e) {
        LOG_ERROR("proxy worker task failed: {}", e.what());
      } catch (...) {
        LOG_ERROR("proxy worker task failed with unknown exception");
      }
    }
  }

  std::mutex mutex_;
  std::condition_variable condition_;
  std::deque<std::function<void()>> tasks_;
  std::vector<std::thread> threads_;
  bool stopped_ = false;
};

struct ProxyMessage {
  bolt::BoltMsg type;
  std::vector<std::any> fields;
};

enum class ProxySessionState { Ready = 0, Streaming, Failed, Defunct };

struct ProxySession {
  explicit ProxySession(size_t max_pending_messages)
      : messages(max_pending_messages) {}

  std::unordered_map<std::string, std::any> hello_meta;
  ProxySessionState state = ProxySessionState::Ready;
  std::optional<std::string> active_backend;
  std::unordered_map<std::string, std::shared_ptr<BoltBackendSession>>
      backend_sessions;
  std::mutex backend_mutex;
  bolt::BlockingQueue<ProxyMessage> messages;
  std::mutex schedule_mutex;
  std::atomic<bool> interrupt_requested{false};
  bool scheduled = false;
};

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

struct ProxyContext {
  explicit ProxyContext(ShardMap shard_map)
      : shard_map(std::make_shared<ShardMap>(std::move(shard_map))),
        leader_cache(std::make_shared<LeaderCache>()) {}

  std::shared_ptr<const ShardMap> shard_map;
  std::shared_ptr<LeaderCache> leader_cache;
};

std::shared_ptr<ProxySession> GetSession(bolt::BoltConnection& conn) {
  auto ctx = conn.GetContextShared();
  if (!ctx) {
    return {};
  }
  return std::static_pointer_cast<ProxySession>(ctx);
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

bool IsProxySessionRequest(bolt::BoltMsg type) {
  return type == bolt::BoltMsg::Run || type == bolt::BoltMsg::PullN ||
         type == bolt::BoltMsg::DiscardN || type == bolt::BoltMsg::Route ||
         IsExplicitTransactionRequest(type);
}

std::string_view ProxySessionStateName(ProxySessionState state) {
  switch (state) {
    case ProxySessionState::Ready:
      return "READY";
    case ProxySessionState::Streaming:
      return "STREAMING";
    case ProxySessionState::Failed:
      return "FAILED";
    case ProxySessionState::Defunct:
      return "DEFUNCT";
  }
  return "UNKNOWN";
}

void SendFailure(const std::shared_ptr<bolt::BoltConnection>& conn,
                 ProxySession* session, const std::string& code,
                 const std::string& message) {
  bolt::PackStream ps;
  ps.AppendFailure({{"code", code}, {"message", message}});
  conn->PostResponse(std::move(ps.MutableBuffer()));
  session->state = ProxySessionState::Failed;
}

void SendIgnored(const std::shared_ptr<bolt::BoltConnection>& conn) {
  bolt::PackStream ps;
  ps.AppendIgnored();
  conn->PostResponse(std::move(ps.MutableBuffer()));
}

void SendSuccess(const std::shared_ptr<bolt::BoltConnection>& conn) {
  bolt::PackStream ps;
  ps.AppendSuccess();
  conn->PostResponse(std::move(ps.MutableBuffer()));
}

void CloseProtocolError(const std::shared_ptr<bolt::BoltConnection>& conn,
                        ProxySession* session, bolt::BoltMsg type) {
  LOG_ERROR("unexpected {} in proxy {} state, close the connection",
            BoltMsgName(type), ProxySessionStateName(session->state));
  session->state = ProxySessionState::Defunct;
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
    if (conn_->has_closed() || session_->interrupt_requested.load()) {
      return false;
    }
    buffer_.append(message.raw);
    if (buffer_.size() >= kFlushBytes) {
      Flush();
    }
    return !conn_->has_closed() && !session_->interrupt_requested.load();
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

std::shared_ptr<BoltBackendSession> GetBackendSession(
    ProxySession* session, const BackendEndpoint& endpoint) {
  std::lock_guard<std::mutex> guard(session->backend_mutex);
  auto iter = session->backend_sessions.find(endpoint.name);
  if (iter != session->backend_sessions.end()) {
    return iter->second;
  }
  auto backend_session =
      std::make_shared<BoltBackendSession>(endpoint, session->hello_meta);
  session->backend_sessions.emplace(endpoint.name, backend_session);
  return backend_session;
}

void SetActiveBackend(ProxySession* session, const std::string& name) {
  std::lock_guard<std::mutex> guard(session->backend_mutex);
  session->active_backend = name;
}

void ClearActiveBackend(ProxySession* session) {
  std::lock_guard<std::mutex> guard(session->backend_mutex);
  session->active_backend.reset();
}

void ClearActiveBackendIf(ProxySession* session,
                          const BackendEndpoint& endpoint) {
  if (endpoint.name.empty()) {
    return;
  }
  std::lock_guard<std::mutex> guard(session->backend_mutex);
  if (session->active_backend.has_value() &&
      *session->active_backend == endpoint.name) {
    session->active_backend.reset();
  }
}

std::shared_ptr<BoltBackendSession> ActiveBackendSession(
    ProxySession* session) {
  std::lock_guard<std::mutex> guard(session->backend_mutex);
  if (!session->active_backend.has_value()) {
    return {};
  }
  auto iter = session->backend_sessions.find(*session->active_backend);
  if (iter == session->backend_sessions.end()) {
    return {};
  }
  return iter->second;
}

std::shared_ptr<BoltBackendSession> RemoveActiveBackendSession(
    ProxySession* session) {
  std::shared_ptr<BoltBackendSession> backend;
  std::lock_guard<std::mutex> guard(session->backend_mutex);
  if (!session->active_backend.has_value()) {
    return {};
  }
  auto iter = session->backend_sessions.find(*session->active_backend);
  if (iter != session->backend_sessions.end()) {
    backend = iter->second;
    session->backend_sessions.erase(iter);
  }
  session->active_backend.reset();
  return backend;
}

void CloseBackendSession(ProxySession* session,
                         const BackendEndpoint& endpoint) {
  std::shared_ptr<BoltBackendSession> backend;
  {
    std::lock_guard<std::mutex> guard(session->backend_mutex);
    auto iter = session->backend_sessions.find(endpoint.name);
    if (iter == session->backend_sessions.end()) {
      return;
    }
    backend = iter->second;
    session->backend_sessions.erase(iter);
    if (session->active_backend.has_value() &&
        *session->active_backend == endpoint.name) {
      session->active_backend.reset();
    }
  }
  backend->Close();
}

void CloseAllBackendSessions(ProxySession* session) {
  std::vector<std::shared_ptr<BoltBackendSession>> backends;
  {
    std::lock_guard<std::mutex> guard(session->backend_mutex);
    for (const auto& pair : session->backend_sessions) {
      backends.emplace_back(pair.second);
    }
    session->backend_sessions.clear();
    session->active_backend.reset();
  }
  for (const auto& backend : backends) {
    backend->Close();
  }
}

void RequestSessionInterrupt(const std::shared_ptr<ProxySession>& session) {
  session->interrupt_requested.store(true);
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
    const std::unordered_map<std::string, std::any>& hello_meta) {
  if (route.replica_group == nullptr || route.replica_group->replicas.empty()) {
    throw std::runtime_error(
        fmt::format("proxy shard {} has no raft replicas", route.shard_id));
  }

  std::string last_error;
  for (const auto& replica : route.replica_group->replicas) {
    try {
      BoltBackendSession session(replica, hello_meta);
      auto node_infos = session.FetchRaftNodeInfos(route.graph_name);
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
    } catch (const std::exception& e) {
      last_error = e.what();
      LOG_WARN("failed to discover leader for graph {} from {}:{}: {}",
               route.graph_name, replica.host, replica.port, e.what());
    }
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

void ProcessReset(const std::shared_ptr<bolt::BoltConnection>& conn,
                  ProxySession* session) {
  const bool interrupted = session->interrupt_requested.exchange(false);
  auto backend = interrupted ? RemoveActiveBackendSession(session)
                             : ActiveBackendSession(session);
  if (interrupted) {
    if (backend) {
      backend->Close();
    }
    session->state = ProxySessionState::Ready;
    SendSuccess(conn);
    return;
  }

  if (!backend) {
    ClearActiveBackend(session);
    session->state = ProxySessionState::Ready;
    SendSuccess(conn);
    return;
  }

  bolt::PackStream ps;
  ps.AppendReset();
  auto messages = backend->SendAndReadUntilTerminal(ps.ConstBuffer());
  ForwardMessages(conn, messages);
  auto last = LastMessage(messages);
  if (last.tag == bolt::BoltMsg::Success) {
    ClearActiveBackend(session);
    session->state = ProxySessionState::Ready;
  } else {
    session->state = ProxySessionState::Failed;
  }
}

void ProcessRun(const std::shared_ptr<ProxyContext>& context,
                const std::shared_ptr<bolt::BoltConnection>& conn,
                ProxySession* session, const std::vector<std::any>& fields) {
  if (fields.size() < 3) {
    SendFailure(conn, session, kArgumentError,
                "RUN requires cypher, parameters, and metadata fields");
    return;
  }
  if (fields.size() != 3) {
    SendFailure(conn, session, kArgumentError, "RUN fields size should be 3");
    return;
  }
  if (session->interrupt_requested.load()) {
    session->state = ProxySessionState::Failed;
    return;
  }

  auto cypher = CastStringField(fields[0], "RUN cypher");
  auto params = CastMapField(fields[1], "RUN parameters");
  auto extra = CastMapField(fields[2], "RUN metadata");

  auto shard_key_iter = params.find(kShardKeyParam);
  if (shard_key_iter == params.end()) {
    SendFailure(conn, session, kArgumentError,
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
        selected_endpoint = cached_leader.has_value()
                                ? *cached_leader
                                : DiscoverLeader(route, session->hello_meta);
      } else {
        selected_endpoint = DiscoverLeader(route, session->hello_meta);
      }

      auto backend = GetBackendSession(session, selected_endpoint);
      SetActiveBackend(session, selected_endpoint.name);
      if (session->interrupt_requested.load()) {
        throw BackendOperationCancelled("proxy RUN");
      }
      messages = backend->SendAndReadUntilTerminal(ps.ConstBuffer());
      if (!IsNotLeaderFailure(messages)) {
        leader_cache->Put(route.graph_name, selected_endpoint);
        break;
      }

      last_error = messages.back().failure_message;
      CloseBackendSession(session, selected_endpoint);
      leader_cache->InvalidateIf(route.graph_name, selected_endpoint);
      LOG_WARN("proxy stale leader for graph {} backend {}:{}: {}",
               route.graph_name, selected_endpoint.host, selected_endpoint.port,
               last_error);
      messages.clear();
    } catch (const BackendOperationCancelled&) {
      CloseBackendSession(session, selected_endpoint);
      throw;
    } catch (const std::exception& e) {
      last_error = e.what();
      CloseBackendSession(session, selected_endpoint);
      if (!selected_endpoint.name.empty()) {
        leader_cache->InvalidateIf(route.graph_name, selected_endpoint);
      }
      LOG_WARN("proxy backend attempt failed for graph {} backend {}:{}: {}",
               route.graph_name, selected_endpoint.host, selected_endpoint.port,
               e.what());
      messages.clear();
    }
  }
  if (messages.empty()) {
    ClearActiveBackendIf(session, selected_endpoint);
    SendFailure(conn, session, kNetworkError,
                fmt::format("failed to route graph {} shard {}: {}",
                            route.graph_name, route.shard_id, last_error));
    return;
  }
  auto last = LastMessage(messages);
  ForwardMessages(conn, messages);

  if (last.tag == bolt::BoltMsg::Success) {
    session->state = ProxySessionState::Streaming;
  } else {
    session->state = ProxySessionState::Failed;
  }
  LOG_DEBUG("proxy routed shard_key [{}] to shard {} graph {} backend {}:{}",
            shard_key, route.shard_id, route.graph_name, selected_endpoint.host,
            selected_endpoint.port);
}

void ProcessPullOrDiscard(const std::shared_ptr<bolt::BoltConnection>& conn,
                          ProxySession* session, bolt::BoltMsg type,
                          const std::vector<std::any>& fields) {
  auto backend = ActiveBackendSession(session);
  if (!backend) {
    SendFailure(conn, session, kNetworkError,
                "active backend session is missing");
    return;
  }
  if (session->interrupt_requested.load()) {
    session->state = ProxySessionState::Failed;
    return;
  }

  int64_t n = 0;
  try {
    n = ExtractPullN(fields);
  } catch (const std::exception& e) {
    SendFailure(conn, session, kRequestError, e.what());
    return;
  }

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

  if (last.tag == bolt::BoltMsg::Success && !last.success_has_more) {
    session->state = ProxySessionState::Ready;
    ClearActiveBackend(session);
  } else if (last.tag == bolt::BoltMsg::Failure) {
    session->state = ProxySessionState::Failed;
  }
}

void ProcessRecoverableState(const std::shared_ptr<bolt::BoltConnection>& conn,
                             ProxySession* session, bolt::BoltMsg type) {
  if (type == bolt::BoltMsg::Reset) {
    ProcessReset(conn, session);
  } else if (IsProxySessionRequest(type)) {
    SendIgnored(conn);
  } else {
    CloseProtocolError(conn, session, type);
  }
}

void ProcessReadyState(const std::shared_ptr<ProxyContext>& context,
                       const std::shared_ptr<bolt::BoltConnection>& conn,
                       ProxySession* session, const ProxyMessage& message) {
  if (message.type == bolt::BoltMsg::Run) {
    ProcessRun(context, conn, session, message.fields);
  } else if (message.type == bolt::BoltMsg::Route) {
    SendFailure(conn, session, kRequestError,
                "routing is not supported by lgraph_proxy");
  } else if (IsExplicitTransactionRequest(message.type)) {
    SendFailure(conn, session, kRequestError,
                "explicit transactions are not supported by lgraph_proxy");
  } else if (message.type == bolt::BoltMsg::Reset) {
    ProcessReset(conn, session);
  } else {
    CloseProtocolError(conn, session, message.type);
  }
}

void ProcessStreamingState(const std::shared_ptr<bolt::BoltConnection>& conn,
                           ProxySession* session, const ProxyMessage& message) {
  if (message.type == bolt::BoltMsg::PullN ||
      message.type == bolt::BoltMsg::DiscardN) {
    ProcessPullOrDiscard(conn, session, message.type, message.fields);
  } else if (message.type == bolt::BoltMsg::Reset) {
    ProcessReset(conn, session);
  } else {
    CloseProtocolError(conn, session, message.type);
  }
}

void ProcessProxyMessage(const std::shared_ptr<ProxyContext>& context,
                         const std::shared_ptr<bolt::BoltConnection>& conn,
                         ProxySession* session, ProxyMessage message) {
  try {
    if (session->interrupt_requested.load() &&
        message.type != bolt::BoltMsg::Reset) {
      if (IsProxySessionRequest(message.type)) {
        SendIgnored(conn);
      } else {
        CloseProtocolError(conn, session, message.type);
      }
      return;
    }
    switch (session->state) {
      case ProxySessionState::Ready:
        ProcessReadyState(context, conn, session, message);
        break;
      case ProxySessionState::Streaming:
        ProcessStreamingState(conn, session, message);
        break;
      case ProxySessionState::Failed:
        ProcessRecoverableState(conn, session, message.type);
        break;
      case ProxySessionState::Defunct:
        CloseProtocolError(conn, session, message.type);
        break;
    }
  } catch (const BackendOperationCancelled& e) {
    LOG_INFO("proxy message cancelled: {}", e.what());
    if (conn->has_closed()) {
      session->state = ProxySessionState::Defunct;
      CloseAllBackendSessions(session);
      return;
    }
    if (session->interrupt_requested.load()) {
      session->state = ProxySessionState::Failed;
      return;
    }
    if (session->state != ProxySessionState::Defunct) {
      SendFailure(conn, session, kNetworkError, e.what());
    }
  } catch (const ProxyClientError& e) {
    LOG_WARN("proxy client message failed: {}", e.what());
    if (!conn->has_closed() && session->state != ProxySessionState::Defunct) {
      SendFailure(conn, session, e.code(), e.what());
    }
  } catch (const LgraphException& e) {
    LOG_WARN("proxy client message failed: {}", e.what());
    if (!conn->has_closed() && session->state != ProxySessionState::Defunct) {
      SendFailure(conn, session, kArgumentError, e.msg());
    }
  } catch (const std::exception& e) {
    LOG_WARN("proxy message failed: {}", e.what());
    if (!conn->has_closed() && session->state != ProxySessionState::Defunct) {
      SendFailure(conn, session, kNetworkError, e.what());
    }
  }
}

void ProcessSession(std::shared_ptr<ProxyContext> context,
                    std::shared_ptr<bolt::BoltConnection> conn,
                    std::shared_ptr<ProxySession> session,
                    std::weak_ptr<ProxyWorkerPool> weak_pool);

void ScheduleSession(const std::shared_ptr<ProxyContext>& context,
                     const std::shared_ptr<ProxyWorkerPool>& pool,
                     std::shared_ptr<bolt::BoltConnection> conn,
                     std::shared_ptr<ProxySession> session) {
  bool should_schedule = false;
  {
    std::unique_lock<std::mutex> lock(session->schedule_mutex);
    if (!session->scheduled) {
      session->scheduled = true;
      should_schedule = true;
    }
  }
  if (!should_schedule) {
    return;
  }

  std::weak_ptr<ProxyWorkerPool> weak_pool = pool;
  if (!pool->Post([context, conn, session, weak_pool]() mutable {
        ProcessSession(context, conn, session, weak_pool);
      })) {
    RequestSessionInterrupt(session);
    conn->Close();
  }
}

void ProcessSession(std::shared_ptr<ProxyContext> context,
                    std::shared_ptr<bolt::BoltConnection> conn,
                    std::shared_ptr<ProxySession> session,
                    std::weak_ptr<ProxyWorkerPool> weak_pool) {
  while (!conn->has_closed()) {
    auto message = session->messages.TryPop();
    if (!message) {
      break;
    }
    ProcessProxyMessage(context, conn, session.get(), std::move(*message));
  }
  if (conn->has_closed()) {
    RequestSessionInterrupt(session);
    CloseAllBackendSessions(session.get());
    return;
  }

  bool should_reschedule = false;
  {
    std::unique_lock<std::mutex> lock(session->schedule_mutex);
    session->scheduled = false;
    if (!conn->has_closed() && !session->messages.Empty()) {
      session->scheduled = true;
      should_reschedule = true;
    }
  }

  if (!should_reschedule) {
    return;
  }
  auto pool = weak_pool.lock();
  if (!pool) {
    RequestSessionInterrupt(session);
    conn->Close();
    return;
  }
  if (!pool->Post([context, conn, session, weak_pool]() mutable {
        ProcessSession(context, conn, session, weak_pool);
      })) {
    RequestSessionInterrupt(session);
    conn->Close();
  }
}

bool EnqueueSessionMessage(const std::shared_ptr<ProxyContext>& context,
                           const std::shared_ptr<ProxyWorkerPool>& pool,
                           bolt::BoltConnection& conn,
                           std::shared_ptr<ProxySession> session,
                           ProxyMessage message) {
  if (!session->messages.Push(std::move(message))) {
    LOG_WARN("close proxy connection {}: pending message queue is full",
             conn.conn_id());
    RequestSessionInterrupt(session);
    conn.Close();
    return false;
  }
  ScheduleSession(context, pool, conn.shared_from_this(), std::move(session));
  return true;
}

void HandleHello(bolt::BoltConnection& conn, std::vector<std::any> fields,
                 size_t max_pending_messages) {
  if (fields.size() != 1) {
    bolt::PackStream ps;
    ps.AppendFailure({{"code", kRequestError},
                      {"message", "HELLO fields size should be 1"}});
    conn.Respond(std::move(ps.MutableBuffer()));
    conn.Close();
    return;
  }
  auto hello_meta = CastMapField(fields[0], "HELLO metadata");
  auto session = std::make_shared<ProxySession>(max_pending_messages);
  session->hello_meta = hello_meta;
  conn.SetContext(session);

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
                size_t max_pending_messages) {
  auto worker_pool = std::make_shared<ProxyWorkerPool>(worker_thread_num);
  auto context = std::make_shared<ProxyContext>(shard_map);
  return [context, worker_pool, max_pending_messages](
             bolt::BoltConnection& conn, bolt::BoltMsg msg,
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
        HandleHello(conn, std::move(fields), max_pending_messages);
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

    auto session = GetSession(conn);
    if (!session) {
      LOG_WARN("receive {} before proxy HELLO, close the connection",
               BoltMsgName(msg));
      conn.Close();
      return;
    }

    if (msg == bolt::BoltMsg::Run || msg == bolt::BoltMsg::PullN ||
        msg == bolt::BoltMsg::DiscardN || msg == bolt::BoltMsg::Reset ||
        msg == bolt::BoltMsg::Begin || msg == bolt::BoltMsg::Commit ||
        msg == bolt::BoltMsg::Rollback || msg == bolt::BoltMsg::Route) {
      if (msg == bolt::BoltMsg::Reset) {
        RequestSessionInterrupt(session);
      }
      EnqueueSessionMessage(context, worker_pool, conn, std::move(session),
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
  auto handler = NewProxyHandler(options_.shard_map, options_.worker_thread_num,
                                 options_.max_pending_messages_per_connection);
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
