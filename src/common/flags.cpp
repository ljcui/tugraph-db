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

/*
 * written by botu.wzy
 */

#include "flags.h"

#include <spdlog/fmt/fmt.h>

#include <iostream>
#include <set>

DEFINE_string(mode, "run",
              "Mode to run the server in. "
              "'run' - run the server directly. "
              "'start/restart/stop' - run the server in daemon mode");
DEFINE_string(data_path, "data", "Directory where the graph data is stored");
DEFINE_string(pid_file, "lgraph.pid", "Pid file");

DEFINE_string(log_path, "log", "Log file path");
DEFINE_string(log_level, "info", "Log level");
DEFINE_uint64(log_max_size, (uint64_t)64 * 1024 * 1024,
              "Maximum size of a single log file");
DEFINE_uint32(log_max_files, 10, "Maximum number of log files");

DEFINE_bool(enable_query_log, false, "Whether to enable query log.");
DEFINE_string(query_log_path, "log", "Query log file path.");
DEFINE_uint64(query_log_max_size, (uint64_t)64 * 1024 * 1024,
              "Maximum size of a single query log file");
DEFINE_uint32(query_log_max_files, 10, "Maximum number of query log files");

DEFINE_uint32(log_flush_interval, 3,
              "The interval to write the log to file, in seconds.");

DEFINE_string(host, "0.0.0.0", "Host ip");
DEFINE_uint32(bolt_port, 7687, "Bolt port");
DEFINE_uint32(raft_port, 7688, "Raft port");
DEFINE_uint32(bolt_io_thread_num, 2, "Number of Bolt io thread");
DEFINE_uint32(bolt_worker_thread_num, 64, "Number of Bolt worker threads");
DEFINE_uint64(max_bolt_connections, 10000,
              "Maximum number of open Bolt connections. 0 means unlimited.");
DEFINE_uint64(max_pending_bolt_messages_per_connection, 1024,
              "Maximum pending Bolt messages per connection. 0 means "
              "unlimited.");
DEFINE_uint32(bolt_handshake_timeout_seconds, 5,
              "Bolt protocol handshake timeout in seconds. 0 disables it.");
DEFINE_uint32(bolt_login_timeout_seconds, 10,
              "Bolt HELLO/login timeout in seconds. 0 disables it.");
DEFINE_uint32(bolt_idle_timeout_seconds, 1800,
              "Bolt idle connection timeout in seconds. 0 disables it.");

DEFINE_uint64(block_cache, (uint64_t)8 * 1024 * 1024 * 1024,
              "Block data cache size, in bytes.");
DEFINE_uint64(row_cache, (uint64_t)8 * 1024 * 1024 * 1024,
              "Row data cache size, in bytes.");
DEFINE_uint64(ft_apply_interval, (uint64_t)1,
              "Fulltext index WAL auto apply interval, in seconds.");
DEFINE_uint64(ft_writer_threads, (uint64_t)1,
              "Number of Tantivy indexing worker threads per fulltext index.");
DEFINE_uint64(ft_writer_memory_budget, (uint64_t)50 * 1000 * 1000,
              "Total Tantivy writer memory budget per fulltext index, in "
              "bytes.");
DEFINE_uint64(vt_apply_interval, (uint64_t)1,
              "Vector index WAL auto apply interval, in seconds.");
DEFINE_uint64(vt_serialize_interval, (uint64_t)10000,
              "Vector index serialize interval.");

bool validate_mode(const char* flagname, const std::string& mode) {
  std::set<std::string> vals = {"run", "start", "restart", "stop"};
  if (!vals.count(mode)) {
    std::cerr << fmt::format("Invalid value for --{}: {}", flagname, mode)
              << std::endl;
    return false;
  }
  return true;
}
DEFINE_validator(mode, &validate_mode);
