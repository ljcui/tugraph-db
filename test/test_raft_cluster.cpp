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
#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <filesystem>
#include <functional>
#include <future>
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
#include "raft/raft_driver.h"
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
  explicit TestServerCluster(std::string base_path, size_t node_count = 3)
      : base_path_(std::move(base_path)), node_count_(node_count) {}

  ~TestServerCluster() { Stop(); }

  void Start() {
    Stop();
    if (node_count_ == 0) {
      throw std::runtime_error("raft test cluster must have at least one node");
    }
    fs::remove_all(base_path_);
    server_configs_ = BuildServerConfigs();
    graph_node_infos_ = BuildNodeInfos(server_configs_, kGraphName);
    galaxy_node_infos_ =
        BuildNodeInfos(server_configs_, server::Galaxy::RaftGraphName());
    servers_.clear();
    servers_.resize(server_configs_.size());
    try {
      for (size_t i = 0; i < server_configs_.size(); ++i) {
        StartServer(i);
      }

      auto* galaxy_leader = WaitForGalaxyLeader(std::chrono::seconds(15));
      if (galaxy_leader == nullptr) {
        throw std::runtime_error("failed to elect galaxy raft leader: " +
                                 GalaxyStatusSummary());
      }
      galaxy_leader->galaxy()->CreateGraphWithRaft(kGraphName,
                                                   graph_node_infos_);
      if (!WaitForGraphCreated(std::chrono::seconds(15))) {
        throw std::runtime_error("failed to create graph on all servers: " +
                                 GalaxyStatusSummary());
      }
    } catch (...) {
      Stop();
      throw;
    }
  }

  void Stop() {
    for (auto& server : servers_) {
      if (server != nullptr) {
        server->Stop();
      }
    }
    servers_.clear();
    fs::remove_all(base_path_);
  }

  void StopServer(size_t index) {
    if (index >= servers_.size()) {
      throw std::out_of_range("raft test server index is out of range");
    }
    if (servers_[index] == nullptr) {
      return;
    }
    servers_[index]->Stop();
    servers_[index].reset();
  }

  void StartServer(size_t index) {
    if (index >= server_configs_.size()) {
      throw std::out_of_range("raft test server index is out of range");
    }
    if (index < servers_.size() && servers_[index] != nullptr) {
      return;
    }
    if (servers_.size() < server_configs_.size()) {
      servers_.resize(server_configs_.size());
    }
    auto server = MakeServer(server_configs_[index]);
    if (!server->Start()) {
      throw std::runtime_error("failed to start lgraph server");
    }
    servers_[index] = std::move(server);
  }

  server::LGraphServer* WaitForLeader(
      std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
    return WaitForLeader(kGraphName, timeout);
  }

  server::LGraphServer* WaitForLeader(
      const std::string& graph_name,
      std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
    auto leader_index = WaitForLeaderIndex(graph_name, timeout);
    if (!leader_index.has_value()) {
      return nullptr;
    }
    return servers_[*leader_index].get();
  }

  std::optional<size_t> WaitForLeaderIndex(
      std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
    return WaitForLeaderIndex(kGraphName, timeout);
  }

  std::optional<size_t> WaitForLeaderIndex(
      const std::string& graph_name,
      std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
    std::optional<size_t> leader_index;
    if (!WaitUntil(
            [this, &graph_name, &leader_index]() {
              leader_index = FindConsistentLeaderIndex(graph_name);
              return leader_index.has_value();
            },
            timeout)) {
      return std::nullopt;
    }
    return leader_index;
  }

  server::LGraphServer* WaitForGalaxyLeader(
      std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
    size_t leader_index = 0;
    if (!WaitUntil(
            [this, &leader_index]() {
              auto found = FindConsistentGalaxyLeaderIndex();
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

  std::optional<size_t> WaitForGalaxyLeaderIndex(
      std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
    std::optional<size_t> leader_index;
    if (!WaitUntil(
            [this, &leader_index]() {
              leader_index = FindConsistentGalaxyLeaderIndex();
              return leader_index.has_value();
            },
            timeout)) {
      return std::nullopt;
    }
    return leader_index;
  }

  bool WaitForGraphCreated(
      std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
    return WaitForGraphCreated(kGraphName, timeout);
  }

  bool WaitForGraphCreated(
      const std::string& graph_name,
      std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
    return WaitUntil(
        [this, &graph_name]() {
          return std::all_of(servers_.begin(), servers_.end(),
                             [&graph_name](const auto& server) {
                               if (server == nullptr) {
                                 return true;
                               }
                               try {
                                 server->galaxy()->OpenGraph(graph_name);
                                 return true;
                               } catch (const std::exception&) {
                                 return false;
                               }
                             });
        },
        timeout);
  }

  bool WaitForApplyIndex(
      uint64_t apply_index,
      std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
    return WaitForApplyIndex(kGraphName, apply_index, timeout);
  }

  bool WaitForApplyIndex(
      const std::string& graph_name, uint64_t apply_index,
      std::chrono::milliseconds timeout = std::chrono::seconds(10)) const {
    return WaitUntil(
        [this, &graph_name, apply_index]() {
          return std::all_of(
              servers_.begin(), servers_.end(),
              [this, &graph_name, apply_index](const auto& server) {
                if (server == nullptr) {
                  return true;
                }
                return OpenGraph(server.get(), graph_name)
                           ->GetRaftApplyIndex() >= apply_index;
              });
        },
        timeout);
  }

  std::vector<server::LGraphServer*> servers() const {
    std::vector<server::LGraphServer*> ret;
    ret.reserve(servers_.size());
    for (const auto& server : servers_) {
      if (server != nullptr) {
        ret.push_back(server.get());
      }
    }
    return ret;
  }

  server::LGraphServer* server(size_t index) const {
    if (index >= servers_.size() || servers_[index] == nullptr) {
      return nullptr;
    }
    return servers_[index].get();
  }

  size_t size() const { return servers_.size(); }

  uint64_t node_id(size_t index) const {
    if (index >= server_configs_.size()) {
      throw std::out_of_range("raft test server index is out of range");
    }
    return server_configs_[index].node_id;
  }

  meta::RaftNodeInfos NodeInfosForGraph(const std::string& graph_name) const {
    return BuildNodeInfos(server_configs_, graph_name);
  }

  std::string StatusSummary() const {
    std::ostringstream out;
    for (size_t i = 0; i < servers_.size(); ++i) {
      const auto& server = servers_[i];
      if (server == nullptr) {
        out << "[node_id=" << server_configs_[i].node_id << ", stopped]";
        continue;
      }
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

  std::string GalaxyStatusSummary() const {
    std::ostringstream out;
    for (size_t i = 0; i < servers_.size(); ++i) {
      const auto& server = servers_[i];
      if (server == nullptr) {
        out << "[node_id=" << server_configs_[i].node_id << ", stopped]";
        continue;
      }
      auto* raft_driver = server->galaxy()->galaxy_raft_driver();
      if (raft_driver == nullptr) {
        out << "[bolt_port=" << server->options().local_node_options.bolt_port
            << ", galaxy_raft=disabled]";
        continue;
      }
      auto status = raft_driver->GetRaftStatus();
      out << "[bolt_port=" << server->options().local_node_options.bolt_port
          << ", raft_port=" << server->options().local_node_options.raft_port
          << ", galaxy_lead=" << status.s.basicStatus_.softState_.lead_
          << ", galaxy_state="
          << eraft::ToString(status.s.basicStatus_.softState_.raftState_)
          << ", first_log=" << status.first_log
          << ", last_log=" << status.last_log << "]";
    }
    return out.str();
  }

 private:
  std::shared_ptr<GraphDB> OpenGraph(
      const server::LGraphServer* server,
      const std::string& graph_name = kGraphName) const {
    return server->galaxy()->OpenGraph(graph_name);
  }

  std::unique_ptr<server::LGraphServer> MakeServer(
      const TestServerConfig& config) const {
    server::LGraphServerOptions options;
    options.data_path = config.data_path;
    options.local_node_options.host = "127.0.0.1";
    options.local_node_options.bolt_port = config.bolt_port;
    options.galaxy_raft_node_infos = galaxy_node_infos_;
    options.bolt_io_thread_num = 1;
    options.local_node_options.raft_port = config.raft_port;
    return std::make_unique<server::LGraphServer>(std::move(options));
  }

  std::vector<TestServerConfig> BuildServerConfigs() const {
    std::vector<TestServerConfig> configs;
    configs.reserve(node_count_);
    std::unordered_set<int32_t> used_ports;
    for (size_t i = 0; i < node_count_; ++i) {
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
      const std::vector<TestServerConfig>& server_configs,
      const std::string& graph_name) const {
    meta::RaftNodeInfos node_infos;
    for (const auto& config : server_configs) {
      meta::RaftNodeInfo node_info;
      node_info.set_node_id(config.node_id);
      node_info.set_ip("127.0.0.1");
      node_info.set_bolt_port(config.bolt_port);
      node_info.set_raft_poft(config.raft_port);
      node_info.set_graph(graph_name);
      (*node_infos.mutable_nodes())[config.node_id] = node_info;
    }
    return node_infos;
  }

  std::optional<size_t> FindConsistentLeaderIndex(
      const std::string& graph_name) const {
    std::optional<size_t> leader_index;
    uint64_t leader_id = 0;
    bool has_running_server = false;

    for (size_t i = 0; i < servers_.size(); ++i) {
      if (servers_[i] == nullptr) {
        continue;
      }
      has_running_server = true;
      auto graph = OpenGraph(servers_[i].get(), graph_name);
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

    if (!has_running_server) {
      return std::nullopt;
    }
    if (!leader_index.has_value()) {
      return std::nullopt;
    }
    if (server_configs_[*leader_index].node_id != leader_id) {
      return std::nullopt;
    }
    return leader_index;
  }

  std::optional<size_t> FindConsistentGalaxyLeaderIndex() const {
    std::optional<size_t> leader_index;
    uint64_t leader_id = 0;
    bool has_running_server = false;

    for (size_t i = 0; i < servers_.size(); ++i) {
      if (servers_[i] == nullptr) {
        continue;
      }
      has_running_server = true;
      auto* raft_driver = servers_[i]->galaxy()->galaxy_raft_driver();
      if (raft_driver == nullptr) {
        return std::nullopt;
      }
      auto status = raft_driver->GetRaftStatus();
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

    if (!has_running_server) {
      return std::nullopt;
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
  size_t node_count_;
  std::vector<TestServerConfig> server_configs_;
  meta::RaftNodeInfos graph_node_infos_;
  meta::RaftNodeInfos galaxy_node_infos_;
  std::vector<std::unique_ptr<server::LGraphServer>> servers_;
};

raft::LocalNodeConfig MakeLocalNodeConfig(const std::string& graph_name) {
  raft::LocalNodeConfig local_node;
  local_node.graph = graph_name;
  local_node.ip = "127.0.0.1";
  local_node.bolt_port = AllocateFreePort();
  local_node.raft_poft = AllocateFreePort();
  return local_node;
}

raft::RaftLogStoreConfig MakeRaftLogStoreConfig(const std::string& path) {
  raft::RaftLogStoreConfig store_config;
  store_config.path = path;
  store_config.block_cache = 64;
  store_config.total_threads = 2;
  store_config.keep_logs = 100000;
  store_config.gc_interval = 1;
  return store_config;
}

raft::RaftConfig MakeRaftConfig() {
  raft::RaftConfig raft_config;
  raft_config.tick_interval = 100;
  raft_config.election_tick = 10;
  raft_config.heartbeat_tick = 1;
  return raft_config;
}

meta::RaftNodeInfo MakeNodeInfo(const raft::LocalNodeConfig& local_node,
                                uint64_t node_id) {
  meta::RaftNodeInfo node_info;
  node_info.set_node_id(node_id);
  node_info.set_graph(local_node.graph);
  node_info.set_ip(local_node.ip);
  node_info.set_bolt_port(local_node.bolt_port);
  node_info.set_raft_poft(local_node.raft_poft);
  return node_info;
}

std::vector<eraft::Peer> MakeInitPeers(const raft::LocalNodeConfig& local_node,
                                       uint64_t node_id = 1) {
  std::vector<eraft::Peer> init_peers;
  eraft::Peer peer;
  peer.id_ = node_id;
  peer.context_ = MakeNodeInfo(local_node, node_id).SerializeAsString();
  init_peers.emplace_back(std::move(peer));
  return init_peers;
}

bool WaitForRaftDriverLeader(
    raft::RaftDriver* driver, uint64_t expected_leader_id = 1,
    std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
  return WaitUntil(
      [driver, expected_leader_id]() {
        auto status = driver->GetRaftStatus();
        return status.s.basicStatus_.softState_.lead_ == expected_leader_id &&
               status.s.basicStatus_.softState_.raftState_ ==
                   eraft::StateLeader;
      },
      timeout);
}

meta::RaftRequest MakeGraphWriteRequest(const std::string& key,
                                        const std::string& value) {
  rocksdb::WriteBatch wb;
  auto status = wb.Put(key, value);
  if (!status.ok()) {
    throw std::runtime_error(status.ToString());
  }

  meta::RaftRequest request;
  request.set_wb_kind(meta::WriteBatchKind::GRAPH_WRITE);
  request.set_wb_data(wb.Data());
  return request;
}

size_t FirstFollowerIndex(const TestServerCluster& cluster,
                          size_t leader_index) {
  for (size_t i = 0; i < cluster.size(); ++i) {
    if (i != leader_index && cluster.server(i) != nullptr) {
      return i;
    }
  }
  throw std::runtime_error("raft test cluster has no running follower");
}

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

TEST(RaftCluster, followerRejectsGraphWriteProposal) {
  TestServerCluster cluster("testdb_raft_cluster");
  ASSERT_NO_THROW(cluster.Start());

  auto leader_index = cluster.WaitForLeaderIndex(std::chrono::seconds(15));
  ASSERT_TRUE(leader_index.has_value()) << cluster.StatusSummary();
  auto follower_index = FirstFollowerIndex(cluster, *leader_index);
  auto* follower = cluster.server(follower_index);
  ASSERT_NE(follower, nullptr);

  auto follower_graph = follower->galaxy()->OpenGraph(kGraphName);
  const auto before_apply_index = follower_graph->GetRaftApplyIndex();

  rocksdb::WriteBatch wb;
  ASSERT_TRUE(wb.Put("follower_rejected_key", "value").ok());
  auto result = follower_graph->raft_driver()->ProposeWriteBatch(
      meta::WriteBatchKind::GRAPH_WRITE, wb);

  ASSERT_NE(result.err, nullptr);
  EXPECT_NE(result.err.String().find("not leader"), std::string::npos);
  EXPECT_EQ(follower_graph->GetRaftApplyIndex(), before_apply_index);
}

TEST(RaftCluster, followerTransactionCommitRollsBackLocalWrites) {
  TestServerCluster cluster("testdb_raft_cluster");
  ASSERT_NO_THROW(cluster.Start());

  auto leader_index = cluster.WaitForLeaderIndex(std::chrono::seconds(15));
  ASSERT_TRUE(leader_index.has_value()) << cluster.StatusSummary();
  auto follower_index = FirstFollowerIndex(cluster, *leader_index);
  auto* follower = cluster.server(follower_index);
  ASSERT_NE(follower, nullptr);

  auto follower_graph = follower->galaxy()->OpenGraph(kGraphName);
  const std::string key = "follower_txn_rollback_key";
  {
    auto txn = follower_graph->BeginTransaction();
    auto status = txn->dbtxn()->Put(follower_graph->graph_cf().graph_topology,
                                    key, "value_that_must_not_commit");
    ASSERT_TRUE(status.ok()) << status.ToString();
    EXPECT_THROW(txn->Commit(), std::exception);
  }

  std::string value;
  auto status = follower_graph->raw_db()->Get(
      rocksdb::ReadOptions(), follower_graph->graph_cf().graph_topology, key,
      &value);
  EXPECT_TRUE(status.IsNotFound()) << status.ToString();
}

TEST(RaftCluster, proposalDoesNotCommitWithoutMajority) {
  TestServerCluster cluster("testdb_raft_cluster");
  ASSERT_NO_THROW(cluster.Start());

  auto leader_index = cluster.WaitForLeaderIndex(std::chrono::seconds(15));
  ASSERT_TRUE(leader_index.has_value()) << cluster.StatusSummary();
  for (size_t i = 0; i < cluster.size(); ++i) {
    if (i != *leader_index) {
      ASSERT_NO_THROW(cluster.StopServer(i));
    }
  }

  auto* leader = cluster.server(*leader_index);
  ASSERT_NE(leader, nullptr);
  auto leader_graph = leader->galaxy()->OpenGraph(kGraphName);
  const auto before_apply_index = leader_graph->GetRaftApplyIndex();

  auto context = leader_graph->raft_driver()->ProposeRaftRequest(
      MakeGraphWriteRequest("no_majority_key", "value"));
  auto applied = context->applied.get_future();
  auto status = applied.wait_for(std::chrono::seconds(2));
  if (status == std::future_status::ready) {
    auto result = applied.get();
    EXPECT_NE(result.err, nullptr);
  }
  EXPECT_EQ(leader_graph->GetRaftApplyIndex(), before_apply_index);
}

TEST(RaftCluster, pendingProposalIsRejectedWhenLeaderStops) {
  TestServerCluster cluster("testdb_raft_cluster");
  ASSERT_NO_THROW(cluster.Start());

  auto leader_index = cluster.WaitForLeaderIndex(std::chrono::seconds(15));
  ASSERT_TRUE(leader_index.has_value()) << cluster.StatusSummary();
  for (size_t i = 0; i < cluster.size(); ++i) {
    if (i != *leader_index) {
      ASSERT_NO_THROW(cluster.StopServer(i));
    }
  }

  auto* leader = cluster.server(*leader_index);
  ASSERT_NE(leader, nullptr);
  auto leader_graph = leader->galaxy()->OpenGraph(kGraphName);

  auto context = leader_graph->raft_driver()->ProposeRaftRequest(
      MakeGraphWriteRequest("leader_stop_key", "value"));
  auto applied = context->applied.get_future();
  leader_graph.reset();
  ASSERT_NO_THROW(cluster.StopServer(*leader_index));

  ASSERT_EQ(applied.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);
  auto result = applied.get();
  EXPECT_NE(result.err, nullptr);
  EXPECT_EQ(result.index, 0U);
}

TEST(RaftCluster, restartedFollowerCatchesUpMultipleTransactions) {
  TestServerCluster cluster("testdb_raft_cluster");
  ASSERT_NO_THROW(cluster.Start());

  auto leader_index = cluster.WaitForLeaderIndex(std::chrono::seconds(15));
  ASSERT_TRUE(leader_index.has_value()) << cluster.StatusSummary();
  auto follower_index = FirstFollowerIndex(cluster, *leader_index);
  ASSERT_NO_THROW(cluster.StopServer(follower_index));

  auto* leader = cluster.server(*leader_index);
  ASSERT_NE(leader, nullptr);
  auto leader_graph = leader->galaxy()->OpenGraph(kGraphName);

  std::vector<int64_t> vertex_ids;
  uint64_t applied_index = 0;
  for (int i = 0; i < 5; ++i) {
    auto txn = leader_graph->BeginTransaction();
    auto vertex = txn->CreateVertex({"catchup_label"}, kProperties);
    vertex_ids.push_back(vertex.GetId());
    txn->Commit();
    applied_index = leader_graph->GetRaftApplyIndex();
  }

  ASSERT_TRUE(
      cluster.WaitForApplyIndex(applied_index, std::chrono::seconds(15)))
      << cluster.StatusSummary();
  leader_graph.reset();

  ASSERT_NO_THROW(cluster.StartServer(follower_index));
  ASSERT_TRUE(
      cluster.WaitForApplyIndex(applied_index, std::chrono::seconds(30)))
      << cluster.StatusSummary();

  auto follower_graph =
      cluster.server(follower_index)->galaxy()->OpenGraph(kGraphName);
  auto read_txn = follower_graph->BeginTransaction();
  for (auto vertex_id : vertex_ids) {
    auto persisted_vertex = read_txn->GetVertexById(vertex_id);
    EXPECT_EQ(persisted_vertex.GetAllProperty(), kProperties);
  }
  read_txn->Commit();
}

TEST(RaftCluster, nodeInfosMarkOnlyCurrentLeader) {
  TestServerCluster cluster("testdb_raft_cluster");
  ASSERT_NO_THROW(cluster.Start());

  auto assert_current_leader_marked = [&cluster](size_t leader_index) {
    auto graph = cluster.server(leader_index)->galaxy()->OpenGraph(kGraphName);
    auto node_infos = graph->raft_driver()->GetNodeInfosWithLeader();
    uint64_t marked_leader = 0;
    size_t leader_marks = 0;
    for (const auto& [node_id, node_info] : node_infos.nodes()) {
      if (node_info.is_leader()) {
        marked_leader = node_id;
        ++leader_marks;
      }
    }
    EXPECT_EQ(leader_marks, 1U) << node_infos.ShortDebugString();
    EXPECT_EQ(marked_leader, cluster.node_id(leader_index))
        << node_infos.ShortDebugString();
  };

  auto leader_index = cluster.WaitForLeaderIndex(std::chrono::seconds(15));
  ASSERT_TRUE(leader_index.has_value()) << cluster.StatusSummary();
  assert_current_leader_marked(*leader_index);

  ASSERT_NO_THROW(cluster.StopServer(*leader_index));
  auto new_leader_index = cluster.WaitForLeaderIndex(std::chrono::seconds(20));
  ASSERT_TRUE(new_leader_index.has_value()) << cluster.StatusSummary();
  ASSERT_NE(*new_leader_index, *leader_index);
  assert_current_leader_marked(*new_leader_index);
}

TEST(RaftCluster, galaxyFailoverCanCreateRaftGraph) {
  constexpr char kFailoverGraphName[] = "galaxy_failover_graph";

  TestServerCluster cluster("testdb_raft_cluster");
  ASSERT_NO_THROW(cluster.Start());

  auto old_galaxy_leader_index =
      cluster.WaitForGalaxyLeaderIndex(std::chrono::seconds(15));
  ASSERT_TRUE(old_galaxy_leader_index.has_value())
      << cluster.GalaxyStatusSummary();
  ASSERT_NO_THROW(cluster.StopServer(*old_galaxy_leader_index));

  auto* new_galaxy_leader =
      cluster.WaitForGalaxyLeader(std::chrono::seconds(20));
  ASSERT_NE(new_galaxy_leader, nullptr) << cluster.GalaxyStatusSummary();

  auto node_infos = cluster.NodeInfosForGraph(kFailoverGraphName);
  ASSERT_NO_THROW(new_galaxy_leader->galaxy()->CreateGraphWithRaft(
      kFailoverGraphName, node_infos));
  ASSERT_TRUE(
      cluster.WaitForGraphCreated(kFailoverGraphName, std::chrono::seconds(20)))
      << cluster.GalaxyStatusSummary();

  auto* graph_leader =
      cluster.WaitForLeader(kFailoverGraphName, std::chrono::seconds(20));
  ASSERT_NE(graph_leader, nullptr) << cluster.StatusSummary();

  int64_t vertex_id = 0;
  uint64_t applied_index = 0;
  {
    auto graph = graph_leader->galaxy()->OpenGraph(kFailoverGraphName);
    auto txn = graph->BeginTransaction();
    auto vertex = txn->CreateVertex({"galaxy_failover_label"}, kProperties);
    vertex_id = vertex.GetId();
    txn->Commit();
    applied_index = graph->GetRaftApplyIndex();
  }
  ASSERT_TRUE(cluster.WaitForApplyIndex(kFailoverGraphName, applied_index,
                                        std::chrono::seconds(15)))
      << cluster.StatusSummary();

  ASSERT_NO_THROW(cluster.StartServer(*old_galaxy_leader_index));
  ASSERT_TRUE(
      cluster.WaitForGraphCreated(kFailoverGraphName, std::chrono::seconds(30)))
      << cluster.GalaxyStatusSummary();
  ASSERT_TRUE(cluster.WaitForApplyIndex(kFailoverGraphName, applied_index,
                                        std::chrono::seconds(30)))
      << cluster.StatusSummary();

  for (auto* server : cluster.servers()) {
    auto graph = server->galaxy()->OpenGraph(kFailoverGraphName);
    auto read_txn = graph->BeginTransaction();
    auto persisted_vertex = read_txn->GetVertexById(vertex_id);
    EXPECT_EQ(persisted_vertex.GetAllProperty(), kProperties);
    read_txn->Commit();
  }
}

TEST(RaftCluster, allServersRestartAndRecoverCommittedData) {
  TestServerCluster cluster("testdb_raft_cluster");
  ASSERT_NO_THROW(cluster.Start());

  auto* leader = cluster.WaitForLeader(std::chrono::seconds(15));
  ASSERT_NE(leader, nullptr) << cluster.StatusSummary();

  int64_t vertex_id = 0;
  uint64_t applied_index = 0;
  {
    auto leader_graph = leader->galaxy()->OpenGraph(kGraphName);
    auto txn = leader_graph->BeginTransaction();
    auto vertex = txn->CreateVertex({"restart_cluster_label"}, kProperties);
    vertex_id = vertex.GetId();
    txn->Commit();

    applied_index = leader_graph->GetRaftApplyIndex();
    ASSERT_TRUE(
        cluster.WaitForApplyIndex(applied_index, std::chrono::seconds(15)))
        << cluster.StatusSummary();
  }

  for (size_t i = 0; i < cluster.size(); ++i) {
    ASSERT_NO_THROW(cluster.StopServer(i));
  }
  for (size_t i = 0; i < cluster.size(); ++i) {
    ASSERT_NO_THROW(cluster.StartServer(i));
  }

  ASSERT_NE(cluster.WaitForGalaxyLeader(std::chrono::seconds(20)), nullptr)
      << cluster.GalaxyStatusSummary();
  ASSERT_TRUE(cluster.WaitForGraphCreated(std::chrono::seconds(20)))
      << cluster.GalaxyStatusSummary();
  ASSERT_NE(cluster.WaitForLeader(std::chrono::seconds(20)), nullptr)
      << cluster.StatusSummary();
  ASSERT_TRUE(
      cluster.WaitForApplyIndex(applied_index, std::chrono::seconds(30)))
      << cluster.StatusSummary();

  for (auto* server : cluster.servers()) {
    auto graph = server->galaxy()->OpenGraph(kGraphName);
    auto read_txn = graph->BeginTransaction();
    auto persisted_vertex = read_txn->GetVertexById(vertex_id);
    EXPECT_EQ(persisted_vertex.GetAllProperty(), kProperties);
    read_txn->Commit();
  }
}

TEST(RaftCluster, singleServerElectsLeaderAndCommitsTransaction) {
  TestServerCluster cluster("testdb_raft_cluster", 1);
  ASSERT_NO_THROW(cluster.Start());

  auto* leader = cluster.WaitForLeader(std::chrono::seconds(15));
  ASSERT_NE(leader, nullptr) << cluster.StatusSummary();

  auto leader_graph = leader->galaxy()->OpenGraph(kGraphName);
  auto txn = leader_graph->BeginTransaction();
  auto vertex = txn->CreateVertex({"single_node_label"}, kProperties);
  txn->Commit();

  const auto applied_index = leader_graph->GetRaftApplyIndex();
  ASSERT_GT(applied_index, 0U);
  ASSERT_TRUE(
      cluster.WaitForApplyIndex(applied_index, std::chrono::seconds(10)))
      << cluster.StatusSummary();

  auto read_txn = leader_graph->BeginTransaction();
  auto persisted_vertex = read_txn->GetVertexById(vertex.GetId());
  EXPECT_EQ(persisted_vertex.GetAllProperty(), kProperties);
  read_txn->Commit();
}

TEST(RaftCluster, stoppedLeaderFailsOverAndCatchesUpAfterRestart) {
  TestServerCluster cluster("testdb_raft_cluster");
  ASSERT_NO_THROW(cluster.Start());

  auto old_leader_index = cluster.WaitForLeaderIndex(std::chrono::seconds(15));
  ASSERT_TRUE(old_leader_index.has_value()) << cluster.StatusSummary();
  ASSERT_NO_THROW(cluster.StopServer(*old_leader_index));

  auto new_leader_index = cluster.WaitForLeaderIndex(std::chrono::seconds(20));
  ASSERT_TRUE(new_leader_index.has_value()) << cluster.StatusSummary();
  ASSERT_NE(*new_leader_index, *old_leader_index);
  auto* new_leader = cluster.server(*new_leader_index);
  ASSERT_NE(new_leader, nullptr);

  auto leader_graph = new_leader->galaxy()->OpenGraph(kGraphName);
  auto txn = leader_graph->BeginTransaction();
  auto vertex = txn->CreateVertex({"failover_label"}, kProperties);
  txn->Commit();

  const auto applied_index = leader_graph->GetRaftApplyIndex();
  ASSERT_TRUE(
      cluster.WaitForApplyIndex(applied_index, std::chrono::seconds(15)))
      << cluster.StatusSummary();

  ASSERT_NO_THROW(cluster.StartServer(*old_leader_index));
  ASSERT_TRUE(
      cluster.WaitForApplyIndex(applied_index, std::chrono::seconds(30)))
      << cluster.StatusSummary();

  auto final_leader_index =
      cluster.WaitForLeaderIndex(std::chrono::seconds(15));
  ASSERT_TRUE(final_leader_index.has_value()) << cluster.StatusSummary();
  for (auto* server : cluster.servers()) {
    auto graph = server->galaxy()->OpenGraph(kGraphName);
    auto read_txn = graph->BeginTransaction();
    auto persisted_vertex = read_txn->GetVertexById(vertex.GetId());
    EXPECT_EQ(persisted_vertex.GetAllProperty(), kProperties);
    read_txn->Commit();
  }
}

TEST(RaftDriver, proposeWriteBatchTimesOutWhenApplyStalls) {
  const std::string raft_path = "testdb_raft_proposal_timeout";
  fs::remove_all(raft_path);

  raft::LocalNodeConfig local_node;
  local_node.graph = "proposal_timeout_graph";
  local_node.ip = "127.0.0.1";
  local_node.bolt_port = AllocateFreePort();
  local_node.raft_poft = AllocateFreePort();

  raft::RaftLogStoreConfig store_config;
  store_config.path = raft_path;
  store_config.block_cache = 64;
  store_config.total_threads = 2;
  store_config.keep_logs = 100000;
  store_config.gc_interval = 1;

  raft::RaftConfig raft_config;
  raft_config.tick_interval = 100;
  raft_config.election_tick = 10;
  raft_config.heartbeat_tick = 1;
  raft_config.proposal_timeout = 50;

  meta::RaftNodeInfo node_info;
  node_info.set_node_id(1);
  node_info.set_graph(local_node.graph);
  node_info.set_ip(local_node.ip);
  node_info.set_bolt_port(local_node.bolt_port);
  node_info.set_raft_poft(local_node.raft_poft);

  std::vector<eraft::Peer> init_peers;
  eraft::Peer peer;
  peer.id_ = 1;
  peer.context_ = node_info.SerializeAsString();
  init_peers.emplace_back(std::move(peer));

  std::atomic<uint64_t> applied_index{0};
  raft::RaftDriver driver(
      [&applied_index](uint64_t index, const meta::RaftRequest&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        applied_index.store(index);
      },
      0, std::move(local_node), std::move(init_peers), store_config,
      raft_config);

  auto err = driver.Run();
  if (err != nullptr) {
    driver.Stop();
    FAIL() << err.String();
  }
  if (!WaitUntil(
          [&driver]() {
            auto status = driver.GetRaftStatus();
            return status.s.basicStatus_.softState_.lead_ == 1 &&
                   status.s.basicStatus_.softState_.raftState_ ==
                       eraft::StateLeader;
          },
          std::chrono::seconds(5))) {
    auto status = driver.GetRaftStatus();
    driver.Stop();
    FAIL() << "raft driver did not become leader, lead="
           << status.s.basicStatus_.softState_.lead_;
  }

  rocksdb::WriteBatch wb;
  ASSERT_TRUE(wb.Put("k", "v").ok());
  auto result = driver.ProposeWriteBatch(meta::WriteBatchKind::GRAPH_WRITE, wb);
  EXPECT_NE(result.err, nullptr);
  EXPECT_NE(result.err.String().find("timed out"), std::string::npos);
  EXPECT_TRUE(WaitUntil([&applied_index]() { return applied_index.load() > 0; },
                        std::chrono::seconds(2)));

  driver.Stop();
  fs::remove_all(raft_path);
}

TEST(RaftDriver, restartRecoversNodeInfosAndContinuesApplying) {
  const std::string raft_path = "testdb_raft_restart_recovery";
  fs::remove_all(raft_path);

  auto local_node = MakeLocalNodeConfig("restart_recovery_graph");
  auto store_config = MakeRaftLogStoreConfig(raft_path);
  auto raft_config = MakeRaftConfig();

  uint64_t first_applied_index = 0;
  {
    std::atomic<uint64_t> applied_index{0};
    raft::RaftDriver driver(
        [&applied_index](uint64_t index, const meta::RaftRequest&) {
          applied_index.store(index);
        },
        0, local_node, MakeInitPeers(local_node), store_config, raft_config);

    auto err = driver.Run();
    if (err != nullptr) {
      driver.Stop();
      FAIL() << err.String();
    }
    if (!WaitForRaftDriverLeader(&driver)) {
      auto status = driver.GetRaftStatus();
      driver.Stop();
      FAIL() << "raft driver did not become leader, lead="
             << status.s.basicStatus_.softState_.lead_;
    }

    rocksdb::WriteBatch wb;
    ASSERT_TRUE(wb.Put("k1", "v1").ok());
    auto result =
        driver.ProposeWriteBatch(meta::WriteBatchKind::GRAPH_WRITE, wb);
    if (result.err != nullptr) {
      driver.Stop();
      FAIL() << result.err.String();
    }
    if (result.index == 0) {
      driver.Stop();
      FAIL() << "raft proposal applied at invalid index 0";
    }
    first_applied_index = result.index;
    EXPECT_EQ(applied_index.load(), first_applied_index);

    driver.Stop();
  }

  {
    std::atomic<uint64_t> applied_index{first_applied_index};
    raft::RaftDriver driver(
        [&applied_index](uint64_t index, const meta::RaftRequest&) {
          applied_index.store(index);
        },
        first_applied_index, local_node, store_config, raft_config);

    auto err = driver.Run();
    if (err != nullptr) {
      driver.Stop();
      FAIL() << err.String();
    }
    if (!WaitForRaftDriverLeader(&driver)) {
      auto status = driver.GetRaftStatus();
      driver.Stop();
      FAIL() << "raft driver did not become leader after restart, lead="
             << status.s.basicStatus_.softState_.lead_;
    }

    auto node_infos = driver.GetNodeInfosWithLeader();
    if (node_infos.nodes_size() != 1 || !node_infos.nodes().count(1)) {
      driver.Stop();
      FAIL() << "unexpected node infos after restart: "
             << node_infos.ShortDebugString();
    }
    EXPECT_TRUE(node_infos.nodes().at(1).is_leader());
    EXPECT_EQ(node_infos.nodes().at(1).graph(), local_node.graph);

    rocksdb::WriteBatch wb;
    ASSERT_TRUE(wb.Put("k2", "v2").ok());
    auto result =
        driver.ProposeWriteBatch(meta::WriteBatchKind::GRAPH_WRITE, wb);
    if (result.err != nullptr) {
      driver.Stop();
      FAIL() << result.err.String();
    }
    EXPECT_GT(result.index, first_applied_index);
    EXPECT_EQ(applied_index.load(), result.index);

    driver.Stop();
  }

  fs::remove_all(raft_path);
}

TEST(RaftDriver, freshBootstrapWithoutInitialPeersFails) {
  const std::string raft_path = "testdb_raft_missing_init_peers";
  fs::remove_all(raft_path);

  auto local_node = MakeLocalNodeConfig("missing_init_peers_graph");
  auto store_config = MakeRaftLogStoreConfig(raft_path);
  auto raft_config = MakeRaftConfig();

  raft::RaftDriver driver([](uint64_t, const meta::RaftRequest&) {}, 0,
                          local_node, store_config, raft_config);

  auto err = driver.Run();
  EXPECT_NE(err, nullptr);
  EXPECT_NE(err.String().find("initial peers are required"), std::string::npos);

  driver.Stop();
  fs::remove_all(raft_path);
}

TEST(RaftDriver, freshBootstrapRejectsInvalidInitialPeerContext) {
  const std::string raft_path = "testdb_raft_invalid_peer_context";
  fs::remove_all(raft_path);

  auto local_node = MakeLocalNodeConfig("invalid_peer_context_graph");
  auto store_config = MakeRaftLogStoreConfig(raft_path);
  auto raft_config = MakeRaftConfig();

  std::vector<eraft::Peer> init_peers;
  eraft::Peer peer;
  peer.id_ = 1;
  peer.context_ = "not a serialized raft node info";
  init_peers.emplace_back(std::move(peer));

  raft::RaftDriver driver([](uint64_t, const meta::RaftRequest&) {}, 0,
                          local_node, std::move(init_peers), store_config,
                          raft_config);

  auto err = driver.Run();
  EXPECT_NE(err, nullptr);
  EXPECT_NE(err.String().find("failed to parse initial peer context"),
            std::string::npos);

  driver.Stop();
  fs::remove_all(raft_path);
}

TEST(RaftDriver, freshBootstrapRejectsMissingLocalInitialPeer) {
  const std::string raft_path = "testdb_raft_missing_local_peer";
  fs::remove_all(raft_path);

  auto local_node = MakeLocalNodeConfig("missing_local_peer_graph");
  auto other_node = MakeLocalNodeConfig(local_node.graph);
  auto store_config = MakeRaftLogStoreConfig(raft_path);
  auto raft_config = MakeRaftConfig();

  raft::RaftDriver driver([](uint64_t, const meta::RaftRequest&) {}, 0,
                          local_node, MakeInitPeers(other_node), store_config,
                          raft_config);

  auto err = driver.Run();
  EXPECT_NE(err, nullptr);
  EXPECT_NE(err.String().find("no initial peer matches local identity"),
            std::string::npos);

  driver.Stop();
  fs::remove_all(raft_path);
}

TEST(RaftDriver, freshBootstrapRejectsDuplicateLocalInitialPeers) {
  const std::string raft_path = "testdb_raft_duplicate_local_peers";
  fs::remove_all(raft_path);

  auto local_node = MakeLocalNodeConfig("duplicate_local_peers_graph");
  auto store_config = MakeRaftLogStoreConfig(raft_path);
  auto raft_config = MakeRaftConfig();

  auto init_peers = MakeInitPeers(local_node, 1);
  eraft::Peer duplicate_peer;
  duplicate_peer.id_ = 2;
  duplicate_peer.context_ = MakeNodeInfo(local_node, 2).SerializeAsString();
  init_peers.emplace_back(std::move(duplicate_peer));

  raft::RaftDriver driver([](uint64_t, const meta::RaftRequest&) {}, 0,
                          local_node, std::move(init_peers), store_config,
                          raft_config);

  auto err = driver.Run();
  EXPECT_NE(err, nullptr);
  EXPECT_NE(err.String().find("multiple initial peers match local identity"),
            std::string::npos);

  driver.Stop();
  fs::remove_all(raft_path);
}

TEST(RaftDriver, restartRejectsLocalNodeMissingFromPersistedNodeInfos) {
  const std::string raft_path = "testdb_raft_restart_identity_mismatch";
  fs::remove_all(raft_path);

  auto local_node = MakeLocalNodeConfig("restart_identity_mismatch_graph");
  auto store_config = MakeRaftLogStoreConfig(raft_path);
  auto raft_config = MakeRaftConfig();

  {
    raft::RaftDriver driver([](uint64_t, const meta::RaftRequest&) {}, 0,
                            local_node, MakeInitPeers(local_node), store_config,
                            raft_config);

    auto err = driver.Run();
    if (err != nullptr) {
      driver.Stop();
      FAIL() << err.String();
    }
    if (!WaitForRaftDriverLeader(&driver)) {
      auto status = driver.GetRaftStatus();
      driver.Stop();
      FAIL() << "raft driver did not become leader, lead="
             << status.s.basicStatus_.softState_.lead_;
    }
    driver.Stop();
  }

  auto mismatched_node = local_node;
  mismatched_node.bolt_port = AllocateFreePort();
  raft::RaftDriver driver([](uint64_t, const meta::RaftRequest&) {}, 0,
                          mismatched_node, store_config, raft_config);

  auto err = driver.Run();
  EXPECT_NE(err, nullptr);
  EXPECT_NE(err.String().find("no raft node matches local identity"),
            std::string::npos);

  driver.Stop();
  fs::remove_all(raft_path);
}

TEST(RaftDriver, concurrentLeaderProposalsAllApply) {
  const std::string raft_path = "testdb_raft_concurrent_proposals";
  fs::remove_all(raft_path);

  auto local_node = MakeLocalNodeConfig("concurrent_proposals_graph");
  auto store_config = MakeRaftLogStoreConfig(raft_path);
  auto raft_config = MakeRaftConfig();

  std::atomic<uint64_t> applied_count{0};
  raft::RaftDriver driver(
      [&applied_count](uint64_t, const meta::RaftRequest&) {
        applied_count.fetch_add(1);
      },
      0, local_node, MakeInitPeers(local_node), store_config, raft_config);

  auto err = driver.Run();
  if (err != nullptr) {
    driver.Stop();
    FAIL() << err.String();
  }
  if (!WaitForRaftDriverLeader(&driver)) {
    auto status = driver.GetRaftStatus();
    driver.Stop();
    FAIL() << "raft driver did not become leader, lead="
           << status.s.basicStatus_.softState_.lead_;
  }

  constexpr int kProposalCount = 8;
  std::vector<std::future<raft::PromiseContext::ApplyResult>> futures;
  futures.reserve(kProposalCount);
  for (int i = 0; i < kProposalCount; ++i) {
    futures.emplace_back(std::async(std::launch::async, [&driver, i]() {
      rocksdb::WriteBatch wb;
      auto status =
          wb.Put("concurrent_key_" + std::to_string(i), "concurrent_value");
      if (!status.ok()) {
        throw std::runtime_error(status.ToString());
      }
      return driver.ProposeWriteBatch(meta::WriteBatchKind::GRAPH_WRITE, wb);
    }));
  }

  std::unordered_set<uint64_t> applied_indexes;
  for (auto& future : futures) {
    auto result = future.get();
    EXPECT_EQ(result.err, nullptr);
    EXPECT_GT(result.index, 0U);
    EXPECT_TRUE(applied_indexes.insert(result.index).second);
  }
  EXPECT_TRUE(WaitUntil(
      [&applied_count]() {
        return applied_count.load() == static_cast<uint64_t>(kProposalCount);
      },
      std::chrono::seconds(5)));

  driver.Stop();
  fs::remove_all(raft_path);
}

TEST(RaftDriver, rejectOversizedWriteBatch) {
  raft::LocalNodeConfig local_node;
  local_node.graph = "oversized_proposal_graph";
  local_node.ip = "127.0.0.1";
  local_node.bolt_port = 1;
  local_node.raft_poft = 1;

  raft::RaftConfig raft_config;
  raft_config.max_proposal_bytes = 32;

  raft::RaftDriver driver([](uint64_t, const meta::RaftRequest&) {}, 0,
                          std::move(local_node), {}, raft_config);

  rocksdb::WriteBatch wb;
  ASSERT_TRUE(wb.Put("k", std::string(1024, 'v')).ok());
  auto result = driver.ProposeWriteBatch(meta::WriteBatchKind::GRAPH_WRITE, wb);
  EXPECT_NE(result.err, nullptr);
  EXPECT_NE(result.err.String().find("too large"), std::string::npos);
}

TEST(RaftDriver, rejectWriteBatchWhenPendingQueueIsFull) {
  const std::string raft_path = "testdb_raft_proposal_backpressure";
  fs::remove_all(raft_path);

  raft::LocalNodeConfig local_node;
  local_node.graph = "proposal_backpressure_graph";
  local_node.ip = "127.0.0.1";
  local_node.bolt_port = AllocateFreePort();
  local_node.raft_poft = AllocateFreePort();

  raft::RaftLogStoreConfig store_config;
  store_config.path = raft_path;
  store_config.block_cache = 64;
  store_config.total_threads = 2;
  store_config.keep_logs = 100000;
  store_config.gc_interval = 1;

  raft::RaftConfig raft_config;
  raft_config.tick_interval = 100;
  raft_config.election_tick = 10;
  raft_config.heartbeat_tick = 1;
  raft_config.max_pending_proposals = 1;

  meta::RaftNodeInfo node_info;
  node_info.set_node_id(1);
  node_info.set_graph(local_node.graph);
  node_info.set_ip(local_node.ip);
  node_info.set_bolt_port(local_node.bolt_port);
  node_info.set_raft_poft(local_node.raft_poft);

  std::vector<eraft::Peer> init_peers;
  eraft::Peer peer;
  peer.id_ = 1;
  peer.context_ = node_info.SerializeAsString();
  init_peers.emplace_back(std::move(peer));

  raft::RaftDriver driver(
      [](uint64_t, const meta::RaftRequest&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      },
      0, std::move(local_node), std::move(init_peers), store_config,
      raft_config);

  auto err = driver.Run();
  if (err != nullptr) {
    driver.Stop();
    FAIL() << err.String();
  }
  if (!WaitUntil(
          [&driver]() {
            auto status = driver.GetRaftStatus();
            return status.s.basicStatus_.softState_.lead_ == 1 &&
                   status.s.basicStatus_.softState_.raftState_ ==
                       eraft::StateLeader;
          },
          std::chrono::seconds(5))) {
    auto status = driver.GetRaftStatus();
    driver.Stop();
    FAIL() << "raft driver did not become leader, lead="
           << status.s.basicStatus_.softState_.lead_;
  }

  rocksdb::WriteBatch wb;
  ASSERT_TRUE(wb.Put("k", "v").ok());
  meta::RaftRequest first_request;
  first_request.set_wb_kind(meta::WriteBatchKind::GRAPH_WRITE);
  first_request.set_wb_data(wb.Data());
  auto first_context = driver.ProposeRaftRequest(std::move(first_request));

  auto second_result =
      driver.ProposeWriteBatch(meta::WriteBatchKind::GRAPH_WRITE, wb);
  EXPECT_NE(second_result.err, nullptr);
  EXPECT_NE(second_result.err.String().find("backpressure"), std::string::npos);

  auto first_future = first_context->applied.get_future();
  ASSERT_EQ(first_future.wait_for(std::chrono::seconds(2)),
            std::future_status::ready);
  EXPECT_EQ(first_future.get().err, nullptr);

  driver.Stop();
  fs::remove_all(raft_path);
}

TEST(RaftDriver, rejectWriteBatchWhenPendingBytesAreFull) {
  const std::string raft_path = "testdb_raft_proposal_bytes_backpressure";
  fs::remove_all(raft_path);

  raft::LocalNodeConfig local_node;
  local_node.graph = "proposal_bytes_backpressure_graph";
  local_node.ip = "127.0.0.1";
  local_node.bolt_port = AllocateFreePort();
  local_node.raft_poft = AllocateFreePort();

  raft::RaftLogStoreConfig store_config;
  store_config.path = raft_path;
  store_config.block_cache = 64;
  store_config.total_threads = 2;
  store_config.keep_logs = 100000;
  store_config.gc_interval = 1;

  raft::RaftConfig raft_config;
  raft_config.tick_interval = 100;
  raft_config.election_tick = 10;
  raft_config.heartbeat_tick = 1;
  raft_config.max_proposal_bytes = 700 * 1024;
  raft_config.max_pending_proposal_bytes = 800 * 1024;

  meta::RaftNodeInfo node_info;
  node_info.set_node_id(1);
  node_info.set_graph(local_node.graph);
  node_info.set_ip(local_node.ip);
  node_info.set_bolt_port(local_node.bolt_port);
  node_info.set_raft_poft(local_node.raft_poft);

  std::vector<eraft::Peer> init_peers;
  eraft::Peer peer;
  peer.id_ = 1;
  peer.context_ = node_info.SerializeAsString();
  init_peers.emplace_back(std::move(peer));

  raft::RaftDriver driver(
      [](uint64_t, const meta::RaftRequest&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      },
      0, std::move(local_node), std::move(init_peers), store_config,
      raft_config);

  auto err = driver.Run();
  if (err != nullptr) {
    driver.Stop();
    FAIL() << err.String();
  }
  if (!WaitUntil(
          [&driver]() {
            auto status = driver.GetRaftStatus();
            return status.s.basicStatus_.softState_.lead_ == 1 &&
                   status.s.basicStatus_.softState_.raftState_ ==
                       eraft::StateLeader;
          },
          std::chrono::seconds(5))) {
    auto status = driver.GetRaftStatus();
    driver.Stop();
    FAIL() << "raft driver did not become leader, lead="
           << status.s.basicStatus_.softState_.lead_;
  }

  rocksdb::WriteBatch wb;
  ASSERT_TRUE(wb.Put("k", std::string(512 * 1024, 'v')).ok());
  meta::RaftRequest first_request;
  first_request.set_wb_kind(meta::WriteBatchKind::GRAPH_WRITE);
  first_request.set_wb_data(wb.Data());
  auto first_context = driver.ProposeRaftRequest(std::move(first_request));

  auto second_result =
      driver.ProposeWriteBatch(meta::WriteBatchKind::GRAPH_WRITE, wb);
  EXPECT_NE(second_result.err, nullptr);
  EXPECT_NE(second_result.err.String().find("pending proposal bytes"),
            std::string::npos);

  auto first_future = first_context->applied.get_future();
  ASSERT_EQ(first_future.wait_for(std::chrono::seconds(2)),
            std::future_status::ready);
  EXPECT_EQ(first_future.get().err, nullptr);

  driver.Stop();
  fs::remove_all(raft_path);
}

}  // namespace
