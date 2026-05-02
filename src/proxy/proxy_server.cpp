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

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>

#include "bolt/blocking_queue.h"
#include "bolt/pack_stream.h"
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

enum class ProxySessionState { Ready = 0, Streaming, Failed };

struct ProxySession {
  explicit ProxySession(size_t max_pending_messages)
      : messages(max_pending_messages) {}

  std::unordered_map<std::string, std::any> hello_meta;
  ProxySessionState state = ProxySessionState::Ready;
  std::optional<std::string> active_backend;
  std::unordered_map<std::string, std::unique_ptr<BoltBackendSession>>
      backend_sessions;
  bolt::BlockingQueue<ProxyMessage> messages;
  std::mutex schedule_mutex;
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

  void Invalidate(const std::string& graph_name) {
    std::lock_guard<std::mutex> guard(mutex_);
    leaders_.erase(graph_name);
  }

 private:
  std::mutex mutex_;
  std::unordered_map<std::string, BackendEndpoint> leaders_;
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
    throw std::runtime_error(
        "routing parameter _shard_key_ should not be null");
  }
  const auto& type = value.type();
  if (type == typeid(std::string)) {
    auto shard_key = std::any_cast<std::string>(value);
    if (shard_key.empty()) {
      throw std::runtime_error(
          "routing parameter _shard_key_ should not be empty");
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
  throw std::runtime_error(
      "routing parameter _shard_key_ type should be String, Integer, Bool, or "
      "Float");
}

std::unordered_map<std::string, std::any> CastMapField(const std::any& value,
                                                       const char* field_name) {
  auto* map = std::any_cast<std::unordered_map<std::string, std::any>>(&value);
  if (map == nullptr) {
    throw std::runtime_error(fmt::format("{} type should be Map", field_name));
  }
  return *map;
}

std::string CastStringField(const std::any& value, const char* field_name) {
  auto* str = std::any_cast<std::string>(&value);
  if (str == nullptr) {
    throw std::runtime_error(
        fmt::format("{} type should be String", field_name));
  }
  return *str;
}

int64_t ExtractPullN(const std::vector<std::any>& fields) {
  if (fields.size() != 1) {
    throw std::runtime_error("PULL/DISCARD fields size should be 1");
  }
  auto metadata = CastMapField(fields[0], "PULL/DISCARD metadata");
  auto iter = metadata.find("n");
  if (iter == metadata.end()) {
    throw std::runtime_error("PULL/DISCARD metadata should contain n");
  }
  auto* n = std::any_cast<int64_t>(&iter->second);
  if (n == nullptr) {
    throw std::runtime_error("PULL/DISCARD n type should be Integer");
  }
  return *n;
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

void ForwardMessages(const std::shared_ptr<bolt::BoltConnection>& conn,
                     const std::vector<BackendMessage>& messages) {
  for (const auto& message : messages) {
    conn->PostResponse(message.raw);
  }
}

BackendMessage LastMessage(const std::vector<BackendMessage>& messages) {
  if (messages.empty()) {
    throw std::runtime_error("backend returned no Bolt message");
  }
  return messages.back();
}

BoltBackendSession& GetBackendSession(ProxySession* session,
                                      const BackendEndpoint& endpoint) {
  auto iter = session->backend_sessions.find(endpoint.name);
  if (iter != session->backend_sessions.end()) {
    return *iter->second;
  }
  auto backend_session =
      std::make_unique<BoltBackendSession>(endpoint, session->hello_meta);
  auto* ptr = backend_session.get();
  session->backend_sessions.emplace(endpoint.name, std::move(backend_session));
  return *ptr;
}

void CloseBackendSession(ProxySession* session,
                         const BackendEndpoint& endpoint) {
  auto iter = session->backend_sessions.find(endpoint.name);
  if (iter != session->backend_sessions.end()) {
    iter->second->Close();
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
  return messages.back().failure_message.find("not leader") !=
         std::string::npos;
}

void ProcessReset(const std::shared_ptr<bolt::BoltConnection>& conn,
                  ProxySession* session) {
  if (!session->active_backend.has_value()) {
    session->state = ProxySessionState::Ready;
    SendSuccess(conn);
    return;
  }

  auto iter = session->backend_sessions.find(*session->active_backend);
  session->active_backend.reset();
  session->state = ProxySessionState::Ready;
  if (iter == session->backend_sessions.end()) {
    SendSuccess(conn);
    return;
  }

  bolt::PackStream ps;
  ps.AppendReset();
  auto messages = iter->second->SendAndReadUntilTerminal(ps.ConstBuffer());
  ForwardMessages(conn, messages);
}

void ProcessRun(const ShardMap& shard_map,
                const std::shared_ptr<LeaderCache>& leader_cache,
                const std::shared_ptr<bolt::BoltConnection>& conn,
                ProxySession* session, const std::vector<std::any>& fields) {
  if (session->state == ProxySessionState::Streaming) {
    SendFailure(conn, session, kRequestError,
                "cannot RUN while previous result stream is open");
    return;
  }
  if (fields.size() < 3) {
    SendFailure(conn, session, kArgumentError,
                "RUN requires cypher, parameters, and metadata fields");
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
    try {
      if (attempt == 0) {
        auto cached_leader = leader_cache->Get(route.graph_name);
        selected_endpoint = cached_leader.has_value()
                                ? *cached_leader
                                : DiscoverLeader(route, session->hello_meta);
      } else {
        selected_endpoint = DiscoverLeader(route, session->hello_meta);
      }

      auto& backend = GetBackendSession(session, selected_endpoint);
      messages = backend.SendAndReadUntilTerminal(ps.ConstBuffer());
      if (!IsNotLeaderFailure(messages)) {
        leader_cache->Put(route.graph_name, selected_endpoint);
        break;
      }

      last_error = messages.back().failure_message;
      CloseBackendSession(session, selected_endpoint);
      leader_cache->Invalidate(route.graph_name);
      LOG_WARN("proxy stale leader for graph {} backend {}:{}: {}",
               route.graph_name, selected_endpoint.host, selected_endpoint.port,
               last_error);
      messages.clear();
    } catch (const std::exception& e) {
      last_error = e.what();
      CloseBackendSession(session, selected_endpoint);
      leader_cache->Invalidate(route.graph_name);
      LOG_WARN("proxy backend attempt failed for graph {} backend {}:{}: {}",
               route.graph_name, selected_endpoint.host, selected_endpoint.port,
               e.what());
      messages.clear();
    }
  }
  if (messages.empty()) {
    SendFailure(conn, session, kNetworkError,
                fmt::format("failed to route graph {} shard {}: {}",
                            route.graph_name, route.shard_id, last_error));
    return;
  }
  auto last = LastMessage(messages);
  ForwardMessages(conn, messages);

  session->active_backend = selected_endpoint.name;
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
  if (session->state != ProxySessionState::Streaming ||
      !session->active_backend.has_value()) {
    SendFailure(conn, session, kRequestError,
                "PULL/DISCARD requires an active result stream");
    return;
  }
  auto iter = session->backend_sessions.find(*session->active_backend);
  if (iter == session->backend_sessions.end()) {
    SendFailure(conn, session, kNetworkError,
                "active backend session is missing");
    return;
  }

  auto n = ExtractPullN(fields);
  bolt::PackStream ps;
  if (type == bolt::BoltMsg::PullN) {
    ps.AppendPullN(n);
  } else {
    ps.AppendDiscardN(n);
  }
  auto last = iter->second->SendAndForwardUntilTerminal(
      ps.ConstBuffer(), [&conn](const BackendMessage& message) {
        conn->PostResponse(message.raw);
      });

  if (last.tag == bolt::BoltMsg::Success && !last.success_has_more) {
    session->state = ProxySessionState::Ready;
    session->active_backend.reset();
  } else if (last.tag == bolt::BoltMsg::Failure) {
    session->state = ProxySessionState::Failed;
  }
}

void ProcessProxyMessage(const ShardMap& shard_map,
                         const std::shared_ptr<LeaderCache>& leader_cache,
                         const std::shared_ptr<bolt::BoltConnection>& conn,
                         ProxySession* session, ProxyMessage message) {
  try {
    if (session->state == ProxySessionState::Failed) {
      if (message.type == bolt::BoltMsg::Reset) {
        ProcessReset(conn, session);
      } else {
        SendIgnored(conn);
      }
      return;
    }

    switch (message.type) {
      case bolt::BoltMsg::Run:
        ProcessRun(shard_map, leader_cache, conn, session, message.fields);
        return;
      case bolt::BoltMsg::PullN:
      case bolt::BoltMsg::DiscardN:
        ProcessPullOrDiscard(conn, session, message.type, message.fields);
        return;
      case bolt::BoltMsg::Reset:
        ProcessReset(conn, session);
        return;
      case bolt::BoltMsg::Begin:
      case bolt::BoltMsg::Commit:
      case bolt::BoltMsg::Rollback:
        SendFailure(conn, session, kRequestError,
                    "explicit transactions are not supported by lgraph_proxy");
        return;
      default:
        SendFailure(conn, session, kRequestError,
                    "unsupported Bolt message for lgraph_proxy");
        return;
    }
  } catch (const std::exception& e) {
    LOG_WARN("proxy message failed: {}", e.what());
    SendFailure(conn, session, kNetworkError, e.what());
  }
}

void ProcessSession(const ShardMap& shard_map,
                    std::shared_ptr<LeaderCache> leader_cache,
                    std::shared_ptr<bolt::BoltConnection> conn,
                    std::shared_ptr<ProxySession> session,
                    std::weak_ptr<ProxyWorkerPool> weak_pool);

void ScheduleSession(const ShardMap& shard_map,
                     const std::shared_ptr<LeaderCache>& leader_cache,
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
  if (!pool->Post(
          [shard_map, leader_cache, conn, session, weak_pool]() mutable {
            ProcessSession(shard_map, leader_cache, conn, session, weak_pool);
          })) {
    conn->Close();
  }
}

void ProcessSession(const ShardMap& shard_map,
                    std::shared_ptr<LeaderCache> leader_cache,
                    std::shared_ptr<bolt::BoltConnection> conn,
                    std::shared_ptr<ProxySession> session,
                    std::weak_ptr<ProxyWorkerPool> weak_pool) {
  while (!conn->has_closed()) {
    auto message = session->messages.TryPop();
    if (!message) {
      break;
    }
    ProcessProxyMessage(shard_map, leader_cache, conn, session.get(),
                        std::move(*message));
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
    conn->Close();
    return;
  }
  if (!pool->Post(
          [shard_map, leader_cache, conn, session, weak_pool]() mutable {
            ProcessSession(shard_map, leader_cache, conn, session, weak_pool);
          })) {
    conn->Close();
  }
}

bool EnqueueSessionMessage(const ShardMap& shard_map,
                           const std::shared_ptr<LeaderCache>& leader_cache,
                           const std::shared_ptr<ProxyWorkerPool>& pool,
                           bolt::BoltConnection& conn,
                           std::shared_ptr<ProxySession> session,
                           ProxyMessage message) {
  if (!session->messages.Push(std::move(message))) {
    LOG_WARN("close proxy connection {}: pending message queue is full",
             conn.conn_id());
    conn.Close();
    return false;
  }
  ScheduleSession(shard_map, leader_cache, pool, conn.shared_from_this(),
                  std::move(session));
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
  conn.MarkAuthenticated();

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
  auto leader_cache = std::make_shared<LeaderCache>();
  return [shard_map, worker_pool, leader_cache, max_pending_messages](
             bolt::BoltConnection& conn, bolt::BoltMsg msg,
             std::vector<std::any> fields) mutable {
    if (msg == bolt::BoltMsg::Hello) {
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
        msg == bolt::BoltMsg::Rollback) {
      EnqueueSessionMessage(shard_map, leader_cache, worker_pool, conn,
                            std::move(session),
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
                          options_.max_connections,
                          options_.bolt_connection_options,
                          std::move(handler))) {
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
