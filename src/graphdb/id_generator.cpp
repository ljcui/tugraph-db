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

#include "id_generator.h"

#include <algorithm>
#include <boost/endian/conversion.hpp>

#include "common/byte_utils.h"
#include "common/exceptions.h"
#include "common/logger.h"
using namespace boost::endian;
using common::AsChars;
namespace graphdb {
namespace {

std::string TokenKey(MetaDataType type, const std::string &name) {
  std::string key;
  key.append(1, static_cast<char>(type));
  key.append(name);
  return key;
}

std::string EntityIdKey(MetaDataType type) {
  return std::string(1, static_cast<char>(type));
}

}  // namespace

void IdGenerator::Bind(rocksdb::TransactionDB *db, GraphCF *graph_cf) {
  db_ = db;
  graph_cf_ = graph_cf;
}

void IdGenerator::LoadToken(MetaDataType type, const std::string &name,
                            uint32_t id) {
  if (type == MetaDataType::VertexLabel) {
    vertex_labels_name_to_id_[name] = id;
    vertex_labels_id_to_name_[id] = name;
  } else if (type == MetaDataType::EdgeType) {
    edge_types_name_to_id_[name] = id;
    edge_types_id_to_name_[id] = name;
  } else if (type == MetaDataType::Property) {
    properties_name_to_id_[name] = id;
    properties_id_to_name_[id] = name;
  } else {
    THROW_CODE(InvalidParameter, "unsupported token metadata type {}",
               static_cast<int>(type));
  }
}

void IdGenerator::SetMaxIds(uint32_t max_lid, uint32_t max_pid,
                            uint32_t max_tid, uint32_t max_index_id) {
  LOG_INFO("max_lid:{}, max_pid:{}, max_tid:{}, max_index_id:{}", max_lid,
           max_pid, max_tid, max_index_id);
  label_next_lid_ = max_lid + 1;
  label_next_pid_ = max_pid + 1;
  label_next_tid_ = max_tid + 1;
  index_next_id_ = max_index_id + 1;
}

void IdGenerator::SetNextEntityIds(int64_t next_vid, int64_t next_eid) {
  if (next_vid < 1 || next_eid < 1) {
    THROW_CODE(InvalidParameter, "entity id range must start from positive id");
  }
  persisted_next_vid_ = next_vid;
  next_vid_ = next_vid;
  vid_range_end_ = next_vid;
  persisted_next_eid_ = next_eid;
  next_eid_ = next_eid;
  eid_range_end_ = next_eid;
}

int64_t IdGenerator::GetNextEntityId(std::atomic<int64_t> *next_id,
                                     std::atomic<int64_t> *range_end,
                                     std::atomic<int64_t> *persisted_next_id,
                                     std::mutex *refill_mutex,
                                     MetaDataType meta_type) {
  for (;;) {
    int64_t candidate = next_id->load();
    int64_t limit = range_end->load();
    while (candidate < limit) {
      if (next_id->compare_exchange_weak(candidate, candidate + 1)) {
        return native_to_big(candidate);
      }
    }

    std::unique_lock refill_lock(*refill_mutex);
    candidate = next_id->load();
    limit = range_end->load();
    if (candidate < limit) {
      continue;
    }

    int64_t start = persisted_next_id->load();
    if (start > std::numeric_limits<int64_t>::max() - kIdRangeSize) {
      THROW_CODE(StorageEngineError, "entity id range exhausted");
    }
    int64_t end = start + kIdRangeSize;
    int64_t bigendian_end = native_to_big(end);
    auto s =
        db_->Put({}, graph_cf_->meta_info, EntityIdKey(meta_type),
                 std::string(AsChars(bigendian_end), sizeof(bigendian_end)));
    if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
    persisted_next_id->store(end);
    next_id->store(start + 1);
    range_end->store(end);
    return native_to_big(start);
  }
}

int64_t IdGenerator::GetNextVid() {
  return GetNextEntityId(&next_vid_, &vid_range_end_, &persisted_next_vid_,
                         &vid_refill_mutex_, MetaDataType::NextVertexId);
}

int64_t IdGenerator::GetNextEid() {
  return GetNextEntityId(&next_eid_, &eid_range_end_, &persisted_next_eid_,
                         &eid_refill_mutex_, MetaDataType::NextEdgeId);
}

uint32_t IdGenerator::GetNextIndexId() {
  return native_to_big(index_next_id_++);
}

std::optional<uint32_t> IdGenerator::GetLid(const std::string &name) {
  if (name.empty()) {
    THROW_CODE(InvalidParameter, "label name is empty");
  }
  std::shared_lock read_lock(vertex_labels_mutex_);
  auto iter = vertex_labels_name_to_id_.find(name);
  if (iter != vertex_labels_name_to_id_.end()) {
    return iter->second;
  } else {
    return {};
  }
}

std::optional<uint32_t> IdGenerator::GetPid(const std::string &name) {
  if (name.empty()) {
    THROW_CODE(InvalidParameter, "property name is empty");
  }
  std::shared_lock read_lock(properties_mutex_);
  auto iter = properties_name_to_id_.find(name);
  if (iter != properties_name_to_id_.end()) {
    return iter->second;
  } else {
    return {};
  }
}

std::optional<uint32_t> IdGenerator::GetTid(const std::string &name) {
  if (name.empty()) {
    THROW_CODE(InvalidParameter, "edge type is empty");
  }
  std::shared_lock read_lock(edge_types_mutex_);
  auto iter = edge_types_name_to_id_.find(name);
  if (iter != edge_types_name_to_id_.end()) {
    return iter->second;
  } else {
    return {};
  }
}

std::optional<std::string> IdGenerator::GetPropertyName(uint32_t pid) {
  std::shared_lock read_lock(properties_mutex_);
  auto iter = properties_id_to_name_.find(pid);
  if (iter != properties_id_to_name_.end()) {
    return iter->second;
  } else {
    return {};
  }
}

std::optional<std::string> IdGenerator::GetVertexLabelName(uint32_t lid) {
  std::shared_lock read_lock(vertex_labels_mutex_);
  auto iter = vertex_labels_id_to_name_.find(lid);
  if (iter != vertex_labels_id_to_name_.end()) {
    return iter->second;
  } else {
    return {};
  }
}

std::optional<std::string> IdGenerator::GetEdgeTypeName(uint32_t tid) {
  std::shared_lock read_lock(edge_types_mutex_);
  auto iter = edge_types_id_to_name_.find(tid);
  if (iter != edge_types_id_to_name_.end()) {
    return iter->second;
  } else {
    return {};
  }
}

uint32_t IdGenerator::GetOrCreateLid(const std::string &name) {
  if (name.empty()) {
    THROW_CODE(InvalidParameter, "label name is empty");
  }
  {
    std::shared_lock read_lock(vertex_labels_mutex_);
    auto iter = vertex_labels_name_to_id_.find(name);
    if (iter != vertex_labels_name_to_id_.end()) {
      return iter->second;
    }
  }
  {
    std::unique_lock write_lock(vertex_labels_mutex_);
    auto iter = vertex_labels_name_to_id_.find(name);
    if (iter != vertex_labels_name_to_id_.end()) {
      return iter->second;
    }
    std::string key = TokenKey(MetaDataType::VertexLabel, name);
    std::string val;
    uint32_t bigendian_lid = native_to_big(label_next_lid_++);
    val.append(AsChars(bigendian_lid), sizeof(bigendian_lid));
    rocksdb::WriteOptions options;
    auto s = db_->Put(options, graph_cf_->meta_info, key, val);
    if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
    vertex_labels_name_to_id_[name] = bigendian_lid;
    vertex_labels_id_to_name_[bigendian_lid] = name;
    return bigendian_lid;
  }
}

uint32_t IdGenerator::GetOrCreateTid(const std::string &name) {
  if (name.empty()) {
    THROW_CODE(InvalidParameter, "edge type is empty");
  }
  {
    std::shared_lock read_lock(edge_types_mutex_);
    auto iter = edge_types_name_to_id_.find(name);
    if (iter != edge_types_name_to_id_.end()) {
      return iter->second;
    }
  }
  {
    std::unique_lock write_lock(edge_types_mutex_);
    auto iter = edge_types_name_to_id_.find(name);
    if (iter != edge_types_name_to_id_.end()) {
      return iter->second;
    }
    std::string key = TokenKey(MetaDataType::EdgeType, name);
    std::string val;
    uint32_t bigendian_tid = native_to_big(label_next_tid_++);
    val.append(AsChars(bigendian_tid), sizeof(bigendian_tid));
    rocksdb::WriteOptions options;
    auto s = db_->Put(options, graph_cf_->meta_info, key, val);
    if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
    edge_types_name_to_id_[name] = bigendian_tid;
    edge_types_id_to_name_[bigendian_tid] = name;
    return bigendian_tid;
  }
}

uint32_t IdGenerator::GetOrCreatePid(const std::string &name) {
  if (name.empty()) {
    THROW_CODE(InvalidParameter, "property name is empty");
  }
  {
    std::shared_lock read_lock(properties_mutex_);
    auto iter = properties_name_to_id_.find(name);
    if (iter != properties_name_to_id_.end()) {
      return iter->second;
    }
  }
  {
    std::unique_lock write_lock(properties_mutex_);
    auto iter = properties_name_to_id_.find(name);
    if (iter != properties_name_to_id_.end()) {
      return iter->second;
    }
    std::string key = TokenKey(MetaDataType::Property, name);
    std::string val;
    uint32_t bigendian_pid = native_to_big(label_next_pid_++);
    val.append(AsChars(bigendian_pid), sizeof(bigendian_pid));
    rocksdb::WriteOptions options;
    auto s = db_->Put(options, graph_cf_->meta_info, key, val);
    if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
    properties_name_to_id_[name] = bigendian_pid;
    properties_id_to_name_[bigendian_pid] = name;
    return bigendian_pid;
  }
}

std::unordered_set<std::string> IdGenerator::GetProperties() {
  std::unordered_set<std::string> ret;
  std::shared_lock read_lock(properties_mutex_);
  for (const auto &[key, val] : properties_name_to_id_) {
    ret.insert(key);
  }
  return ret;
}

std::unordered_set<std::string> IdGenerator::GetVertexLabels() {
  std::unordered_set<std::string> ret;
  std::shared_lock read_lock(vertex_labels_mutex_);
  for (const auto &[key, val] : vertex_labels_name_to_id_) {
    ret.insert(key);
  }
  return ret;
}

std::unordered_set<std::string> IdGenerator::GetEdgeTypes() {
  std::unordered_set<std::string> ret;
  std::shared_lock read_lock(edge_types_mutex_);
  for (const auto &[key, val] : edge_types_name_to_id_) {
    ret.insert(key);
  }
  return ret;
}
}  // namespace graphdb
