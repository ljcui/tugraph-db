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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

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

struct ProxyMessage {
  bolt::BoltMsg type;
  std::vector<std::any> fields;
};

using ProxyStrand = boost::asio::io_service::strand;

struct ProxyStrandHandle {
  boost::asio::io_service* service = nullptr;
  std::shared_ptr<ProxyStrand> strand;
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
  explicit ProxySessionContext(ProxyStrandHandle strand_handle)
      : session(std::make_shared<ProxySession>()),
        io_service(strand_handle.service),
        strand(std::move(strand_handle.strand)) {}

  std::shared_ptr<ProxySession> session;
  boost::asio::io_service* io_service = nullptr;
  std::shared_ptr<ProxyStrand> strand;
  std::deque<ProxyMessage> pending_messages;
  bool processing = false;
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

void DropActiveBackend(const std::shared_ptr<BackendSessionPool>& pool,
                       ProxySession* session);

class LeaderCache {
 public:
  explicit LeaderCache(size_t shard_count) : leaders_(shard_count) {}

  std::optional<BackendEndpoint> Get(size_t shard_id) {
    std::shared_lock<std::shared_mutex> guard(mutex_);
    if (shard_id >= leaders_.size()) {
      return std::nullopt;
    }
    return leaders_[shard_id];
  }

  void Put(size_t shard_id, BackendEndpoint endpoint) {
    std::unique_lock<std::shared_mutex> guard(mutex_);
    if (shard_id < leaders_.size()) {
      leaders_[shard_id] = std::move(endpoint);
    }
  }

  void InvalidateIf(size_t shard_id, const BackendEndpoint& endpoint) {
    std::unique_lock<std::shared_mutex> guard(mutex_);
    if (shard_id < leaders_.size() && leaders_[shard_id].has_value() &&
        leaders_[shard_id]->name == endpoint.name) {
      leaders_[shard_id].reset();
    }
  }

 private:
  std::shared_mutex mutex_;
  std::vector<std::optional<BackendEndpoint>> leaders_;
};

class BackendPoolExhausted : public std::runtime_error {
 public:
  explicit BackendPoolExhausted(const std::string& message)
      : std::runtime_error(message) {}
};

class ProxyIoPool {
 public:
  ProxyIoPool(uint32_t thread_num, std::string thread_name_prefix)
      : thread_name_prefix_(std::move(thread_name_prefix)) {
    auto threads = thread_num == 0 ? 1 : thread_num;
    for (uint32_t i = 0; i < threads; ++i) {
      auto service = std::make_unique<boost::asio::io_service>(1);
      works_.push_back(
          std::make_unique<boost::asio::io_service::work>(*service));
      services_.push_back(std::move(service));
    }
    for (uint32_t i = 0; i < services_.size(); ++i) {
      auto& service = *services_[i];
      threads_.emplace_back([this, i, &service]() {
        auto name = thread_name_prefix_ + std::to_string(i);
        pthread_setname_np(pthread_self(), name.c_str());
        service.run();
      });
    }
  }

  ~ProxyIoPool() { Stop(); }

  boost::asio::io_service& Get() {
    auto index = next_service_.fetch_add(1, std::memory_order_relaxed);
    return *services_[index % services_.size()];
  }

  ProxyStrandHandle MakeStrand() {
    auto& service = Get();
    return {.service = &service,
            .strand = std::make_shared<ProxyStrand>(service)};
  }

  void Stop() {
    bool expected = false;
    if (!stopped_.compare_exchange_strong(expected, true,
                                          std::memory_order_acq_rel)) {
      return;
    }
    works_.clear();
    for (const auto& service : services_) {
      service->stop();
    }
    for (auto& thread : threads_) {
      thread.join();
    }
    threads_.clear();
  }

 private:
  std::string thread_name_prefix_;
  std::vector<std::unique_ptr<boost::asio::io_service>> services_;
  std::vector<std::unique_ptr<boost::asio::io_service::work>> works_;
  std::vector<std::thread> threads_;
  std::atomic<size_t> next_service_{0};
  std::atomic<bool> stopped_{false};
};

class BackendSessionPool
    : public std::enable_shared_from_this<BackendSessionPool> {
 public:
  using BorrowCallback = std::function<void(
      std::exception_ptr, std::shared_ptr<BoltBackendSession>)>;

  BackendSessionPool(size_t max_connections_per_backend,
                     std::chrono::milliseconds borrow_timeout)
      : max_connections_per_backend_(max_connections_per_backend),
        borrow_timeout_(borrow_timeout) {}

  ~BackendSessionPool() { CloseIdleSessions(); }

  void AsyncBorrow(const BackendEndpoint& endpoint,
                   boost::asio::io_service& io_service,
                   const std::unordered_map<std::string, std::any>& hello_meta,
                   BorrowCallback callback) {
    BorrowCompletion completion;
    auto bucket = GetOrCreateBucket(endpoint.name);
    {
      std::lock_guard<std::mutex> guard(bucket->mutex);
      auto backend = TakeIdleBackendLocked(*bucket, io_service);
      if (backend) {
        completion.callback = std::move(callback);
        completion.backend = std::move(backend);
      } else if (max_connections_per_backend_ == 0 ||
                 bucket->total < max_connections_per_backend_) {
        ++bucket->total;
        completion.callback = std::move(callback);
        try {
          completion.backend = std::make_shared<BoltBackendSession>(
              io_service, endpoint, hello_meta);
        } catch (...) {
          --bucket->total;
          completion.error = std::current_exception();
        }
      } else if (borrow_timeout_.count() == 0) {
        completion.callback = std::move(callback);
        completion.error =
            std::make_exception_ptr(BackendPoolExhausted(fmt::format(
                "backend connection pool exhausted for {}", endpoint.name)));
      } else {
        auto pending = std::make_shared<PendingBorrow>();
        pending->endpoint = endpoint;
        pending->io_service = &io_service;
        pending->hello_meta = hello_meta;
        pending->callback = std::move(callback);
        pending->timer =
            std::make_shared<boost::asio::steady_timer>(io_service);
        pending->timer->expires_from_now(borrow_timeout_);
        bucket->waiters.emplace_back(pending);

        std::weak_ptr<BackendSessionPool> weak_pool = shared_from_this();
        std::weak_ptr<PendingBorrow> weak_pending = pending;
        pending->timer->async_wait(
            [weak_pool, weak_pending](const boost::system::error_code& ec) {
              if (ec) {
                return;
              }
              auto pool = weak_pool.lock();
              if (!pool) {
                return;
              }
              auto pending = weak_pending.lock();
              if (!pending) {
                return;
              }
              pool->TimeoutBorrow(pending);
            });
      }
    }
    RunBorrowCompletion(std::move(completion));
  }

  void Return(const BackendEndpoint& endpoint,
              std::shared_ptr<BoltBackendSession> backend) {
    if (!backend) {
      return;
    }
    BorrowCompletion completion;
    auto bucket = FindBucket(endpoint.name);
    if (!bucket) {
      backend->Close();
      return;
    }
    {
      std::lock_guard<std::mutex> guard(bucket->mutex);
      auto pending = PopNextPendingBorrowLocked(*bucket);
      if (pending) {
        pending->completed = true;
        CancelPendingTimer(pending);
        completion.pending = std::move(pending);
        completion.callback = std::move(completion.pending->callback);
        completion.backend = std::move(backend);
      } else {
        bucket->idle.emplace_back(std::move(backend));
      }
    }
    RunBorrowCompletion(std::move(completion));
  }

  void Drop(const BackendEndpoint& endpoint,
            std::shared_ptr<BoltBackendSession> backend) {
    if (backend) {
      backend->Close();
    }
    std::vector<BorrowCompletion> completions;
    auto bucket = FindBucket(endpoint.name);
    if (!bucket) {
      return;
    }
    {
      std::lock_guard<std::mutex> guard(bucket->mutex);
      if (bucket->total > 0) {
        --bucket->total;
      }
      completions = SatisfyPendingBorrowsLocked(*bucket);
    }
    RunBorrowCompletions(std::move(completions));
  }

 private:
  struct PendingBorrow {
    BackendEndpoint endpoint;
    boost::asio::io_service* io_service = nullptr;
    std::unordered_map<std::string, std::any> hello_meta;
    BorrowCallback callback;
    std::shared_ptr<boost::asio::steady_timer> timer;
    bool completed = false;
  };

  struct BorrowCompletion {
    std::shared_ptr<PendingBorrow> pending;
    BorrowCallback callback;
    std::shared_ptr<BoltBackendSession> backend;
    std::exception_ptr error;
  };

  struct Bucket {
    std::mutex mutex;
    std::deque<std::shared_ptr<BoltBackendSession>> idle;
    std::deque<std::shared_ptr<PendingBorrow>> waiters;
    size_t total = 0;
  };

  std::shared_ptr<Bucket> GetOrCreateBucket(const std::string& key) {
    std::lock_guard<std::mutex> guard(buckets_mutex_);
    auto& bucket = buckets_[key];
    if (!bucket) {
      bucket = std::make_shared<Bucket>();
    }
    return bucket;
  }

  std::shared_ptr<Bucket> FindBucket(const std::string& key) {
    std::lock_guard<std::mutex> guard(buckets_mutex_);
    auto iter = buckets_.find(key);
    if (iter == buckets_.end()) {
      return {};
    }
    return iter->second;
  }

  std::shared_ptr<PendingBorrow> PopNextPendingBorrowLocked(Bucket& bucket) {
    while (!bucket.waiters.empty()) {
      auto pending = std::move(bucket.waiters.front());
      bucket.waiters.pop_front();
      if (!pending->completed) {
        return pending;
      }
    }
    return {};
  }

  std::shared_ptr<BoltBackendSession> TakeIdleBackendLocked(
      Bucket& bucket, boost::asio::io_service& io_service) {
    for (auto iter = bucket.idle.begin(); iter != bucket.idle.end(); ++iter) {
      if ((*iter)->RunsOn(io_service)) {
        auto backend = std::move(*iter);
        bucket.idle.erase(iter);
        return backend;
      }
    }
    if (max_connections_per_backend_ == 0 ||
        bucket.total < max_connections_per_backend_) {
      return {};
    }
    if (bucket.idle.empty()) {
      return {};
    }
    auto backend = std::move(bucket.idle.front());
    bucket.idle.pop_front();
    return backend;
  }

  std::vector<BorrowCompletion> SatisfyPendingBorrowsLocked(Bucket& bucket) {
    std::vector<BorrowCompletion> completions;
    while (true) {
      auto pending = PopNextPendingBorrowLocked(bucket);
      if (!pending) {
        break;
      }

      BorrowCompletion completion;
      pending->completed = true;
      CancelPendingTimer(pending);
      completion.pending = pending;
      completion.callback = std::move(pending->callback);

      auto backend = TakeIdleBackendLocked(bucket, *pending->io_service);
      if (backend) {
        completion.backend = std::move(backend);
      } else if (max_connections_per_backend_ == 0 ||
                 bucket.total < max_connections_per_backend_) {
        ++bucket.total;
        try {
          completion.backend = std::make_shared<BoltBackendSession>(
              *pending->io_service, pending->endpoint, pending->hello_meta);
        } catch (...) {
          --bucket.total;
          completion.error = std::current_exception();
        }
      } else {
        pending->completed = false;
        bucket.waiters.emplace_front(std::move(pending));
        break;
      }

      completions.emplace_back(std::move(completion));
    }
    return completions;
  }

  void TimeoutBorrow(const std::shared_ptr<PendingBorrow>& pending) {
    BorrowCompletion completion;
    auto bucket = FindBucket(pending->endpoint.name);
    if (!bucket) {
      return;
    }
    {
      std::lock_guard<std::mutex> guard(bucket->mutex);
      if (pending->completed) {
        return;
      }
      auto& waiters = bucket->waiters;
      auto waiter_iter = std::find(waiters.begin(), waiters.end(), pending);
      if (waiter_iter == waiters.end()) {
        return;
      }
      waiters.erase(waiter_iter);
      pending->completed = true;
      completion.pending = pending;
      completion.callback = std::move(pending->callback);
      completion.error = std::make_exception_ptr(BackendPoolExhausted(
          fmt::format("backend connection pool exhausted for {}",
                      pending->endpoint.name)));
    }
    RunBorrowCompletion(std::move(completion));
  }

  void CloseIdleSessions() {
    std::vector<std::shared_ptr<BoltBackendSession>> idle_sessions;
    std::vector<BorrowCompletion> completions;
    std::vector<std::shared_ptr<Bucket>> buckets;
    {
      std::lock_guard<std::mutex> guard(buckets_mutex_);
      for (auto& pair : buckets_) {
        buckets.emplace_back(pair.second);
      }
      buckets_.clear();
    }
    for (auto& bucket : buckets) {
      std::lock_guard<std::mutex> guard(bucket->mutex);
      while (!bucket->idle.empty()) {
        idle_sessions.emplace_back(std::move(bucket->idle.front()));
        bucket->idle.pop_front();
        if (bucket->total > 0) {
          --bucket->total;
        }
      }
      while (!bucket->waiters.empty()) {
        auto pending = std::move(bucket->waiters.front());
        bucket->waiters.pop_front();
        if (pending->completed) {
          continue;
        }
        pending->completed = true;
        CancelPendingTimer(pending);
        completions.push_back(
            {.pending = pending,
             .callback = std::move(pending->callback),
             .backend = {},
             .error = std::make_exception_ptr(
                 std::runtime_error("backend connection pool is closed"))});
      }
    }
    for (const auto& backend : idle_sessions) {
      backend->Close();
    }
    RunBorrowCompletions(std::move(completions));
  }

  void CancelPendingTimer(const std::shared_ptr<PendingBorrow>& pending) {
    if (!pending->timer) {
      return;
    }
    boost::system::error_code ignored;
    pending->timer->cancel(ignored);
    pending->timer.reset();
  }

  void RunBorrowCompletion(BorrowCompletion completion) {
    if (!completion.callback) {
      return;
    }
    auto callback = std::move(completion.callback);
    callback(completion.error, std::move(completion.backend));
  }

  void RunBorrowCompletions(std::vector<BorrowCompletion> completions) {
    for (auto& completion : completions) {
      RunBorrowCompletion(std::move(completion));
    }
  }

  size_t max_connections_per_backend_;
  std::chrono::milliseconds borrow_timeout_;
  std::mutex buckets_mutex_;
  std::unordered_map<std::string, std::shared_ptr<Bucket>> buckets_;
};

struct ProxyContext {
  ProxyContext(ShardMap shard_map, size_t backend_max_connections_per_backend,
               std::chrono::milliseconds backend_borrow_timeout,
               uint32_t proxy_io_thread_num)
      : shard_map(std::make_shared<ShardMap>(std::move(shard_map))),
        leader_cache(
            std::make_shared<LeaderCache>(this->shard_map->shard_count())),
        proxy_io_pool(
            std::make_shared<ProxyIoPool>(proxy_io_thread_num, "proxy-io-")),
        backend_pool(std::make_shared<BackendSessionPool>(
            backend_max_connections_per_backend, backend_borrow_timeout)) {}

  std::shared_ptr<const ShardMap> shard_map;
  std::shared_ptr<LeaderCache> leader_cache;
  std::shared_ptr<ProxyIoPool> proxy_io_pool;
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

const std::unordered_map<std::string, std::any>* CastMapFieldPtr(
    const std::any& value, const char* field_name) {
  auto* map = std::any_cast<std::unordered_map<std::string, std::any>>(&value);
  if (map == nullptr) {
    throw ProxyClientError(kArgumentError,
                           fmt::format("{} type should be Map", field_name));
  }
  return map;
}

std::unordered_map<std::string, std::any>* CastMutableMapField(
    std::any& value, const char* field_name) {
  auto* map = std::any_cast<std::unordered_map<std::string, std::any>>(&value);
  if (map == nullptr) {
    throw ProxyClientError(kArgumentError,
                           fmt::format("{} type should be Map", field_name));
  }
  return map;
}

const std::string* CastStringFieldPtr(const std::any& value,
                                      const char* field_name) {
  auto* str = std::any_cast<std::string>(&value);
  if (str == nullptr) {
    throw ProxyClientError(kArgumentError,
                           fmt::format("{} type should be String", field_name));
  }
  return str;
}

int64_t ExtractPullN(const std::vector<std::any>& fields) {
  if (fields.size() != 1) {
    throw ProxyClientError(kRequestError,
                           "PULL/DISCARD fields size should be 1");
  }
  const auto* metadata = CastMapFieldPtr(fields[0], "PULL/DISCARD metadata");
  auto iter = metadata->find("n");
  if (iter == metadata->end()) {
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
  DropActiveBackend(session->backend_pool, session);
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

std::string ExceptionMessage(std::exception_ptr error) {
  if (!error) {
    return {};
  }
  try {
    std::rethrow_exception(error);
  } catch (const std::exception& e) {
    return e.what();
  } catch (...) {
    return "unknown backend error";
  }
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

void CancelActiveBackend(ProxySession* session) {
  auto backend = ActiveBackendSession(session);
  if (backend) {
    backend->Cancel();
  }
}

ProxySession::~ProxySession() {
  auto active = TakeActiveBackend(this);
  if (!active.has_value()) {
    return;
  }
  backend_pool->Drop(active->endpoint, std::move(active->backend));
}

void FailSession(const std::shared_ptr<BackendSessionPool>& pool,
                 const std::shared_ptr<bolt::BoltConnection>& conn,
                 ProxySession* session, const std::string& code,
                 const std::string& message) {
  DropActiveBackend(pool, session);
  SendFailure(conn, code, message);
  session->state = ProxySessionState::FAILED;
}

void MarkSessionInterrupted(const std::shared_ptr<ProxySession>& session) {
  session->RequestInterrupt();
  CancelActiveBackend(session.get());
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

bool IsBackendPoolExhausted(std::exception_ptr error) {
  if (!error) {
    return false;
  }
  try {
    std::rethrow_exception(error);
  } catch (const BackendPoolExhausted&) {
    return true;
  } catch (...) {
    return false;
  }
}

using LeaderCallback = std::function<void(std::exception_ptr, BackendEndpoint)>;

class DiscoverLeaderState
    : public std::enable_shared_from_this<DiscoverLeaderState> {
 public:
  DiscoverLeaderState(ShardRoute route,
                      std::unordered_map<std::string, std::any> hello_meta,
                      std::shared_ptr<BackendSessionPool> pool,
                      std::shared_ptr<ProxyIoPool> io_pool,
                      LeaderCallback callback)
      : route_(std::move(route)),
        hello_meta_(std::move(hello_meta)),
        pool_(std::move(pool)),
        io_pool_(std::move(io_pool)),
        callback_(std::move(callback)) {}

  void Start() {
    if (route_.replica_group == nullptr ||
        route_.replica_group->replicas.empty()) {
      callback_(std::make_exception_ptr(std::runtime_error(fmt::format(
                    "proxy shard {} has no raft replicas", route_.shard_id))),
                {});
      return;
    }
    TryNextReplica();
  }

 private:
  void TryNextReplica() {
    if (next_replica_ >= route_.replica_group->replicas.size()) {
      FinishFailure();
      return;
    }

    const auto replica = route_.replica_group->replicas[next_replica_++];
    auto self = shared_from_this();
    auto& io_service = io_pool_->Get();
    pool_->AsyncBorrow(
        replica, io_service, hello_meta_,
        [self, replica](std::exception_ptr error,
                        std::shared_ptr<BoltBackendSession> backend) mutable {
          self->HandleBorrowResult(replica, error, std::move(backend));
        });
  }

  void HandleBorrowResult(const BackendEndpoint& replica,
                          std::exception_ptr error,
                          std::shared_ptr<BoltBackendSession> backend) {
    if (error) {
      auto message = ExceptionMessage(error);
      if (IsBackendPoolExhausted(error)) {
        has_pool_exhaustion_ = true;
      } else {
        has_non_pool_error_ = true;
      }
      last_error_ = message;
      LOG_WARN("failed to discover leader for graph {} from {}:{}: {}",
               route_.graph_name, replica.host, replica.port, message);
      TryNextReplica();
      return;
    }
    if (!backend) {
      has_pool_exhaustion_ = true;
      last_error_ =
          fmt::format("backend connection pool exhausted for {}", replica.name);
      LOG_WARN("failed to discover leader for graph {} from {}:{}: {}",
               route_.graph_name, replica.host, replica.port, last_error_);
      TryNextReplica();
      return;
    }

    auto self = shared_from_this();
    backend->AsyncFetchRaftNodeInfos(
        route_.graph_name,
        [self, replica, backend = std::move(backend)](
            std::exception_ptr error,
            std::vector<RaftNodeEndpoint> node_infos) mutable {
          self->HandleReplicaResult(replica, std::move(backend), error,
                                    std::move(node_infos));
        });
  }

  void HandleReplicaResult(const BackendEndpoint& replica,
                           std::shared_ptr<BoltBackendSession> backend,
                           std::exception_ptr error,
                           std::vector<RaftNodeEndpoint> node_infos) {
    if (error) {
      auto message = ExceptionMessage(error);
      pool_->Drop(replica, std::move(backend));
      has_non_pool_error_ = true;
      last_error_ = message;
      LOG_WARN("failed to discover leader for graph {} from {}:{}: {}",
               route_.graph_name, replica.host, replica.port, message);
      TryNextReplica();
      return;
    }

    pool_->Return(replica, std::move(backend));
    for (const auto& node_info : node_infos) {
      if (!node_info.is_leader) {
        continue;
      }
      auto leader = ResolveConfiguredEndpoint(*route_.replica_group, node_info);
      LOG_INFO("proxy discovered leader for graph {}: {}:{} node {}",
               route_.graph_name, leader.host, leader.port, leader.node_id);
      callback_(nullptr, std::move(leader));
      return;
    }
    last_error_ = "raft leader is not known";
    has_non_pool_error_ = true;
    TryNextReplica();
  }

  void FinishFailure() {
    if (has_pool_exhaustion_ && !has_non_pool_error_) {
      callback_(std::make_exception_ptr(BackendPoolExhausted(fmt::format(
                    "failed to discover raft leader for graph {}: {}",
                    route_.graph_name, last_error_))),
                {});
      return;
    }
    callback_(std::make_exception_ptr(std::runtime_error(
                  fmt::format("failed to discover raft leader for graph {}: {}",
                              route_.graph_name, last_error_))),
              {});
  }

  ShardRoute route_;
  std::unordered_map<std::string, std::any> hello_meta_;
  std::shared_ptr<BackendSessionPool> pool_;
  std::shared_ptr<ProxyIoPool> io_pool_;
  LeaderCallback callback_;
  size_t next_replica_ = 0;
  std::string last_error_;
  bool has_pool_exhaustion_ = false;
  bool has_non_pool_error_ = false;
};

void DiscoverLeaderAsync(
    const ShardRoute& route,
    const std::unordered_map<std::string, std::any>& hello_meta,
    const std::shared_ptr<BackendSessionPool>& pool,
    const std::shared_ptr<ProxyIoPool>& io_pool, LeaderCallback callback) {
  std::make_shared<DiscoverLeaderState>(route, hello_meta, pool, io_pool,
                                        std::move(callback))
      ->Start();
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

void HandleProxyError(const std::shared_ptr<ProxyContext>& context,
                      const std::shared_ptr<bolt::BoltConnection>& conn,
                      ProxySession* session, std::exception_ptr error) {
  try {
    std::rethrow_exception(error);
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
  } catch (...) {
    LOG_WARN("proxy message failed with unknown exception");
    if (!conn->has_closed()) {
      FailSession(context->backend_pool, conn, session, kNetworkError,
                  "unknown proxy error");
    }
  }
}

using MessageDone = std::function<void()>;

void PostSessionContinuation(
    const std::shared_ptr<ProxyContext>& context,
    const std::shared_ptr<bolt::BoltConnection>& conn,
    const std::shared_ptr<ProxySessionContext>& session_context,
    std::function<void()> task) {
  if (!session_context->io_service || !session_context->strand) {
    DropActiveBackend(context->backend_pool, session_context->session.get());
    conn->Close();
    return;
  }
  session_context->io_service->post(session_context->strand->wrap(
      [task = std::move(task)]() mutable { task(); }));
}

class RunRequestState : public std::enable_shared_from_this<RunRequestState> {
 public:
  RunRequestState(std::shared_ptr<ProxyContext> context,
                  std::shared_ptr<bolt::BoltConnection> conn,
                  std::shared_ptr<ProxySessionContext> session_context,
                  ShardRoute route, std::string shard_key, std::string request,
                  MessageDone done)
      : context_(std::move(context)),
        conn_(std::move(conn)),
        session_context_(std::move(session_context)),
        route_(std::move(route)),
        shard_key_(std::move(shard_key)),
        request_(std::move(request)),
        done_(std::move(done)) {}

  void Start() { TryAttempt(); }

 private:
  ProxySession* session() { return session_context_->session.get(); }

  void Post(std::function<void()> task) {
    PostSessionContinuation(context_, conn_, session_context_, std::move(task));
  }

  void TryAttempt() {
    if (conn_->has_closed() || IsSessionInterrupted(session())) {
      Complete();
      return;
    }
    if (attempt_ >= 2) {
      FailRoute();
      return;
    }

    const int attempt = attempt_++;
    selected_endpoint_ = {};
    selected_endpoint_from_cache_ = false;
    auto cached_leader = attempt == 0
                             ? context_->leader_cache->Get(route_.shard_id)
                             : std::nullopt;
    if (cached_leader.has_value()) {
      selected_endpoint_ = *cached_leader;
      selected_endpoint_from_cache_ = true;
      SendToSelectedEndpoint();
      return;
    }

    auto self = shared_from_this();
    DiscoverLeaderAsync(
        route_, session()->hello_meta, context_->backend_pool,
        context_->proxy_io_pool,
        [self](std::exception_ptr error, BackendEndpoint endpoint) mutable {
          self->Post([self, error, endpoint = std::move(endpoint)]() mutable {
            self->HandleLeader(error, std::move(endpoint));
          });
        });
  }

  void HandleLeader(std::exception_ptr error, BackendEndpoint endpoint) {
    if (conn_->has_closed() || IsSessionInterrupted(session())) {
      Complete();
      return;
    }
    if (error) {
      if (IsBackendPoolExhausted(error)) {
        HandleProxyError(context_, conn_, session(), error);
        Complete();
        return;
      }
      last_error_ = ExceptionMessage(error);
      TryAttempt();
      return;
    }
    selected_endpoint_ = std::move(endpoint);
    SendToSelectedEndpoint();
  }

  void SendToSelectedEndpoint() {
    if (conn_->has_closed() || IsSessionInterrupted(session())) {
      Complete();
      return;
    }

    auto self = shared_from_this();
    if (session_context_->io_service == nullptr) {
      DropActiveBackend(context_->backend_pool, session());
      conn_->Close();
      Complete();
      return;
    }
    auto& io_service = *session_context_->io_service;
    context_->backend_pool->AsyncBorrow(
        selected_endpoint_, io_service, session()->hello_meta,
        [self](std::exception_ptr error,
               std::shared_ptr<BoltBackendSession> backend) mutable {
          self->Post([self, error, backend = std::move(backend)]() mutable {
            self->HandleBorrowedBackend(error, std::move(backend));
          });
        });
  }

  void HandleBorrowedBackend(std::exception_ptr error,
                             std::shared_ptr<BoltBackendSession> backend) {
    if (conn_->has_closed()) {
      if (backend) {
        context_->backend_pool->Drop(selected_endpoint_, std::move(backend));
      }
      Complete();
      return;
    }
    if (IsSessionInterrupted(session())) {
      if (backend) {
        context_->backend_pool->Drop(selected_endpoint_, std::move(backend));
      }
      Complete();
      return;
    }
    if (error) {
      if (IsBackendPoolExhausted(error)) {
        HandleProxyError(context_, conn_, session(), error);
        Complete();
        return;
      }
      HandleAttemptError(ExceptionMessage(error));
      return;
    }
    if (!backend) {
      HandleAttemptError(
          fmt::format("failed to borrow backend {}", selected_endpoint_.name));
      return;
    }

    SetActiveBackend(session(), selected_endpoint_, backend);
    SendToBackend(std::move(backend));
  }

  void SendToBackend(std::shared_ptr<BoltBackendSession> backend) {
    auto self = shared_from_this();
    backend->AsyncSendAndReadUntilTerminal(
        request_, false,
        [self](std::exception_ptr error,
               std::vector<BackendMessage> messages) mutable {
          self->Post([self, error, messages = std::move(messages)]() mutable {
            self->HandleBackendMessages(error, std::move(messages));
          });
        });
  }

  void HandleBackendMessages(std::exception_ptr error,
                             std::vector<BackendMessage> messages) {
    if (conn_->has_closed()) {
      DropActiveBackendIf(context_->backend_pool, session(),
                          selected_endpoint_);
      Complete();
      return;
    }
    if (IsSessionInterrupted(session())) {
      Complete();
      return;
    }
    if (error) {
      HandleAttemptError(ExceptionMessage(error));
      return;
    }
    if (IsNotLeaderFailure(messages)) {
      last_error_ = messages.back().failure_message;
      DropActiveBackendIf(context_->backend_pool, session(),
                          selected_endpoint_);
      context_->leader_cache->InvalidateIf(route_.shard_id, selected_endpoint_);
      LOG_WARN("proxy stale leader for graph {} backend {}:{}: {}",
               route_.graph_name, selected_endpoint_.host,
               selected_endpoint_.port, last_error_);
      TryAttempt();
      return;
    }

    FinishWithMessages(std::move(messages));
  }

  void HandleAttemptError(const std::string& message) {
    last_error_ = message;
    DropActiveBackendIf(context_->backend_pool, session(), selected_endpoint_);
    if (!selected_endpoint_.name.empty()) {
      context_->leader_cache->InvalidateIf(route_.shard_id, selected_endpoint_);
    }
    LOG_WARN("proxy backend attempt failed for graph {} backend {}:{}: {}",
             route_.graph_name, selected_endpoint_.host,
             selected_endpoint_.port, message);
    TryAttempt();
  }

  void FinishWithMessages(std::vector<BackendMessage> messages) {
    if (messages.empty()) {
      FailRoute();
      return;
    }
    auto last = LastMessage(messages);
    ForwardMessages(conn_, messages);

    if (last.tag == bolt::BoltMsg::Success) {
      if (!selected_endpoint_from_cache_) {
        context_->leader_cache->Put(route_.shard_id, selected_endpoint_);
      }
      session()->state = ProxySessionState::STREAMING;
    } else {
      DropActiveBackendIf(context_->backend_pool, session(),
                          selected_endpoint_);
      session()->state = ProxySessionState::FAILED;
    }
    LOG_DEBUG("proxy routed shard_key [{}] to shard {} graph {} backend {}:{}",
              shard_key_, route_.shard_id, route_.graph_name,
              selected_endpoint_.host, selected_endpoint_.port);
    Complete();
  }

  void FailRoute() {
    DropActiveBackendIf(context_->backend_pool, session(), selected_endpoint_);
    FailSession(context_->backend_pool, conn_, session(), kNetworkError,
                fmt::format("failed to route graph {} shard {}: {}",
                            route_.graph_name, route_.shard_id, last_error_));
    Complete();
  }

  void Complete() {
    if (done_) {
      auto done = std::move(done_);
      done();
    }
  }

  std::shared_ptr<ProxyContext> context_;
  std::shared_ptr<bolt::BoltConnection> conn_;
  std::shared_ptr<ProxySessionContext> session_context_;
  ShardRoute route_;
  std::string shard_key_;
  std::string request_;
  MessageDone done_;
  BackendEndpoint selected_endpoint_;
  std::string last_error_;
  int attempt_ = 0;
  bool selected_endpoint_from_cache_ = false;
};

void ProcessRun(const std::shared_ptr<ProxyContext>& context,
                const std::shared_ptr<bolt::BoltConnection>& conn,
                const std::shared_ptr<ProxySessionContext>& session_context,
                std::vector<std::any>& fields, MessageDone done) {
  auto session = session_context->session.get();
  auto& backend_pool = context->backend_pool;
  if (fields.size() < 3) {
    FailSession(backend_pool, conn, session, kArgumentError,
                "RUN requires cypher, parameters, and metadata fields");
    done();
    return;
  }
  if (fields.size() != 3) {
    FailSession(backend_pool, conn, session, kArgumentError,
                "RUN fields size should be 3");
    done();
    return;
  }

  auto* cypher = CastStringFieldPtr(fields[0], "RUN cypher");
  auto* params = CastMutableMapField(fields[1], "RUN parameters");
  auto* extra = CastMutableMapField(fields[2], "RUN metadata");

  auto shard_key_iter = params->find(kShardKeyParam);
  if (shard_key_iter == params->end()) {
    FailSession(backend_pool, conn, session, kArgumentError,
                "missing required routing parameter _shard_key_");
    done();
    return;
  }
  auto shard_key = AnyToShardKey(shard_key_iter->second);
  params->erase(shard_key_iter);

  const auto& shard_map = *context->shard_map;
  std::string logical_graph = shard_map.logical_graph();
  auto db_iter = extra->find("db");
  if (db_iter != extra->end() && db_iter->second.has_value()) {
    logical_graph = *CastStringFieldPtr(db_iter->second, "RUN metadata.db");
  }

  auto route = shard_map.Route(logical_graph, shard_key);
  (*extra)["db"] = route.graph_name;

  bolt::PackStream ps;
  ps.AppendRun(*cypher, *params, *extra);
  std::make_shared<RunRequestState>(context, conn, session_context, route,
                                    shard_key, std::move(ps.MutableBuffer()),
                                    std::move(done))
      ->Start();
}

void ProcessPullOrDiscard(
    const std::shared_ptr<ProxyContext>& context,
    const std::shared_ptr<bolt::BoltConnection>& conn,
    const std::shared_ptr<ProxySessionContext>& session_context,
    bolt::BoltMsg type, const std::vector<std::any>& fields, MessageDone done) {
  auto session = session_context->session.get();
  auto backend = ActiveBackendSession(session);
  if (!backend) {
    throw std::runtime_error("active backend session is missing");
  }
  if (IsSessionInterrupted(session)) {
    done();
    return;
  }

  int64_t n = ExtractPullN(fields);

  bolt::PackStream ps;
  if (type == bolt::BoltMsg::PullN) {
    ps.AppendPullN(n);
  } else {
    ps.AppendDiscardN(n);
  }
  auto request = std::move(ps.MutableBuffer());
  auto batcher = std::make_shared<ClientResponseBatcher>(conn, session);
  backend->AsyncSendAndForwardUntilTerminal(
      std::move(request),
      [batcher](const BackendMessage& message) {
        return batcher->Forward(message);
      },
      false,
      [context, conn, session_context, batcher, done = std::move(done)](
          std::exception_ptr error, BackendMessage last) mutable {
        PostSessionContinuation(
            context, conn, session_context,
            [context, conn, session_context, batcher, error,
             last = std::move(last), done = std::move(done)]() mutable {
              auto session = session_context->session.get();
              if (error) {
                HandleProxyError(context, conn, session, error);
                done();
                return;
              }
              batcher->Flush();
              if (IsSessionInterrupted(session)) {
                done();
                return;
              }
              if (last.tag == bolt::BoltMsg::Success &&
                  !last.success_has_more) {
                ReturnActiveBackend(context->backend_pool, session);
                session->state = ProxySessionState::READY;
              } else if (last.tag == bolt::BoltMsg::Success) {
                session->state = ProxySessionState::STREAMING;
              } else if (last.tag != bolt::BoltMsg::Success) {
                DropActiveBackend(context->backend_pool, session);
                session->state = ProxySessionState::FAILED;
              }
              done();
            });
      });
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

void ProcessReadyState(
    const std::shared_ptr<ProxyContext>& context,
    const std::shared_ptr<bolt::BoltConnection>& conn,
    const std::shared_ptr<ProxySessionContext>& session_context,
    bolt::BoltMsg type, std::vector<std::any>& fields, MessageDone done) {
  auto session = session_context->session.get();
  if (IsExplicitTransactionRequest(type)) {
    FailSession(context->backend_pool, conn, session, kRequestError,
                "explicit transactions are not supported by lgraph_proxy");
    done();
  } else if (type == bolt::BoltMsg::Route) {
    FailSession(context->backend_pool, conn, session, kRequestError,
                "routing is not supported by lgraph_proxy");
    done();
  } else if (type == bolt::BoltMsg::Run) {
    ProcessRun(context, conn, session_context, fields, done);
  } else {
    CloseProtocolError(conn, session, type);
    done();
  }
}

void ProcessStreamingState(
    const std::shared_ptr<ProxyContext>& context,
    const std::shared_ptr<bolt::BoltConnection>& conn,
    const std::shared_ptr<ProxySessionContext>& session_context,
    bolt::BoltMsg type, const std::vector<std::any>& fields, MessageDone done) {
  auto session = session_context->session.get();
  if (type == bolt::BoltMsg::PullN || type == bolt::BoltMsg::DiscardN) {
    ProcessPullOrDiscard(context, conn, session_context, type, fields, done);
  } else {
    CloseProtocolError(conn, session, type);
    done();
  }
}

void ProcessProxyMessage(
    const std::shared_ptr<ProxyContext>& context,
    const std::shared_ptr<bolt::BoltConnection>& conn,
    const std::shared_ptr<ProxySessionContext>& session_context,
    ProxyMessage message, MessageDone done) {
  auto session = session_context->session.get();
  try {
    if (HandleInterruptedSessionMessage(context->backend_pool, conn, session,
                                        message.type)) {
      done();
      return;
    }

    switch (session->state) {
      case ProxySessionState::FAILED:
        ProcessRecoverableState(conn, session, message.type);
        done();
        break;
      case ProxySessionState::READY:
        ProcessReadyState(context, conn, session_context, message.type,
                          message.fields, done);
        break;
      case ProxySessionState::STREAMING:
        ProcessStreamingState(context, conn, session_context, message.type,
                              message.fields, done);
        break;
      case ProxySessionState::DEFUNCT:
        CloseProtocolError(conn, session, message.type);
        done();
        break;
    }
  } catch (...) {
    HandleProxyError(context, conn, session, std::current_exception());
    if (done) {
      done();
    }
  }
}

void ProcessNextSessionMessage(
    const std::shared_ptr<ProxyContext>& context,
    const std::shared_ptr<bolt::BoltConnection>& conn,
    const std::shared_ptr<ProxySessionContext>& session_context) {
  if (session_context->processing || conn->has_closed()) {
    return;
  }
  if (session_context->pending_messages.empty()) {
    return;
  }

  auto message = std::move(session_context->pending_messages.front());
  session_context->pending_messages.pop_front();
  session_context->processing = true;

  ProcessProxyMessage(context, conn, session_context, std::move(message),
                      [context, conn, session_context]() {
                        session_context->processing = false;
                        ProcessNextSessionMessage(context, conn,
                                                  session_context);
                      });
}

bool EnqueueSessionMessage(const std::shared_ptr<ProxyContext>& context,
                           bolt::BoltConnection& conn,
                           std::shared_ptr<ProxySessionContext> session_context,
                           ProxyMessage message) {
  if (!session_context->io_service || !session_context->strand) {
    DropActiveBackend(context->backend_pool, session_context->session.get());
    conn.Close();
    return false;
  }
  session_context->io_service->post(session_context->strand->wrap(
      [context, conn = conn.shared_from_this(), session_context,
       message = std::move(message)]() mutable {
        if (!conn->has_closed()) {
          session_context->pending_messages.emplace_back(std::move(message));
          ProcessNextSessionMessage(context, conn, session_context);
        }
      }));
  return true;
}

void HandleHello(const std::shared_ptr<ProxyContext>& context,
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
  auto session_context = std::make_shared<ProxySessionContext>(
      context->proxy_io_pool->MakeStrand());
  auto session = session_context->session;
  session->hello_meta = hello_meta;
  session->backend_pool = context->backend_pool;
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
NewProxyHandler(const ShardMap& shard_map, uint32_t proxy_io_thread_num,
                size_t backend_max_connections_per_backend,
                std::chrono::milliseconds backend_borrow_timeout) {
  auto context = std::make_shared<ProxyContext>(
      shard_map, backend_max_connections_per_backend, backend_borrow_timeout,
      proxy_io_thread_num);
  return [context](bolt::BoltConnection& conn, bolt::BoltMsg msg,
                   std::vector<std::any> fields) mutable {
    if (msg == bolt::BoltMsg::Hello) {
      auto existing_session = GetSession(conn);
      if (existing_session) {
        LOG_WARN("receive duplicate proxy HELLO, close the connection");
        DropActiveBackend(context->backend_pool, existing_session.get());
        conn.Close();
        return;
      }
      try {
        HandleHello(context, conn, std::move(fields));
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
        DropActiveBackend(context->backend_pool, session.get());
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
        MarkSessionInterrupted(session);
      }
      EnqueueSessionMessage(context, conn, std::move(session_context),
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
