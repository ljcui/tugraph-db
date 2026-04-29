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

#include <algorithm>
#include <boost/asio.hpp>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common/value.h"
#include "etcd-raft-cpp/raft.h"
#include "graphdb/graph_db.h"
#include "proto/meta.pb.h"
#include "server/lgraph_server.h"
#include "transaction/transaction.h"

using namespace graphdb;
namespace fs = std::filesystem;

namespace {

constexpr char kGraphName[] = "raft_cluster_graph";

const std::unordered_map<std::string, Value> kProperties = {
    {"property1", Value::Bool(true)},
    {"property2", Value::Integer(100)},
    {"property3", Value::String("string")},
    {"property4", Value::Double(1.1314)},
    {"property5", Value::BoolArray({true, false})},
    {"property6", Value::IntegerArray({1, 2, 3})},
    {"property7", Value::StringArray({"string1", "string2"})},
    {"property8", Value::DoubleArray({11.11, 22.22})}};

bool WaitUntil(
    const std::function<bool()>& condition, std::chrono::milliseconds timeout,
    std::chrono::milliseconds interval = std::chrono::milliseconds(10)) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (condition()) {
      return true;
    }
    std::this_thread::sleep_for(interval);
  }
  return condition();
}

int32_t AllocateFreePort() {
  boost::asio::io_service service;
  boost::asio::ip::tcp::acceptor acceptor(
      service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));
  return static_cast<int32_t>(acceptor.local_endpoint().port());
}

struct TestServerConfig {
  uint64_t node_id = 0;
  int32_t bolt_port = 0;
  int32_t raft_port = 0;
  std::string data_path;
};

class TestServerCluster final {
 public:
  explicit TestServerCluster(std::string base_path)
      : base_path_(std::move(base_path)) {}

  ~TestServerCluster() { Stop(); }

  void Start() {
    Stop();
    fs::remove_all(base_path_);
    server_configs_ = BuildServerConfigs();
    node_infos_ = BuildNodeInfos(server_configs_);
    try {
      for (const auto& config : server_configs_) {
        server::LGraphServerOptions options;
        options.data_path = config.data_path;
        options.local_node_options.host = "127.0.0.1";
        options.local_node_options.bolt_port = config.bolt_port;
        options.bolt_io_thread_num = 1;
        options.local_node_options.raft_port = config.raft_port;

        auto server =
            std::make_unique<server::LGraphServer>(std::move(options));
        if (!server->Start()) {
          throw std::runtime_error("failed to start lgraph server");
        }
        servers_.emplace_back(std::move(server));
      }

      for (const auto& server : servers_) {
        server->galaxy()->CreateGraphWithRaft(kGraphName, node_infos_);
      }
    } catch (...) {
      Stop();
      throw;
    }
  }

  void Stop() {
    for (auto& server : servers_) {
      server->Stop();
    }
    servers_.clear();
    fs::remove_all(base_path_);
  }

  server::LGraphServer* WaitForLeader(
      std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
    size_t leader_index = 0;
    if (!WaitUntil(
            [this, &leader_index]() {
              auto found = FindConsistentLeaderIndex();
              if (!found.has_value()) {
                return false;
              }
              leader_index = *found;
              return true;
            },
            timeout)) {
      return nullptr;
    }
    return servers_[leader_index].get();
  }

  bool WaitForApplyIndex(
      uint64_t apply_index,
      std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
    return WaitUntil(
        [this, apply_index]() {
          return std::all_of(
              servers_.begin(), servers_.end(),
              [this, apply_index](const auto& server) {
                return OpenGraph(server.get())->GetRaftApplyIndex() >=
                       apply_index;
              });
        },
        timeout);
  }

  std::vector<server::LGraphServer*> servers() const {
    std::vector<server::LGraphServer*> ret;
    ret.reserve(servers_.size());
    for (const auto& server : servers_) {
      ret.push_back(server.get());
    }
    return ret;
  }

  std::string StatusSummary() const {
    std::ostringstream out;
    for (const auto& server : servers_) {
      auto graph = OpenGraph(server.get());
      auto status = graph->raft_driver()->GetRaftStatus();
      out << "[bolt_port=" << server->options().local_node_options.bolt_port
          << ", raft_port=" << server->options().local_node_options.raft_port
          << ", lead=" << status.s.basicStatus_.softState_.lead_ << ", state="
          << eraft::ToString(status.s.basicStatus_.softState_.raftState_)
          << ", apply=" << graph->GetRaftApplyIndex()
          << ", first_log=" << status.first_log
          << ", last_log=" << status.last_log << "]";
    }
    return out.str();
  }

 private:
  std::shared_ptr<GraphDB> OpenGraph(const server::LGraphServer* server) const {
    return server->galaxy()->OpenGraph(kGraphName);
  }

  std::vector<TestServerConfig> BuildServerConfigs() const {
    std::vector<TestServerConfig> configs;
    configs.reserve(3);
    std::unordered_set<int32_t> used_ports;
    for (size_t i = 0; i < 3; ++i) {
      TestServerConfig config;
      config.node_id = static_cast<uint64_t>(i + 1);
      do {
        config.bolt_port = AllocateFreePort();
      } while (!used_ports.insert(config.bolt_port).second);
      do {
        config.raft_port = AllocateFreePort();
      } while (!used_ports.insert(config.raft_port).second);
      config.data_path = base_path_ + "/node" + std::to_string(i + 1);
      configs.emplace_back(std::move(config));
    }
    return configs;
  }

  meta::RaftNodeInfos BuildNodeInfos(
      const std::vector<TestServerConfig>& server_configs) const {
    meta::RaftNodeInfos node_infos;
    for (const auto& config : server_configs) {
      meta::RaftNodeInfo node_info;
      node_info.set_node_id(config.node_id);
      node_info.set_ip("127.0.0.1");
      node_info.set_bolt_port(config.bolt_port);
      node_info.set_raft_poft(config.raft_port);
      node_info.set_graph(kGraphName);
      (*node_infos.mutable_nodes())[config.node_id] = node_info;
    }
    return node_infos;
  }

  std::optional<size_t> FindConsistentLeaderIndex() const {
    std::optional<size_t> leader_index;
    uint64_t leader_id = 0;

    for (size_t i = 0; i < servers_.size(); ++i) {
      auto graph = OpenGraph(servers_[i].get());
      auto status = graph->raft_driver()->GetRaftStatus();
      auto reported_leader = status.s.basicStatus_.softState_.lead_;
      if (reported_leader == 0) {
        return std::nullopt;
      }
      if (leader_id == 0) {
        leader_id = reported_leader;
      } else if (leader_id != reported_leader) {
        return std::nullopt;
      }
      if (status.s.basicStatus_.softState_.raftState_ == eraft::StateLeader) {
        if (leader_index.has_value()) {
          return std::nullopt;
        }
        leader_index = i;
      }
    }

    if (!leader_index.has_value()) {
      return std::nullopt;
    }
    if (server_configs_[*leader_index].node_id != leader_id) {
      return std::nullopt;
    }
    return leader_index;
  }

  std::string base_path_;
  std::vector<TestServerConfig> server_configs_;
  meta::RaftNodeInfos node_infos_;
  std::vector<std::unique_ptr<server::LGraphServer>> servers_;
};

TEST(RaftCluster, threeServersElectLeaderAndReplicateTransaction) {
  TestServerCluster cluster("testdb_raft_cluster");
  ASSERT_NO_THROW(cluster.Start());

  auto* leader = cluster.WaitForLeader(std::chrono::seconds(15));
  ASSERT_NE(leader, nullptr) << cluster.StatusSummary();

  auto leader_graph = leader->galaxy()->OpenGraph(kGraphName);
  auto txn = leader_graph->BeginTransaction();
  auto v1 = txn->CreateVertex({"label1"}, kProperties);
  auto v2 = txn->CreateVertex({"label2"}, kProperties);
  auto e1 = txn->CreateEdge(v1, v2, "edge_type12", kProperties);
  txn->Commit();

  const auto applied_index = leader_graph->GetRaftApplyIndex();
  ASSERT_GT(applied_index, 0U);
  ASSERT_TRUE(
      cluster.WaitForApplyIndex(applied_index, std::chrono::seconds(15)))
      << cluster.StatusSummary();

  for (auto* server : cluster.servers()) {
    auto graph = server->galaxy()->OpenGraph(kGraphName);
    EXPECT_GE(graph->GetRaftApplyIndex(), applied_index)
        << cluster.StatusSummary();
    auto read_txn = graph->BeginTransaction();
    auto persisted_v1 = read_txn->GetVertexById(v1.GetId());
    auto persisted_v2 = read_txn->GetVertexById(v2.GetId());
    auto persisted_e1 = read_txn->GetEdgeById(e1.GetTypeId(), e1.GetId());
    EXPECT_EQ(persisted_v1.GetAllProperty(), kProperties);
    EXPECT_EQ(persisted_v2.GetAllProperty(), kProperties);
    EXPECT_EQ(persisted_e1.GetAllProperty(), kProperties);
    read_txn->Commit();
  }
}

}  // namespace
