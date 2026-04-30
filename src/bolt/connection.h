/**
 * Copyright 2022 AntGroup CO., Ltd.
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
#pragma once
#include <pthread.h>

#include <atomic>
#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <regex>
#include <unordered_map>
#include <utility>

#include "bolt/hydrator.h"
#include "bolt/pack_stream.h"
#include "common/logger.h"
namespace bolt {
using boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
void socket_set_options(tcp::socket& socket);

struct BoltConnectionOptions {
  uint32_t handshake_timeout_seconds = 5;
  uint32_t login_timeout_seconds = 10;
  uint32_t idle_timeout_seconds = 1800;
};

class Connection : private boost::asio::noncopyable {
 public:
  explicit Connection(boost::asio::io_service& io_service)
      : io_service_(io_service), socket_(io_service_), has_closed_(false) {}
  virtual ~Connection() { LOG_DEBUG("destroy connection[id:{}]", conn_id_); }
  tcp::socket& socket() { return socket_; }
  websocket::stream<tcp::socket>& ws() { return *ws_; }
  void ResetToWebSocket() {
    ws_ = std::make_unique<websocket::stream<tcp::socket>>(std::move(socket_));
    ws_->binary(true);
  }
  virtual void Close() {
    if (has_closed_.exchange(true)) {
      return;
    }
    boost::system::error_code ec;
    if (ws_) {
      ws_->close(websocket::close_code::normal, ec);
    } else {
      socket_.close(ec);
    }
    if (ec) {
      LOG_WARN("Close error: {}", ec.message());
    }
  }
  bool has_closed() const { return has_closed_.load(); }
  int64_t& conn_id() { return conn_id_; }
  boost::asio::io_service& io_service() { return io_service_; }
  virtual void Start() = 0;

 private:
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  std::unique_ptr<websocket::stream<tcp::socket>> ws_;
  int64_t conn_id_ = 0;
  std::atomic<bool> has_closed_;
};

class BoltConnection : public Connection,
                       public std::enable_shared_from_this<BoltConnection> {
 public:
  using Options = BoltConnectionOptions;
  BoltConnection(boost::asio::io_service& io_service,
                 std::function<void(BoltConnection& conn, BoltMsg msg,
                                    std::vector<std::any> fields)>
                     handle,
                 BoltConnectionOptions options = {})
      : Connection(io_service),
        timeout_timer_(io_service),
        handle_(std::move(handle)),
        options_(options) {}
  void Start() override;
  void Close() override;
  void PostResponse(std::string res);
  void Respond(std::string str);
  void SetContext(std::shared_ptr<void> ctx) { context_ = std::move(ctx); }
  void* GetContext() { return context_.get(); }
  std::shared_ptr<void> GetContextShared() { return context_; }
  void MarkAuthenticated();

 private:
  enum class Protocol { None = 0, Socket, WebSocket };
  void ReadMagicDone(const boost::system::error_code& ec);
  void ReadBoltIdentificationDone(const boost::system::error_code& ec);
  void ReadVersionNegotiationDone(const boost::system::error_code& ec);
  void ReadChunkSizeDone(const boost::system::error_code& ec);
  void ReadChunkDone(const boost::system::error_code& ec);
  void WriteResponseDone(const boost::system::error_code& ec);
  void WebSocketAcceptDone(const boost::system::error_code& ec);
  void WebSocketAsyncRead(
      const boost::asio::mutable_buffer& buffer,
      const std::function<void(const boost::system::error_code& ec)>& cb);
  void WebSocketReadSome();
  void WebSocketReadSomeDone(const boost::system::error_code& ec,
                             std::size_t bytes_transferred);
  void DoSend();
  void ArmTimeout(uint32_t seconds, const char* reason);
  void TimeoutDone(const boost::system::error_code& ec);
  void RefreshIdleTimeout();

  boost::asio::deadline_timer timeout_timer_;
  std::function<void(BoltConnection& conn, BoltMsg msg,
                     std::vector<std::any> fields)>
      handle_;
  BoltConnectionOptions options_;
  const uint8_t bolt_magic_[4] = {0x60, 0x60, 0xB0, 0x17};
  const uint8_t ws_magic_[4] = {'G', 'E', 'T', ' '};  // websocket
  uint8_t buffer4_[4] = {0};
  uint8_t buffer16_[16] = {0};
  uint16_t chunk_size_ = 0;
  std::vector<uint8_t> chunk_;
  Unpacker unpacker_;
  std::deque<std::string> msg_queue_;
  std::atomic<int> msg_queue_size_ = 0;
  std::vector<boost::asio::const_buffer> send_buffers_;
  // only shared_ptr can store void pointer
  std::shared_ptr<void> context_;
  Protocol protocol_ = Protocol::None;
  char* ws_buffer_ = nullptr;
  size_t ws_buffer_size_ = 0;
  size_t ws_total_read_ = 0;
  std::function<void(const boost::system::error_code& ec)> ws_cb_;
  std::atomic<bool> authenticated_{false};
  const char* timeout_reason_ = "bolt connection";
};

}  // namespace bolt
