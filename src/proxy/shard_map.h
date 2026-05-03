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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace proxy {

struct BackendEndpoint {
  uint64_t node_id = 0;
  std::string name;
  std::string host;
  uint32_t port = 0;
  uint32_t raft_port = 0;
};

struct ShardReplicaGroup {
  size_t shard_id = 0;
  std::string graph_name;
  std::vector<BackendEndpoint> replicas;
};

struct ShardRoute {
  size_t shard_id = 0;
  std::string graph_name;
  const ShardReplicaGroup* replica_group = nullptr;
};

class ShardMap {
 public:
  static ShardMap FromConfig(std::string logical_graph,
                             std::string physical_graph_prefix,
                             size_t shard_count, size_t shard_id_width,
                             const std::string& backend_specs);

  ShardRoute Route(const std::string& logical_graph,
                   const std::string& shard_key) const;
  const std::string& logical_graph() const { return logical_graph_; }
  size_t shard_count() const { return shards_.size(); }
  static uint64_t StableHash(const std::string& key);

 private:
  std::string FormatGraphName(size_t shard_id) const;

  std::string logical_graph_;
  std::string physical_graph_prefix_;
  size_t shard_id_width_ = 2;
  std::vector<ShardReplicaGroup> shards_;
};

}  // namespace proxy
