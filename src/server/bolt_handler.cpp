/**
 * Copyright 2022 AntGroup CO., Ltd.
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

//
// Created by botu.wzy
//

#include "server/bolt_handler.h"

#include <pthread.h>
#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/fmt.h>

#include <boost/algorithm/string.hpp>
#include <cctype>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>
#include <utility>

#include "bolt/connection.h"
#include "common/exceptions.h"
#include "common/logger.h"
#include "cypher/execution_plan/result_iterator.h"
#include "cypher/execution_plan/runtime_context.h"
#include "geax-front-end/ast/Ast.h"
#include "geax-front-end/common/ObjectAllocator.h"
#include "server/bolt_session.h"

using namespace bolt;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
DECLARE_bool(enable_query_log);

namespace bolt {
namespace {

constexpr std::string_view kSystemDatabaseName = "system";
constexpr std::string_view kSystemProcedurePrefix = "dbms.graph.";

std::string ToLowerAscii(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  for (char ch : text) {
    out.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  return out;
}

bool ConsumeWord(std::string_view text, size_t* pos, std::string_view word) {
  if (text.size() - *pos < word.size()) {
    return false;
  }
  auto candidate = ToLowerAscii(text.substr(*pos, word.size()));
  if (candidate != std::string(word)) {
    return false;
  }
  *pos += word.size();
  return true;
}

void SkipWhitespace(std::string_view text, size_t* pos) {
  while (*pos < text.size() &&
         std::isspace(static_cast<unsigned char>(text[*pos]))) {
    ++*pos;
  }
}

std::string ExtractLeadingProcedureName(std::string_view cypher) {
  size_t pos = 0;
  SkipWhitespace(cypher, &pos);
  if (!ConsumeWord(cypher, &pos, "call")) {
    return {};
  }
  SkipWhitespace(cypher, &pos);
  size_t start = pos;
  while (pos < cypher.size()) {
    char ch = cypher[pos];
    if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' ||
        ch == '.') {
      ++pos;
      continue;
    }
    break;
  }
  return ToLowerAscii(cypher.substr(start, pos - start));
}

bool IsSystemDatabase(std::string_view graph) {
  return graph.empty() || graph == kSystemDatabaseName;
}

bool IsSystemProcedure(std::string_view cypher) {
  auto procedure_name = ExtractLeadingProcedureName(cypher);
  return procedure_name.rfind(std::string(kSystemProcedurePrefix), 0) == 0;
}

}  // namespace

ActiveBoltQuery::~ActiveBoltQuery() { Rollback(); }

void ActiveBoltQuery::Commit() {
  result.reset();
  if (txn && !transaction_closed) {
    txn->Commit();
    transaction_closed = true;
  }
  txn.reset();
}

void ActiveBoltQuery::Rollback() noexcept {
  result.reset();
  if (txn && !transaction_closed) {
    try {
      txn->Rollback();
    } catch (const std::exception& e) {
      LOG_WARN("bolt active query rollback failed: {}", e.what());
    } catch (...) {
      LOG_WARN("bolt active query rollback failed with unknown exception");
    }
    transaction_closed = true;
  }
  txn.reset();
}

}  // namespace bolt

namespace server {

namespace {

class BoltWorkerPool {
 public:
  explicit BoltWorkerPool(uint32_t thread_num) {
    if (thread_num == 0) {
      thread_num = 1;
    }
    for (uint32_t i = 0; i < thread_num; ++i) {
      threads_.emplace_back([this, i]() { Run(i); });
    }
  }

  ~BoltWorkerPool() { Stop(); }

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
    std::string name = "bolt-worker-" + std::to_string(worker_id);
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
        LOG_ERROR("bolt worker task failed: {}", e.what());
      } catch (...) {
        LOG_ERROR("bolt worker task failed with unknown exception");
      }
    }
  }

  std::mutex mutex_;
  std::condition_variable condition_;
  std::deque<std::function<void()>> tasks_;
  std::vector<std::thread> threads_;
  bool stopped_ = false;
};

void ProcessSession(Galaxy* galaxy, std::shared_ptr<BoltConnection> conn,
                    std::shared_ptr<BoltSession> session,
                    std::weak_ptr<BoltWorkerPool> weak_pool);

}  // namespace

geax::frontend::Expr* ConvertParameters(
    geax::common::ObjectArenaAllocator& obj_alloc_, std::any data) {
  geax::frontend::Expr* ret;
  if (data.type() == typeid(std::string)) {
    ret = obj_alloc_.allocate<geax::frontend::VString>();
    auto& str = std::any_cast<std::string&>(data);
    ((geax::frontend::VString*)ret)->setVal(std::move(str));
  } else if (data.type() == typeid(int64_t)) {
    ret = obj_alloc_.allocate<geax::frontend::VInt>();
    ((geax::frontend::VInt*)ret)->setVal(std::any_cast<int64_t>(data));
  } else if (data.type() == typeid(double)) {
    ret = obj_alloc_.allocate<geax::frontend::VDouble>();
    ((geax::frontend::VDouble*)ret)->setVal(std::any_cast<double>(data));
  } else if (data.type() == typeid(bool)) {
    ret = obj_alloc_.allocate<geax::frontend::VBool>();
    ((geax::frontend::VBool*)ret)->setVal(std::any_cast<bool>(data));
  } else if (data.type() == typeid(void)) {
    ret = obj_alloc_.allocate<geax::frontend::VNull>();
  } else if (data.type() == typeid(std::unordered_map<std::string, std::any>)) {
    ret = obj_alloc_.allocate<geax::frontend::MkMap>();
    auto& map = std::any_cast<std::unordered_map<std::string, std::any>&>(data);
    for (auto& pair : map) {
      auto key = obj_alloc_.allocate<geax::frontend::VString>();
      std::string key_val = pair.first;
      key->setVal(std::move(key_val));
      ((geax::frontend::MkMap*)ret)
          ->appendElem(key,
                       ConvertParameters(obj_alloc_, std::move(pair.second)));
    }
  } else if (data.type() == typeid(std::vector<std::any>)) {
    ret = obj_alloc_.allocate<geax::frontend::MkList>();
    auto& list = std::any_cast<std::vector<std::any>&>(data);
    for (auto& item : list) {
      ((geax::frontend::MkList*)ret)
          ->appendElem(ConvertParameters(obj_alloc_, std::move(item)));
    }
  } else {
    THROW_CODE(InputError, "Unexpected cypher parameter type : {}",
               data.type().name());
  }
  return ret;
}

namespace {

static void FlushSessionBuffer(const std::shared_ptr<BoltConnection>& conn,
                               BoltSession* session) {
  if (session->ps.ConstBuffer().empty()) {
    return;
  }
  conn->PostResponse(std::move(session->ps.MutableBuffer()));
  session->ps.Reset();
}

static void AbortActiveQuery(BoltSession* session) {
  if (session->active_query) {
    session->active_query->Rollback();
    session->active_query.reset();
  }
  session->streaming_msg.reset();
  session->ps.Reset();
}

static int64_t ExtractPullOrDiscardN(BoltMsg type,
                                     const std::vector<std::any>& fields) {
  if (fields.size() != 1) {
    THROW_CODE(InputError, "{} msg fields size error, size: {}",
               bolt::ToString(type), fields.size());
  }
  auto* metadata =
      std::any_cast<std::unordered_map<std::string, std::any>>(&fields[0]);
  if (metadata == nullptr) {
    THROW_CODE(InputError, "{} metadata should be a map", bolt::ToString(type));
  }
  auto iter = metadata->find("n");
  if (iter == metadata->end()) {
    THROW_CODE(InputError, "{} metadata should contain n",
               bolt::ToString(type));
  }
  auto* n = std::any_cast<int64_t>(&iter->second);
  if (n == nullptr) {
    THROW_CODE(InputError, "{} n should be an integer", bolt::ToString(type));
  }
  if (*n == 0) {
    THROW_CODE(InputError, "{} n should not be 0", bolt::ToString(type));
  }
  return *n;
}

static void ProcessPullOrDiscard(const std::shared_ptr<BoltConnection>& conn,
                                 BoltSession* session, BoltMsg type,
                                 const std::vector<std::any>& fields) {
  if (!session->active_query || !session->active_query->result) {
    THROW_CODE(InputError, "{} requires an active result stream",
               bolt::ToString(type));
  }

  const int64_t n = ExtractPullOrDiscardN(type, fields);
  const bool unlimited = n < 0;
  int64_t remaining = n;
  auto* result = session->active_query->result.get();

  while (!conn->has_closed() && result->Valid() &&
         (unlimited || remaining > 0)) {
    if (session->interrupt_requested.load()) {
      session->state = SessionState::INTERRUPTED;
      AbortActiveQuery(session);
      LOG_INFO("The bolt session is interrupted, cancel the op execution.");
      return;
    }

    if (type == BoltMsg::PullN) {
      session->ps.AppendRecord(result->GetBoltRecord());
      if (session->ps.ConstBuffer().size() > 1024) {
        FlushSessionBuffer(conn, session);
      }
    }
    result->Next();
    if (!unlimited) {
      --remaining;
    }
  }

  if (conn->has_closed()) {
    AbortActiveQuery(session);
    LOG_INFO("The bolt connection is closed, cancel the op execution.");
    return;
  }
  if (session->interrupt_requested.load()) {
    session->state = SessionState::INTERRUPTED;
    AbortActiveQuery(session);
    LOG_INFO("The bolt session is interrupted, cancel the op execution.");
    return;
  }

  if (result->Valid()) {
    std::unordered_map<std::string, std::any> meta;
    meta["has_more"] = true;
    session->ps.AppendSuccess(meta);
    session->state = SessionState::STREAMING;
    session->streaming_msg.reset();
    FlushSessionBuffer(conn, session);
    return;
  }

  auto* active_query = session->active_query.get();
  auto elapsed = duration_cast<milliseconds>(steady_clock::now() -
                                             active_query->start_time);
  auto graph_name = active_query->graph_name;
  auto cypher = active_query->cypher;
  active_query->Commit();
  session->active_query.reset();
  session->streaming_msg.reset();
  session->state = SessionState::READY;
  session->ps.AppendSuccess();
  FlushSessionBuffer(conn, session);
  LOG_DEBUG("Cypher execution completed");
  QUERY_LOG("{} {} {}", graph_name, elapsed, cypher.substr(0, 256));
}

static void ProcessBoltMessage(Galaxy* galaxy,
                               const std::shared_ptr<BoltConnection>& conn,
                               BoltSession* session, BoltMsgDetail msg) {
  auto RespondFailure = [&conn, session](ErrorCode code,
                                         const std::string& msg) {
    AbortActiveQuery(session);
    bolt::PackStream ps;
    ps.AppendFailure({{"code", ErrorCodeToString(code)}, {"message", msg}});
    conn->PostResponse(std::move(ps.MutableBuffer()));
    session->state = SessionState::FAILED;
  };
  auto& fields = msg.fields;
  auto type = msg.type;
  if (session->state == SessionState::FAILED) {
    if (type == bolt::BoltMsg::Run || type == bolt::BoltMsg::PullN ||
        type == bolt::BoltMsg::DiscardN) {
      bolt::PackStream ps;
      ps.AppendIgnored();
      conn->PostResponse(std::move(ps.MutableBuffer()));
    } else if (type == bolt::BoltMsg::Reset) {
      AbortActiveQuery(session);
      bolt::PackStream ps;
      ps.AppendSuccess();
      conn->PostResponse(std::move(ps.MutableBuffer()));
      session->interrupt_requested.store(false);
      session->state = SessionState::READY;
    } else {
      LOG_ERROR(
          "Unexpected msg:{} in FAILED state, "
          "close the connection",
          ToString(type));
      conn->Close();
    }
  } else if (session->state == SessionState::INTERRUPTED) {
    if (type == bolt::BoltMsg::Run || type == bolt::BoltMsg::PullN ||
        type == bolt::BoltMsg::DiscardN || type == bolt::BoltMsg::Begin ||
        type == bolt::BoltMsg::Commit || type == bolt::BoltMsg::Rollback) {
      bolt::PackStream ps;
      ps.AppendIgnored();
      conn->PostResponse(std::move(ps.MutableBuffer()));
    } else if (type == bolt::BoltMsg::Reset) {
      AbortActiveQuery(session);
      bolt::PackStream ps;
      ps.AppendSuccess();
      conn->PostResponse(std::move(ps.MutableBuffer()));
      session->interrupt_requested.store(false);
      session->state = SessionState::READY;
    } else {
      LOG_ERROR(
          "Unexpected msg:{} in INTERRUPTED state, "
          "close the connection",
          ToString(type));
      conn->Close();
    }
  } else if (session->state == SessionState::READY) {
    if (type == bolt::BoltMsg::Begin) {
      std::string err = fmt::format(
          "Receive {}, but explicit transactions are "
          "not currently supported.",
          ToString(type));
      LOG_ERROR(err);
      bolt::PackStream ps;
      ps.AppendFailure({{"code", "error"}, {"message", err}});
      conn->PostResponse(std::move(ps.MutableBuffer()));
      session->state = SessionState::FAILED;
    } else if (type == bolt::BoltMsg::Reset) {
      AbortActiveQuery(session);
      bolt::PackStream ps;
      ps.AppendSuccess();
      conn->PostResponse(std::move(ps.MutableBuffer()));
      session->interrupt_requested.store(false);
      session->state = SessionState::READY;
    } else if (type == bolt::BoltMsg::Run) {
      try {
        if (fields.size() < 3) {
          THROW_CODE(InputError, "Run msg fields size error, size: {}",
                     fields.size());
        }
        auto& cypher = std::any_cast<const std::string&>(fields[0]);
        auto& extra =
            std::any_cast<const std::unordered_map<std::string, std::any>&>(
                fields[2]);
        std::string graph;
        auto db_iter = extra.find("db");
        if (db_iter != extra.end()) {
          graph = std::any_cast<const std::string&>(db_iter->second);
        }
        auto& field1 =
            std::any_cast<std::unordered_map<std::string, std::any>&>(
                fields[1]);
        const bool system_context = IsSystemDatabase(graph);
        if (system_context && !IsSystemProcedure(cypher)) {
          THROW_CODE(InvalidParameter,
                     "system database only supports dbms.graph.* procedures");
        }
        auto active_query = std::make_unique<ActiveBoltQuery>();
        active_query->graph_name =
            system_context ? std::string(kSystemDatabaseName) : graph;
        active_query->cypher = cypher;
        active_query->start_time = steady_clock::now();
        active_query->ctx = std::make_unique<cypher::RTContext>(
            galaxy, session->user, active_query->graph_name);
        for (auto& pair : field1) {
          active_query->ctx->bolt_parameters_.emplace(
              "$" + pair.first, ConvertParameters(active_query->ctx->obj_alloc_,
                                                  std::move(pair.second)));
        }
        session->streaming_msg.reset();
        session->interrupt_requested.store(false);
        if (!system_context) {
          active_query->graph_db = galaxy->OpenGraph(graph);
          active_query->txn = active_query->graph_db->BeginTransaction();
          active_query->txn->SetConn(conn);
        }
        LOG_DEBUG("Execute {}", cypher.substr(0, 256));
        active_query->result = std::make_unique<ResultIterator>(
            active_query->ctx.get(), active_query->txn.get(), cypher);
        auto header = active_query->result->GetHeader();

        std::unordered_map<std::string, std::any> meta;
        meta["fields"] = header;
        bolt::PackStream ps;
        ps.AppendSuccess(meta);
        conn->PostResponse(std::move(ps.MutableBuffer()));
        session->active_query = std::move(active_query);
        session->state = bolt::SessionState::STREAMING;
      } catch (const LgraphException& e) {
        LOG_ERROR(e.what());
        RespondFailure(e.code(), e.msg());
      } catch (std::exception& e) {
        LOG_ERROR(e.what());
        RespondFailure(ErrorCode::UnknownError, e.what());
      }
    } else {
      LOG_ERROR("Unexpected msg:{} in READY state, close the connection",
                ToString(type));
      conn->Close();
    }
  } else if (session->state == SessionState::STREAMING) {
    try {
      if (type == bolt::BoltMsg::PullN || type == bolt::BoltMsg::DiscardN) {
        ProcessPullOrDiscard(conn, session, type, fields);
      } else if (type == bolt::BoltMsg::Reset) {
        AbortActiveQuery(session);
        bolt::PackStream ps;
        ps.AppendSuccess();
        conn->PostResponse(std::move(ps.MutableBuffer()));
        session->interrupt_requested.store(false);
        session->state = SessionState::READY;
      } else {
        LOG_ERROR("Unexpected msg:{} in STREAMING state, close the connection",
                  ToString(type));
        conn->Close();
      }
    } catch (const LgraphException& e) {
      LOG_ERROR(e.what());
      RespondFailure(e.code(), e.msg());
    } catch (std::exception& e) {
      LOG_ERROR(e.what());
      RespondFailure(ErrorCode::UnknownError, e.what());
    }
  } else {
    LOG_ERROR("Unexpected msg:{} in session state, close the connection",
              ToString(type));
    conn->Close();
  }
}

static void ScheduleSession(Galaxy* galaxy,
                            const std::shared_ptr<BoltWorkerPool>& pool,
                            std::shared_ptr<BoltConnection> conn,
                            std::shared_ptr<BoltSession> session) {
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

  std::weak_ptr<BoltWorkerPool> weak_pool = pool;
  if (!pool->Post([galaxy, conn, session, weak_pool]() mutable {
        ProcessSession(galaxy, conn, session, weak_pool);
      })) {
    LOG_WARN("failed to schedule bolt session: worker pool is stopped");
    AbortActiveQuery(session.get());
    conn->Close();
  }
}

void ProcessSession(Galaxy* galaxy, std::shared_ptr<BoltConnection> conn,
                    std::shared_ptr<BoltSession> session,
                    std::weak_ptr<BoltWorkerPool> weak_pool) {
  while (!conn->has_closed()) {
    auto msg = session->msgs.TryPop();
    if (!msg) {
      break;
    }
    ProcessBoltMessage(galaxy, conn, session.get(), std::move(msg.value()));
  }
  if (conn->has_closed()) {
    AbortActiveQuery(session.get());
    return;
  }

  bool should_reschedule = false;
  {
    std::unique_lock<std::mutex> lock(session->schedule_mutex);
    session->scheduled = false;
    if (!conn->has_closed() && !session->msgs.Empty()) {
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
  if (!pool->Post([galaxy, conn, session, weak_pool]() mutable {
        ProcessSession(galaxy, conn, session, weak_pool);
      })) {
    LOG_WARN("failed to reschedule bolt session: worker pool is stopped");
    AbortActiveQuery(session.get());
    conn->Close();
  }
}

static std::shared_ptr<BoltSession> GetSession(BoltConnection& conn) {
  auto ctx = conn.GetContextShared();
  if (!ctx) {
    return {};
  }
  return std::static_pointer_cast<BoltSession>(ctx);
}

static bool EnqueueSessionMessage(Galaxy* galaxy,
                                  const std::shared_ptr<BoltWorkerPool>& pool,
                                  BoltConnection& conn,
                                  std::shared_ptr<BoltSession> session,
                                  BoltMsgDetail msg) {
  if (!session->msgs.Push(std::move(msg))) {
    LOG_WARN("close bolt connection {}: pending message queue is full",
             conn.conn_id());
    AbortActiveQuery(session.get());
    conn.Close();
    return false;
  }
  ScheduleSession(galaxy, pool, conn.shared_from_this(), std::move(session));
  return true;
}

}  // namespace

BoltHandler NewBoltHandler(Galaxy* galaxy, BoltHandlerOptions options) {
  auto worker_pool =
      std::make_shared<BoltWorkerPool>(options.worker_thread_num);
  return [galaxy, options, worker_pool](BoltConnection& conn, BoltMsg msg,
                                        std::vector<std::any> fields) {
    if (msg == BoltMsg::Hello) {
      if (fields.size() != 1) {
        LOG_ERROR("Hello msg fields size error, size: {}", fields.size());
        bolt::PackStream ps;
        ps.AppendFailure(
            {{"code", "error"}, {"message", "Hello msg fields size error"}});
        conn.Respond(std::move(ps.MutableBuffer()));
        conn.Close();
        return;
      }
      auto& val =
          std::any_cast<const std::unordered_map<std::string, std::any>&>(
              fields[0]);
      if (!val.count("principal") || !val.count("credentials")) {
        std::string err = "Miss 'principal' or 'credentials' in Hello msg";
        LOG_ERROR(err);
        bolt::PackStream ps;
        ps.AppendFailure({{"code", "error"}, {"message", err}});
        conn.Respond(std::move(ps.MutableBuffer()));
        conn.Close();
        return;
      }
      auto& principal = std::any_cast<const std::string&>(val.at("principal"));
      auto& credentials =
          std::any_cast<const std::string&>(val.at("credentials"));
      (void)credentials;
      /* TODO(anyone): wire real authentication through the server-owned galaxy.
       */
      std::unordered_map<std::string, std::any> meta;
      meta["connection_id"] =
          std::string("bolt") + std::to_string(conn.conn_id());
      // Neo4j python client check that the returned server info must start
      // with 'Neo4j/'
      meta["server"] = "Neo4j/tugraph-db";
      auto session = std::make_shared<BoltSession>(
          options.max_pending_messages_per_connection);
      if (val.count("user_agent")) {
        auto& user_agent =
            std::any_cast<const std::string&>(val.at("user_agent"));
        if (boost::algorithm::starts_with(user_agent, "neo4j-python")) {
          session->python_driver = true;
        }
      }
      if (val.count("patch_bolt")) {
        auto& patch =
            std::any_cast<const std::vector<std::any>&>(val.at("patch_bolt"));
        if (patch.size() == 1) {
          auto item = std::any_cast<std::string>(patch[0]);
          if (item == "utc") {
            session->utc_patch = true;
            meta["patch_bolt"] = std::vector<std::string>{"utc"};
          }
        }
      }
      session->state = SessionState::READY;
      session->user = principal;
      conn.SetContext(session);
      conn.MarkAuthenticated();
      bolt::PackStream ps;
      ps.AppendSuccess(meta);
      conn.Respond(std::move(ps.MutableBuffer()));
    } else if (msg == BoltMsg::Run || msg == BoltMsg::PullN ||
               msg == BoltMsg::DiscardN || msg == BoltMsg::Begin ||
               msg == BoltMsg::Commit || msg == BoltMsg::Rollback) {
      auto session = GetSession(conn);
      if (!session) {
        LOG_WARN("receive {} before Bolt HELLO, close the connection",
                 ToString(msg));
        conn.Close();
        return;
      }
      EnqueueSessionMessage(galaxy, worker_pool, conn, std::move(session),
                            {msg, std::move(fields)});
    } else if (msg == BoltMsg::Reset) {
      auto session = GetSession(conn);
      if (!session) {
        LOG_WARN("receive RESET before Bolt HELLO, close the connection");
        conn.Close();
        return;
      }
      session->interrupt_requested.store(true);
      EnqueueSessionMessage(galaxy, worker_pool, conn, std::move(session),
                            {BoltMsg::Reset, std::move(fields)});
    } else if (msg == BoltMsg::Goodbye) {
      auto session = GetSession(conn);
      if (session) {
        AbortActiveQuery(session.get());
      }
      conn.Close();
    } else {
      LOG_WARN("receive unknown bolt message: {}", ToString(msg));
      conn.Close();
    }
  };
}
}  // namespace server
