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

#include "common/exceptions.h"
#include "common/flags.h"
#include "common/logger.h"
using namespace graphdb;
using namespace boost::endian;
namespace server {
namespace {

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

}  // namespace

std::unique_ptr<server::Galaxy> g_galaxy;
Galaxy::~Galaxy() {
  graphs_.clear();
  auto s = meta_db_->Close();
  if (!s.ok()) {
    LOG_WARN("meta db close error : {}", s.ToString());
  }
  delete meta_db_;
  meta_db_ = nullptr;
  LOG_INFO("Close galaxy");
}

std::unique_ptr<Galaxy> Galaxy::Open(const std::string &path,
                                     const GalaxyOptions &galaxy_options) {
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
  galaxy->block_cache_ = rocksdb::NewLRUCache(galaxy_options.block_cache_size);
  galaxy->row_cache_ = rocksdb::NewLRUCache(galaxy_options.row_cache_size);
  galaxy->options_ = galaxy_options;
  galaxy->meta_db_ = db;

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
         .ft_apply_interval_ = galaxy->options_.ft_apply_interval,
         .ft_writer_threads_ = galaxy->options_.ft_writer_threads,
         .ft_writer_memory_budget_ = galaxy->options_.ft_writer_memory_budget,
         .vt_apply_interval_ = galaxy->options_.vt_apply_interval});
    graph_db->db_meta() = meta;
    if (meta.enable_raft()) {
      raft::LocalNodeConfig local_node;
      local_node.graph = meta.graph_name();
      local_node.ip = FLAGS_host;
      local_node.bolt_port = static_cast<int32_t>(FLAGS_bolt_port);
      local_node.raft_poft = static_cast<int32_t>(FLAGS_raft_port);

      raft::RaftLogStoreConfig store_config;
      store_config.path = graph_path + "/raft";
      store_config.block_cache = 64;
      store_config.total_threads = 2;
      store_config.keep_logs = 100000;
      store_config.gc_interval = 1;

      raft::RaftConfig raft_config;
      raft_config.tick_interval = 100;
      raft_config.election_tick = 10;
      raft_config.heartbeat_tick = 1;

      auto *graph_db_ptr = graph_db.get();
      auto apply_id = graph_db->GetRaftApplyIndex();
      auto raft_driver = std::make_unique<raft::RaftDriver>(
          [graph_db_ptr](uint64_t index, const meta::RaftRequest &request) {
            ApplyRaftRequest(graph_db_ptr, index, request);
          },
          apply_id, std::move(local_node), store_config, raft_config);
      auto err = raft_driver->Run();
      if (err != nullptr) {
        THROW_CODE(StorageEngineError,
                   "failed to run raft driver for graph [{}]: {}",
                   meta.graph_name(), err.String());
      }
      graph_db->SetRaftDriver(std::move(raft_driver));
    }
    galaxy->graphs_.emplace(meta.graph_name(), std::move(graph_db));
  }
  if (galaxy->graphs_.empty()) {
    galaxy->CreateGraph("default");
  }
  return galaxy;
}

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
  return CreateGraphInternal(name, nullptr);
}

GraphDB *Galaxy::CreateGraphWithRaft(const std::string &name,
                                     const meta::RaftNodeInfos &node_infos) {
  return CreateGraphInternal(name, &node_infos);
}

GraphDB *Galaxy::CreateGraphInternal(const std::string &name,
                                     const meta::RaftNodeInfos *node_infos) {
  std::unique_lock<std::shared_mutex> write_lock(graphs_mutex_);
  auto iter = graphs_.find(name);
  if (iter != graphs_.end()) {
    THROW_CODE(GraphAlreadyExists, "The graph already exists: {}", name);
  }
  meta::GraphDBMetaInfo meta;
  uint64_t graph_id = next_graph_id_.load();
  uint64_t next = graph_id + 1;
  meta.set_graph_id(graph_id);
  meta.set_graph_name(name);
  meta.set_enable_raft(node_infos != nullptr);
  std::string graph_path = path_ + "/graph" + std::to_string(meta.graph_id());
  auto graph_db = GraphDB::Open(
      graph_path, {.block_cache = block_cache_,
                   .row_cache = row_cache_,
                   .ft_apply_interval_ = options_.ft_apply_interval,
                   .ft_writer_threads_ = options_.ft_writer_threads,
                   .ft_writer_memory_budget_ = options_.ft_writer_memory_budget,
                   .vt_apply_interval_ = options_.vt_apply_interval});
  graph_db->db_meta() = meta;
  if (node_infos) {
    ValidateRaftNodeInfos(*node_infos, name);
    raft::LocalNodeConfig local_node;
    local_node.graph = name;
    local_node.ip = FLAGS_host;
    local_node.bolt_port = static_cast<int32_t>(FLAGS_bolt_port);
    local_node.raft_poft = static_cast<int32_t>(FLAGS_raft_port);

    raft::RaftLogStoreConfig store_config;
    store_config.path = graph_path + "/raft";
    store_config.block_cache = 64;
    store_config.total_threads = 2;
    store_config.keep_logs = 100000;
    store_config.gc_interval = 1;

    raft::RaftConfig raft_config;
    raft_config.tick_interval = 100;
    raft_config.election_tick = 10;
    raft_config.heartbeat_tick = 1;

    std::vector<eraft::Peer> init_peers;
    init_peers.reserve(node_infos->nodes_size());
    for (const auto &[node_id, node_info] : node_infos->nodes()) {
      eraft::Peer peer;
      peer.id_ = node_id;
      peer.context_ = node_info.SerializeAsString();
      init_peers.emplace_back(std::move(peer));
    }

    auto *graph_db_ptr = graph_db.get();
    auto apply_id = graph_db->GetRaftApplyIndex();
    auto raft_driver = std::make_unique<raft::RaftDriver>(
        [graph_db_ptr](uint64_t index, const meta::RaftRequest &request) {
          ApplyRaftRequest(graph_db_ptr, index, request);
        },
        apply_id, std::move(local_node), std::move(init_peers), store_config,
        raft_config);
    auto err = raft_driver->Run();
    if (err != nullptr) {
      THROW_CODE(StorageEngineError,
                 "failed to run raft driver for graph [{}]: {}", name,
                 err.String());
    }
    graph_db->SetRaftDriver(std::move(raft_driver));
  }
  rocksdb::WriteOptions wo;
  rocksdb::WriteBatch wb;
  std::string key;
  key.append(1, static_cast<char>(GalaxyMetaDataType::GraphDB));
  native_to_big_inplace(graph_id);
  key.append((const char *)&graph_id, sizeof(graph_id));
  wb.Put(key, meta.SerializeAsString());
  wb.Put(std::string(1, static_cast<char>(GalaxyMetaDataType::NextGraphID)),
         std::string((const char *)&next, sizeof(next)));
  auto s = meta_db_->Write(wo, {}, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  next_graph_id_ = next;
  LOG_INFO("Create graph:{}, path:{}", name, graph_path);
  graphs_.emplace(meta.graph_name(), std::move(graph_db));
  return graphs_[name].get();
}

GraphDB *Galaxy::ClearGraph(const std::string &name) {
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
