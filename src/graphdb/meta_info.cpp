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

#include "meta_info.h"

#include <algorithm>
#include <boost/endian/conversion.hpp>
#include <filesystem>

#include "common/byte_utils.h"
#include "common/exceptions.h"
#include "common/logger.h"
#include "proto/meta.pb.h"
using namespace boost::endian;
using common::AsChars;
using common::ReadValue;
namespace graphdb {
namespace {

std::string BuildVertexPropertyIndexKey(uint32_t lid,
                                        const std::vector<uint32_t>& pids) {
  std::vector<uint32_t> sorted_pids = pids;
  std::sort(sorted_pids.begin(), sorted_pids.end());
  std::string key(AsChars(lid), sizeof(lid));
  for (auto pid : sorted_pids) {
    key.append(AsChars(pid), sizeof(pid));
  }
  return key;
}

uint64_t BuildVertexVectorIndexKey(uint32_t lid, uint32_t pid) {
  return (static_cast<uint64_t>(lid) << 32) | static_cast<uint64_t>(pid);
}

template <typename Map>
void AppendNamedIndexes(const Map& indexes,
                        std::vector<typename Map::mapped_type>* out,
                        bool skip_failed) {
  out->reserve(out->size() + indexes.size());
  for (const auto& [_, index] : indexes) {
    if (skip_failed && index->state() == meta::IndexBuildState::FAILED) {
      continue;
    }
    out->push_back(index);
  }
}

}  // namespace

std::shared_ptr<VertexPropertyIndex> MetaInfo::GetReadyVertexPropertyIndex(
    uint32_t lid, uint32_t pid) {
  return GetReadyVertexPropertyIndex(lid, std::vector<uint32_t>{pid});
}

std::shared_ptr<VertexPropertyIndex> MetaInfo::GetReadyVertexPropertyIndex(
    uint32_t lid, const std::vector<uint32_t>& pids) {
  std::shared_lock lock(mutex_);
  auto iter = ready_vertex_property_indexes_by_schema_.find(
      BuildVertexPropertyIndexKey(lid, pids));
  if (iter != ready_vertex_property_indexes_by_schema_.end()) {
    return iter->second;
  }
  return nullptr;
}

std::shared_ptr<VertexPropertyIndex> MetaInfo::GetReadyVertexPropertyIndex(
    const std::string& index_name) {
  std::shared_lock lock(mutex_);
  auto iter = ready_vertex_property_indexes_by_name_.find(index_name);
  if (iter != ready_vertex_property_indexes_by_name_.end()) {
    return iter->second;
  }
  return nullptr;
}

std::shared_ptr<VertexPropertyIndex> MetaInfo::GetVertexPropertyIndex(
    uint32_t lid, uint32_t pid) {
  return GetVertexPropertyIndex(lid, std::vector<uint32_t>{pid});
}

std::shared_ptr<VertexPropertyIndex> MetaInfo::GetVertexPropertyIndex(
    uint32_t lid, const std::vector<uint32_t>& pids) {
  std::shared_lock lock(mutex_);
  auto schema_key = BuildVertexPropertyIndexKey(lid, pids);
  auto iter = ready_vertex_property_indexes_by_schema_.find(schema_key);
  if (iter != ready_vertex_property_indexes_by_schema_.end()) {
    return iter->second;
  }
  iter = building_vertex_property_indexes_by_schema_.find(schema_key);
  if (iter != building_vertex_property_indexes_by_schema_.end()) {
    return iter->second;
  }
  return nullptr;
}

std::shared_ptr<VertexPropertyIndex> MetaInfo::GetBestVertexPropertyUniqueIndex(
    uint32_t lid, const std::unordered_set<uint32_t>& pids) {
  std::shared_lock lock(mutex_);
  std::shared_ptr<VertexPropertyIndex> best;
  for (const auto& [_, index] : ready_vertex_property_indexes_by_name_) {
    if (!index->is_unique() || index->lid() != lid ||
        !index->AllPropertiesPresent(pids)) {
      continue;
    }
    if (!best || index->PropertyCount() > best->PropertyCount()) {
      best = index;
    }
  }
  return best;
}

std::shared_ptr<VertexPropertyIndex> MetaInfo::GetVertexPropertyIndex(
    const std::string& index_name) {
  std::shared_lock lock(mutex_);
  auto iter = ready_vertex_property_indexes_by_name_.find(index_name);
  if (iter != ready_vertex_property_indexes_by_name_.end()) {
    return iter->second;
  }
  iter = building_vertex_property_indexes_by_name_.find(index_name);
  if (iter != building_vertex_property_indexes_by_name_.end()) {
    return iter->second;
  }
  return nullptr;
}

std::vector<std::shared_ptr<VertexPropertyIndex>>
MetaInfo::GetVertexPropertyIndexes() {
  std::shared_lock lock(mutex_);
  std::vector<std::shared_ptr<VertexPropertyIndex>> indexes;
  AppendNamedIndexes(ready_vertex_property_indexes_by_name_, &indexes, false);
  AppendNamedIndexes(building_vertex_property_indexes_by_name_, &indexes, true);
  return indexes;
}

std::vector<std::shared_ptr<VertexPropertyIndex>>
MetaInfo::GetBuildingVertexPropertyIndexes() {
  std::shared_lock lock(mutex_);
  std::vector<std::shared_ptr<VertexPropertyIndex>> indexes;
  AppendNamedIndexes(building_vertex_property_indexes_by_name_, &indexes, true);
  return indexes;
}

bool MetaInfo::AddVertexPropertyIndex(
    std::shared_ptr<graphdb::VertexPropertyIndex> vpi) {
  auto name = vpi->meta().name();
  auto schema_key = BuildVertexPropertyIndexKey(vpi->lid(), vpi->pids());
  std::unique_lock lock(mutex_);
  if (ready_vertex_property_indexes_by_name_.count(name) ||
      building_vertex_property_indexes_by_name_.count(name) ||
      ready_vertex_property_indexes_by_schema_.count(schema_key) ||
      building_vertex_property_indexes_by_schema_.count(schema_key)) {
    return false;
  }
  auto* by_name = vpi->IsReady() ? &ready_vertex_property_indexes_by_name_
                                 : &building_vertex_property_indexes_by_name_;
  auto* by_schema = vpi->IsReady()
                        ? &ready_vertex_property_indexes_by_schema_
                        : &building_vertex_property_indexes_by_schema_;
  by_schema->emplace(schema_key, vpi);
  by_name->emplace(std::move(name), std::move(vpi));
  return true;
}

void MetaInfo::PublishVertexPropertyIndex(const std::string& index_name) {
  std::unique_lock lock(mutex_);
  auto name_iter = building_vertex_property_indexes_by_name_.find(index_name);
  if (name_iter == building_vertex_property_indexes_by_name_.end()) {
    return;
  }
  auto index = name_iter->second;
  auto schema_key = BuildVertexPropertyIndexKey(index->lid(), index->pids());
  building_vertex_property_indexes_by_name_.erase(name_iter);
  building_vertex_property_indexes_by_schema_.erase(schema_key);
  ready_vertex_property_indexes_by_schema_.emplace(schema_key, index);
  ready_vertex_property_indexes_by_name_.emplace(index->meta().name(),
                                                 std::move(index));
}

void MetaInfo::DeleteVertexPropertyIndex(const std::string& index_name) {
  std::unique_lock lock(mutex_);
  auto name_iter = ready_vertex_property_indexes_by_name_.find(index_name);
  if (name_iter != ready_vertex_property_indexes_by_name_.end()) {
    ready_vertex_property_indexes_by_schema_.erase(BuildVertexPropertyIndexKey(
        name_iter->second->lid(), name_iter->second->pids()));
    ready_vertex_property_indexes_by_name_.erase(name_iter);
    return;
  }
  name_iter = building_vertex_property_indexes_by_name_.find(index_name);
  if (name_iter == building_vertex_property_indexes_by_name_.end()) {
    return;
  }
  building_vertex_property_indexes_by_schema_.erase(BuildVertexPropertyIndexKey(
      name_iter->second->lid(), name_iter->second->pids()));
  building_vertex_property_indexes_by_name_.erase(name_iter);
}

std::vector<std::shared_ptr<VertexFullTextIndex>>
MetaInfo::GetReadyVertexFullTextIndexes() {
  std::shared_lock lock(mutex_);
  std::vector<std::shared_ptr<VertexFullTextIndex>> indexes;
  AppendNamedIndexes(ready_vertex_ft_indexes_, &indexes, false);
  return indexes;
}

std::shared_ptr<VertexFullTextIndex> MetaInfo::GetReadyVertexFullTextIndex(
    const std::string& name) {
  std::shared_lock lock(mutex_);
  auto iter = ready_vertex_ft_indexes_.find(name);
  if (iter != ready_vertex_ft_indexes_.end()) {
    return iter->second;
  }
  return nullptr;
}

std::vector<std::shared_ptr<VertexFullTextIndex>>
MetaInfo::GetVertexFullTextIndexes() {
  std::shared_lock lock(mutex_);
  std::vector<std::shared_ptr<VertexFullTextIndex>> indexes;
  AppendNamedIndexes(ready_vertex_ft_indexes_, &indexes, false);
  AppendNamedIndexes(building_vertex_ft_indexes_, &indexes, true);
  return indexes;
}

std::shared_ptr<VertexFullTextIndex> MetaInfo::GetVertexFullTextIndex(
    const std::string& name) {
  std::shared_lock lock(mutex_);
  auto iter = ready_vertex_ft_indexes_.find(name);
  if (iter != ready_vertex_ft_indexes_.end()) {
    return iter->second;
  }
  iter = building_vertex_ft_indexes_.find(name);
  if (iter != building_vertex_ft_indexes_.end()) {
    return iter->second;
  }
  return nullptr;
}

std::vector<std::shared_ptr<VertexFullTextIndex>>
MetaInfo::GetBuildingVertexFullTextIndexes() {
  std::shared_lock lock(mutex_);
  std::vector<std::shared_ptr<VertexFullTextIndex>> indexes;
  AppendNamedIndexes(building_vertex_ft_indexes_, &indexes, true);
  return indexes;
}

void MetaInfo::DeleteVertexFullTextIndex(const std::string& name) {
  std::unique_lock lock(mutex_);
  ready_vertex_ft_indexes_.erase(name);
  building_vertex_ft_indexes_.erase(name);
}

bool MetaInfo::AddVertexFullTextIndex(std::shared_ptr<VertexFullTextIndex> ft) {
  std::string name = ft->Name();
  std::unique_lock lock(mutex_);
  if (ready_vertex_ft_indexes_.count(name) ||
      building_vertex_ft_indexes_.count(name)) {
    return false;
  }
  auto* indexes =
      ft->IsReady() ? &ready_vertex_ft_indexes_ : &building_vertex_ft_indexes_;
  indexes->emplace(name, std::move(ft));
  return true;
}

void MetaInfo::PublishVertexFullTextIndex(const std::string& name) {
  std::unique_lock lock(mutex_);
  auto iter = building_vertex_ft_indexes_.find(name);
  if (iter == building_vertex_ft_indexes_.end()) {
    return;
  }
  auto index = iter->second;
  building_vertex_ft_indexes_.erase(iter);
  ready_vertex_ft_indexes_.emplace(name, std::move(index));
}

void MetaInfo::ClearVertexFullTextIndexes() {
  std::unique_lock lock(mutex_);
  ready_vertex_ft_indexes_.clear();
  building_vertex_ft_indexes_.clear();
}

std::shared_ptr<VertexVectorIndex> MetaInfo::GetReadyVertexVectorIndex(
    uint32_t lid, uint32_t pid) {
  auto index_key = BuildVertexVectorIndexKey(lid, pid);
  std::shared_lock lock(mutex_);
  auto iter = ready_vertex_vector_indexes_.find(index_key);
  if (iter != ready_vertex_vector_indexes_.end()) {
    return iter->second;
  }
  return nullptr;
}

std::shared_ptr<VertexVectorIndex> MetaInfo::GetReadyVertexVectorIndex(
    const std::string& name) {
  std::shared_lock lock(mutex_);
  for (const auto& [_, index] : ready_vertex_vector_indexes_) {
    if (index->meta().name() == name) {
      return index;
    }
  }
  return nullptr;
}

std::shared_ptr<VertexVectorIndex> MetaInfo::GetVertexVectorIndex(
    uint32_t lid, uint32_t pid) {
  auto index_key = BuildVertexVectorIndexKey(lid, pid);
  std::shared_lock lock(mutex_);
  auto iter = ready_vertex_vector_indexes_.find(index_key);
  if (iter != ready_vertex_vector_indexes_.end()) {
    return iter->second;
  }
  iter = building_vertex_vector_indexes_.find(index_key);
  if (iter != building_vertex_vector_indexes_.end()) {
    return iter->second;
  }
  return nullptr;
}

void MetaInfo::AddVertexVectorIndex(std::shared_ptr<VertexVectorIndex> vvi) {
  auto index_key = BuildVertexVectorIndexKey(vvi->lid(), vvi->pid());
  std::unique_lock lock(mutex_);
  if (ready_vertex_vector_indexes_.count(index_key) ||
      building_vertex_vector_indexes_.count(index_key)) {
    return;
  }
  auto* indexes = vvi->IsReady() ? &ready_vertex_vector_indexes_
                                 : &building_vertex_vector_indexes_;
  indexes->emplace(index_key, std::move(vvi));
}

std::vector<std::shared_ptr<VertexVectorIndex>>
MetaInfo::GetVertexVectorIndexes() {
  std::shared_lock lock(mutex_);
  std::vector<std::shared_ptr<VertexVectorIndex>> indexes;
  indexes.reserve(ready_vertex_vector_indexes_.size() +
                  building_vertex_vector_indexes_.size());
  for (const auto& [_, index] : ready_vertex_vector_indexes_) {
    indexes.push_back(index);
  }
  for (const auto& [_, index] : building_vertex_vector_indexes_) {
    if (index->state() != meta::IndexBuildState::FAILED) {
      indexes.push_back(index);
    }
  }
  return indexes;
}

std::vector<std::shared_ptr<VertexVectorIndex>>
MetaInfo::GetBuildingVertexVectorIndexes() {
  std::shared_lock lock(mutex_);
  std::vector<std::shared_ptr<VertexVectorIndex>> indexes;
  indexes.reserve(building_vertex_vector_indexes_.size());
  for (const auto& [_, index] : building_vertex_vector_indexes_) {
    if (index->state() != meta::IndexBuildState::FAILED) {
      indexes.push_back(index);
    }
  }
  return indexes;
}

std::shared_ptr<VertexVectorIndex> MetaInfo::GetVertexVectorIndex(
    const std::string& name) {
  std::shared_lock lock(mutex_);
  for (const auto& [_, index] : ready_vertex_vector_indexes_) {
    if (index->meta().name() == name) {
      return index;
    }
  }
  for (const auto& [_, index] : building_vertex_vector_indexes_) {
    if (index->meta().name() == name) {
      return index;
    }
  }
  return nullptr;
}

void MetaInfo::PublishVertexVectorIndex(const std::string& name) {
  std::unique_lock lock(mutex_);
  for (auto iter = building_vertex_vector_indexes_.begin();
       iter != building_vertex_vector_indexes_.end(); ++iter) {
    if (iter->second->meta().name() != name) {
      continue;
    }
    auto index = iter->second;
    building_vertex_vector_indexes_.erase(iter);
    ready_vertex_vector_indexes_.emplace(
        BuildVertexVectorIndexKey(index->lid(), index->pid()),
        std::move(index));
    return;
  }
}

void MetaInfo::DeleteVertexVectorIndex(const std::string& name) {
  std::unique_lock lock(mutex_);
  for (auto iter = ready_vertex_vector_indexes_.begin();
       iter != ready_vertex_vector_indexes_.end(); ++iter) {
    if (iter->second->meta().name() == name) {
      ready_vertex_vector_indexes_.erase(iter);
      return;
    }
  }
  for (auto iter = building_vertex_vector_indexes_.begin();
       iter != building_vertex_vector_indexes_.end(); ++iter) {
    if (iter->second->meta().name() == name) {
      building_vertex_vector_indexes_.erase(iter);
      return;
    }
  }
}

void MetaInfo::ClearVertexVectorIndexes() {
  std::unique_lock lock(mutex_);
  ready_vertex_vector_indexes_.clear();
  building_vertex_vector_indexes_.clear();
}

void MetaInfo::Init(rocksdb::TransactionDB* db,
                    boost::asio::io_service& service, GraphCF* graph_cf,
                    uint16_t server_id, size_t ft_commit_interval,
                    size_t ft_writer_threads, size_t ft_writer_memory_budget,
                    size_t vt_commit_interval) {
  id_generator_.Bind(db, graph_cf, server_id);
  uint32_t max_lid = 0;
  uint32_t max_pid = 0;
  uint32_t max_tid = 0;
  uint32_t max_index_id = 0;
  rocksdb::ReadOptions ro;
  std::unique_ptr<rocksdb::Iterator> iter(
      db->NewIterator(ro, graph_cf->meta_info));
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    auto key = iter->key();
    if (key.empty()) {
      continue;
    }
    auto val = iter->value();
    auto prefix = static_cast<MetaDataType>(key.data()[0]);
    if (prefix == MetaDataType::VertexLabel ||
        prefix == MetaDataType::EdgeType || prefix == MetaDataType::Property) {
      std::string name(key.data() + 1, key.size() - 1);
      uint32_t id = ReadValue<uint32_t>(val.data());
      id_generator_.LoadToken(prefix, name, id);
      uint32_t native_id = big_to_native(id);
      if (prefix == MetaDataType::VertexLabel) {
        max_lid = std::max(max_lid, native_id);
      } else if (prefix == MetaDataType::EdgeType) {
        max_tid = std::max(max_tid, native_id);
      } else {
        max_pid = std::max(max_pid, native_id);
      }
      continue;
    }
    if (prefix == MetaDataType::VertexPropertyIndex) {
      meta::VertexPropertyIndex meta;
      bool ret = meta.ParseFromString(val.ToString());
      assert(ret);
      LOG_INFO("vertex property index: [{}]", meta.ShortDebugString());
      max_index_id = std::max(max_index_id, meta.index_id());
      uint32_t lid = native_to_big(meta.label_id());
      std::vector<uint32_t> pids;
      pids.reserve(meta.property_ids_size());
      for (auto pid : meta.property_ids()) {
        pids.push_back(native_to_big(pid));
      }
      uint32_t index_id = native_to_big(meta.index_id());
      auto vi = std::make_shared<VertexPropertyIndex>(
          db, graph_cf, meta, graph_cf->index, index_id, lid, std::move(pids));
      AddVertexPropertyIndex(std::move(vi));
      continue;
    }
    if (prefix == MetaDataType::VertexFullTextIndex) {
      meta::VertexFullTextIndex meta;
      bool ret = meta.ParseFromString(val.ToString());
      assert(ret);
      LOG_INFO("vertex fulltext index: [{}]", meta.ShortDebugString());
      max_index_id = std::max(max_index_id, meta.index_id());
      std::unordered_set<uint32_t> lids, pids;
      for (auto id : meta.label_ids()) {
        lids.insert(native_to_big(id));
      }
      for (auto id : meta.property_ids()) {
        pids.insert(native_to_big(id));
      }
      auto v_ft_index = std::make_shared<VertexFullTextIndex>(
          db, service, graph_cf, &id_generator_, meta,
          native_to_big(meta.index_id()), ft_writer_threads,
          ft_writer_memory_budget, lids, pids, ft_commit_interval);
      AddVertexFullTextIndex(v_ft_index);
      if (meta.state() == meta::IndexBuildState::READY) {
        v_ft_index->Start();
      }
      continue;
    }
    if (prefix == MetaDataType::VertexVectorIndex) {
      meta::VertexVectorIndex meta;
      bool ret = meta.ParseFromString(val.ToString());
      assert(ret);
      LOG_INFO("vertex vector index: [{}]", meta.ShortDebugString());
      max_index_id = std::max(max_index_id, meta.index_id());
      auto index = std::make_shared<VertexVectorIndex>(
          db, service, graph_cf, native_to_big(meta.index_id()),
          native_to_big(meta.label_id()), native_to_big(meta.property_id()),
          meta, vt_commit_interval);
      AddVertexVectorIndex(index);
      if (meta.state() == meta::IndexBuildState::READY) {
        index->Start();
      }
    }
  }
  id_generator_.SetMaxIds(max_lid, max_pid, max_tid, max_index_id);
}
}  // namespace graphdb
