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

#include "server/raft_server.h"

#include <pthread.h>

#include <future>
#include <stdexcept>

#include "common/logger.h"
#include "raft/connection.h"
#include "raft/io_service.h"
#include "server/galaxy.h"

namespace server {

bool RaftServer::Start(Galaxy* galaxy, uint32_t port) {
  if (started_) {
    return true;
  }

  galaxy_ = galaxy;
  listener_.reset();

  std::promise<bool> promise;
  auto future = promise.get_future();
  threads_.emplace_back([this, port, &promise]() {
    bool promise_done = false;
    try {
      protobuf_handler_ = [this](std::string graph_name, raftpb::Message msg) {
        if (galaxy_ == nullptr) {
          LOG_WARN("receive raft message for graph [{}] while galaxy is null",
                   graph_name);
          return;
        }
        try {
          auto graph = galaxy_->OpenGraph(graph_name);
          auto* raft_driver = graph->raft_driver();
          if (raft_driver == nullptr) {
            LOG_WARN("graph [{}] does not enable raft, drop message",
                     graph_name);
            return;
          }
          raft_driver->Step(std::move(msg));
        } catch (const std::exception& e) {
          LOG_WARN("failed to route raft message for graph [{}]: {}",
                   graph_name, e.what());
        }
      };

      raft::IOService<raft::RaftConnection, decltype(protobuf_handler_)>
          raft_service(listener_, port, 1, protobuf_handler_);
      boost::asio::io_service::work holder(listener_);

      started_ = true;
      promise.set_value(true);
      promise_done = true;

      LOG_INFO("Raft server run");
      pthread_setname_np(pthread_self(), "raft_listener");
      listener_.run();
    } catch (const std::exception& e) {
      LOG_ERROR("raft server exception: {}", e.what());
      if (!promise_done) {
        promise.set_value(false);
      }
    }
  });

  if (future.get()) {
    return true;
  }

  for (auto& t : threads_) {
    t.join();
  }
  threads_.clear();
  galaxy_ = nullptr;
  protobuf_handler_ = {};
  listener_.reset();
  return false;
}

void RaftServer::Stop() {
  if (!started_) {
    for (auto& t : threads_) {
      t.join();
    }
    threads_.clear();
    galaxy_ = nullptr;
    protobuf_handler_ = {};
    listener_.reset();
    return;
  }

  listener_.stop();
  for (auto& t : threads_) {
    t.join();
  }
  threads_.clear();

  started_ = false;
  galaxy_ = nullptr;
  protobuf_handler_ = {};
  listener_.reset();
  LOG_INFO("Raft server stopped");
}

}  // namespace server
