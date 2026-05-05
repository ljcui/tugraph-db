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
#include <array>
#include <boost/asio.hpp>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
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

class BackendOperationCancelled : public std::runtime_error {
 public:
  explicit BackendOperationCancelled(const std::string& operation)
      : std::runtime_error(operation + " cancelled") {}
};

class BoltBackendSession
    : public std::enable_shared_from_this<BoltBackendSession> {
 public:
  using MessagesCallback =
      std::function<void(std::exception_ptr, std::vector<BackendMessage>)>;
  using MessageCallback =
      std::function<void(std::exception_ptr, BackendMessage)>;
  using RaftNodeInfosCallback =
      std::function<void(std::exception_ptr, std::vector<RaftNodeEndpoint>)>;

  BoltBackendSession(boost::asio::io_service& io_service,
                     BackendEndpoint endpoint,
                     std::unordered_map<std::string, std::any> hello_meta);
  ~BoltBackendSession();

  void AsyncSendAndReadUntilTerminal(const std::string& request,
                                     bool decode_records,
                                     MessagesCallback callback);
  void AsyncSendAndForwardUntilTerminal(
      const std::string& request,
      const std::function<bool(const BackendMessage&)>& forward,
      bool decode_records, MessageCallback callback);
  void AsyncFetchRaftNodeInfos(const std::string& graph_name,
                               RaftNodeInfosCallback callback);
  void Close();
  void Cancel();

 private:
  enum class OperationMode { COLLECT = 0, FORWARD };

  struct AsyncMessageReadState {
    BackendMessage message;
    std::array<char, 2> header{};
    std::string chunk;
  };

  using ErrorCallback = std::function<void(std::exception_ptr)>;
  using MessageReadCallback =
      std::function<void(std::exception_ptr, BackendMessage)>;

  void StartSendAndRead(std::string request, bool decode_records,
                        MessagesCallback callback);
  void StartSendAndForward(std::string request,
                           std::function<bool(const BackendMessage&)> forward,
                           bool decode_records, MessageCallback callback);
  void EnsureConnected(ErrorCallback callback);
  void StartConnect(ErrorCallback callback);
  void StartResolve();
  void StartTcpConnect();
  void StartBoltHandshakeWrite();
  void StartBoltHandshakeRead();
  void StartHelloWrite();
  void StartHelloRead();
  void CompleteConnect(std::exception_ptr error);
  void StartRequestWrite();
  void StartReadNextResponse();
  void FinishCollect(std::exception_ptr error);
  void FinishForward(std::exception_ptr error, BackendMessage message);
  void FinishCurrentOperation(std::exception_ptr error);
  void StartTimedStep(
      const char* operation,
      boost::asio::steady_timer::clock_type::time_point deadline,
      uint32_t timeout_seconds,
      const std::function<void(
          const std::function<void(const boost::system::error_code&)>&)>& start,
      ErrorCallback callback);
  void AsyncWrite(const void* data, size_t size, const char* operation,
                  boost::asio::steady_timer::clock_type::time_point deadline,
                  ErrorCallback callback);
  void AsyncRead(void* data, size_t size, const char* operation,
                 boost::asio::steady_timer::clock_type::time_point deadline,
                 ErrorCallback callback);
  void AsyncReadMessage(
      boost::asio::steady_timer::clock_type::time_point deadline,
      MessageReadCallback callback);
  void AsyncReadMessageForCurrentOperation(MessageReadCallback callback);
  void AsyncReadMessageChunkHeader(
      std::shared_ptr<AsyncMessageReadState> state,
      boost::asio::steady_timer::clock_type::time_point deadline,
      MessageReadCallback callback);
  void AsyncReadMessageChunkBody(
      std::shared_ptr<AsyncMessageReadState> state, uint16_t size,
      boost::asio::steady_timer::clock_type::time_point deadline,
      MessageReadCallback callback);
  void AsyncReadMessageChunkHeaderForCurrentOperation(
      std::shared_ptr<AsyncMessageReadState> state,
      MessageReadCallback callback);
  void AsyncReadMessageChunkBodyForCurrentOperation(
      std::shared_ptr<AsyncMessageReadState> state, uint16_t size,
      MessageReadCallback callback);
  void DecodeAndCompleteReadMessage(
      std::shared_ptr<AsyncMessageReadState> state,
      MessageReadCallback callback);
  void StartCurrentOperationTimer();
  void CancelCurrentOperationTimer();
  std::exception_ptr CurrentOperationIoError(
      const boost::system::error_code& ec, const char* operation) const;
  void AsyncWriteForCurrentOperation(const void* data, size_t size,
                                     const char* operation,
                                     ErrorCallback callback);
  void AsyncReadForCurrentOperation(void* data, size_t size,
                                    const char* operation,
                                    ErrorCallback callback);
  void CloseOnStrand();
  void CancelOnStrand();
  static bolt::BoltMsg DecodeTag(std::string_view payload);
  static bool DecodeSuccessHasMore(std::string_view payload);
  static bool IsTerminal(bolt::BoltMsg tag);

  BackendEndpoint endpoint_;
  std::unordered_map<std::string, std::any> hello_meta_;
  boost::asio::io_service& io_service_;
  boost::asio::io_service::strand strand_;
  boost::asio::ip::tcp::resolver resolver_;
  boost::asio::ip::tcp::resolver::results_type resolved_endpoints_;
  boost::asio::steady_timer timeout_timer_;
  std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
  bolt::Hydrator hydrator_;
  std::array<uint8_t, 4> accepted_version_{};
  int selected_minor_ = -1;
  std::string hello_request_;
  std::function<bool(const BackendMessage&)> forward_;
  MessagesCallback messages_callback_;
  MessageCallback message_callback_;
  ErrorCallback connect_callback_;
  OperationMode operation_mode_ = OperationMode::COLLECT;
  std::string request_;
  boost::asio::steady_timer::clock_type::time_point request_deadline_;
  std::vector<BackendMessage> messages_;
  bool operation_in_progress_ = false;
  bool decode_records_ = false;
  bool decode_success_has_more_ = false;
  bool timed_out_ = false;
  bool connected_ = false;
};

}  // namespace proxy
