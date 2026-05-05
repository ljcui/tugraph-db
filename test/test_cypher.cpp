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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <numeric>
#include <set>
#include <vector>

#include "common/value.h"
#include "cypher/execution_plan/result_iterator.h"
#include "geax-front-end/ast/Ast.h"
#include "graphdb/graph_db.h"
#include "test_util.h"

using namespace graphdb;
namespace fs = std::filesystem;
static std::string testdb = "cypher_testdb";

namespace {

int PerfEnv(const char* name, int default_value) {
  const char* value = std::getenv(name);
  if (value == nullptr) return default_value;
  char* end = nullptr;
  long parsed = std::strtol(value, &end, 10);
  if (end == value || parsed <= 0 || parsed > std::numeric_limits<int>::max()) {
    return default_value;
  }
  return static_cast<int>(parsed);
}

float PerfEmbeddingValue(int seed, int dimension) {
  uint32_t x = static_cast<uint32_t>(seed + 1) * 747796405u;
  x += static_cast<uint32_t>(dimension + 11) * 2891336453u;
  x ^= x >> 16;
  x *= 2246822519u;
  x ^= x >> 13;
  x *= 3266489917u;
  x ^= x >> 16;
  return static_cast<float>((x & 0x00FFFFFFu) + 1u) / 16777216.0f;
}

std::vector<Value> MakePerfEmbedding(int seed, int dimensions) {
  std::vector<Value> embedding;
  embedding.reserve(dimensions);
  for (int i = 0; i < dimensions; ++i) {
    embedding.emplace_back(PerfEmbeddingValue(seed, i));
  }
  return embedding;
}

double TestNumericValueAsDouble(const Value& value) {
  if (value.IsFloat()) return static_cast<double>(value.AsFloat());
  if (value.IsDouble()) return value.AsDouble();
  return static_cast<double>(value.AsInteger());
}

double ExpectedCosineScore(int seed, const std::vector<Value>& query) {
  double dot = 0.0;
  double node_norm = 0.0;
  double query_norm = 0.0;
  for (size_t i = 0; i < query.size(); ++i) {
    double x =
        static_cast<double>(PerfEmbeddingValue(seed, static_cast<int>(i)));
    double y = TestNumericValueAsDouble(query[i]);
    dot += x * y;
    node_norm += x * x;
    query_norm += y * y;
  }
  return dot / (std::sqrt(node_norm) * std::sqrt(query_norm));
}

std::vector<std::pair<int, double>> ExpectedTop10(
    int candidates, const std::vector<Value>& query) {
  std::vector<std::pair<int, double>> scores;
  scores.reserve(candidates);
  for (int i = 0; i < candidates; ++i) {
    scores.emplace_back(i, ExpectedCosineScore(i, query));
  }
  std::sort(scores.begin(), scores.end(), [](const auto& lhs, const auto& rhs) {
    if (lhs.second != rhs.second) return lhs.second > rhs.second;
    return lhs.first < rhs.first;
  });
  if (scores.size() > 10) scores.resize(10);
  return scores;
}

void SetStringParam(cypher::RTContext* rtx, const std::string& name,
                    std::string value) {
  auto* expr = rtx->obj_alloc_.allocate<geax::frontend::VString>();
  expr->setVal(std::move(value));
  rtx->bolt_parameters_[name] = expr;
}

void SetVectorParam(cypher::RTContext* rtx, const std::string& name,
                    const std::vector<Value>& values) {
  auto* list = rtx->obj_alloc_.allocate<geax::frontend::MkList>();
  for (const auto& value : values) {
    auto* item = rtx->obj_alloc_.allocate<geax::frontend::VDouble>();
    if (value.IsFloat()) {
      item->setVal(static_cast<double>(value.AsFloat()));
    } else if (value.IsDouble()) {
      item->setVal(value.AsDouble());
    } else {
      item->setVal(static_cast<double>(value.AsInteger()));
    }
    list->appendElem(item);
  }
  rtx->bolt_parameters_[name] = list;
}

int RunVectorTop10Query(
    txn::Transaction* txn, cypher::RTContext* rtx, const std::string& query,
    const std::vector<std::pair<int, double>>* expected_top10) {
  auto result = txn->Execute(rtx, query);
  int rows = 0;
  double previous_score = std::numeric_limits<double>::infinity();
  while (result->Valid()) {
    const auto& record = result->GetRecord();
    if (expected_top10 != nullptr) {
      EXPECT_EQ(record.size(), 2);
      EXPECT_EQ(record[0].type, common::ResultType::Node);
      EXPECT_EQ(record[1].type, common::ResultType::Value);
      const auto& score = std::any_cast<const Value&>(record[1].data);
      EXPECT_TRUE(score.IsDouble());
      double current_score = score.AsDouble();
      EXPECT_LE(current_score, previous_score + 1e-12);
      previous_score = current_score;
      if (static_cast<size_t>(rows) < expected_top10->size()) {
        const auto& node = std::any_cast<const common::Node&>(record[0].data);
        auto id_iter = node.properties.find("id");
        EXPECT_NE(id_iter, node.properties.end());
        if (id_iter != node.properties.end()) {
          EXPECT_EQ(id_iter->second.AsInteger(), (*expected_top10)[rows].first);
        }
        EXPECT_NEAR(current_score, (*expected_top10)[rows].second, 1e-8);
      }
    }
    rows++;
    result->Next();
  }
  if (expected_top10 != nullptr) {
    EXPECT_EQ(rows, static_cast<int>(expected_top10->size()));
  }
  return rows;
}

}  // namespace

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

TEST(Cypher, vector_similarity_user_index_top10_perf) {
  fs::remove_all(testdb);
  auto graphDB = GraphDB::Open(testdb, testutil::NewGraphDBOptions());

  constexpr int kDimensions = 1024;
  constexpr char kLabel[] = "MemoryPerf";
  constexpr char kIndexName[] = "memory_perf_user_id";
  constexpr char kUserId[] = "perf_user";
  int candidates = PerfEnv("TUGRAPH_VECTOR_PERF_CANDIDATES", 2000);
  int query_count = PerfEnv("TUGRAPH_VECTOR_PERF_QUERIES", 30);

  graphDB->AddVertexVectorField(kLabel, "embedding", kDimensions);
  graphDB->AddVertexPropertyIndex(kIndexName, false, kLabel, {"user_id"});
  ASSERT_TRUE(WaitUntilPropertyIndexReady(graphDB.get(), kIndexName));

  for (int begin = 0; begin < candidates; begin += 250) {
    auto txn = graphDB->BeginTransaction();
    int end = std::min(begin + 250, candidates);
    for (int i = begin; i < end; ++i) {
      std::unordered_map<std::string, Value> properties;
      properties.emplace("id", Value::Integer(i));
      properties.emplace("user_id", Value::String(kUserId));
      properties.emplace("embedding",
                         Value::Array(MakePerfEmbedding(i, kDimensions)));
      txn->CreateVertex({kLabel}, properties);
    }
    txn->Commit();
  }

  cypher::RTContext rtx;
  SetStringParam(&rtx, "$index_name", kIndexName);
  SetStringParam(&rtx, "$user_id", kUserId);
  SetVectorParam(&rtx, "$embedding",
                 MakePerfEmbedding(candidates / 3, kDimensions));

  const std::string query =
      "CALL db.index.queryNodes($index_name, $user_id) YIELD node "
      "WITH node, vector.similarity.cosine(node.embedding, $embedding) AS "
      "score "
      "RETURN node AS n, score "
      "ORDER BY score DESC "
      "LIMIT 10";

  auto read_txn = graphDB->BeginTransaction();
  auto query_embedding = MakePerfEmbedding(candidates / 3, kDimensions);
  auto expected_top10 = ExpectedTop10(candidates, query_embedding);
  ASSERT_EQ(RunVectorTop10Query(read_txn.get(), &rtx, query, &expected_top10),
            10);

  auto second_query_embedding =
      MakePerfEmbedding(candidates * 2 / 3, kDimensions);
  SetVectorParam(&rtx, "$embedding", second_query_embedding);
  auto second_expected_top10 =
      ExpectedTop10(candidates, second_query_embedding);
  ASSERT_EQ(
      RunVectorTop10Query(read_txn.get(), &rtx, query, &second_expected_top10),
      10);

  SetVectorParam(&rtx, "$embedding", query_embedding);
  for (int i = 0; i < 3; ++i) {
    ASSERT_EQ(RunVectorTop10Query(read_txn.get(), &rtx, query, nullptr), 10);
  }

  std::vector<double> latencies_ms;
  latencies_ms.reserve(query_count);
  for (int i = 0; i < query_count; ++i) {
    auto start = std::chrono::steady_clock::now();
    int rows = RunVectorTop10Query(read_txn.get(), &rtx, query, nullptr);
    auto end = std::chrono::steady_clock::now();
    ASSERT_EQ(rows, 10);
    latencies_ms.push_back(
        std::chrono::duration<double, std::milli>(end - start).count());
  }
  read_txn->Rollback();

  std::sort(latencies_ms.begin(), latencies_ms.end());
  double total = std::accumulate(latencies_ms.begin(), latencies_ms.end(), 0.0);
  double avg = total / static_cast<double>(latencies_ms.size());
  double p50 = latencies_ms[latencies_ms.size() / 2];
  double p95 = latencies_ms[latencies_ms.size() * 95 / 100];
  std::cout << "[ PERF ] vector_similarity_user_index_top10 candidates="
            << candidates << " dimensions=" << kDimensions
            << " queries=" << query_count << " avg_ms=" << avg
            << " p50_ms=" << p50 << " p95_ms=" << p95 << std::endl;
}
