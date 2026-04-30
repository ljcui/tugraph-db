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

//
// Created by botu.wzy
//

#include "galaxy.h"

#include <boost/endian/conversion.hpp>
#include <filesystem>
#include <optional>
#include <utility>

#include "common/exceptions.h"
#include "common/logger.h"
using namespace graphdb;
using namespace boost::endian;
namespace server {
namespace {

const std::string kGalaxyRaftGraphName = "__galaxy__";

std::string BuildGraphMetaKey(uint64_t graph_id) {
  std::string key;
  key.append(1, static_cast<char>(GalaxyMetaDataType::GraphDB));
  native_to_big_inplace(graph_id);
  key.append((const char *)&graph_id, sizeof(graph_id));
  return key;
}

std::string BuildGalaxyMetaKey(GalaxyMetaDataType type) {
  return std::string(1, static_cast<char>(type));
}

void ValidateRaftNodeInfos(const meta::RaftNodeInfos &node_infos,
                           std::string_view graph_name) {
  if (node_infos.nodes().empty()) {
    THROW_CODE(InvalidParameter, "raft node infos should not be empty");
  }
  for (const auto &[node_id, node_info] : node_infos.nodes()) {
    if (node_id == 0) {
      THROW_CODE(InvalidParameter, "raft node_id should be greater than 0");
    }
    if (node_info.node_id() != node_id) {
      THROW_CODE(InvalidParameter,
                 "raft node info key [{}] does not match node_id [{}]", node_id,
                 node_info.node_id());
    }
    if (node_info.ip().empty()) {
      THROW_CODE(InvalidParameter, "raft node [{}] ip should not be empty",
                 node_id);
    }
    if (node_info.bolt_port() <= 0) {
      THROW_CODE(InvalidParameter,
                 "raft node [{}] bolt_port should be greater than 0", node_id);
    }
    if (node_info.raft_poft() <= 0) {
      THROW_CODE(InvalidParameter,
                 "raft node [{}] raft_port should be greater than 0", node_id);
    }
    if (node_info.graph().empty()) {
      THROW_CODE(InvalidParameter, "raft node [{}] graph should not be empty",
                 node_id);
    }
    if (!graph_name.empty() && node_info.graph() != graph_name) {
      THROW_CODE(InvalidParameter,
                 "raft node [{}] graph [{}] does not match graph [{}]", node_id,
                 node_info.graph(), graph_name);
    }
  }
}

void ApplyRaftRequest(GraphDB *graph_db, uint64_t index,
                      const meta::RaftRequest &request) {
  graph_db->ApplyRaftRequest(index, request);
}

raft::LocalNodeConfig BuildLocalNodeConfig(
    const std::string &graph_name, const LocalNodeOptions &local_node_options) {
  raft::LocalNodeConfig local_node;
  local_node.graph = graph_name;
  local_node.ip = local_node_options.host;
  local_node.bolt_port = static_cast<int32_t>(local_node_options.bolt_port);
  local_node.raft_poft = static_cast<int32_t>(local_node_options.raft_port);
  return local_node;
}

raft::RaftLogStoreConfig BuildRaftLogStoreConfig(
    const std::string &path,
    std::shared_ptr<rocksdb::Cache> shared_block_cache) {
  raft::RaftLogStoreConfig store_config;
  store_config.path = path;
  store_config.shared_block_cache = std::move(shared_block_cache);
  store_config.total_threads = 2;
  store_config.keep_logs = 100000;
  store_config.gc_interval = 1;
  return store_config;
}

raft::RaftConfig BuildRaftConfig() {
  raft::RaftConfig raft_config;
  raft_config.tick_interval = 100;
  raft_config.election_tick = 10;
  raft_config.heartbeat_tick = 1;
  return raft_config;
}

std::vector<eraft::Peer> BuildInitPeers(const meta::RaftNodeInfos &node_infos) {
  std::vector<eraft::Peer> init_peers;
  init_peers.reserve(node_infos.nodes_size());
  for (const auto &[node_id, node_info] : node_infos.nodes()) {
    eraft::Peer peer;
    peer.id_ = node_id;
    peer.context_ = node_info.SerializeAsString();
    init_peers.emplace_back(std::move(peer));
  }
  return init_peers;
}

meta::RaftNodeInfos CopyRaftNodeInfosForGraph(
    const meta::RaftNodeInfos &node_infos, std::string_view graph_name) {
  meta::RaftNodeInfos ret = node_infos;
  for (auto &[_, node_info] : *ret.mutable_nodes()) {
    node_info.set_graph(std::string(graph_name));
    node_info.clear_is_leader();
  }
  return ret;
}

meta::GraphLifecycleRequest BuildGraphLifecycleRequest(
    const meta::GraphDBMetaInfo &meta) {
  meta::GraphLifecycleRequest request;
  request.set_graph_name(meta.graph_name());
  request.set_graph_id(meta.graph_id());
  return request;
}

void RemoveRaftLogDirectory(const std::string &graph_path) {
  std::error_code ec;
  auto raft_path = graph_path + "/raft";
  std::filesystem::remove_all(raft_path, ec);
  if (ec) {
    THROW_CODE(StorageEngineError, "failed to remove raft log directory {}: {}",
               raft_path, ec.message());
  }
}

}  // namespace

Galaxy::~Galaxy() {
  if (galaxy_raft_driver_) {
    galaxy_raft_driver_->Stop();
    galaxy_raft_driver_.reset();
  }
  graphs_.clear();
  auto s = meta_db_->Close();
  if (!s.ok()) {
    LOG_WARN("meta db close error : {}", s.ToString());
  }
  delete meta_db_;
  meta_db_ = nullptr;
  LOG_INFO("Close galaxy");
}

std::unique_ptr<Galaxy> Galaxy::Open(
    const std::string &path, const GalaxyOptions &galaxy_options,
    LocalNodeOptions local_node_options,
    std::optional<meta::RaftNodeInfos> galaxy_raft_node_infos) {
  LOG_INFO("Open galaxy: {}", path);
  rocksdb::Options options;
  options.create_if_missing = true;
  options.create_missing_column_families = true;
  rocksdb::TransactionDBOptions txn_db_options;

  std::string meta_path = path + "/meta";
  std::filesystem::create_directories(meta_path);
  rocksdb::TransactionDB *db = nullptr;
  auto s =
      rocksdb::TransactionDB::Open(options, txn_db_options, meta_path, &db);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  auto galaxy = std::make_unique<Galaxy>();
  galaxy->path_ = path;
  raft::RaftManager::Configure(galaxy_options.raft_scheduler_shards);
  galaxy->block_cache_ = rocksdb::NewLRUCache(galaxy_options.block_cache_size);
  galaxy->row_cache_ = rocksdb::NewLRUCache(galaxy_options.row_cache_size);
  galaxy->options_ = galaxy_options;
  galaxy->local_node_options_ = std::move(local_node_options);
  galaxy->meta_db_ = db;
  galaxy->raft_log_block_cache_ =
      rocksdb::NewLRUCache(galaxy_options.raft_log_block_cache_size);
  galaxy->assistant_pool_ = std::make_shared<graphdb::AssistantPool>(
      galaxy_options.assistant_thread_num);

  rocksdb::ReadOptions ro;
  {
    std::string next_graph_id_key(
        1, static_cast<char>(GalaxyMetaDataType::NextGraphID));
    std::string val;
    s = galaxy->meta_db_->Get(ro, next_graph_id_key, &val);
    if (s.ok()) {
      galaxy->next_graph_id_ = *(uint64_t *)val.data();
    } else if (s.IsNotFound()) {
      galaxy->next_graph_id_ = 1;
    } else {
      THROW_CODE(StorageEngineError, s.ToString());
    }
  }
  std::unique_ptr<rocksdb::Iterator> iter(galaxy->meta_db_->NewIterator(ro));
  std::string prefix;
  prefix.append(1, static_cast<char>(GalaxyMetaDataType::GraphDB));
  for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix);
       iter->Next()) {
    auto val = iter->value();
    meta::GraphDBMetaInfo meta;
    bool f = meta.ParseFromString(val.ToString());
    assert(f);
    LOG_INFO("Load GraphDB: [{}]", meta.ShortDebugString());
    std::string graph_path =
        galaxy->path_ + "/graph" + std::to_string(meta.graph_id());
    auto graph_db = GraphDB::Open(
        graph_path,
        {.block_cache = galaxy->block_cache_,
         .row_cache = galaxy->row_cache_,
         .assistant_pool = galaxy->assistant_pool_,
         .ft_apply_interval_ = galaxy->options_.ft_apply_interval,
         .ft_writer_threads_ = galaxy->options_.ft_writer_threads,
         .ft_writer_memory_budget_ = galaxy->options_.ft_writer_memory_budget,
         .vt_apply_interval_ = galaxy->options_.vt_apply_interval});
    graph_db->db_meta() = meta;
    if (meta.enable_raft()) {
      galaxy->StartGraphRaft(graph_db.get(), nullptr);
    }
    galaxy->graphs_.emplace(meta.graph_name(), std::move(graph_db));
  }
  if (galaxy->graphs_.empty()) {
    galaxy->CreateGraph("default");
  }
  if (galaxy_raft_node_infos.has_value() ||
      std::filesystem::exists(path + "/galaxy_raft")) {
    galaxy->StartGalaxyRaft(galaxy_raft_node_infos.has_value()
                                ? &galaxy_raft_node_infos.value()
                                : nullptr);
  }
  return galaxy;
}

const std::string &Galaxy::RaftGraphName() { return kGalaxyRaftGraphName; }

std::shared_ptr<GraphDB> Galaxy::OpenGraph(const std::string &name) {
  std::shared_lock<std::shared_mutex> read_lock(graphs_mutex_);
  auto iter = graphs_.find(name);
  if (iter != graphs_.end()) {
    return iter->second;
  } else {
    THROW_CODE(NoSuchGraph, "No such graph: {}", name);
  }
}

GraphDB *Galaxy::CreateGraph(const std::string &name) {
  std::lock_guard<std::mutex> guard(create_graph_mutex_);
  return CreateGraphInternal(name, nullptr);
}

GraphDB *Galaxy::CreateGraphWithRaft(const std::string &name,
                                     const meta::RaftNodeInfos &node_infos) {
  std::lock_guard<std::mutex> guard(create_graph_mutex_);
  ValidateRaftNodeInfos(node_infos, name);
  if (galaxy_raft_driver_ == nullptr) {
    THROW_CODE(InvalidParameter,
               "galaxy raft is not enabled; configure galaxy raft node infos "
               "before creating raft graphs");
  }
  {
    std::shared_lock<std::shared_mutex> read_lock(graphs_mutex_);
    if (graphs_.find(name) != graphs_.end()) {
      THROW_CODE(GraphAlreadyExists, "The graph already exists: {}", name);
    }
  }

  meta::CreateGraphRequest request;
  request.set_graph_name(name);
  request.set_graph_id(static_cast<uint32_t>(next_graph_id_.load()));
  *request.mutable_node_infos() = node_infos;

  meta::RaftRequest raft_request;
  raft_request.set_wb_kind(meta::WriteBatchKind::GALAXY_CREATE_GRAPH);
  raft_request.set_wb_data(request.SerializeAsString());
  auto apply_result =
      galaxy_raft_driver_->ProposeRaftRequestAndWait(std::move(raft_request));
  if (apply_result.err != nullptr) {
    THROW_CODE(StorageEngineError, apply_result.err.String());
  }
  return OpenGraph(name).get();
}

GraphDB *Galaxy::CreateGraphInternal(const std::string &name,
                                     const meta::RaftNodeInfos *node_infos) {
  meta::GraphDBMetaInfo meta;
  uint64_t graph_id = next_graph_id_.load();
  meta.set_graph_id(graph_id);
  meta.set_graph_name(name);
  meta.set_enable_raft(node_infos != nullptr);
  return CreateGraphWithId(meta, node_infos, 0);
}

GraphDB *Galaxy::CreateGraphWithId(const meta::GraphDBMetaInfo &meta,
                                   const meta::RaftNodeInfos *node_infos,
                                   uint64_t apply_index) {
  std::unique_lock<std::shared_mutex> write_lock(graphs_mutex_);
  auto iter = graphs_.find(meta.graph_name());
  if (iter != graphs_.end()) {
    if (apply_index == 0) {
      THROW_CODE(GraphAlreadyExists, "The graph already exists: {}",
                 meta.graph_name());
    }
    auto &existing_meta = iter->second->db_meta();
    if (existing_meta.graph_id() != meta.graph_id() ||
        existing_meta.enable_raft() != meta.enable_raft()) {
      THROW_CODE(StorageEngineError,
                 "graph [{}] already exists with different metadata",
                 meta.graph_name());
    }
    rocksdb::WriteBatch wb;
    auto s = SetGalaxyRaftApplyIndex(apply_index, &wb);
    if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
    s = meta_db_->Write({}, {}, &wb);
    if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
    return iter->second.get();
  }

  if (node_infos != nullptr) {
    ValidateRaftNodeInfos(*node_infos, meta.graph_name());
  }

  uint64_t next = static_cast<uint64_t>(meta.graph_id()) + 1;
  std::string graph_path = path_ + "/graph" + std::to_string(meta.graph_id());
  auto graph_db = GraphDB::Open(
      graph_path, {.block_cache = block_cache_,
                   .row_cache = row_cache_,
                   .assistant_pool = assistant_pool_,
                   .ft_apply_interval_ = options_.ft_apply_interval,
                   .ft_writer_threads_ = options_.ft_writer_threads,
                   .ft_writer_memory_budget_ = options_.ft_writer_memory_budget,
                   .vt_apply_interval_ = options_.vt_apply_interval});
  graph_db->db_meta() = meta;
  if (node_infos) {
    StartGraphRaft(graph_db.get(), node_infos);
  }
  rocksdb::WriteBatch wb;
  wb.Put(BuildGraphMetaKey(meta.graph_id()), meta.SerializeAsString());
  wb.Put(BuildGalaxyMetaKey(GalaxyMetaDataType::NextGraphID),
         std::string((const char *)&next, sizeof(next)));
  if (apply_index > 0) {
    auto s = SetGalaxyRaftApplyIndex(apply_index, &wb);
    if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  }
  auto s = meta_db_->Write({}, {}, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  uint64_t current_next = next_graph_id_.load();
  while (current_next < next &&
         !next_graph_id_.compare_exchange_weak(current_next, next)) {
  }
  LOG_INFO("Create graph:{}, path:{}", meta.graph_name(), graph_path);
  graphs_.emplace(meta.graph_name(), std::move(graph_db));
  return graphs_[meta.graph_name()].get();
}

void Galaxy::StartGraphRaft(GraphDB *graph_db,
                            const meta::RaftNodeInfos *node_infos) {
  auto local_node = BuildLocalNodeConfig(graph_db->db_meta().graph_name(),
                                         local_node_options_);
  auto store_config = BuildRaftLogStoreConfig(graph_db->path() + "/raft",
                                              raft_log_block_cache_);
  auto raft_config = BuildRaftConfig();
  auto *graph_db_ptr = graph_db;
  auto apply_id = graph_db->GetRaftApplyIndex();

  std::unique_ptr<raft::RaftDriver> raft_driver;
  if (node_infos == nullptr) {
    raft_driver = std::make_unique<raft::RaftDriver>(
        [graph_db_ptr](uint64_t index, const meta::RaftRequest &request) {
          ApplyRaftRequest(graph_db_ptr, index, request);
        },
        apply_id, std::move(local_node), store_config, raft_config);
  } else {
    auto init_peers = BuildInitPeers(*node_infos);
    raft_driver = std::make_unique<raft::RaftDriver>(
        [graph_db_ptr](uint64_t index, const meta::RaftRequest &request) {
          ApplyRaftRequest(graph_db_ptr, index, request);
        },
        apply_id, std::move(local_node), std::move(init_peers), store_config,
        raft_config);
  }

  auto err = raft_driver->Run();
  if (err != nullptr) {
    THROW_CODE(StorageEngineError,
               "failed to run raft driver for graph [{}]: {}",
               graph_db->db_meta().graph_name(), err.String());
  }
  graph_db->SetRaftDriver(std::move(raft_driver));
}

void Galaxy::StartGalaxyRaft(const meta::RaftNodeInfos *node_infos) {
  if (galaxy_raft_driver_ != nullptr) {
    return;
  }

  std::optional<meta::RaftNodeInfos> galaxy_node_infos;
  std::vector<eraft::Peer> init_peers;
  if (node_infos != nullptr) {
    galaxy_node_infos = CopyRaftNodeInfosForGraph(*node_infos, RaftGraphName());
    ValidateRaftNodeInfos(*galaxy_node_infos, RaftGraphName());
    init_peers = BuildInitPeers(*galaxy_node_infos);
  }

  auto local_node = BuildLocalNodeConfig(RaftGraphName(), local_node_options_);
  auto store_config =
      BuildRaftLogStoreConfig(path_ + "/galaxy_raft", raft_log_block_cache_);
  auto raft_config = BuildRaftConfig();
  auto apply_id = GetGalaxyRaftApplyIndex();

  std::unique_ptr<raft::RaftDriver> raft_driver;
  if (node_infos == nullptr) {
    raft_driver = std::make_unique<raft::RaftDriver>(
        [this](uint64_t index, const meta::RaftRequest &request) {
          ApplyGalaxyRaftRequest(index, request);
        },
        apply_id, std::move(local_node), store_config, raft_config);
  } else {
    raft_driver = std::make_unique<raft::RaftDriver>(
        [this](uint64_t index, const meta::RaftRequest &request) {
          ApplyGalaxyRaftRequest(index, request);
        },
        apply_id, std::move(local_node), std::move(init_peers), store_config,
        raft_config);
  }

  auto err = raft_driver->Run();
  if (err != nullptr) {
    THROW_CODE(StorageEngineError, "failed to run galaxy raft driver: {}",
               err.String());
  }
  galaxy_raft_driver_ = std::move(raft_driver);
}

uint64_t Galaxy::GetGalaxyRaftApplyIndex() const {
  std::string val;
  auto s = meta_db_->Get(
      {}, BuildGalaxyMetaKey(GalaxyMetaDataType::RaftApplyIndex), &val);
  if (s.IsNotFound()) {
    return 0;
  }
  if (!s.ok()) {
    THROW_CODE(StorageEngineError, "failed to load galaxy raft apply index: {}",
               s.ToString());
  }
  if (val.size() != sizeof(uint64_t)) {
    THROW_CODE(StorageEngineError,
               "galaxy raft apply index has invalid size, expect {}, actual {}",
               sizeof(uint64_t), val.size());
  }
  return *(uint64_t *)val.data();
}

rocksdb::Status Galaxy::SetGalaxyRaftApplyIndex(uint64_t apply_index,
                                                rocksdb::WriteBatch *wb) const {
  return wb->Put(BuildGalaxyMetaKey(GalaxyMetaDataType::RaftApplyIndex),
                 std::string((const char *)&apply_index, sizeof(apply_index)));
}

void Galaxy::ApplyGalaxyRaftRequest(uint64_t index,
                                    const meta::RaftRequest &request) {
  switch (request.wb_kind()) {
    case meta::WriteBatchKind::GALAXY_CREATE_GRAPH: {
      meta::CreateGraphRequest create_graph_request;
      if (!create_graph_request.ParseFromString(request.wb_data())) {
        THROW_CODE(InvalidParameter, "failed to parse CreateGraphRequest");
      }
      ApplyCreateGraphWithRaft(index, create_graph_request);
      return;
    }
    case meta::WriteBatchKind::GALAXY_DELETE_GRAPH: {
      meta::GraphLifecycleRequest delete_graph_request;
      if (!delete_graph_request.ParseFromString(request.wb_data())) {
        THROW_CODE(InvalidParameter, "failed to parse GraphLifecycleRequest");
      }
      ApplyDeleteGraphWithRaft(index, delete_graph_request);
      return;
    }
    case meta::WriteBatchKind::GALAXY_CLEAR_GRAPH: {
      meta::GraphLifecycleRequest clear_graph_request;
      if (!clear_graph_request.ParseFromString(request.wb_data())) {
        THROW_CODE(InvalidParameter, "failed to parse GraphLifecycleRequest");
      }
      ApplyClearGraphWithRaft(index, clear_graph_request);
      return;
    }
    case meta::WriteBatchKind::UNKNOWN:
      THROW_CODE(InvalidParameter,
                 "write batch kind must be specified for galaxy raft request");
    default:
      THROW_CODE(InvalidParameter,
                 "unsupported galaxy raft request kind {} at index {}",
                 static_cast<int>(request.wb_kind()), index);
  }
}

GraphDB *Galaxy::ApplyCreateGraphWithRaft(
    uint64_t apply_index, const meta::CreateGraphRequest &request) {
  ValidateRaftNodeInfos(request.node_infos(), request.graph_name());
  meta::GraphDBMetaInfo meta;
  meta.set_graph_name(request.graph_name());
  meta.set_graph_id(request.graph_id());
  meta.set_enable_raft(true);
  return CreateGraphWithId(meta, &request.node_infos(), apply_index);
}

void Galaxy::ApplyDeleteGraphWithRaft(
    uint64_t apply_index, const meta::GraphLifecycleRequest &request) {
  std::unique_lock<std::shared_mutex> write_lock(graphs_mutex_);
  auto iter = graphs_.find(request.graph_name());
  if (iter == graphs_.end()) {
    rocksdb::WriteBatch wb;
    auto s = SetGalaxyRaftApplyIndex(apply_index, &wb);
    if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
    s = meta_db_->Write({}, {}, &wb);
    if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
    return;
  }

  auto graph = iter->second;
  auto &graph_meta = graph->db_meta();
  if (graph_meta.graph_id() != request.graph_id()) {
    THROW_CODE(StorageEngineError,
               "delete graph [{}] id mismatch, request id {}, local id {}",
               request.graph_name(), request.graph_id(), graph_meta.graph_id());
  }
  if (!graph_meta.enable_raft()) {
    THROW_CODE(StorageEngineError,
               "delete graph [{}] expected raft graph, but local graph is not "
               "raft-enabled",
               request.graph_name());
  }

  graph->StopRaft();

  rocksdb::WriteBatch wb;
  wb.Delete(BuildGraphMetaKey(graph_meta.graph_id()));
  auto s = SetGalaxyRaftApplyIndex(apply_index, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  s = meta_db_->Write({}, {}, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());

  const auto graph_path = graph->path();
  graph->ClearDataInternal();
  RemoveRaftLogDirectory(graph_path);
  graph->drop_on_close() = true;
  LOG_INFO("Erase raft graph:{}, path:{}", request.graph_name(), graph_path);
  graphs_.erase(iter);
}

GraphDB *Galaxy::ApplyClearGraphWithRaft(
    uint64_t apply_index, const meta::GraphLifecycleRequest &request) {
  std::unique_lock<std::shared_mutex> write_lock(graphs_mutex_);
  auto iter = graphs_.find(request.graph_name());
  if (iter == graphs_.end()) {
    rocksdb::WriteBatch wb;
    auto s = SetGalaxyRaftApplyIndex(apply_index, &wb);
    if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
    s = meta_db_->Write({}, {}, &wb);
    if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
    return nullptr;
  }

  auto &graph_meta = iter->second->db_meta();
  if (graph_meta.graph_id() != request.graph_id()) {
    THROW_CODE(StorageEngineError,
               "clear graph [{}] id mismatch, request id {}, local id {}",
               request.graph_name(), request.graph_id(), graph_meta.graph_id());
  }
  if (!graph_meta.enable_raft()) {
    THROW_CODE(StorageEngineError,
               "clear graph [{}] expected raft graph, but local graph is not "
               "raft-enabled",
               request.graph_name());
  }

  LOG_INFO("Clear raft graph:{}, path:{}", request.graph_name(),
           iter->second->path());
  iter->second->ClearDataInternal();

  rocksdb::WriteBatch wb;
  auto s = SetGalaxyRaftApplyIndex(apply_index, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  s = meta_db_->Write({}, {}, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  return iter->second.get();
}

void Galaxy::StepGalaxyRaftMessage(raftpb::Message msg) {
  if (galaxy_raft_driver_ == nullptr) {
    LOG_WARN("galaxy raft is not enabled, drop message");
    return;
  }
  galaxy_raft_driver_->Step(std::move(msg));
}

raft::RaftDriver *Galaxy::galaxy_raft_driver() const {
  return galaxy_raft_driver_.get();
}

GraphDB *Galaxy::ClearGraph(const std::string &name) {
  std::lock_guard<std::mutex> guard(create_graph_mutex_);
  meta::GraphDBMetaInfo graph_meta;
  {
    std::shared_lock<std::shared_mutex> read_lock(graphs_mutex_);
    auto iter = graphs_.find(name);
    if (iter == graphs_.end()) {
      THROW_CODE(NoSuchGraph, "No such graph: {}", name);
    }
    graph_meta = iter->second->db_meta();
  }

  if (graph_meta.enable_raft()) {
    if (galaxy_raft_driver_ == nullptr) {
      THROW_CODE(StorageEngineError,
                 "galaxy raft is not enabled for raft graph [{}]", name);
    }
    meta::RaftRequest raft_request;
    raft_request.set_wb_kind(meta::WriteBatchKind::GALAXY_CLEAR_GRAPH);
    raft_request.set_wb_data(
        BuildGraphLifecycleRequest(graph_meta).SerializeAsString());
    auto apply_result =
        galaxy_raft_driver_->ProposeRaftRequestAndWait(std::move(raft_request));
    if (apply_result.err != nullptr) {
      THROW_CODE(StorageEngineError, apply_result.err.String());
    }
    return OpenGraph(name).get();
  }

  std::unique_lock<std::shared_mutex> write_lock(graphs_mutex_);
  auto iter = graphs_.find(name);
  if (iter == graphs_.end()) {
    THROW_CODE(NoSuchGraph, "No such graph: {}", name);
  }
  LOG_INFO("Clear graph:{}, path:{}", name, iter->second->path());
  iter->second->ClearData();
  return iter->second.get();
}

void Galaxy::DeleteGraph(const std::string &name) {
  std::lock_guard<std::mutex> guard(create_graph_mutex_);
  meta::GraphDBMetaInfo graph_meta;
  {
    std::shared_lock<std::shared_mutex> read_lock(graphs_mutex_);
    auto iter = graphs_.find(name);
    if (iter == graphs_.end()) {
      THROW_CODE(NoSuchGraph, "No such graph: {}", name);
    }
    graph_meta = iter->second->db_meta();
  }

  if (graph_meta.enable_raft()) {
    if (galaxy_raft_driver_ == nullptr) {
      THROW_CODE(StorageEngineError,
                 "galaxy raft is not enabled for raft graph [{}]", name);
    }
    meta::RaftRequest raft_request;
    raft_request.set_wb_kind(meta::WriteBatchKind::GALAXY_DELETE_GRAPH);
    raft_request.set_wb_data(
        BuildGraphLifecycleRequest(graph_meta).SerializeAsString());
    auto apply_result =
        galaxy_raft_driver_->ProposeRaftRequestAndWait(std::move(raft_request));
    if (apply_result.err != nullptr) {
      THROW_CODE(StorageEngineError, apply_result.err.String());
    }
    return;
  }

  std::unique_lock<std::shared_mutex> write_lock(graphs_mutex_);
  auto iter = graphs_.find(name);
  if (iter == graphs_.end()) {
    THROW_CODE(NoSuchGraph, "No such graph: {}", name);
  }

  rocksdb::WriteOptions wo;
  std::string key;
  uint64_t graph_id = iter->second->db_meta().graph_id();
  key.append(1, static_cast<char>(GalaxyMetaDataType::GraphDB));
  native_to_big_inplace(graph_id);
  key.append((const char *)&graph_id, sizeof(graph_id));
  auto s = meta_db_->Delete(wo, key);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  iter->second->drop_on_close() = true;
  std::string graph_path = iter->second->path();
  LOG_INFO("Erase graph:{}, path:{}", name, graph_path);
  graphs_.erase(iter);
}

}  // namespace server
