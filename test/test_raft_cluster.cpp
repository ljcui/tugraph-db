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
#include <limits>
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

raft::PromiseContext::ApplyResult WaitForAppliedResult(
    const std::shared_ptr<raft::PromiseContext>& context,
    std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
  auto future = context->applied.get_future();
  if (future.wait_for(timeout) != std::future_status::ready) {
    return {eraft::Error("raft proposal did not apply before timeout"), 0};
  }
  return future.get();
}

struct TestRaftLogStorage {
  explicit TestRaftLogStorage(std::unique_ptr<raft::RaftLogStorage> storage)
      : storage(std::move(storage)) {}

  TestRaftLogStorage(TestRaftLogStorage&&) = default;
  TestRaftLogStorage& operator=(TestRaftLogStorage&&) = default;

  ~TestRaftLogStorage() { Close(); }

  raft::RaftLogStorage* operator->() const { return storage.get(); }

  void Close() {
    if (storage != nullptr) {
      storage->Close();
      storage.reset();
    }
  }

  std::unique_ptr<raft::RaftLogStorage> storage;
};

TestRaftLogStorage OpenRaftLogStorage(const std::string& path) {
  rocksdb::Options options;
  options.create_if_missing = true;
  options.create_missing_column_families = true;

  std::vector<rocksdb::ColumnFamilyDescriptor> cfs;
  cfs.emplace_back(rocksdb::kDefaultColumnFamilyName, options);
  cfs.emplace_back("meta", options);

  std::vector<rocksdb::ColumnFamilyHandle*> cf_handles;
  rocksdb::DB* db = nullptr;
  fs::create_directories(path);
  auto status = rocksdb::DB::Open(options, path, cfs, &cf_handles, &db);
  if (!status.ok()) {
    throw std::runtime_error(status.ToString());
  }
  if (cf_handles.size() != 2) {
    throw std::runtime_error("unexpected raft log storage column families");
  }
  return TestRaftLogStorage(
      std::make_unique<raft::RaftLogStorage>(db, cf_handles[0], cf_handles[1]));
}

raftpb::Entry MakeLogEntry(uint64_t index, uint64_t term,
                           std::string data = {}) {
  raftpb::Entry entry;
  entry.set_index(index);
  entry.set_term(term);
  entry.set_data(std::move(data));
  return entry;
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

std::optional<size_t> WaitForLeaderIndexExcluding(
    const TestServerCluster& cluster,
    const std::unordered_set<size_t>& excluded,
    std::chrono::milliseconds timeout = std::chrono::seconds(10)) {
  std::optional<size_t> leader_index;
  if (!WaitUntil(
          [&cluster, &excluded, &leader_index]() {
            leader_index.reset();
            uint64_t leader_id = 0;
            bool has_running_server = false;

            for (size_t i = 0; i < cluster.size(); ++i) {
              if (excluded.count(i)) {
                continue;
              }
              auto* server = cluster.server(i);
              if (server == nullptr) {
                continue;
              }
              has_running_server = true;
              auto graph = server->galaxy()->OpenGraph(kGraphName);
              auto status = graph->raft_driver()->GetRaftStatus();
              auto reported_leader = status.s.basicStatus_.softState_.lead_;
              if (reported_leader == 0) {
                return false;
              }
              if (leader_id == 0) {
                leader_id = reported_leader;
              } else if (leader_id != reported_leader) {
                return false;
              }
              if (status.s.basicStatus_.softState_.raftState_ ==
                  eraft::StateLeader) {
                if (leader_index.has_value()) {
                  return false;
                }
                leader_index = i;
              }
            }

            if (!has_running_server || !leader_index.has_value()) {
              return false;
            }
            return cluster.node_id(*leader_index) == leader_id;
          },
          timeout)) {
    return std::nullopt;
  }
  return leader_index;
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

TEST(RaftCluster, removeLeaderFromGraphRaftElectsNewLeaderAndCommits) {
  TestServerCluster cluster("testdb_raft_cluster");
  ASSERT_NO_THROW(cluster.Start());

  auto old_leader_index = cluster.WaitForLeaderIndex(std::chrono::seconds(15));
  ASSERT_TRUE(old_leader_index.has_value()) << cluster.StatusSummary();
  auto old_leader_node_id = cluster.node_id(*old_leader_index);

  auto* old_leader = cluster.server(*old_leader_index);
  ASSERT_NE(old_leader, nullptr);
  auto old_leader_graph = old_leader->galaxy()->OpenGraph(kGraphName);

  auto initial_node_infos = cluster.NodeInfosForGraph(kGraphName);
  ASSERT_TRUE(initial_node_infos.nodes().count(old_leader_node_id));
  auto old_leader_node_info = initial_node_infos.nodes().at(old_leader_node_id);

  raftpb::ConfChange remove_leader;
  remove_leader.set_type(raftpb::ConfChangeRemoveNode);
  remove_leader.set_node_id(old_leader_node_id);
  remove_leader.set_context(old_leader_node_info.SerializeAsString());

  auto remove_result = WaitForAppliedResult(
      old_leader_graph->raft_driver()->ProposeConfChange(remove_leader),
      std::chrono::seconds(15));
  ASSERT_EQ(remove_result.err, nullptr) << remove_result.err.String();
  ASSERT_GT(remove_result.index, 0U);
  old_leader_graph.reset();

  auto new_leader_index = WaitForLeaderIndexExcluding(
      cluster, {*old_leader_index}, std::chrono::seconds(20));
  ASSERT_TRUE(new_leader_index.has_value()) << cluster.StatusSummary();
  ASSERT_NE(*new_leader_index, *old_leader_index);

  ASSERT_TRUE(WaitUntil(
      [&cluster, old_leader_index, old_leader_node_id]() {
        for (size_t i = 0; i < cluster.size(); ++i) {
          if (i == *old_leader_index) {
            continue;
          }
          auto* server = cluster.server(i);
          if (server == nullptr) {
            continue;
          }
          auto graph = server->galaxy()->OpenGraph(kGraphName);
          auto node_infos = graph->raft_driver()->GetNodeInfosWithLeader();
          if (node_infos.nodes().count(old_leader_node_id)) {
            return false;
          }
        }
        return true;
      },
      std::chrono::seconds(15)))
      << cluster.StatusSummary();

  auto* new_leader = cluster.server(*new_leader_index);
  ASSERT_NE(new_leader, nullptr);
  auto new_leader_graph = new_leader->galaxy()->OpenGraph(kGraphName);

  int64_t vertex_id = 0;
  uint64_t applied_index = 0;
  {
    auto txn = new_leader_graph->BeginTransaction();
    auto vertex = txn->CreateVertex({"remove_leader_label"}, kProperties);
    vertex_id = vertex.GetId();
    txn->Commit();
    applied_index = new_leader_graph->GetRaftApplyIndex();
  }

  ASSERT_TRUE(WaitUntil(
      [&cluster, old_leader_index, applied_index]() {
        for (size_t i = 0; i < cluster.size(); ++i) {
          if (i == *old_leader_index) {
            continue;
          }
          auto* server = cluster.server(i);
          if (server == nullptr) {
            continue;
          }
          auto graph = server->galaxy()->OpenGraph(kGraphName);
          if (graph->GetRaftApplyIndex() < applied_index) {
            return false;
          }
        }
        return true;
      },
      std::chrono::seconds(15)))
      << cluster.StatusSummary();

  for (size_t i = 0; i < cluster.size(); ++i) {
    if (i == *old_leader_index) {
      continue;
    }
    auto* server = cluster.server(i);
    ASSERT_NE(server, nullptr);
    auto graph = server->galaxy()->OpenGraph(kGraphName);
    auto read_txn = graph->BeginTransaction();
    auto persisted_vertex = read_txn->GetVertexById(vertex_id);
    EXPECT_EQ(persisted_vertex.GetAllProperty(), kProperties);
    read_txn->Commit();
  }
}

TEST(RaftCluster, removeFollowerFromGraphRaftKeepsMajorityWritable) {
  TestServerCluster cluster("testdb_raft_cluster");
  ASSERT_NO_THROW(cluster.Start());

  auto leader_index = cluster.WaitForLeaderIndex(std::chrono::seconds(15));
  ASSERT_TRUE(leader_index.has_value()) << cluster.StatusSummary();
  auto removed_index = FirstFollowerIndex(cluster, *leader_index);
  auto removed_node_id = cluster.node_id(removed_index);

  auto* leader = cluster.server(*leader_index);
  ASSERT_NE(leader, nullptr);
  auto leader_graph = leader->galaxy()->OpenGraph(kGraphName);

  auto initial_node_infos = cluster.NodeInfosForGraph(kGraphName);
  ASSERT_TRUE(initial_node_infos.nodes().count(removed_node_id));
  auto removed_node_info = initial_node_infos.nodes().at(removed_node_id);

  raftpb::ConfChange remove_follower;
  remove_follower.set_type(raftpb::ConfChangeRemoveNode);
  remove_follower.set_node_id(removed_node_id);
  remove_follower.set_context(removed_node_info.SerializeAsString());

  auto remove_result = WaitForAppliedResult(
      leader_graph->raft_driver()->ProposeConfChange(remove_follower),
      std::chrono::seconds(15));
  ASSERT_EQ(remove_result.err, nullptr) << remove_result.err.String();
  ASSERT_GT(remove_result.index, 0U);

  ASSERT_TRUE(WaitUntil(
      [&cluster, removed_index, removed_node_id]() {
        for (size_t i = 0; i < cluster.size(); ++i) {
          if (i == removed_index) {
            continue;
          }
          auto* server = cluster.server(i);
          if (server == nullptr) {
            continue;
          }
          auto graph = server->galaxy()->OpenGraph(kGraphName);
          auto node_infos = graph->raft_driver()->GetNodeInfosWithLeader();
          if (node_infos.nodes().count(removed_node_id)) {
            return false;
          }
        }
        return true;
      },
      std::chrono::seconds(15)))
      << cluster.StatusSummary();

  int64_t vertex_id = 0;
  uint64_t applied_index = 0;
  {
    auto txn = leader_graph->BeginTransaction();
    auto vertex = txn->CreateVertex({"remove_follower_label"}, kProperties);
    vertex_id = vertex.GetId();
    txn->Commit();
    applied_index = leader_graph->GetRaftApplyIndex();
  }

  ASSERT_TRUE(WaitUntil(
      [&cluster, removed_index, applied_index]() {
        for (size_t i = 0; i < cluster.size(); ++i) {
          if (i == removed_index) {
            continue;
          }
          auto* server = cluster.server(i);
          if (server == nullptr) {
            continue;
          }
          auto graph = server->galaxy()->OpenGraph(kGraphName);
          if (graph->GetRaftApplyIndex() < applied_index) {
            return false;
          }
        }
        return true;
      },
      std::chrono::seconds(15)))
      << cluster.StatusSummary();

  for (size_t i = 0; i < cluster.size(); ++i) {
    if (i == removed_index) {
      continue;
    }
    auto* server = cluster.server(i);
    ASSERT_NE(server, nullptr);
    auto graph = server->galaxy()->OpenGraph(kGraphName);
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

TEST(RaftLogStorage, persistsStateEntriesAndMetadataAcrossReopen) {
  const std::string raft_path = "testdb_raft_log_storage_persistence";
  fs::remove_all(raft_path);

  auto local_node = MakeLocalNodeConfig("log_storage_persistence_graph");
  {
    auto storage = OpenRaftLogStorage(raft_path);
    ASSERT_FALSE(storage->Init());

    auto first_index = storage->FirstIndex();
    EXPECT_EQ(first_index.second, nullptr);
    EXPECT_EQ(first_index.first, 1U);
    auto last_index = storage->LastIndex();
    EXPECT_EQ(last_index.second, nullptr);
    EXPECT_EQ(last_index.first, 0U);
    EXPECT_EQ(storage->GetApplyIndex(), 0U);
    EXPECT_FALSE(storage->GetNodeInfos().has_value());

    raftpb::HardState hard_state;
    hard_state.set_term(3);
    hard_state.set_vote(1);
    hard_state.set_commit(2);

    raftpb::ConfState conf_state;
    conf_state.add_voters(1);
    conf_state.add_voters(2);

    meta::RaftNodeInfos node_infos;
    (*node_infos.mutable_nodes())[1] = MakeNodeInfo(local_node, 1);

    rocksdb::WriteBatch wb;
    EXPECT_EQ(storage->SetHardState(hard_state, wb), nullptr);
    EXPECT_EQ(storage->SetConfState(conf_state, wb), nullptr);
    EXPECT_EQ(storage->SetNodeInfos(node_infos.SerializeAsString(), wb),
              nullptr);
    EXPECT_EQ(storage->SetApplyIndex(2, wb), nullptr);
    EXPECT_EQ(storage->Append(
                  {MakeLogEntry(1, 1, "one"), MakeLogEntry(2, 1, "two")}, wb),
              nullptr);
    storage->WriteBatch(wb);
    storage.Close();
  }

  {
    auto storage = OpenRaftLogStorage(raft_path);
    ASSERT_TRUE(storage->Init());

    auto [hard_state, conf_state, err] = storage->InitialState();
    EXPECT_EQ(err, nullptr);
    EXPECT_EQ(hard_state.term(), 3U);
    EXPECT_EQ(hard_state.vote(), 1U);
    EXPECT_EQ(hard_state.commit(), 2U);
    ASSERT_EQ(conf_state.voters_size(), 2);
    EXPECT_EQ(conf_state.voters(0), 1U);
    EXPECT_EQ(conf_state.voters(1), 2U);
    EXPECT_EQ(storage->GetApplyIndex(), 2U);

    auto node_infos_data = storage->GetNodeInfos();
    ASSERT_TRUE(node_infos_data.has_value());
    meta::RaftNodeInfos node_infos;
    ASSERT_TRUE(node_infos.ParseFromString(*node_infos_data));
    ASSERT_TRUE(node_infos.nodes().count(1));
    EXPECT_EQ(node_infos.nodes().at(1).graph(), local_node.graph);

    auto first_index = storage->FirstIndex();
    EXPECT_EQ(first_index.second, nullptr);
    EXPECT_EQ(first_index.first, 1U);
    auto last_index = storage->LastIndex();
    EXPECT_EQ(last_index.second, nullptr);
    EXPECT_EQ(last_index.first, 2U);

    auto term = storage->Term(2);
    EXPECT_EQ(term.second, nullptr);
    EXPECT_EQ(term.first, 1U);

    auto entries = storage->Entries(1, 3, std::numeric_limits<uint64_t>::max());
    EXPECT_EQ(entries.second, nullptr);
    ASSERT_EQ(entries.first.size(), 2U);
    EXPECT_EQ(entries.first[0].data(), "one");
    EXPECT_EQ(entries.first[1].data(), "two");

    auto snapshot = storage->Snapshot();
    EXPECT_EQ(snapshot.second, eraft::ErrSnapshotTemporarilyUnavailable);
  }

  fs::remove_all(raft_path);
}

TEST(RaftLogStorage, appendOverwritesConflictingSuffix) {
  const std::string raft_path = "testdb_raft_log_storage_append";
  fs::remove_all(raft_path);

  auto storage = OpenRaftLogStorage(raft_path);
  ASSERT_FALSE(storage->Init());

  rocksdb::WriteBatch first_wb;
  ASSERT_EQ(
      storage->Append({MakeLogEntry(1, 1, "one"), MakeLogEntry(2, 1, "two"),
                       MakeLogEntry(3, 1, "stale")},
                      first_wb),
      nullptr);
  storage->WriteBatch(first_wb);

  rocksdb::WriteBatch overwrite_wb;
  ASSERT_EQ(
      storage->Append({MakeLogEntry(2, 2, "two-overwritten")}, overwrite_wb),
      nullptr);
  storage->WriteBatch(overwrite_wb);

  auto last_index = storage->LastIndex();
  EXPECT_EQ(last_index.second, nullptr);
  EXPECT_EQ(last_index.first, 2U);

  auto term = storage->Term(2);
  EXPECT_EQ(term.second, nullptr);
  EXPECT_EQ(term.first, 2U);
  EXPECT_EQ(storage->Term(3).second, eraft::ErrUnavailable);

  auto entries = storage->Entries(1, 3, std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(entries.second, nullptr);
  ASSERT_EQ(entries.first.size(), 2U);
  EXPECT_EQ(entries.first[0].data(), "one");
  EXPECT_EQ(entries.first[1].data(), "two-overwritten");

  auto limited_entries = storage->Entries(1, 3, 1);
  EXPECT_EQ(limited_entries.second, nullptr);
  ASSERT_EQ(limited_entries.first.size(), 1U);
  EXPECT_EQ(limited_entries.first[0].index(), 1U);

  storage.Close();
  fs::remove_all(raft_path);
}

TEST(RaftLogStorage, compactMovesFirstIndexAndRejectsCompactedEntries) {
  const std::string raft_path = "testdb_raft_log_storage_compact";
  fs::remove_all(raft_path);

  auto storage = OpenRaftLogStorage(raft_path);
  ASSERT_FALSE(storage->Init());

  rocksdb::WriteBatch wb;
  ASSERT_EQ(
      storage->Append({MakeLogEntry(1, 1, "one"), MakeLogEntry(2, 1, "two"),
                       MakeLogEntry(3, 2, "three"), MakeLogEntry(4, 2, "four"),
                       MakeLogEntry(5, 2, "five")},
                      wb),
      nullptr);
  storage->WriteBatch(wb);

  storage->Compact(3);

  auto first_index = storage->FirstIndex();
  EXPECT_EQ(first_index.second, nullptr);
  EXPECT_EQ(first_index.first, 4U);
  EXPECT_EQ(storage->Term(2).second, eraft::ErrCompacted);
  auto compacted_entries = storage->Entries(3, 5, 1024);
  EXPECT_EQ(compacted_entries.second, eraft::ErrCompacted);

  auto term = storage->Term(3);
  EXPECT_EQ(term.second, nullptr);
  EXPECT_EQ(term.first, 2U);

  auto entries = storage->Entries(4, 6, std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(entries.second, nullptr);
  ASSERT_EQ(entries.first.size(), 2U);
  EXPECT_EQ(entries.first[0].data(), "four");
  EXPECT_EQ(entries.first[1].data(), "five");

  storage.Close();
  fs::remove_all(raft_path);
}

TEST(RaftDriver, startupCompactsLogWhenAppliedIndexExceedsRetention) {
  const std::string raft_path = "testdb_raft_driver_compaction";
  fs::remove_all(raft_path);

  auto local_node = MakeLocalNodeConfig("driver_compaction_graph");
  constexpr uint64_t kAppliedIndex = 100001;
  constexpr uint64_t kLastIndex = kAppliedIndex + 1;
  {
    auto storage = OpenRaftLogStorage(raft_path);
    ASSERT_FALSE(storage->Init());

    raftpb::HardState hard_state;
    hard_state.set_term(2);
    hard_state.set_vote(1);
    hard_state.set_commit(kAppliedIndex);

    raftpb::ConfState conf_state;
    conf_state.add_voters(1);

    meta::RaftNodeInfos node_infos;
    (*node_infos.mutable_nodes())[1] = MakeNodeInfo(local_node, 1);

    std::vector<raftpb::Entry> entries;
    entries.reserve(kLastIndex);
    for (uint64_t i = 1; i <= kLastIndex; ++i) {
      entries.emplace_back(MakeLogEntry(i, 2));
    }

    rocksdb::WriteBatch wb;
    ASSERT_EQ(storage->SetHardState(hard_state, wb), nullptr);
    ASSERT_EQ(storage->SetConfState(conf_state, wb), nullptr);
    ASSERT_EQ(storage->SetNodeInfos(node_infos.SerializeAsString(), wb),
              nullptr);
    ASSERT_EQ(storage->SetApplyIndex(kAppliedIndex, wb), nullptr);
    ASSERT_EQ(storage->Append(std::move(entries), wb), nullptr);
    storage->WriteBatch(wb);
    storage.Close();
  }

  auto store_config = MakeRaftLogStoreConfig(raft_path);
  store_config.keep_logs = 0;
  auto raft_config = MakeRaftConfig();

  raft::RaftDriver driver([](uint64_t, const meta::RaftRequest&) {},
                          kAppliedIndex, local_node, store_config, raft_config);

  auto err = driver.Run();
  if (err != nullptr) {
    driver.Stop();
    FAIL() << err.String();
  }

  ASSERT_TRUE(WaitUntil(
      [&driver]() {
        auto status = driver.GetRaftStatus();
        return status.first_log >= kAppliedIndex;
      },
      std::chrono::seconds(5)));

  auto status = driver.GetRaftStatus();
  EXPECT_EQ(status.first_log, kAppliedIndex);
  EXPECT_GE(status.last_log, kLastIndex);

  driver.Stop();
  fs::remove_all(raft_path);
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

TEST(RaftDriver, configValidationRejectsInvalidValues) {
  auto raft_config = MakeRaftConfig();
  EXPECT_TRUE(raft_config.Check());

  auto invalid_raft_config = raft_config;
  invalid_raft_config.tick_interval = 99;
  EXPECT_FALSE(invalid_raft_config.Check());
  invalid_raft_config = raft_config;
  invalid_raft_config.heartbeat_tick = 0;
  EXPECT_FALSE(invalid_raft_config.Check());
  invalid_raft_config = raft_config;
  invalid_raft_config.election_tick = 9;
  EXPECT_FALSE(invalid_raft_config.Check());
  invalid_raft_config = raft_config;
  invalid_raft_config.proposal_timeout = 0;
  EXPECT_FALSE(invalid_raft_config.Check());
  invalid_raft_config = raft_config;
  invalid_raft_config.max_proposal_bytes = 0;
  EXPECT_FALSE(invalid_raft_config.Check());
  invalid_raft_config = raft_config;
  invalid_raft_config.max_pending_proposals = 0;
  EXPECT_FALSE(invalid_raft_config.Check());
  invalid_raft_config = raft_config;
  invalid_raft_config.max_pending_proposal_bytes =
      invalid_raft_config.max_proposal_bytes - 1;
  EXPECT_FALSE(invalid_raft_config.Check());

  auto store_config = MakeRaftLogStoreConfig("testdb_raft_config_validation");
  EXPECT_TRUE(store_config.Check());

  auto invalid_store_config = store_config;
  invalid_store_config.path.clear();
  EXPECT_FALSE(invalid_store_config.Check());
  invalid_store_config = store_config;
  invalid_store_config.block_cache = 9;
  EXPECT_FALSE(invalid_store_config.Check());
  invalid_store_config = store_config;
  invalid_store_config.total_threads = 1;
  EXPECT_FALSE(invalid_store_config.Check());
  invalid_store_config = store_config;
  invalid_store_config.keep_logs = 99999;
  EXPECT_FALSE(invalid_store_config.Check());
  invalid_store_config = store_config;
  invalid_store_config.gc_interval = 0;
  EXPECT_FALSE(invalid_store_config.Check());

  auto local_node = MakeLocalNodeConfig("config_validation_graph");
  EXPECT_TRUE(local_node.Check());

  auto invalid_local_node = local_node;
  invalid_local_node.graph.clear();
  EXPECT_FALSE(invalid_local_node.Check());
  invalid_local_node = local_node;
  invalid_local_node.ip.clear();
  EXPECT_FALSE(invalid_local_node.Check());
  invalid_local_node = local_node;
  invalid_local_node.bolt_port = 0;
  EXPECT_FALSE(invalid_local_node.Check());
  invalid_local_node = local_node;
  invalid_local_node.raft_poft = 0;
  EXPECT_FALSE(invalid_local_node.Check());
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

TEST(RaftDriver, rejectUnknownWriteBatchKind) {
  auto local_node = MakeLocalNodeConfig("unknown_wb_kind_graph");
  raft::RaftDriver driver([](uint64_t, const meta::RaftRequest&) {}, 0,
                          std::move(local_node), {}, MakeRaftConfig());

  meta::RaftRequest request;
  EXPECT_THROW(driver.ProposeRaftRequest(std::move(request)), std::exception);
}

TEST(RaftDriver, rejectWriteBatchAfterStop) {
  const std::string raft_path = "testdb_raft_proposal_after_stop";
  fs::remove_all(raft_path);

  auto local_node = MakeLocalNodeConfig("proposal_after_stop_graph");
  auto store_config = MakeRaftLogStoreConfig(raft_path);
  auto raft_config = MakeRaftConfig();

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

  rocksdb::WriteBatch wb;
  ASSERT_TRUE(wb.Put("after_stop_key", "value").ok());
  auto result = driver.ProposeWriteBatch(meta::WriteBatchKind::GRAPH_WRITE, wb);
  EXPECT_NE(result.err, nullptr);
  EXPECT_NE(result.err.String().find("raft driver stopped"), std::string::npos);
  EXPECT_EQ(result.index, 0U);

  fs::remove_all(raft_path);
}

TEST(RaftDriver, learnerConfChangeUpdatesNodeInfos) {
  const std::string raft_path = "testdb_raft_learner_confchange";
  fs::remove_all(raft_path);

  auto local_node = MakeLocalNodeConfig("learner_confchange_graph");
  auto learner_node = MakeLocalNodeConfig(local_node.graph);
  auto store_config = MakeRaftLogStoreConfig(raft_path);
  auto raft_config = MakeRaftConfig();

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

  auto learner_info = MakeNodeInfo(learner_node, 2);
  raftpb::ConfChange add_learner;
  add_learner.set_type(raftpb::ConfChangeAddLearnerNode);
  add_learner.set_node_id(learner_info.node_id());
  add_learner.set_context(learner_info.SerializeAsString());

  auto add_result = WaitForAppliedResult(driver.ProposeConfChange(add_learner));
  if (add_result.err != nullptr) {
    driver.Stop();
    FAIL() << add_result.err.String();
  }
  EXPECT_GT(add_result.index, 0U);

  auto node_infos = driver.GetNodeInfosWithLeader();
  ASSERT_EQ(node_infos.nodes_size(), 2);
  ASSERT_TRUE(node_infos.nodes().count(1));
  ASSERT_TRUE(node_infos.nodes().count(2));
  EXPECT_TRUE(node_infos.nodes().at(1).is_leader());
  EXPECT_FALSE(node_infos.nodes().at(2).is_leader());
  EXPECT_EQ(node_infos.nodes().at(2).graph(), local_node.graph);

  raftpb::ConfChange remove_learner;
  remove_learner.set_type(raftpb::ConfChangeRemoveNode);
  remove_learner.set_node_id(learner_info.node_id());
  remove_learner.set_context(learner_info.SerializeAsString());

  auto remove_result =
      WaitForAppliedResult(driver.ProposeConfChange(remove_learner));
  if (remove_result.err != nullptr) {
    driver.Stop();
    FAIL() << remove_result.err.String();
  }
  EXPECT_GT(remove_result.index, add_result.index);

  node_infos = driver.GetNodeInfosWithLeader();
  ASSERT_EQ(node_infos.nodes_size(), 1);
  ASSERT_TRUE(node_infos.nodes().count(1));
  EXPECT_FALSE(node_infos.nodes().count(2));
  EXPECT_TRUE(node_infos.nodes().at(1).is_leader());

  driver.Stop();
  fs::remove_all(raft_path);
}

TEST(RaftDriver, restartRejectsDuplicatePersistedNodeInfos) {
  const std::string raft_path = "testdb_raft_restart_duplicate_node_infos";
  fs::remove_all(raft_path);

  auto local_node = MakeLocalNodeConfig("restart_duplicate_node_infos_graph");
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

    auto duplicate_info = MakeNodeInfo(local_node, 2);
    raftpb::ConfChange add_duplicate_learner;
    add_duplicate_learner.set_type(raftpb::ConfChangeAddLearnerNode);
    add_duplicate_learner.set_node_id(duplicate_info.node_id());
    add_duplicate_learner.set_context(duplicate_info.SerializeAsString());

    auto result =
        WaitForAppliedResult(driver.ProposeConfChange(add_duplicate_learner));
    if (result.err != nullptr) {
      driver.Stop();
      FAIL() << result.err.String();
    }
    driver.Stop();
  }

  raft::RaftDriver driver([](uint64_t, const meta::RaftRequest&) {}, 0,
                          local_node, store_config, raft_config);

  auto err = driver.Run();
  EXPECT_NE(err, nullptr);
  EXPECT_NE(err.String().find("multiple raft nodes match local identity"),
            std::string::npos);

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
