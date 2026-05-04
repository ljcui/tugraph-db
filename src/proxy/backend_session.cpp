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
    BackendEndpoint endpoint,
    std::unordered_map<std::string, std::any> hello_meta)
    : endpoint_(std::move(endpoint)),
      hello_meta_(std::move(hello_meta)),
      timeout_timer_(io_context_) {}

BoltBackendSession::~BoltBackendSession() { Close(); }

std::vector<BackendMessage> BoltBackendSession::SendAndReadUntilTerminal(
    const std::string& request, bool decode_records) {
  try {
    EnsureConnected();
    const auto deadline = BackendIoDeadline();
    WriteWithTimeoutUntil(request.data(), request.size(), "backend write",
                          deadline);
    std::vector<BackendMessage> messages;
    while (true) {
      auto message = ReadMessageUntil(deadline, decode_records);
      auto terminal = IsTerminal(message.tag);
      messages.emplace_back(std::move(message));
      if (terminal) {
        return messages;
      }
    }
  } catch (...) {
    Close();
    throw;
  }
}

BackendMessage BoltBackendSession::SendAndForwardUntilTerminal(
    const std::string& request,
    const std::function<bool(const BackendMessage&)>& forward,
    bool decode_records) {
  try {
    EnsureConnected();
    const auto deadline = BackendIoDeadline();
    WriteWithTimeoutUntil(request.data(), request.size(), "backend write",
                          deadline);
    while (true) {
      auto message = ReadMessageUntil(deadline, decode_records);
      const bool terminal = IsTerminal(message.tag);
      if (!forward(message)) {
        throw BackendOperationCancelled("backend forwarding");
      }
      if (terminal) {
        return message;
      }
    }
  } catch (...) {
    Close();
    throw;
  }
}

std::vector<RaftNodeEndpoint> BoltBackendSession::FetchRaftNodeInfos(
    const std::string& graph_name) {
  bolt::PackStream ps;
  ps.AppendRun(
      "CALL dbms.graph.getRaftNodeInfos($graph_name) "
      "YIELD node_id, ip, bolt_port, raft_port, is_leader "
      "RETURN node_id, ip, bolt_port, raft_port, is_leader",
      {{"graph_name", graph_name}}, {{"db", graph_name}});
  auto run_messages = SendAndReadUntilTerminal(ps.ConstBuffer());
  if (run_messages.empty() ||
      run_messages.back().tag != bolt::BoltMsg::Success) {
    throw std::runtime_error(run_messages.empty()
                                 ? "raft node info query returned no response"
                                 : run_messages.back().failure_message);
  }

  ps.Reset();
  ps.AppendPullN(-1);
  auto pull_messages = SendAndReadUntilTerminal(ps.ConstBuffer(), true);
  std::vector<RaftNodeEndpoint> node_infos;
  for (const auto& message : pull_messages) {
    if (message.tag != bolt::BoltMsg::Record) {
      continue;
    }
    if (!message.record.has_value()) {
      throw std::runtime_error("raft node info record is missing");
    }
    const auto& values = message.record->values;
    if (values.size() != 5) {
      throw std::runtime_error(
          fmt::format("raft node info record should contain 5 fields, got {}",
                      values.size()));
    }
    auto node_id = CastInt64Field(values[0], "node_id");
    auto bolt_port = CastInt64Field(values[2], "bolt_port");
    auto raft_port = CastInt64Field(values[3], "raft_port");
    if (node_id <= 0 || bolt_port <= 0 || raft_port <= 0) {
      throw std::runtime_error("raft node info contains non-positive id/port");
    }
    node_infos.push_back({.node_id = static_cast<uint64_t>(node_id),
                          .host = CastStringField(values[1], "ip"),
                          .port = static_cast<uint32_t>(bolt_port),
                          .raft_port = static_cast<uint32_t>(raft_port),
                          .is_leader = CastBoolField(values[4], "is_leader")});
  }
  if (pull_messages.empty() ||
      pull_messages.back().tag != bolt::BoltMsg::Success) {
    throw std::runtime_error(pull_messages.empty()
                                 ? "raft node info pull returned no response"
                                 : pull_messages.back().failure_message);
  }
  return node_infos;
}

void BoltBackendSession::Close() {
  if (socket_) {
    boost::system::error_code ec;
    socket_->close(ec);
  }
  connected_ = false;
}

void BoltBackendSession::EnsureConnected() {
  if (!connected_) {
    Connect();
  }
}

void BoltBackendSession::Connect() {
  Close();
  socket_ = std::make_unique<tcp::socket>(io_context_);
  tcp::resolver resolver(io_context_);
  auto endpoints =
      resolver.resolve(endpoint_.host, std::to_string(endpoint_.port));
  ConnectWithTimeout(endpoints);
  bolt::socket_set_options(*socket_);

  WriteWithTimeout(kBoltHandshake, sizeof(kBoltHandshake),
                   "backend Bolt handshake write");
  uint8_t accepted_version[4] = {0};
  ReadWithTimeout(accepted_version, sizeof(accepted_version),
                  "backend Bolt handshake read");
  int selected_minor = -1;
  if (!IsAcceptedBoltVersion(accepted_version, &selected_minor)) {
    throw std::runtime_error("backend does not accept Bolt v4.0-v4.4");
  }

  bolt::PackStream ps;
  ps.AppendHello(hello_meta_);
  WriteWithTimeout(ps.ConstBuffer().data(), ps.ConstBuffer().size(),
                   "backend HELLO write");
  auto hello_response = ReadMessage();
  if (hello_response.tag == bolt::BoltMsg::Success) {
    connected_ = true;
    LOG_INFO("proxy connected backend {}:{} with Bolt v4.{}", endpoint_.host,
             endpoint_.port, selected_minor);
    return;
  }
  auto failure = DecodeFailure(hello_response, &hydrator_);
  Close();
  throw std::runtime_error("backend HELLO failed: " + failure.msg);
}

void BoltBackendSession::ConnectWithTimeout(
    const tcp::resolver::results_type& endpoints) {
  RunWithTimeout(
      "backend connect", kBackendConnectTimeoutSeconds,
      [this, &endpoints](
          const std::function<void(const boost::system::error_code&)>& done) {
        boost::asio::async_connect(*socket_, endpoints,
                                   [done](const boost::system::error_code& ec,
                                          const tcp::endpoint&) { done(ec); });
      });
}

void BoltBackendSession::WriteWithTimeout(const void* data, size_t size,
                                          const char* operation) {
  WriteWithTimeoutUntil(data, size, operation, BackendIoDeadline());
}

void BoltBackendSession::WriteWithTimeoutUntil(
    const void* data, size_t size, const char* operation,
    boost::asio::steady_timer::clock_type::time_point deadline) {
  RunWithTimeoutUntil(
      operation, deadline, kBackendIoTimeoutSeconds,
      [this, data, size](
          const std::function<void(const boost::system::error_code&)>& done) {
        boost::asio::async_write(
            *socket_, boost::asio::buffer(data, size),
            [done](const boost::system::error_code& ec, size_t) { done(ec); });
      });
}

void BoltBackendSession::ReadWithTimeout(void* data, size_t size,
                                         const char* operation) {
  RunWithTimeout(
      operation, kBackendIoTimeoutSeconds,
      [this, data, size](
          const std::function<void(const boost::system::error_code&)>& done) {
        boost::asio::async_read(
            *socket_, boost::asio::buffer(data, size),
            [done](const boost::system::error_code& ec, size_t) { done(ec); });
      });
}

void BoltBackendSession::RunWithTimeout(
    const char* operation, uint32_t timeout_seconds,
    const std::function<void(
        const std::function<void(const boost::system::error_code&)>&)>& start) {
  RunWithTimeoutUntil(operation,
                      boost::asio::steady_timer::clock_type::now() +
                          std::chrono::seconds(timeout_seconds),
                      timeout_seconds, start);
}

void BoltBackendSession::RunWithTimeoutUntil(
    const char* operation,
    boost::asio::steady_timer::clock_type::time_point deadline,
    uint32_t timeout_seconds,
    const std::function<void(
        const std::function<void(const boost::system::error_code&)>&)>& start) {
  boost::system::error_code result;
  bool completed = false;
  bool timed_out = false;
  timeout_timer_.expires_at(deadline);
  timeout_timer_.async_wait(
      [this, &timed_out](const boost::system::error_code& ec) {
        if (ec) {
          return;
        }
        timed_out = true;
        if (socket_) {
          boost::system::error_code ignored;
          socket_->cancel(ignored);
        }
      });

  start([&](const boost::system::error_code& ec) {
    result = ec;
    completed = true;
    boost::system::error_code ignored;
    timeout_timer_.cancel(ignored);
  });

  io_context_.restart();
  while (!completed) {
    io_context_.run_one();
  }
  io_context_.run();

  if (timed_out && result == boost::asio::error::operation_aborted) {
    throw std::runtime_error(
        fmt::format("{} timed out after {}s", operation, timeout_seconds));
  }
  if (result) {
    throw boost::system::system_error(result, operation);
  }
}

void BoltBackendSession::ReadMessageWithTimeout(BackendMessage* message,
                                                const char* operation) {
  ReadMessageWithTimeoutUntil(message, operation, BackendIoDeadline());
}

void BoltBackendSession::ReadMessageWithTimeoutUntil(
    BackendMessage* message, const char* operation,
    boost::asio::steady_timer::clock_type::time_point deadline) {
  RunWithTimeoutUntil(
      operation, deadline, kBackendIoTimeoutSeconds,
      [this, message](
          const std::function<void(const boost::system::error_code&)>& done) {
        AsyncReadMessageChunkHeader(message, done);
      });
}

void BoltBackendSession::AsyncReadMessageChunkHeader(
    BackendMessage* message,
    const std::function<void(const boost::system::error_code&)>& done) {
  boost::asio::async_read(
      *socket_, boost::asio::buffer(chunk_header_buffer_),
      [this, message, done](const boost::system::error_code& ec, size_t) {
        if (ec) {
          done(ec);
          return;
        }
        message->raw.append(chunk_header_buffer_.data(),
                            chunk_header_buffer_.size());

        uint16_t size = 0;
        std::memcpy(&size, chunk_header_buffer_.data(), sizeof(size));
        size = big_to_native(size);
        if (size == 0) {
          if (!message->payload.empty()) {
            done(ec);
            return;
          }
          AsyncReadMessageChunkHeader(message, done);
          return;
        }
        AsyncReadMessageChunkBody(message, size, done);
      });
}

void BoltBackendSession::AsyncReadMessageChunkBody(
    BackendMessage* message, uint16_t size,
    const std::function<void(const boost::system::error_code&)>& done) {
  chunk_buffer_.assign(size, '\0');
  boost::asio::async_read(
      *socket_, boost::asio::buffer(chunk_buffer_),
      [this, message, done](const boost::system::error_code& ec, size_t) {
        if (ec) {
          done(ec);
          return;
        }
        message->raw.append(chunk_buffer_);
        message->payload.append(chunk_buffer_);
        AsyncReadMessageChunkHeader(message, done);
      });
}

BackendMessage BoltBackendSession::ReadMessage(bool decode_records) {
  return ReadMessageUntil(BackendIoDeadline(), decode_records);
}

BackendMessage BoltBackendSession::ReadMessageUntil(
    boost::asio::steady_timer::clock_type::time_point deadline,
    bool decode_records) {
  BackendMessage message;
  ReadMessageWithTimeoutUntil(&message, "backend message read", deadline);

  message.tag = DecodeTag(message.payload);
  if (message.tag == bolt::BoltMsg::Success) {
    message.success_has_more = DecodeSuccessHasMore(message.payload);
  } else if (message.tag == bolt::BoltMsg::Failure) {
    auto failure = DecodeFailure(message, &hydrator_);
    message.failure_code = std::move(failure.code);
    message.failure_message = std::move(failure.msg);
  } else if (decode_records && message.tag == bolt::BoltMsg::Record) {
    message.record = DecodeRecord(message, &hydrator_);
  }
  return message;
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
