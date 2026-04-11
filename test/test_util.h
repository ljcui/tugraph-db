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

#pragma once

#include <array>
#include <chrono>
#include <filesystem>
#include <string_view>
#include <thread>

#include "graphdb/graph_db.h"

#define EXPECT_THROW_CODE(statement, error_code)                \
  {                                                             \
    try {                                                       \
      statement;                                                \
      FAIL() << "Expecting exception, but nothing is thrown.";  \
    } catch (LgraphException & e) {                             \
      if (e.code() != ErrorCode::error_code) {                  \
        FAIL() << "Unexpected exception message: " << e.what(); \
      } else {                                                  \
        SUCCEED() << "Expected exception: " << e.what();        \
      }                                                         \
    } catch (std::exception & e) {                              \
      FAIL() << "Unexpected exception message: " << e.what();   \
    }                                                           \
  }

#define EXPECT_THROW_CODE_MSG(statement, error_code, msg)       \
  {                                                             \
    try {                                                       \
      statement;                                                \
      FAIL() << "Expecting exception, but nothing is thrown.";  \
    } catch (LgraphException & e) {                             \
      if (e.code() != ErrorCode::error_code) {                  \
        FAIL() << "Unexpected exception message: " << e.what(); \
      } else {                                                  \
        std::string what = e.what();                            \
        if (what.find(msg) != what.npos) {                      \
          SUCCEED() << "Expected exception: " << e.what();      \
        } else {                                                \
          FAIL() << "Unexpected exception message: " << what;   \
        }                                                       \
      }                                                         \
    } catch (std::exception & e) {                              \
      FAIL() << "Unexpected exception message: " << e.what();   \
    }                                                           \
  }

inline bool WaitUntilPropertyIndexReady(
    graphdb::GraphDB* graph_db, const std::string& index_name,
    std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (graph_db->meta_info().GetReadyVertexPropertyIndex(index_name)) {
      return true;
    }
    auto index = graph_db->meta_info().GetVertexPropertyIndex(index_name);
    if (index && index->state() == meta::IndexBuildState::FAILED) {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return false;
}

inline bool WaitUntilPropertyIndexFailed(
    graphdb::GraphDB* graph_db, const std::string& index_name,
    std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    auto index = graph_db->meta_info().GetVertexPropertyIndex(index_name);
    if (index && index->state() == meta::IndexBuildState::FAILED) {
      return true;
    }
    if (graph_db->meta_info().GetReadyVertexPropertyIndex(index_name)) {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return false;
}

namespace testutil {

inline constexpr std::array<std::string_view, 7> kTestDataDirectories = {
    "testdb",       "cypher_testdb", "temporal_db", "test_galaxy",
    "test_ftindex", "testkv",        "varlendb"};

inline void CleanupTestDataDirectories() {
  std::error_code ec;
  for (const auto dir : kTestDataDirectories) {
    ec.clear();
    std::filesystem::remove_all(std::filesystem::path(dir), ec);
  }
}

}  // namespace testutil
