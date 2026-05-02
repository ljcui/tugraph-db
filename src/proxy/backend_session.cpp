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
#include <cstring>
#include <stdexcept>

#include "bolt/connection.h"
#include "bolt/pack_stream.h"
#include "common/logger.h"

namespace proxy {
namespace {

using boost::asio::ip::tcp;
using boost::endian::big_to_native;

constexpr uint8_t kBoltHandshake[] = {
    0x60, 0x60, 0xb0, 0x17, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

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

std::string FailureMessage(const BackendMessage& msg,
                           bolt::Hydrator* hydrator) {
  if (msg.tag != bolt::BoltMsg::Failure) {
    return {};
  }
  hydrator->ClearErr();
  auto parsed = hydrator->Hydrate({msg.payload.data(), msg.payload.size()});
  if (parsed.second) {
    return parsed.second.value();
  }
  if (parsed.first.type() == typeid(std::optional<bolt::Neo4jError>)) {
    const auto& error =
        std::any_cast<const std::optional<bolt::Neo4jError>&>(parsed.first);
    if (error.has_value()) {
      return error->msg;
    }
  }
  return "backend returned failure";
}

}  // namespace

BoltBackendSession::BoltBackendSession(
    BackendEndpoint endpoint,
    std::unordered_map<std::string, std::any> hello_meta)
    : endpoint_(std::move(endpoint)), hello_meta_(std::move(hello_meta)) {}

BoltBackendSession::~BoltBackendSession() { Close(); }

std::vector<BackendMessage> BoltBackendSession::SendAndReadUntilTerminal(
    const std::string& request) {
  EnsureConnected();
  try {
    boost::asio::write(*socket_, boost::asio::buffer(request));
    std::vector<BackendMessage> messages;
    while (true) {
      auto message = ReadMessage();
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

void BoltBackendSession::Close() {
  if (socket_) {
    boost::system::error_code ec;
    socket_->close(ec);
    socket_.reset();
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
  boost::asio::connect(*socket_, endpoints);
  bolt::socket_set_options(*socket_);

  boost::asio::write(*socket_, boost::asio::buffer(kBoltHandshake));
  uint8_t accepted_version[4] = {0};
  boost::asio::read(*socket_, boost::asio::buffer(accepted_version));
  if (accepted_version[2] != 4 || accepted_version[3] != 4) {
    throw std::runtime_error("backend does not accept Bolt v4.4");
  }

  bolt::PackStream ps;
  ps.AppendHello(hello_meta_);
  boost::asio::write(*socket_, boost::asio::buffer(ps.ConstBuffer()));
  auto hello_response = ReadMessage();
  if (hello_response.tag == bolt::BoltMsg::Success) {
    connected_ = true;
    LOG_INFO("proxy connected backend {}:{}", endpoint_.host, endpoint_.port);
    return;
  }
  auto failure = FailureMessage(hello_response, &hydrator_);
  Close();
  throw std::runtime_error("backend HELLO failed: " + failure);
}

BackendMessage BoltBackendSession::ReadMessage() {
  BackendMessage message;
  while (true) {
    char header[2] = {0};
    boost::asio::read(*socket_, boost::asio::buffer(header));
    message.raw.append(header, sizeof(header));

    uint16_t size = 0;
    std::memcpy(&size, header, sizeof(size));
    size = big_to_native(size);
    if (size == 0) {
      if (!message.payload.empty()) {
        break;
      }
      continue;
    }

    std::string chunk(size, '\0');
    boost::asio::read(*socket_, boost::asio::buffer(chunk));
    message.raw.append(chunk);
    message.payload.append(chunk);
  }

  message.tag = DecodeTag(message.payload);
  if (message.tag == bolt::BoltMsg::Success) {
    message.success_has_more = DecodeSuccessHasMore(message.payload);
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
