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

#include "common/value.h"
#include "server/galaxy.h"
#include "test_util.h"
#include "transaction/transaction.h"
namespace fs = std::filesystem;
std::string test_galaxy = "test_galaxy";
TEST(Galaxy, basic) {
  fs::remove_all(test_galaxy);
  auto galaxy = server::Galaxy::Open(test_galaxy, {});
  {
    auto graphDB = galaxy->OpenGraph("default");
    EXPECT_TRUE(graphDB != nullptr);
  }
  galaxy->CreateGraph("graph1");
  galaxy.reset(nullptr);
  galaxy = server::Galaxy::Open(test_galaxy, {});
  EXPECT_TRUE(galaxy->OpenGraph("default") != nullptr);
  EXPECT_TRUE(galaxy->OpenGraph("graph1") != nullptr);
  galaxy->DeleteGraph("graph1");
  EXPECT_THROW_CODE(galaxy->OpenGraph("graph1"), NoSuchGraph);
  galaxy.reset();
  galaxy = server::Galaxy::Open(test_galaxy, {});
  EXPECT_TRUE(galaxy->OpenGraph("default") != nullptr);
  EXPECT_THROW_CODE(galaxy->OpenGraph("graph1"), NoSuchGraph);
  galaxy.reset();
}

TEST(Galaxy, DISABLED_createGraph) {
  fs::remove_all(test_galaxy);
  auto galaxy = server::Galaxy::Open(test_galaxy, {});
  for (int i = 0; i < 10000; i++) {
    galaxy->CreateGraph("graph" + std::to_string(i));
  }
  std::this_thread::sleep_for(std::chrono::seconds(30));
}

TEST(Galaxy, clearGraph) {
  fs::remove_all(test_galaxy);
  auto galaxy = server::Galaxy::Open(test_galaxy, {});
  galaxy->CreateGraph("graph1");
  auto old_graph = galaxy->OpenGraph("graph1");
  auto old_graph_id = old_graph->db_meta().graph_id();
  auto old_path = old_graph->path();
  old_graph->AddVertexPropertyIndex("person_id", true, "person", {"id"});
  old_graph->AddVertexFullTextIndex("person_name_ft", {"person"}, {"name"});
  old_graph->AddVertexVectorIndex("person_embedding_vt", "person", "embedding",
                                  2, "l2", 16, 100);

  auto txn = old_graph->BeginTransaction();
  txn->CreateVertex({"person"},
                    {{"id", Value::Integer(1)},
                     {"name", Value::String("before_clear")},
                     {"embedding", Value::DoubleArray({1.0, 1.0})}});
  txn->Commit();

  auto cleared_graph = galaxy->ClearGraph("graph1");
  EXPECT_EQ(cleared_graph, old_graph.get());
  EXPECT_EQ(cleared_graph->db_meta().graph_id(), old_graph_id);
  EXPECT_EQ(cleared_graph->db_meta().graph_name(), "graph1");
  EXPECT_EQ(cleared_graph->path(), old_path);
  EXPECT_TRUE(fs::exists(old_path));
  EXPECT_EQ(cleared_graph->meta_info().GetVertexPropertyIndexes().size(), 1);
  EXPECT_EQ(cleared_graph->meta_info().GetVertexFullTextIndexes().size(), 1);
  EXPECT_EQ(cleared_graph->meta_info().GetVertexVectorIndexes().size(), 1);
  EXPECT_TRUE(cleared_graph->id_generator().GetVertexLabels().count("person"));
  EXPECT_TRUE(cleared_graph->id_generator().GetProperties().count("id"));
  EXPECT_TRUE(cleared_graph->id_generator().GetProperties().count("name"));
  EXPECT_TRUE(cleared_graph->id_generator().GetProperties().count("embedding"));

  auto cleared_txn = cleared_graph->BeginTransaction();
  int vertex_count = 0;
  for (auto viter = cleared_txn->NewVertexIterator(); viter->Valid();
       viter->Next()) {
    vertex_count++;
  }
  EXPECT_EQ(vertex_count, 0);
  cleared_txn->Commit();

  auto reload_txn = cleared_graph->BeginTransaction();
  auto alice = reload_txn->CreateVertex(
      {"person"}, {{"id", Value::Integer(2)},
                   {"name", Value::String("alice")},
                   {"embedding", Value::DoubleArray({2.0, 2.0})}});
  auto alice_id = alice.GetNativeId();
  reload_txn->Commit();

  auto ft_index =
      cleared_graph->meta_info().GetVertexFullTextIndex("person_name_ft");
  ASSERT_TRUE(ft_index != nullptr);
  ft_index->ApplyWAL();

  auto vector_index =
      cleared_graph->meta_info().GetVertexVectorIndex("person_embedding_vt");
  ASSERT_TRUE(vector_index != nullptr);
  vector_index->ApplyWAL();

  auto verify_txn = cleared_graph->BeginTransaction();
  auto viter =
      verify_txn->QueryVertexByPropertyIndex("person_id", Value::Integer(2));
  ASSERT_TRUE(viter->Valid());
  EXPECT_EQ(viter->GetVertex().GetNativeId(), alice_id);
  viter->Next();
  EXPECT_FALSE(viter->Valid());

  int ft_count = 0;
  for (auto result =
           verify_txn->QueryVertexByFTIndex("person_name_ft", "alice", 10);
       result->Valid(); result->Next()) {
    ft_count++;
    EXPECT_EQ(result->GetVertexScore().vertex.GetNativeId(), alice_id);
  }
  EXPECT_EQ(ft_count, 1);

  int vector_count = 0;
  for (auto result = verify_txn->QueryVertexByKnnSearch("person_embedding_vt",
                                                        {2.0f, 2.0f}, 10, 100);
       result->Valid(); result->Next()) {
    vector_count++;
    EXPECT_EQ(result->GetVertexScore().vertex.GetNativeId(), alice_id);
  }
  EXPECT_EQ(vector_count, 1);
  verify_txn->Commit();

  txn.reset();
  old_graph.reset();
  EXPECT_TRUE(fs::exists(old_path));
}
