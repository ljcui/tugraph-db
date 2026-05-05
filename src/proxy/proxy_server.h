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

#include <atomic>
#include <cstdint>
#include <memory>

#include "bolt/bolt_server.h"
#include "bolt/connection.h"
#include "proxy/shard_map.h"

namespace proxy {

struct ProxyServerOptions {
  uint32_t listen_port = 7687;
  uint32_t bolt_io_thread_num = 2;
  uint32_t worker_thread_num = 8;
  uint64_t max_connections = 10000;
  uint64_t backend_max_connections_per_backend = 32;
  uint64_t backend_borrow_timeout_ms = 1000;
  ShardMap shard_map;
};

class ProxyServer final {
 public:
  explicit ProxyServer(ProxyServerOptions options);
  bool Start();
  void Stop();
  bool Started() const;
  ~ProxyServer() { Stop(); }

 private:
  ProxyServerOptions options_;
  bolt::BoltServer bolt_server_;
  std::atomic<bool> started_{false};
};

}  // namespace proxy
