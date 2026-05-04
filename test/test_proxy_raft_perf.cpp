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

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <string>

namespace {

int RunShellCommand(const std::string& command) {
  return std::system(command.c_str());
}

}  // namespace

TEST(ProxyRaftPerfIntegration,
     DISABLED_Neo4jDriverThroughProxyThreeServerRaftGraphs) {
  if (RunShellCommand("python3 -c 'import neo4j'") != 0) {
    GTEST_SKIP() << "neo4j Python driver is not installed";
  }
  if (!std::filesystem::exists("./lgraph_server") ||
      !std::filesystem::exists("./lgraph_proxy")) {
    GTEST_SKIP() << "lgraph_server and lgraph_proxy must exist in build dir";
  }

  const char* script_env = std::getenv("LGRAPH_PROXY_RAFT_PERF_SCRIPT");
  const char* extra_args_env = std::getenv("LGRAPH_PROXY_RAFT_PERF_ARGS");
  std::string command =
      std::string("python3 ") +
      (script_env ? script_env
                  : "../test/scripts/proxy_raft_perf_integration.py") +
      " --build-dir .";
  if (extra_args_env != nullptr) {
    command += " ";
    command += extra_args_env;
  }
  EXPECT_EQ(RunShellCommand(command), 0);
}
