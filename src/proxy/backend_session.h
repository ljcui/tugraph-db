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

#include <any>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "bolt/hydrator.h"
#include "bolt/messages.h"
#include "bolt/record.h"
#include "proxy/shard_map.h"

namespace proxy {

struct RaftNodeEndpoint {
  uint64_t node_id = 0;
  std::string host;
  uint32_t port = 0;
  uint32_t raft_port = 0;
  bool is_leader = false;
};

struct BackendMessage {
  std::string raw;
  std::string payload;
  bolt::BoltMsg tag = bolt::BoltMsg::Ignored;
  bool success_has_more = false;
  std::string failure_code;
  std::string failure_message;
  std::optional<bolt::Record> record;
};

class BoltBackendSession {
 public:
  BoltBackendSession(BackendEndpoint endpoint,
                     std::unordered_map<std::string, std::any> hello_meta);
  ~BoltBackendSession();

  std::vector<BackendMessage> SendAndReadUntilTerminal(
      const std::string& request, bool decode_records = false);
  BackendMessage SendAndForwardUntilTerminal(
      const std::string& request,
      const std::function<void(const BackendMessage&)>& forward,
      bool decode_records = false);
  std::vector<RaftNodeEndpoint> FetchRaftNodeInfos(
      const std::string& graph_name);
  void Close();

 private:
  void EnsureConnected();
  void Connect();
  void ConnectWithTimeout(
      const boost::asio::ip::tcp::resolver::results_type& endpoints);
  void WriteWithTimeout(const void* data, size_t size, const char* operation);
  void ReadWithTimeout(void* data, size_t size, const char* operation);
  void RunWithTimeout(
      const char* operation, uint32_t timeout_seconds,
      const std::function<
          void(const std::function<void(const boost::system::error_code&)>&)>&
          start);
  BackendMessage ReadMessage(bool decode_records = false);
  static bolt::BoltMsg DecodeTag(std::string_view payload);
  static bool DecodeSuccessHasMore(std::string_view payload);
  static bool IsTerminal(bolt::BoltMsg tag);

  BackendEndpoint endpoint_;
  std::unordered_map<std::string, std::any> hello_meta_;
  boost::asio::io_context io_context_;
  std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
  bolt::Hydrator hydrator_;
  bool connected_ = false;
};

}  // namespace proxy
