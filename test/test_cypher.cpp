/**
 * Copyright 2024 AntGroup CO., Ltd.
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

#include <gtest/gtest.h>

#include <filesystem>
#include <set>

#include "common/value.h"
#include "cypher/execution_plan/result_iterator.h"
#include "geax-front-end/ast/Ast.h"
#include "graphdb/graph_db.h"
#include "test_util.h"

using namespace graphdb;
namespace fs = std::filesystem;
static std::string testdb = "cypher_testdb";

TEST(Cypher, unwind_create_three_vertices) {
  fs::remove_all(testdb);
  auto graphDB = GraphDB::Open(testdb, testutil::NewGraphDBOptions());
  cypher::RTContext rtx;

  auto txn = graphDB->BeginTransaction();
  txn->Execute(&rtx, "unwind [1,2,3] as num create(n:test) set n.id = num;")
      ->Consume();
  txn->Commit();

  txn = graphDB->BeginTransaction();
  size_t count = 0;
  std::set<int64_t> ids;
  for (auto viter = txn->NewVertexIterator("test"); viter->Valid();
       viter->Next()) {
    count++;
    ids.insert(viter->GetVertex().GetProperty("id").AsInteger());
  }

  EXPECT_EQ(count, 3);
  EXPECT_EQ(ids, (std::set<int64_t>{1, 2, 3}));
  txn->Commit();
}

TEST(Cypher, create_redefine_local_alias_should_fail) {
  fs::remove_all(testdb);
  auto graphDB = GraphDB::Open(testdb, testutil::NewGraphDBOptions());
  cypher::RTContext rtx;

  auto txn = graphDB->BeginTransaction();
  EXPECT_THROW_CODE_MSG(
      txn->Execute(&rtx, "create(n:test {id:1}) create(n:test {id:2})"),
      InputError, "already defined");
}

TEST(Cypher, fulltext_query_rejects_non_positive_top_n) {
  fs::remove_all(testdb);
  auto graphDB = GraphDB::Open(testdb, testutil::NewGraphDBOptions());
  graphDB->AddVertexFullTextIndex("ft_index", {"test"}, {"name"});
  cypher::RTContext rtx;

  auto txn = graphDB->BeginTransaction();
  EXPECT_THROW_CODE_MSG(
      txn->Execute(&rtx,
                   "CALL db.index.fulltext.queryNodes('ft_index', 'alice', 0) "
                   "YIELD node RETURN node"),
      ReminderException, "top_n should be greater than 0");
  EXPECT_THROW_CODE_MSG(
      txn->Execute(&rtx,
                   "CALL db.index.fulltext.queryNodes('ft_index', 'alice', -1) "
                   "YIELD node RETURN node"),
      ReminderException, "top_n should be greater than 0");
  txn->Rollback();
}

TEST(Cypher, ast_cache_keeps_bolt_parameters_runtime_bound) {
  fs::remove_all(testdb);
  auto graphDB = GraphDB::Open(testdb, testutil::NewGraphDBOptions());
  cypher::RTContext rtx;

  auto write_txn = graphDB->BeginTransaction();
  write_txn->CreateVertex({"AstCacheParam"}, {{"id", Value::Integer(1)},
                                              {"score", Value::Integer(10)}});
  write_txn->CreateVertex({"AstCacheParam"}, {{"id", Value::Integer(2)},
                                              {"score", Value::Integer(20)}});
  write_txn->Commit();

  auto txn = graphDB->BeginTransaction();
  auto set_params = [&](int64_t id, int64_t offset) {
    rtx.bolt_parameters_.clear();
    auto id_expr = rtx.obj_alloc_.allocate<geax::frontend::VInt>();
    id_expr->setVal(id);
    rtx.bolt_parameters_.emplace("$id", id_expr);
    auto offset_expr = rtx.obj_alloc_.allocate<geax::frontend::VInt>();
    offset_expr->setVal(offset);
    rtx.bolt_parameters_.emplace("$offset", offset_expr);
  };

  const std::string query =
      "MATCH (n:AstCacheParam) WHERE n.id = $id "
      "WITH n, $offset AS o "
      "RETURN n.id AS id, n.score AS score, "
      "(n.score + o) AS total, (n.id + o) AS mixed";

  set_params(1, 10);
  auto iter = txn->Execute(&rtx, query);
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ(iter->GetRecord().size(), 4);
  EXPECT_EQ(iter->GetRecord()[0].ToString(), "1");
  EXPECT_EQ(iter->GetRecord()[1].ToString(), "10");
  EXPECT_EQ(iter->GetRecord()[2].ToString(), "20");
  EXPECT_EQ(iter->GetRecord()[3].ToString(), "11");
  iter->Consume();

  set_params(2, 20);
  iter = txn->Execute(&rtx, query);
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ(iter->GetRecord().size(), 4);
  EXPECT_EQ(iter->GetRecord()[0].ToString(), "2");
  EXPECT_EQ(iter->GetRecord()[1].ToString(), "20");
  EXPECT_EQ(iter->GetRecord()[2].ToString(), "40");
  EXPECT_EQ(iter->GetRecord()[3].ToString(), "22");
  iter->Consume();

  txn->Rollback();
}
