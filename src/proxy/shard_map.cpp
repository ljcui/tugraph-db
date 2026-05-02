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

#include "proxy/shard_map.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "common/exceptions.h"

namespace proxy {
namespace {

std::string Trim(std::string value) {
  auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
  value.erase(value.begin(),
              std::find_if(value.begin(), value.end(),
                           [&](unsigned char c) { return !is_space(c); }));
  value.erase(std::find_if(value.rbegin(), value.rend(),
                           [&](unsigned char c) { return !is_space(c); })
                  .base(),
              value.end());
  return value;
}

std::vector<std::string> Split(const std::string& value, char delim) {
  std::vector<std::string> parts;
  std::stringstream ss(value);
  std::string item;
  while (std::getline(ss, item, delim)) {
    parts.emplace_back(Trim(std::move(item)));
  }
  return parts;
}

uint64_t ParsePositiveUInt64(const std::string& value,
                             const std::string& field_name) {
  size_t pos = 0;
  auto parsed = std::stoull(value, &pos, 10);
  if (pos != value.size() || parsed == 0) {
    THROW_CODE(InvalidParameter, "invalid {} [{}]", field_name, value);
  }
  return parsed;
}

uint32_t ParsePort(const std::string& value, const std::string& field_name) {
  size_t pos = 0;
  auto parsed = std::stoul(value, &pos, 10);
  if (pos != value.size() || parsed == 0 ||
      parsed > std::numeric_limits<uint32_t>::max()) {
    THROW_CODE(InvalidParameter, "invalid {} [{}]", field_name, value);
  }
  return static_cast<uint32_t>(parsed);
}

std::pair<size_t, size_t> ParseShardRange(const std::string& value,
                                          size_t shard_count) {
  auto dash = value.find('-');
  std::string first = value;
  std::string second = value;
  if (dash != std::string::npos) {
    first = value.substr(0, dash);
    second = value.substr(dash + 1);
  }
  size_t pos = 0;
  auto begin = std::stoull(first, &pos, 10);
  if (pos != first.size()) {
    THROW_CODE(InvalidParameter, "invalid proxy shard range [{}]", value);
  }
  pos = 0;
  auto end = std::stoull(second, &pos, 10);
  if (pos != second.size()) {
    THROW_CODE(InvalidParameter, "invalid proxy shard range [{}]", value);
  }
  if (begin > end || end >= shard_count) {
    THROW_CODE(InvalidParameter,
               "proxy shard range [{}] is outside shard count {}", value,
               shard_count);
  }
  return {begin, end};
}

BackendEndpoint ParseReplica(const std::string& value) {
  auto at = value.find('@');
  if (at == std::string::npos || at == 0 || at + 1 >= value.size()) {
    THROW_CODE(InvalidParameter,
               "invalid proxy raft replica [{}], expected "
               "node_id@host:bolt_port:raft_port",
               value);
  }
  auto node_id = ParsePositiveUInt64(value.substr(0, at), "raft node_id");
  auto parts = Split(value.substr(at + 1), ':');
  if (parts.size() != 3 || parts[0].empty() || parts[1].empty() ||
      parts[2].empty()) {
    THROW_CODE(InvalidParameter,
               "invalid proxy raft replica [{}], expected "
               "node_id@host:bolt_port:raft_port",
               value);
  }

  BackendEndpoint endpoint;
  endpoint.node_id = node_id;
  endpoint.host = parts[0];
  endpoint.port = ParsePort(parts[1], "bolt_port");
  endpoint.raft_port = ParsePort(parts[2], "raft_port");
  endpoint.name = std::to_string(endpoint.node_id) + "@" + endpoint.host + ":" +
                  std::to_string(endpoint.port);
  return endpoint;
}

std::vector<BackendEndpoint> ParseReplicas(const std::string& value) {
  std::vector<BackendEndpoint> replicas;
  for (const auto& raw_replica : Split(value, ',')) {
    if (raw_replica.empty()) {
      continue;
    }
    replicas.emplace_back(ParseReplica(raw_replica));
  }
  if (replicas.empty()) {
    THROW_CODE(InvalidParameter, "proxy raft replica list should not be empty");
  }
  return replicas;
}

}  // namespace

ShardMap ShardMap::FromConfig(std::string logical_graph,
                              std::string physical_graph_prefix,
                              size_t shard_count, size_t shard_id_width,
                              const std::string& backend_specs) {
  if (logical_graph.empty()) {
    THROW_CODE(InvalidParameter, "proxy logical graph should not be empty");
  }
  if (physical_graph_prefix.empty()) {
    THROW_CODE(InvalidParameter,
               "proxy physical graph prefix should not be empty");
  }
  if (shard_count == 0) {
    THROW_CODE(InvalidParameter, "proxy shard count should be greater than 0");
  }
  if (backend_specs.empty()) {
    THROW_CODE(InvalidParameter, "proxy backend specs should not be empty");
  }

  ShardMap map;
  map.logical_graph_ = std::move(logical_graph);
  map.physical_graph_prefix_ = std::move(physical_graph_prefix);
  map.shard_id_width_ = shard_id_width;
  map.shards_.resize(shard_count);

  for (size_t shard_id = 0; shard_id < map.shards_.size(); ++shard_id) {
    map.shards_[shard_id].shard_id = shard_id;
  }

  for (const auto& raw_spec : Split(backend_specs, ';')) {
    if (raw_spec.empty()) {
      continue;
    }
    auto equal = raw_spec.find('=');
    if (equal == std::string::npos || equal == 0 ||
        equal + 1 >= raw_spec.size()) {
      THROW_CODE(InvalidParameter,
                 "invalid proxy raft backend spec [{}], expected "
                 "shard_begin-shard_end=node_id@host:bolt_port:raft_port,...",
                 raw_spec);
    }
    auto [begin, end] = ParseShardRange(raw_spec.substr(0, equal), shard_count);
    auto replicas = ParseReplicas(raw_spec.substr(equal + 1));
    for (size_t shard_id = begin; shard_id <= end; ++shard_id) {
      if (!map.shards_[shard_id].replicas.empty()) {
        THROW_CODE(InvalidParameter, "proxy shard {} is assigned twice",
                   shard_id);
      }
      map.shards_[shard_id].replicas = replicas;
    }
  }

  for (size_t shard_id = 0; shard_id < map.shards_.size(); ++shard_id) {
    if (map.shards_[shard_id].replicas.empty()) {
      THROW_CODE(InvalidParameter, "proxy shard {} has no backend", shard_id);
    }
  }
  return map;
}

ShardRoute ShardMap::Route(const std::string& logical_graph,
                           const std::string& shard_key) const {
  if (!logical_graph.empty() && logical_graph != logical_graph_) {
    THROW_CODE(InvalidParameter,
               "proxy only serves logical graph [{}], request graph [{}]",
               logical_graph_, logical_graph);
  }
  if (shard_key.empty()) {
    THROW_CODE(InvalidParameter, "proxy shard key should not be empty");
  }
  auto shard_id = StableHash(shard_key) % shards_.size();
  return {.shard_id = shard_id,
          .graph_name = FormatGraphName(shard_id),
          .replica_group = &shards_[shard_id]};
}

uint64_t ShardMap::StableHash(const std::string& key) {
  uint64_t hash = 14695981039346656037ULL;
  for (unsigned char c : key) {
    hash ^= static_cast<uint64_t>(c);
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::string ShardMap::FormatGraphName(size_t shard_id) const {
  std::ostringstream out;
  out << physical_graph_prefix_;
  if (shard_id_width_ > 0) {
    out << std::setw(static_cast<int>(shard_id_width_)) << std::setfill('0');
  }
  out << shard_id;
  return out.str();
}

}  // namespace proxy
