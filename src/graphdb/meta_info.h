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

#pragma once
#include <cinttypes>
#include <memory>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "graph_cf.h"
#include "id_generator.h"
#include "index.h"
namespace graphdb {
struct MetaInfo {
  // property index
  void Init(rocksdb::TransactionDB* db, boost::asio::io_service& service,
            GraphCF* graph_cf, uint16_t server_id, size_t ft_commit_interval,
            size_t ft_writer_threads, size_t ft_writer_memory_budget,
            size_t vt_commit_interval);
  IdGenerator& id_generator() { return id_generator_; }
  std::shared_ptr<VertexPropertyIndex> GetReadyVertexPropertyIndex(uint32_t lid,
                                                                   uint32_t pid);
  std::shared_ptr<VertexPropertyIndex> GetReadyVertexPropertyIndex(
      uint32_t lid, const std::vector<uint32_t>& pids);
  std::shared_ptr<VertexPropertyIndex> GetReadyVertexPropertyIndex(
      const std::string& index_name);
  std::shared_ptr<VertexPropertyIndex> GetVertexPropertyIndex(uint32_t lid,
                                                              uint32_t pid);
  std::shared_ptr<VertexPropertyIndex> GetVertexPropertyIndex(
      uint32_t lid, const std::vector<uint32_t>& pids);
  std::shared_ptr<VertexPropertyIndex> GetBestVertexPropertyUniqueIndex(
      uint32_t lid, const std::unordered_set<uint32_t>& pids);
  std::shared_ptr<VertexPropertyIndex> GetVertexPropertyIndex(
      const std::string& index_name);
  std::vector<std::shared_ptr<VertexPropertyIndex>> GetVertexPropertyIndexes();
  std::vector<std::shared_ptr<VertexPropertyIndex>>
  GetBuildingVertexPropertyIndexes();
  bool AddVertexPropertyIndex(std::shared_ptr<VertexPropertyIndex> vpi);
  void PublishVertexPropertyIndex(const std::string& index_name);
  void DeleteVertexPropertyIndex(const std::string& index_name);

  // fulltext index
  std::vector<std::shared_ptr<VertexFullTextIndex>> GetReadyVertexFullTextIndexes();
  std::shared_ptr<VertexFullTextIndex> GetReadyVertexFullTextIndex(
      const std::string& name);
  std::vector<std::shared_ptr<VertexFullTextIndex>> GetVertexFullTextIndexes();
  std::shared_ptr<VertexFullTextIndex> GetVertexFullTextIndex(
      const std::string& name);
  std::vector<std::shared_ptr<VertexFullTextIndex>>
  GetBuildingVertexFullTextIndexes();
  void DeleteVertexFullTextIndex(const std::string& name);
  bool AddVertexFullTextIndex(std::shared_ptr<VertexFullTextIndex> ft);
  void PublishVertexFullTextIndex(const std::string& name);
  void ClearVertexFullTextIndexes();

  // vector index
  std::shared_ptr<VertexVectorIndex> GetReadyVertexVectorIndex(uint32_t lid,
                                                               uint32_t pid);
  std::shared_ptr<VertexVectorIndex> GetReadyVertexVectorIndex(
      const std::string& index_name);
  std::shared_ptr<VertexVectorIndex> GetVertexVectorIndex(uint32_t lid,
                                                          uint32_t pid);
  void AddVertexVectorIndex(std::shared_ptr<VertexVectorIndex> vvi);
  std::vector<std::shared_ptr<VertexVectorIndex>> GetVertexVectorIndexes();
  std::vector<std::shared_ptr<VertexVectorIndex>>
  GetBuildingVertexVectorIndexes();
  std::shared_ptr<VertexVectorIndex> GetVertexVectorIndex(
      const std::string& index_name);
  void PublishVertexVectorIndex(const std::string& name);
  void DeleteVertexVectorIndex(const std::string& name);
  void ClearVertexVectorIndexes();

 private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<std::string, std::shared_ptr<VertexPropertyIndex>>
      ready_vertex_property_indexes_by_name_;
  std::unordered_map<std::string, std::shared_ptr<VertexPropertyIndex>>
      ready_vertex_property_indexes_by_schema_;
  std::unordered_map<std::string, std::shared_ptr<VertexPropertyIndex>>
      building_vertex_property_indexes_by_name_;
  std::unordered_map<std::string, std::shared_ptr<VertexPropertyIndex>>
      building_vertex_property_indexes_by_schema_;
  std::unordered_map<uint64_t, std::shared_ptr<VertexVectorIndex>>
      ready_vertex_vector_indexes_;
  std::unordered_map<uint64_t, std::shared_ptr<VertexVectorIndex>>
      building_vertex_vector_indexes_;
  std::unordered_map<std::string, std::shared_ptr<VertexFullTextIndex>>
      ready_vertex_ft_indexes_;
  std::unordered_map<std::string, std::shared_ptr<VertexFullTextIndex>>
      building_vertex_ft_indexes_;
  IdGenerator id_generator_;
};
}  // namespace graphdb
