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

#include <spdlog/fmt/chrono.h>

#include <boost/algorithm/string.hpp>
#include <cctype>
#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

#include "bolt/connection.h"
#include "bolt/worker_pool.h"
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

struct BoltSessionContext {
  explicit BoltSessionContext(bolt::BoltWorkerPool::Strand strand)
      : session(std::make_shared<BoltSession>()), strand(std::move(strand)) {}

  std::shared_ptr<BoltSession> session;
  bolt::BoltWorkerPool::Strand strand;
};

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
  session->ps.Reset();
}

static void RequestSessionInterrupt(BoltSession* session) {
  session->RequestInterrupt();
}

static bool ConsumeSessionInterrupt(BoltSession* session) {
  return session->ConsumeInterrupt();
}

static bool IsSessionInterrupted(BoltSession* session) {
  return session->HasInterrupt();
}

static bool IsExplicitTransactionRequest(BoltMsg type) {
  return type == BoltMsg::Begin || type == BoltMsg::Commit ||
         type == BoltMsg::Rollback;
}

static bool IsSessionRequest(BoltMsg type) {
  return type == BoltMsg::Run || type == BoltMsg::PullN ||
         type == BoltMsg::DiscardN || type == BoltMsg::Route ||
         IsExplicitTransactionRequest(type);
}

static bool IsResetMessage(BoltMsg type) { return type == BoltMsg::Reset; }

static std::string_view SessionStateName(SessionState state) {
  switch (state) {
    case SessionState::READY:
      return "READY";
    case SessionState::STREAMING:
      return "STREAMING";
    case SessionState::TX_READY:
      return "TX_READY";
    case SessionState::TX_STREAMING:
      return "TX_STREAMING";
    case SessionState::FAILED:
      return "FAILED";
    case SessionState::DEFUNCT:
      return "DEFUNCT";
  }
  return "UNKNOWN";
}

static void PostSuccess(const std::shared_ptr<BoltConnection>& conn) {
  bolt::PackStream ps;
  ps.AppendSuccess();
  conn->PostResponse(std::move(ps.MutableBuffer()));
}

static void PostIgnored(const std::shared_ptr<BoltConnection>& conn) {
  bolt::PackStream ps;
  ps.AppendIgnored();
  conn->PostResponse(std::move(ps.MutableBuffer()));
}

static void PostFailure(const std::shared_ptr<BoltConnection>& conn,
                        ErrorCode code, const std::string& msg) {
  bolt::PackStream ps;
  ps.AppendFailure({{"code", ErrorCodeToString(code)}, {"message", msg}});
  conn->PostResponse(std::move(ps.MutableBuffer()));
}

static bool InterruptedOrClosed(const std::shared_ptr<BoltConnection>& conn,
                                BoltSession* session) {
  if (conn->has_closed()) {
    LOG_INFO("The bolt connection is closed, cancel the op execution.");
    return true;
  }
  if (IsSessionInterrupted(session)) {
    LOG_INFO("The bolt session is interrupted, cancel the op execution.");
    return true;
  }
  return false;
}

static void FailSession(const std::shared_ptr<BoltConnection>& conn,
                        BoltSession* session, ErrorCode code,
                        const std::string& msg) {
  AbortActiveQuery(session);
  PostFailure(conn, code, msg);
  session->state = SessionState::FAILED;
}

static void CloseProtocolError(const std::shared_ptr<BoltConnection>& conn,
                               BoltSession* session, BoltMsg type) {
  LOG_ERROR("Unexpected msg:{} in {} state, close the connection",
            ToString(type), SessionStateName(session->state));
  AbortActiveQuery(session);
  session->state = SessionState::DEFUNCT;
  conn->Close();
}

static void ProcessRecoverableState(const std::shared_ptr<BoltConnection>& conn,
                                    BoltSession* session, BoltMsg type) {
  if (IsSessionRequest(type)) {
    PostIgnored(conn);
  } else {
    CloseProtocolError(conn, session, type);
  }
}

static void FailUnsupportedRequest(const std::shared_ptr<BoltConnection>& conn,
                                   BoltSession* session, BoltMsg type,
                                   std::string_view feature) {
  std::string err = fmt::format(
      "The {} feature is not currently supported for Bolt connections.",
      feature);
  LOG_ERROR("Receive {}, but {}", ToString(type), err);
  FailSession(conn, session, ErrorCode::Unimplemented, err);
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
  try {
    if (!session->active_query || !session->active_query->result) {
      THROW_CODE(InputError, "{} requires an active result stream",
                 bolt::ToString(type));
    }

    const int64_t n = ExtractPullOrDiscardN(type, fields);
    const bool unlimited = n < 0;
    int64_t remaining = n;
    auto* result = session->active_query->result.get();

    while (result->Valid() && (unlimited || remaining > 0)) {
      if (InterruptedOrClosed(conn, session)) {
        AbortActiveQuery(session);
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

    if (result->Valid()) {
      session->ps.AppendSuccessHasMore(true);
      session->state = SessionState::STREAMING;
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
    session->state = SessionState::READY;
    session->ps.AppendSuccess();
    FlushSessionBuffer(conn, session);
    LOG_DEBUG("Cypher execution completed");
    QUERY_LOG("{} {} {}", graph_name, elapsed, cypher.substr(0, 256));
  } catch (const LgraphException& e) {
    LOG_ERROR("{}", e.msg());
    FailSession(conn, session, e.code(), e.msg());
  } catch (std::exception& e) {
    LOG_ERROR("{}", e.what());
    FailSession(conn, session, ErrorCode::UnknownError, e.what());
  }
}

static void ProcessRun(Galaxy* galaxy,
                       const std::shared_ptr<BoltConnection>& conn,
                       BoltSession* session, std::vector<std::any>& fields) {
  try {
    if (fields.size() != 3) {
      THROW_CODE(InputError, "Run msg fields size error, size: {}",
                 fields.size());
    }
    auto* cypher = std::any_cast<std::string>(&fields[0]);
    auto* params =
        std::any_cast<std::unordered_map<std::string, std::any>>(&fields[1]);
    auto* extra =
        std::any_cast<std::unordered_map<std::string, std::any>>(&fields[2]);
    if (cypher == nullptr || params == nullptr || extra == nullptr) {
      THROW_CODE(InputError,
                 "Run msg fields should be (string, map, map), got ({}, {}, "
                 "{})",
                 fields[0].type().name(), fields[1].type().name(),
                 fields[2].type().name());
    }

    std::string graph;
    auto db_iter = extra->find("db");
    if (db_iter != extra->end()) {
      auto* db = std::any_cast<std::string>(&db_iter->second);
      if (db == nullptr) {
        THROW_CODE(InputError, "Run msg db metadata should be a string");
      }
      graph = *db;
    }
    const bool system_context = IsSystemDatabase(graph);
    if (system_context && !IsSystemProcedure(*cypher)) {
      THROW_CODE(InvalidParameter,
                 "system database only supports dbms.graph.* procedures");
    }

    auto active_query = std::make_unique<ActiveBoltQuery>();
    active_query->graph_name =
        system_context ? std::string(kSystemDatabaseName) : graph;
    active_query->cypher = *cypher;
    active_query->start_time = steady_clock::now();
    active_query->ctx = std::make_unique<cypher::RTContext>(
        galaxy, session->user, active_query->graph_name);
    for (auto& pair : *params) {
      active_query->ctx->bolt_parameters_.emplace(
          "$" + pair.first, ConvertParameters(active_query->ctx->obj_alloc_,
                                              std::move(pair.second)));
    }
    if (!system_context) {
      active_query->graph_db = galaxy->OpenGraph(graph);
      active_query->txn = active_query->graph_db->BeginTransaction();
      active_query->txn->SetConn(conn);
    }
    LOG_DEBUG("Execute {}", active_query->cypher.substr(0, 256));
    active_query->result = std::make_unique<ResultIterator>(
        active_query->ctx.get(), active_query->txn.get(), active_query->cypher);
    auto header = active_query->result->GetHeader();

    bolt::PackStream ps;
    ps.AppendSuccessFields(header);
    conn->PostResponse(std::move(ps.MutableBuffer()));
    session->active_query = std::move(active_query);
    session->state = bolt::SessionState::STREAMING;
  } catch (const LgraphException& e) {
    LOG_ERROR("{}", e.msg());
    FailSession(conn, session, e.code(), e.msg());
  } catch (std::exception& e) {
    LOG_ERROR("{}", e.what());
    FailSession(conn, session, ErrorCode::UnknownError, e.what());
  }
}

static void ProcessReadyState(Galaxy* galaxy,
                              const std::shared_ptr<BoltConnection>& conn,
                              BoltSession* session, BoltMsg type,
                              std::vector<std::any>& fields) {
  if (IsExplicitTransactionRequest(type)) {
    FailUnsupportedRequest(conn, session, type, "explicit transactions");
  } else if (type == bolt::BoltMsg::Route) {
    FailUnsupportedRequest(conn, session, type, "routing");
  } else if (type == bolt::BoltMsg::Run) {
    ProcessRun(galaxy, conn, session, fields);
  } else {
    CloseProtocolError(conn, session, type);
  }
}

static void ProcessStreamingState(const std::shared_ptr<BoltConnection>& conn,
                                  BoltSession* session, BoltMsg type,
                                  const std::vector<std::any>& fields) {
  if (type == bolt::BoltMsg::PullN || type == bolt::BoltMsg::DiscardN) {
    ProcessPullOrDiscard(conn, session, type, fields);
  } else {
    CloseProtocolError(conn, session, type);
  }
}

static void ProcessBoltMessage(Galaxy* galaxy,
                               const std::shared_ptr<BoltConnection>& conn,
                               BoltSession* session, BoltMsgDetail msg) {
  auto& fields = msg.fields;
  auto type = msg.type;

  if (IsSessionInterrupted(session)) {
    AbortActiveQuery(session);
    if (IsResetMessage(type)) {
      if (ConsumeSessionInterrupt(session)) {
        session->state = SessionState::READY;
        PostSuccess(conn);
      } else {
        PostIgnored(conn);
      }
    } else {
      PostIgnored(conn);
    }
    return;
  }

  switch (session->state) {
    case SessionState::FAILED:
      ProcessRecoverableState(conn, session, type);
      break;
    case SessionState::READY:
      ProcessReadyState(galaxy, conn, session, type, fields);
      break;
    case SessionState::STREAMING:
      ProcessStreamingState(conn, session, type, fields);
      break;
    case SessionState::TX_READY:
    case SessionState::TX_STREAMING:
    case SessionState::DEFUNCT:
      CloseProtocolError(conn, session, type);
      break;
  }
}

static std::shared_ptr<BoltSessionContext> GetSessionContext(
    BoltConnection& conn) {
  auto ctx = conn.GetContextShared();
  if (!ctx) {
    return {};
  }
  return std::static_pointer_cast<BoltSessionContext>(ctx);
}

static bool EnqueueSessionMessage(
    Galaxy* galaxy, const std::shared_ptr<bolt::BoltWorkerPool>& pool,
    BoltConnection& conn, std::shared_ptr<BoltSessionContext> context,
    BoltMsgDetail msg) {
  if (!pool->Post(context->strand, [galaxy, conn = conn.shared_from_this(),
                                    context, msg = std::move(msg)]() mutable {
        if (!conn->has_closed()) {
          ProcessBoltMessage(galaxy, conn, context->session.get(),
                             std::move(msg));
        }
        if (conn->has_closed()) {
          AbortActiveQuery(context->session.get());
        }
      })) {
    LOG_WARN("failed to schedule bolt session: worker pool is stopped");
    conn.Close();
    return false;
  }
  return true;
}

}  // namespace

BoltHandler NewBoltHandler(Galaxy* galaxy, BoltHandlerOptions options) {
  auto worker_pool = std::make_shared<bolt::BoltWorkerPool>(
      options.worker_thread_num, "bolt-worker-", "bolt");
  return [galaxy, options, worker_pool](BoltConnection& conn, BoltMsg msg,
                                        std::vector<std::any> fields) {
    if (msg == BoltMsg::Hello) {
      auto existing_context = GetSessionContext(conn);
      if (existing_context) {
        LOG_WARN("receive duplicate Bolt HELLO, close the connection");
        conn.Close();
        return;
      }
      if (fields.size() != 1) {
        LOG_ERROR("Hello msg fields size error, size: {}", fields.size());
        bolt::PackStream ps;
        ps.AppendFailure(
            {{"code", "error"}, {"message", "Hello msg fields size error"}});
        conn.Respond(std::move(ps.MutableBuffer()));
        conn.Close();
        return;
      }
      auto* val =
          std::any_cast<std::unordered_map<std::string, std::any>>(&fields[0]);
      if (val == nullptr) {
        std::string err = "Hello msg metadata should be a map";
        LOG_ERROR(err);
        bolt::PackStream ps;
        ps.AppendFailure({{"code", "error"}, {"message", err}});
        conn.Respond(std::move(ps.MutableBuffer()));
        conn.Close();
        return;
      }
      auto principal_iter = val->find("principal");
      auto credentials_iter = val->find("credentials");
      if (principal_iter == val->end() || credentials_iter == val->end()) {
        std::string err = "Miss 'principal' or 'credentials' in Hello msg";
        LOG_ERROR(err);
        bolt::PackStream ps;
        ps.AppendFailure({{"code", "error"}, {"message", err}});
        conn.Respond(std::move(ps.MutableBuffer()));
        conn.Close();
        return;
      }
      auto* principal = std::any_cast<std::string>(&principal_iter->second);
      auto* credentials = std::any_cast<std::string>(&credentials_iter->second);
      if (principal == nullptr || credentials == nullptr) {
        std::string err =
            "'principal' and 'credentials' in Hello msg should be strings";
        LOG_ERROR(err);
        bolt::PackStream ps;
        ps.AppendFailure({{"code", "error"}, {"message", err}});
        conn.Respond(std::move(ps.MutableBuffer()));
        conn.Close();
        return;
      }
      (void)credentials;
      /* TODO(anyone): wire real authentication through the server-owned galaxy.
       */
      std::unordered_map<std::string, std::any> meta;
      meta["connection_id"] =
          std::string("bolt") + std::to_string(conn.conn_id());
      // Neo4j python client check that the returned server info must start
      // with 'Neo4j/'
      meta["server"] = "Neo4j/tugraph-db";
      auto context =
          std::make_shared<BoltSessionContext>(worker_pool->MakeStrand());
      auto session = context->session;
      auto user_agent_iter = val->find("user_agent");
      if (user_agent_iter != val->end()) {
        auto* user_agent = std::any_cast<std::string>(&user_agent_iter->second);
        if (user_agent != nullptr &&
            boost::algorithm::starts_with(*user_agent, "neo4j-python")) {
          session->python_driver = true;
        }
      }
      auto patch_iter = val->find("patch_bolt");
      if (patch_iter != val->end()) {
        auto* patch = std::any_cast<std::vector<std::any>>(&patch_iter->second);
        if (patch != nullptr && patch->size() == 1) {
          auto* item = std::any_cast<std::string>(&(*patch)[0]);
          if (item != nullptr && *item == "utc") {
            session->utc_patch = true;
            meta["patch_bolt"] = std::vector<std::string>{"utc"};
          }
        }
      }
      session->state = SessionState::READY;
      session->user = *principal;
      conn.SetContext(context);
      bolt::PackStream ps;
      ps.AppendSuccess(meta);
      conn.Respond(std::move(ps.MutableBuffer()));
    } else if (msg == BoltMsg::Goodbye) {
      conn.Close();
      return;
    } else if (msg == BoltMsg::Run || msg == BoltMsg::PullN ||
               msg == BoltMsg::DiscardN || msg == BoltMsg::Begin ||
               msg == BoltMsg::Commit || msg == BoltMsg::Rollback ||
               msg == BoltMsg::Route || msg == BoltMsg::Reset) {
      auto context = GetSessionContext(conn);
      if (!context) {
        LOG_WARN("receive {} before Bolt HELLO, close the connection",
                 ToString(msg));
        conn.Close();
        return;
      }
      if (msg == BoltMsg::Reset) {
        RequestSessionInterrupt(context->session.get());
      }
      EnqueueSessionMessage(galaxy, worker_pool, conn, std::move(context),
                            {msg, std::move(fields)});
    } else {
      LOG_WARN("receive unknown bolt message: {}", ToString(msg));
      conn.Close();
    }
  };
}
}  // namespace server
