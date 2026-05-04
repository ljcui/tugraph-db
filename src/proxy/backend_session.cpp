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

#include "proxy/backend_session.h"

#include <boost/endian/conversion.hpp>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <utility>

#include "bolt/connection.h"
#include "bolt/pack_stream.h"
#include "common/logger.h"

namespace proxy {
namespace {

using boost::asio::ip::tcp;
using boost::endian::big_to_native;

constexpr int kSupportedBoltMajor = 4;
constexpr int kMinSupportedBoltMinor = 0;
constexpr int kMaxSupportedBoltMinor = 4;
constexpr uint32_t kBackendConnectTimeoutSeconds = 5;
constexpr uint32_t kBackendIoTimeoutSeconds = 30;
constexpr uint8_t kBoltHandshake[] = {
    0x60, 0x60, 0xb0, 0x17, 0x00, 0x04, 0x04, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

boost::asio::steady_timer::clock_type::time_point BackendIoDeadline() {
  return boost::asio::steady_timer::clock_type::now() +
         std::chrono::seconds(kBackendIoTimeoutSeconds);
}

std::exception_ptr MakeRuntimeError(const std::string& message) {
  return std::make_exception_ptr(std::runtime_error(message));
}

bool IsAcceptedBoltVersion(const uint8_t* version, int* minor) {
  if (version[0] != 0 || version[1] != 0 || version[3] != kSupportedBoltMajor) {
    return false;
  }
  auto selected_minor = static_cast<int>(version[2]);
  if (selected_minor < kMinSupportedBoltMinor ||
      selected_minor > kMaxSupportedBoltMinor) {
    return false;
  }
  *minor = selected_minor;
  return true;
}

void SkipValue(bolt::Unpacker& unpacker) {
  switch (unpacker.CurrentType()) {
    case bolt::PackType::Integer:
      unpacker.Int();
      return;
    case bolt::PackType::Float:
      unpacker.Double();
      return;
    case bolt::PackType::String:
      unpacker.String();
      return;
    case bolt::PackType::Bytes:
      unpacker.ByteArray();
      return;
    case bolt::PackType::List: {
      auto len = unpacker.Len();
      for (uint32_t i = 0; i < len; ++i) {
        unpacker.Next();
        SkipValue(unpacker);
      }
      return;
    }
    case bolt::PackType::Dictionary: {
      auto len = unpacker.Len();
      for (uint32_t i = 0; i < len; ++i) {
        unpacker.Next();
        unpacker.String();
        unpacker.Next();
        SkipValue(unpacker);
      }
      return;
    }
    case bolt::PackType::Structure: {
      auto len = unpacker.Len();
      unpacker.StructTag();
      for (uint32_t i = 0; i < len; ++i) {
        unpacker.Next();
        SkipValue(unpacker);
      }
      return;
    }
    case bolt::PackType::True:
    case bolt::PackType::False:
      unpacker.Bool();
      return;
    case bolt::PackType::Null:
      return;
    default:
      throw std::runtime_error("unsupported bolt value in proxy response");
  }
}

bolt::Neo4jError DecodeFailure(const BackendMessage& msg,
                               bolt::Hydrator* hydrator) {
  bolt::Neo4jError failure;
  if (msg.tag != bolt::BoltMsg::Failure) {
    return failure;
  }
  hydrator->ClearErr();
  auto parsed = hydrator->Hydrate({msg.payload.data(), msg.payload.size()});
  if (parsed.second) {
    failure.msg = parsed.second.value();
    return failure;
  }
  if (parsed.first.type() == typeid(std::optional<bolt::Neo4jError>)) {
    const auto& error =
        std::any_cast<const std::optional<bolt::Neo4jError>&>(parsed.first);
    if (error.has_value()) {
      return *error;
    }
  }
  failure.msg = "backend returned failure";
  return failure;
}

bolt::Record DecodeRecord(const BackendMessage& msg, bolt::Hydrator* hydrator) {
  hydrator->ClearErr();
  auto parsed = hydrator->Hydrate({msg.payload.data(), msg.payload.size()});
  if (parsed.second) {
    throw std::runtime_error(parsed.second.value());
  }
  if (parsed.first.type() == typeid(std::optional<bolt::Record>)) {
    const auto& record =
        std::any_cast<const std::optional<bolt::Record>&>(parsed.first);
    if (record.has_value()) {
      return *record;
    }
  }
  throw std::runtime_error("backend returned invalid record");
}

int64_t CastInt64Field(const std::any& value, const char* field_name) {
  auto* integer = std::any_cast<int64_t>(&value);
  if (integer == nullptr) {
    throw std::runtime_error(
        fmt::format("{} type should be Integer", field_name));
  }
  return *integer;
}

std::string CastStringField(const std::any& value, const char* field_name) {
  auto* str = std::any_cast<std::string>(&value);
  if (str == nullptr) {
    throw std::runtime_error(
        fmt::format("{} type should be String", field_name));
  }
  return *str;
}

bool CastBoolField(const std::any& value, const char* field_name) {
  auto* boolean = std::any_cast<bool>(&value);
  if (boolean == nullptr) {
    throw std::runtime_error(fmt::format("{} type should be Bool", field_name));
  }
  return *boolean;
}

}  // namespace

BoltBackendSession::BoltBackendSession(
    boost::asio::io_service& io_service, BackendEndpoint endpoint,
    std::unordered_map<std::string, std::any> hello_meta)
    : endpoint_(std::move(endpoint)),
      hello_meta_(std::move(hello_meta)),
      io_service_(io_service),
      strand_(io_service_),
      resolver_(io_service_),
      timeout_timer_(io_service_) {}

BoltBackendSession::~BoltBackendSession() {
  boost::system::error_code ignored;
  resolver_.cancel();
  timeout_timer_.cancel(ignored);
  if (socket_) {
    socket_->close(ignored);
  }
}

void BoltBackendSession::AsyncSendAndReadUntilTerminal(
    const std::string& request, bool decode_records,
    MessagesCallback callback) {
  auto self = shared_from_this();
  io_service_.post(strand_.wrap([self, request, decode_records,
                                 callback = std::move(callback)]() mutable {
    self->StartSendAndRead(request, decode_records, std::move(callback));
  }));
}

void BoltBackendSession::AsyncSendAndForwardUntilTerminal(
    const std::string& request,
    const std::function<bool(const BackendMessage&)>& forward,
    bool decode_records, MessageCallback callback) {
  auto self = shared_from_this();
  io_service_.post(strand_.wrap([self, request, forward, decode_records,
                                 callback = std::move(callback)]() mutable {
    self->StartSendAndForward(request, forward, decode_records,
                              std::move(callback));
  }));
}

void BoltBackendSession::AsyncFetchRaftNodeInfos(
    const std::string& graph_name, RaftNodeInfosCallback callback) {
  auto self = shared_from_this();

  bolt::PackStream ps;
  ps.AppendRun(
      "CALL dbms.graph.getRaftNodeInfos($graph_name) "
      "YIELD node_id, ip, bolt_port, raft_port, is_leader "
      "RETURN node_id, ip, bolt_port, raft_port, is_leader",
      {{"graph_name", graph_name}}, {{"db", graph_name}});
  auto run_request = ps.ConstBuffer();

  AsyncSendAndReadUntilTerminal(
      run_request, false,
      [self, graph_name, callback = std::move(callback)](
          std::exception_ptr error,
          std::vector<BackendMessage> run_messages) mutable {
        if (error) {
          callback(error, {});
          return;
        }
        if (run_messages.empty() ||
            run_messages.back().tag != bolt::BoltMsg::Success) {
          callback(
              MakeRuntimeError(run_messages.empty()
                                   ? "raft node info query returned no response"
                                   : run_messages.back().failure_message),
              {});
          return;
        }

        bolt::PackStream pull;
        pull.AppendPullN(-1);
        auto pull_request = pull.ConstBuffer();
        self->AsyncSendAndReadUntilTerminal(
            pull_request, true,
            [callback = std::move(callback)](
                std::exception_ptr error,
                std::vector<BackendMessage> pull_messages) mutable {
              if (error) {
                callback(error, {});
                return;
              }

              try {
                std::vector<RaftNodeEndpoint> node_infos;
                for (const auto& message : pull_messages) {
                  if (message.tag != bolt::BoltMsg::Record) {
                    continue;
                  }
                  if (!message.record.has_value()) {
                    throw std::runtime_error(
                        "raft node info record is missing");
                  }
                  const auto& values = message.record->values;
                  if (values.size() != 5) {
                    throw std::runtime_error(fmt::format(
                        "raft node info record should contain 5 fields, got {}",
                        values.size()));
                  }
                  auto node_id = CastInt64Field(values[0], "node_id");
                  auto bolt_port = CastInt64Field(values[2], "bolt_port");
                  auto raft_port = CastInt64Field(values[3], "raft_port");
                  if (node_id <= 0 || bolt_port <= 0 || raft_port <= 0) {
                    throw std::runtime_error(
                        "raft node info contains non-positive id/port");
                  }
                  node_infos.push_back(
                      {.node_id = static_cast<uint64_t>(node_id),
                       .host = CastStringField(values[1], "ip"),
                       .port = static_cast<uint32_t>(bolt_port),
                       .raft_port = static_cast<uint32_t>(raft_port),
                       .is_leader = CastBoolField(values[4], "is_leader")});
                }
                if (pull_messages.empty() ||
                    pull_messages.back().tag != bolt::BoltMsg::Success) {
                  throw std::runtime_error(
                      pull_messages.empty()
                          ? "raft node info pull returned no response"
                          : pull_messages.back().failure_message);
                }
                callback(nullptr, std::move(node_infos));
              } catch (...) {
                callback(std::current_exception(), {});
              }
            });
      });
}

void BoltBackendSession::Close() {
  auto self = shared_from_this();
  io_service_.post(strand_.wrap([self]() { self->CloseOnStrand(); }));
}

void BoltBackendSession::Cancel() {
  auto self = shared_from_this();
  io_service_.post(strand_.wrap([self]() { self->CancelOnStrand(); }));
}

void BoltBackendSession::StartSendAndRead(std::string request,
                                          bool decode_records,
                                          MessagesCallback callback) {
  if (operation_in_progress_) {
    callback(
        MakeRuntimeError("backend session already has an active operation"),
        {});
    return;
  }

  operation_in_progress_ = true;
  operation_mode_ = OperationMode::COLLECT;
  decode_records_ = decode_records;
  request_ = std::move(request);
  messages_.clear();
  messages_callback_ = std::move(callback);

  auto self = shared_from_this();
  EnsureConnected([self](std::exception_ptr error) {
    if (error) {
      self->FinishCollect(error);
      return;
    }
    self->StartRequestWrite();
  });
}

void BoltBackendSession::StartSendAndForward(
    std::string request, std::function<bool(const BackendMessage&)> forward,
    bool decode_records, MessageCallback callback) {
  if (operation_in_progress_) {
    callback(
        MakeRuntimeError("backend session already has an active operation"),
        {});
    return;
  }

  operation_in_progress_ = true;
  operation_mode_ = OperationMode::FORWARD;
  decode_records_ = decode_records;
  request_ = std::move(request);
  forward_ = std::move(forward);
  message_callback_ = std::move(callback);

  auto self = shared_from_this();
  EnsureConnected([self](std::exception_ptr error) {
    if (error) {
      self->FinishForward(error, {});
      return;
    }
    self->StartRequestWrite();
  });
}

void BoltBackendSession::EnsureConnected(ErrorCallback callback) {
  if (connected_) {
    callback(nullptr);
    return;
  }
  StartConnect(std::move(callback));
}

void BoltBackendSession::StartConnect(ErrorCallback callback) {
  CloseOnStrand();
  socket_ = std::make_unique<tcp::socket>(io_service_);
  connect_callback_ = std::move(callback);
  StartResolve();
}

void BoltBackendSession::StartResolve() {
  auto self = shared_from_this();
  auto deadline = boost::asio::steady_timer::clock_type::now() +
                  std::chrono::seconds(kBackendConnectTimeoutSeconds);
  StartTimedStep(
      "backend resolve", deadline, kBackendConnectTimeoutSeconds,
      [this](
          const std::function<void(const boost::system::error_code&)>& done) {
        resolver_.async_resolve(
            endpoint_.host, std::to_string(endpoint_.port),
            strand_.wrap(
                [this, done](const boost::system::error_code& ec,
                             tcp::resolver::results_type results) mutable {
                  if (!ec) {
                    resolved_endpoints_ = std::move(results);
                  }
                  done(ec);
                }));
      },
      [self](std::exception_ptr error) {
        if (error) {
          self->CompleteConnect(error);
          return;
        }
        self->StartTcpConnect();
      });
}

void BoltBackendSession::StartTcpConnect() {
  auto self = shared_from_this();
  auto deadline = boost::asio::steady_timer::clock_type::now() +
                  std::chrono::seconds(kBackendConnectTimeoutSeconds);
  StartTimedStep(
      "backend connect", deadline, kBackendConnectTimeoutSeconds,
      [this](
          const std::function<void(const boost::system::error_code&)>& done) {
        boost::asio::async_connect(
            *socket_, resolved_endpoints_,
            strand_.wrap([done](const boost::system::error_code& ec,
                                const tcp::endpoint&) { done(ec); }));
      },
      [self](std::exception_ptr error) {
        if (error) {
          self->CompleteConnect(error);
          return;
        }
        try {
          bolt::socket_set_options(*self->socket_);
          self->StartBoltHandshakeWrite();
        } catch (...) {
          self->CompleteConnect(std::current_exception());
        }
      });
}

void BoltBackendSession::StartBoltHandshakeWrite() {
  auto self = shared_from_this();
  AsyncWrite(kBoltHandshake, sizeof(kBoltHandshake),
             "backend Bolt handshake write", BackendIoDeadline(),
             [self](std::exception_ptr error) {
               if (error) {
                 self->CompleteConnect(error);
                 return;
               }
               self->StartBoltHandshakeRead();
             });
}

void BoltBackendSession::StartBoltHandshakeRead() {
  auto self = shared_from_this();
  accepted_version_.fill(0);
  AsyncRead(accepted_version_.data(), accepted_version_.size(),
            "backend Bolt handshake read", BackendIoDeadline(),
            [self](std::exception_ptr error) {
              if (error) {
                self->CompleteConnect(error);
                return;
              }
              self->selected_minor_ = -1;
              if (!IsAcceptedBoltVersion(self->accepted_version_.data(),
                                         &self->selected_minor_)) {
                self->CompleteConnect(
                    MakeRuntimeError("backend does not accept Bolt v4.0-v4.4"));
                return;
              }
              self->StartHelloWrite();
            });
}

void BoltBackendSession::StartHelloWrite() {
  bolt::PackStream ps;
  ps.AppendHello(hello_meta_);
  hello_request_ = ps.ConstBuffer();

  auto self = shared_from_this();
  AsyncWrite(hello_request_.data(), hello_request_.size(),
             "backend HELLO write", BackendIoDeadline(),
             [self](std::exception_ptr error) {
               if (error) {
                 self->CompleteConnect(error);
                 return;
               }
               self->StartHelloRead();
             });
}

void BoltBackendSession::StartHelloRead() {
  auto self = shared_from_this();
  AsyncReadMessage(BackendIoDeadline(),
                   [self](std::exception_ptr error, BackendMessage message) {
                     if (error) {
                       self->CompleteConnect(error);
                       return;
                     }
                     if (message.tag == bolt::BoltMsg::Success) {
                       self->connected_ = true;
                       LOG_INFO("proxy connected backend {}:{} with Bolt v4.{}",
                                self->endpoint_.host, self->endpoint_.port,
                                self->selected_minor_);
                       self->CompleteConnect(nullptr);
                       return;
                     }
                     auto failure = DecodeFailure(message, &self->hydrator_);
                     self->CompleteConnect(MakeRuntimeError(
                         "backend HELLO failed: " + failure.msg));
                   });
}

void BoltBackendSession::CompleteConnect(std::exception_ptr error) {
  if (error) {
    CloseOnStrand();
  }
  auto callback = std::move(connect_callback_);
  connect_callback_ = {};
  if (callback) {
    callback(error);
  }
}

void BoltBackendSession::StartRequestWrite() {
  request_deadline_ = BackendIoDeadline();
  auto self = shared_from_this();
  AsyncWrite(request_.data(), request_.size(), "backend write",
             request_deadline_, [self](std::exception_ptr error) {
               if (error) {
                 self->FinishCurrentOperation(error);
                 return;
               }
               self->StartReadNextResponse();
             });
}

void BoltBackendSession::StartReadNextResponse() {
  auto self = shared_from_this();
  AsyncReadMessage(request_deadline_, [self](std::exception_ptr error,
                                             BackendMessage message) mutable {
    if (error) {
      self->FinishCurrentOperation(error);
      return;
    }

    const bool terminal = IsTerminal(message.tag);
    if (self->operation_mode_ == OperationMode::COLLECT) {
      self->messages_.emplace_back(std::move(message));
      if (terminal) {
        self->FinishCollect(nullptr);
      } else {
        self->StartReadNextResponse();
      }
      return;
    }

    try {
      if (!self->forward_(message)) {
        self->FinishForward(std::make_exception_ptr(BackendOperationCancelled(
                                "backend forwarding")),
                            {});
        return;
      }
    } catch (...) {
      self->FinishForward(std::current_exception(), {});
      return;
    }

    if (terminal) {
      self->FinishForward(nullptr, std::move(message));
    } else {
      self->StartReadNextResponse();
    }
  });
}

void BoltBackendSession::FinishCollect(std::exception_ptr error) {
  if (error) {
    CloseOnStrand();
  }
  operation_in_progress_ = false;
  request_.clear();
  decode_records_ = false;

  auto callback = std::move(messages_callback_);
  auto messages = std::move(messages_);
  messages_callback_ = {};
  messages_.clear();
  if (callback) {
    callback(error, std::move(messages));
  }
}

void BoltBackendSession::FinishForward(std::exception_ptr error,
                                       BackendMessage message) {
  if (error) {
    CloseOnStrand();
  }
  operation_in_progress_ = false;
  request_.clear();
  decode_records_ = false;

  auto callback = std::move(message_callback_);
  forward_ = {};
  message_callback_ = {};
  if (callback) {
    callback(error, std::move(message));
  }
}

void BoltBackendSession::FinishCurrentOperation(std::exception_ptr error) {
  if (operation_mode_ == OperationMode::COLLECT) {
    FinishCollect(error);
  } else {
    FinishForward(error, {});
  }
}

void BoltBackendSession::StartTimedStep(
    const char* operation,
    boost::asio::steady_timer::clock_type::time_point deadline,
    uint32_t timeout_seconds,
    const std::function<void(
        const std::function<void(const boost::system::error_code&)>&)>& start,
    ErrorCallback callback) {
  auto self = shared_from_this();
  timed_out_ = false;
  timeout_timer_.expires_at(deadline);
  timeout_timer_.async_wait(
      strand_.wrap([self](const boost::system::error_code& ec) {
        if (ec) {
          return;
        }
        self->timed_out_ = true;
        self->CancelOnStrand();
      }));

  try {
    start([self, operation = std::string(operation), timeout_seconds,
           callback = std::move(callback)](
              const boost::system::error_code& ec) mutable {
      boost::system::error_code ignored;
      self->timeout_timer_.cancel(ignored);
      if (self->timed_out_ && ec == boost::asio::error::operation_aborted) {
        callback(MakeRuntimeError(
            fmt::format("{} timed out after {}s", operation, timeout_seconds)));
        return;
      }
      if (ec) {
        callback(std::make_exception_ptr(
            boost::system::system_error(ec, operation)));
        return;
      }
      callback(nullptr);
    });
  } catch (...) {
    boost::system::error_code ignored;
    timeout_timer_.cancel(ignored);
    callback(std::current_exception());
  }
}

void BoltBackendSession::AsyncWrite(
    const void* data, size_t size, const char* operation,
    boost::asio::steady_timer::clock_type::time_point deadline,
    ErrorCallback callback) {
  StartTimedStep(
      operation, deadline, kBackendIoTimeoutSeconds,
      [this, data, size](
          const std::function<void(const boost::system::error_code&)>& done) {
        boost::asio::async_write(
            *socket_, boost::asio::buffer(data, size),
            strand_.wrap([done](const boost::system::error_code& ec, size_t) {
              done(ec);
            }));
      },
      std::move(callback));
}

void BoltBackendSession::AsyncRead(
    void* data, size_t size, const char* operation,
    boost::asio::steady_timer::clock_type::time_point deadline,
    ErrorCallback callback) {
  StartTimedStep(
      operation, deadline, kBackendIoTimeoutSeconds,
      [this, data, size](
          const std::function<void(const boost::system::error_code&)>& done) {
        boost::asio::async_read(
            *socket_, boost::asio::buffer(data, size),
            strand_.wrap([done](const boost::system::error_code& ec, size_t) {
              done(ec);
            }));
      },
      std::move(callback));
}

void BoltBackendSession::AsyncReadMessage(
    boost::asio::steady_timer::clock_type::time_point deadline,
    MessageReadCallback callback) {
  auto state = std::make_shared<AsyncMessageReadState>();
  AsyncReadMessageChunkHeader(std::move(state), deadline, std::move(callback));
}

void BoltBackendSession::AsyncReadMessageChunkHeader(
    std::shared_ptr<AsyncMessageReadState> state,
    boost::asio::steady_timer::clock_type::time_point deadline,
    MessageReadCallback callback) {
  auto self = shared_from_this();
  auto read_state = state;
  AsyncRead(state->header.data(), state->header.size(), "backend message read",
            deadline,
            [self, state = std::move(read_state), deadline,
             callback = std::move(callback)](std::exception_ptr error) mutable {
              if (error) {
                callback(error, {});
                return;
              }
              state->message.raw.append(state->header.data(),
                                        state->header.size());

              uint16_t size = 0;
              std::memcpy(&size, state->header.data(), sizeof(size));
              size = big_to_native(size);
              if (size == 0) {
                if (!state->message.payload.empty()) {
                  self->DecodeAndCompleteReadMessage(std::move(state),
                                                     std::move(callback));
                  return;
                }
                self->AsyncReadMessageChunkHeader(std::move(state), deadline,
                                                  std::move(callback));
                return;
              }
              self->AsyncReadMessageChunkBody(std::move(state), size, deadline,
                                              std::move(callback));
            });
}

void BoltBackendSession::AsyncReadMessageChunkBody(
    std::shared_ptr<AsyncMessageReadState> state, uint16_t size,
    boost::asio::steady_timer::clock_type::time_point deadline,
    MessageReadCallback callback) {
  state->chunk.assign(size, '\0');
  auto self = shared_from_this();
  auto read_state = state;
  AsyncRead(state->chunk.data(), state->chunk.size(), "backend message read",
            deadline,
            [self, state = std::move(read_state), deadline,
             callback = std::move(callback)](std::exception_ptr error) mutable {
              if (error) {
                callback(error, {});
                return;
              }
              state->message.raw.append(state->chunk);
              state->message.payload.append(state->chunk);
              self->AsyncReadMessageChunkHeader(std::move(state), deadline,
                                                std::move(callback));
            });
}

void BoltBackendSession::DecodeAndCompleteReadMessage(
    std::shared_ptr<AsyncMessageReadState> state,
    MessageReadCallback callback) {
  try {
    auto& message = state->message;
    message.tag = DecodeTag(message.payload);
    if (message.tag == bolt::BoltMsg::Success) {
      message.success_has_more = DecodeSuccessHasMore(message.payload);
    } else if (message.tag == bolt::BoltMsg::Failure) {
      auto failure = DecodeFailure(message, &hydrator_);
      message.failure_code = std::move(failure.code);
      message.failure_message = std::move(failure.msg);
    } else if (decode_records_ && message.tag == bolt::BoltMsg::Record) {
      message.record = DecodeRecord(message, &hydrator_);
    }
    callback(nullptr, std::move(message));
  } catch (...) {
    callback(std::current_exception(), {});
  }
}

void BoltBackendSession::CloseOnStrand() {
  boost::system::error_code ignored;
  resolver_.cancel();
  timeout_timer_.cancel(ignored);
  if (socket_) {
    socket_->cancel(ignored);
    socket_->close(ignored);
  }
  connected_ = false;
}

void BoltBackendSession::CancelOnStrand() {
  boost::system::error_code ignored;
  resolver_.cancel();
  if (socket_) {
    socket_->cancel(ignored);
  }
}

bolt::BoltMsg BoltBackendSession::DecodeTag(std::string_view payload) {
  bolt::Unpacker unpacker;
  unpacker.Reset(payload);
  unpacker.Next();
  if (unpacker.CurrentType() != bolt::PackType::Structure) {
    throw std::runtime_error("backend returned non-struct Bolt message");
  }
  unpacker.Len();
  return static_cast<bolt::BoltMsg>(unpacker.StructTag());
}

bool BoltBackendSession::DecodeSuccessHasMore(std::string_view payload) {
  bolt::Unpacker unpacker;
  unpacker.Reset(payload);
  unpacker.Next();
  if (unpacker.CurrentType() != bolt::PackType::Structure) {
    return false;
  }
  auto field_count = unpacker.Len();
  auto tag = static_cast<bolt::BoltMsg>(unpacker.StructTag());
  if (tag != bolt::BoltMsg::Success || field_count != 1) {
    return false;
  }
  unpacker.Next();
  if (unpacker.CurrentType() != bolt::PackType::Dictionary) {
    return false;
  }
  auto map_len = unpacker.Len();
  for (uint32_t i = 0; i < map_len; ++i) {
    unpacker.Next();
    auto key = unpacker.String();
    unpacker.Next();
    if (key == "has_more") {
      return unpacker.Bool();
    }
    SkipValue(unpacker);
  }
  return false;
}

bool BoltBackendSession::IsTerminal(bolt::BoltMsg tag) {
  return tag == bolt::BoltMsg::Success || tag == bolt::BoltMsg::Failure ||
         tag == bolt::BoltMsg::Ignored;
}

}  // namespace proxy
