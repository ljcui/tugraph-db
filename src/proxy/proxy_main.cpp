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

#include <gflags/gflags.h>

#include <chrono>
#include <csignal>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "common/flags.h"
#include "common/logger.h"
#include "common/version.h"
#include "proxy/proxy_server.h"

DEFINE_uint32(proxy_bolt_port, 17687, "Bolt port listened by lgraph_proxy");
DEFINE_uint32(proxy_bolt_io_thread_num, 2,
              "Number of lgraph_proxy Bolt io threads");
DEFINE_uint32(proxy_worker_thread_num, 16,
              "Number of lgraph_proxy worker threads");
DEFINE_uint64(proxy_max_connections, 10000,
              "Maximum number of open proxy Bolt connections. 0 means "
              "unlimited.");
DEFINE_uint64(proxy_max_pending_messages_per_connection, 1024,
              "Maximum pending proxy Bolt messages per connection. 0 means "
              "unlimited.");
DEFINE_string(proxy_logical_graph, "default",
              "Logical graph name exposed by lgraph_proxy");
DEFINE_string(proxy_physical_graph_prefix, "default_s",
              "Physical graph name prefix. Shard id is appended after it.");
DEFINE_uint64(proxy_shard_count, 64, "Number of physical graph shards");
DEFINE_uint64(proxy_shard_id_width, 2,
              "Zero-padding width used when formatting physical shard graph "
              "names.");
DEFINE_string(
    proxy_backends, "",
    "Comma-separated backend specs in host:port:shard_begin-shard_end "
    "format, for example "
    "127.0.0.1:7687:0-15,127.0.0.1:7688:16-31.");

namespace {

volatile std::sig_atomic_t g_shutdown_signal = 0;

void ShutDownHandler(int sig) { g_shutdown_signal = sig; }

void SetupSignalHandler() {
  struct sigaction sa {};
  sa.sa_handler = ShutDownHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);
}

std::string Version() {
  std::ostringstream info;
  std::string version;
  version.append(std::to_string(LGRAPH_VERSION_MAJOR))
      .append(".")
      .append(std::to_string(LGRAPH_VERSION_MINOR))
      .append(".")
      .append(std::to_string(LGRAPH_VERSION_PATCH));
  info << "TuGraph lgraph_proxy v" << version << ", commit " << GIT_COMMIT_HASH;
  return info.str();
}

}  // namespace

int main(int argc, char* argv[]) {
  gflags::SetVersionString(Version());
  gflags::SetUsageMessage("Usage: " + std::string(argv[0]) +
                          " --proxy_backends=host:port:0-63");
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  spdlog::set_pattern("%v");
  if (!SetupLogger()) {
    return -1;
  }
  SetupSignalHandler();

  try {
    auto shard_map = proxy::ShardMap::FromConfig(
        FLAGS_proxy_logical_graph, FLAGS_proxy_physical_graph_prefix,
        FLAGS_proxy_shard_count, FLAGS_proxy_shard_id_width,
        FLAGS_proxy_backends);
    proxy::ProxyServer server(
        {.listen_port = FLAGS_proxy_bolt_port,
         .bolt_io_thread_num = FLAGS_proxy_bolt_io_thread_num,
         .worker_thread_num = FLAGS_proxy_worker_thread_num,
         .max_connections = FLAGS_proxy_max_connections,
         .max_pending_messages_per_connection =
             FLAGS_proxy_max_pending_messages_per_connection,
         .bolt_connection_options =
             {.handshake_timeout_seconds = FLAGS_bolt_handshake_timeout_seconds,
              .login_timeout_seconds = FLAGS_bolt_login_timeout_seconds,
              .idle_timeout_seconds = FLAGS_bolt_idle_timeout_seconds},
         .shard_map = std::move(shard_map)});
    if (!server.Start()) {
      throw std::runtime_error("failed to start lgraph_proxy");
    }
    LOG_INFO("{}", Version());
    g_shutdown_signal = 0;
    while (g_shutdown_signal == 0 && server.Started()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (g_shutdown_signal != 0) {
      LOG_INFO("lgraph_proxy received signal {}, shutdown",
               static_cast<int>(g_shutdown_signal));
    }
    server.Stop();
    spdlog::shutdown();
    return 0;
  } catch (const std::exception& e) {
    LOG_ERROR("lgraph_proxy error: {}", e.what());
    return -1;
  }
}
