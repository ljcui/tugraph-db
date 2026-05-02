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
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "bolt/hydrator.h"
#include "bolt/messages.h"
#include "proxy/shard_map.h"

namespace proxy {

struct BackendMessage {
  std::string raw;
  std::string payload;
  bolt::BoltMsg tag = bolt::BoltMsg::Ignored;
  bool success_has_more = false;
};

class BoltBackendSession {
 public:
  BoltBackendSession(BackendEndpoint endpoint,
                     std::unordered_map<std::string, std::any> hello_meta);
  ~BoltBackendSession();

  std::vector<BackendMessage> SendAndReadUntilTerminal(
      const std::string& request);
  void Close();

 private:
  void EnsureConnected();
  void Connect();
  BackendMessage ReadMessage();
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
