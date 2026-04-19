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

// written by botu.wzy

#include "raft/raft_driver.h"

#include <gflags/gflags.h>

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/lexical_cast.hpp>
#include <filesystem>
#include <shared_mutex>

#include "common/logger.h"

using boost::asio::async_write;
using boost::asio::ip::tcp;
using namespace std::chrono;

namespace raft {
namespace {
std::string LocalNodeIdentityString(const LocalNodeConfig& local_node) {
  return fmt::format("graph={}, ip={}, bolt_port={}, raft_poft={}",
                     local_node.graph, local_node.ip, local_node.bolt_port,
                     local_node.raft_poft);
}

bool MatchLocalNode(const LocalNodeConfig& local_node,
                    const meta::RaftNodeInfo& node_info) {
  return node_info.graph() == local_node.graph &&
         node_info.ip() == local_node.ip &&
         node_info.bolt_port() == local_node.bolt_port &&
         node_info.raft_poft() == local_node.raft_poft;
}

eraft::Error InferNodeIdFromNodeInfos(const LocalNodeConfig& local_node,
                                      const meta::RaftNodeInfos& node_infos,
                                      uint64_t* node_id) {
  std::optional<uint64_t> matched;
  for (const auto& [id, node_info] : node_infos.nodes()) {
    if (!MatchLocalNode(local_node, node_info)) {
      continue;
    }
    if (matched.has_value()) {
      return eraft::Error(
          fmt::format("multiple raft nodes match local identity [{}]",
                      LocalNodeIdentityString(local_node)));
    }
    matched = id;
  }
  if (!matched.has_value()) {
    return eraft::Error(
        fmt::format("no raft node matches local identity [{}] in node infos",
                    LocalNodeIdentityString(local_node)));
  }
  *node_id = matched.value();
  return nullptr;
}

eraft::Error InferNodeIdFromInitPeers(const LocalNodeConfig& local_node,
                                      const std::vector<eraft::Peer>& peers,
                                      uint64_t* node_id) {
  std::optional<uint64_t> matched;
  for (const auto& peer : peers) {
    meta::RaftNodeInfo node_info;
    if (!node_info.ParseFromString(peer.context_)) {
      return eraft::Error(fmt::format(
          "failed to parse initial peer context for peer {}", peer.id_));
    }
    if (!MatchLocalNode(local_node, node_info)) {
      continue;
    }
    if (matched.has_value()) {
      return eraft::Error(
          fmt::format("multiple initial peers match local identity [{}]",
                      LocalNodeIdentityString(local_node)));
    }
    matched = peer.id_;
  }
  if (!matched.has_value()) {
    return eraft::Error(
        fmt::format("no initial peer matches local identity [{}]",
                    LocalNodeIdentityString(local_node)));
  }
  *node_id = matched.value();
  return nullptr;
}
}  // namespace

void NodeClient::reconnect() {
  if (has_closed_) {
    return;
  }
  boost::system::error_code ec;
  socket_.close(ec);
  if (ec) {
    LOG_WARN("socket close error: {}", ec.message());
  }
  connected_ = false;
  send_buffers_.clear();
  msg_queue_.clear();
  timer_.expires_from_now(interval_);
  timer_.async_wait(
      [this, self = shared_from_this()](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted) {
          return;
        }
        Connect();
      });
}

void NodeClient::Close() {
  has_closed_ = true;
  io_service_.post([this, self = shared_from_this()]() {
    boost::system::error_code ec;
    socket_.close(ec);
    timer_.cancel(ec);
  });
}

void NodeClient::Send(std::string str) {
  if (has_closed_) {
    LOG_DEBUG("connection[{}:{}] is closed, drop this message",
              endpoint_.address().to_string(), endpoint_.port());
    return;
  }
  io_service_.post(
      [this, self = shared_from_this(), msg = std::move(str)]() mutable {
        if (!connected_) {
          LOG_DEBUG("connection[{}:{}] is not available, drop this message",
                    endpoint_.address().to_string(), endpoint_.port());
          return;
        }
        bool need_invoke = msg_queue_.empty();
        msg_queue_.emplace_back(std::move(msg));
        if (need_invoke) {
          do_send();
        }
      });
}

void NodeClient::send_magic_code() {
  async_write(socket_, boost::asio::buffer(magic_code),
              [this, self = shared_from_this()](
                  const boost::system::error_code& ec, size_t) {
                if (ec) {
                  LOG_WARN("async write error {}", ec.message());
                  if (ec == boost::asio::error::operation_aborted) {
                    return;
                  }
                  reconnect();
                  return;
                }
                connected_ = true;
                do_read_some();
              });
}

void NodeClient::do_send() {
  for (size_t i = 0; i < msg_queue_.size(); i++) {
    send_buffers_.emplace_back(boost::asio::buffer(msg_queue_[i]));
    if (send_buffers_.size() >= 5) {
      break;
    }
  }
  async_write(socket_, send_buffers_,
              [this, self = shared_from_this()](
                  const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                  LOG_WARN("async write error {}", ec.message());
                  if (ec == boost::asio::error::operation_aborted) {
                    return;
                  }
                  reconnect();
                  return;
                }
                assert(msg_queue_.size() >= send_buffers_.size());
                msg_queue_.erase(msg_queue_.begin(),
                                 msg_queue_.begin() + send_buffers_.size());
                send_buffers_.clear();
                if (!msg_queue_.empty()) {
                  do_send();
                }
              });
}

void NodeClient::do_read_some() {
  socket_.async_read_some(
      boost::asio::buffer(buffer4_),
      [this, self = shared_from_this()](const boost::system::error_code& ec,
                                        size_t) {
        if (ec) {
          LOG_WARN("async_read_some error: {}", ec.message());
          if (ec == boost::asio::error::operation_aborted) {
            return;
          }
          reconnect();
        } else {
          LOG_ERROR("unexpected read in node client do_read_some");
          reconnect();
        }
      });
}

void socket_set_options(tcp::socket& socket) {
  try {
    socket.set_option(tcp::no_delay(true));
    socket.set_option(boost::asio::socket_base::keep_alive(true));
    socket.set_option(tcp::socket::reuse_address(true));
  } catch (const boost::system::system_error& e) {
    LOG_ERROR("socket_set_options error: {}", e.what());
  }
}

void NodeClient::Connect() {
  if (has_closed_) {
    return;
  }
  boost::system::error_code ec;
  socket_.open(tcp::v4(), ec);
  if (ec) {
    LOG_WARN("socket open error: {}", ec.message());
  }
  socket_.async_connect(endpoint_, [this, self = shared_from_this()](
                                       const boost::system::error_code& ec) {
    if (ec) {
      if (ec == boost::asio::error::operation_aborted) {
        return;
      }
      LOG_WARN("async connect {} error: {}",
               boost::lexical_cast<std::string>(endpoint_), ec.message());
      reconnect();
    } else {
      LOG_INFO("connect to {} successfully",
               boost::lexical_cast<std::string>(endpoint_));
      socket_set_options(socket_);
      send_magic_code();
    }
  });
}

std::string MessageToNetString(const google::protobuf::Message& msg) {
  uint32_t msg_size = msg.ByteSizeLong();
  std::string str;
  str.reserve(msg_size + sizeof(uint32_t));
  boost::endian::native_to_big_inplace(msg_size);
  str.append(reinterpret_cast<const char*>(&msg_size), sizeof(msg_size));
  str.append(msg.SerializeAsString());
  return str;
}

bool RaftConfig::Check() {
  if (tick_interval < 100) {
    LOG_WARN("tick_interval should be greater than 100");
    return false;
  }
  if (heartbeat_tick < 1) {
    LOG_WARN("heartbeat_tick should be greater than 1");
    return false;
  }
  if (election_tick < 10) {
    LOG_WARN("election_tick should be greater than 10");
    return false;
  }
  return true;
}

bool RaftLogStoreConfig::Check() {
  if (path.empty()) {
    LOG_WARN("raft logstore path is empty.");
    return false;
  }
  if (block_cache < 10) {
    LOG_WARN("block_cache should be greater than 10 MB");
    return false;
  }
  if (total_threads < 2) {
    LOG_WARN("total_threads should be greater than 2");
    return false;
  }
  if (keep_logs < 100000) {
    LOG_WARN("keep_logs should be greater than 100000");
    return false;
  }
  if (gc_interval < 1) {
    LOG_WARN("gc_interval should be greater than 1");
    return false;
  }
  return true;
}

bool LocalNodeConfig::Check() {
  if (graph.empty()) {
    LOG_WARN("local node graph is empty.");
    return false;
  }
  if (ip.empty()) {
    LOG_WARN("local node ip is empty.");
    return false;
  }
  if (bolt_port < 1) {
    LOG_WARN("local node bolt_port should be greater than 0");
    return false;
  }
  if (raft_poft < 1) {
    LOG_WARN("local node raft_poft should be greater than 0");
    return false;
  }
  return true;
}

RaftDriver::RaftDriver(
    std::function<void(uint64_t index, const meta::RaftRequest&)> apply,
    uint64_t apply_id, LocalNodeConfig local_node,
    const RaftLogStoreConfig& store_config, const RaftConfig& config)
    : apply_(std::move(apply)),
      apply_id_(apply_id),
      local_node_(std::move(local_node)),
      node_id_(0),
      tick_interval_(config.tick_interval),
      tick_timer_(timer_service_, tick_interval_),
      compact_interval_(store_config.gc_interval * 60 * 1000),
      compact_timer_(timer_service_, compact_interval_),
      store_config_(store_config),
      raft_config_(config) {}

RaftDriver::RaftDriver(
    std::function<void(uint64_t index, const meta::RaftRequest&)> apply,
    uint64_t apply_id, LocalNodeConfig local_node,
    std::vector<eraft::Peer> init_peers, const RaftLogStoreConfig& store_config,
    const RaftConfig& config)
    : apply_(std::move(apply)),
      apply_id_(apply_id),
      local_node_(std::move(local_node)),
      node_id_(0),
      init_peers_(std::move(init_peers)),
      tick_interval_(config.tick_interval),
      tick_timer_(timer_service_, tick_interval_),
      compact_interval_(store_config.gc_interval * 60 * 1000),
      compact_timer_(timer_service_, compact_interval_),
      store_config_(store_config),
      raft_config_(config) {}

eraft::Error RaftDriver::Run() {
  if (!local_node_.Check()) {
    return eraft::Error("invalid local node config");
  }
  rocksdb::Options options;
  options.create_if_missing = true;
  options.create_missing_column_families = true;
  rocksdb::BlockBasedTableOptions table_options;
  table_options.cache_index_and_filter_blocks = true;
  table_options.block_cache =
      rocksdb::NewLRUCache(store_config_.block_cache * 1024 * 1024L);
  table_options.data_block_index_type =
      rocksdb::BlockBasedTableOptions::kDataBlockBinaryAndHash;
  table_options.partition_filters = true;
  table_options.index_type =
      rocksdb::BlockBasedTableOptions::IndexType::kTwoLevelIndexSearch;
  options.table_factory.reset(
      rocksdb::NewBlockBasedTableFactory(table_options));
  options.IncreaseParallelism(store_config_.total_threads);
  options.OptimizeLevelStyleCompaction();
  std::vector<rocksdb::ColumnFamilyDescriptor> cfs;
  cfs.emplace_back(rocksdb::kDefaultColumnFamilyName, options);
  cfs.emplace_back("meta", options);
  std::vector<rocksdb::ColumnFamilyHandle*> cf_handles;
  rocksdb::DB* db;
  std::filesystem::create_directories(store_config_.path);
  auto s =
      rocksdb::DB::Open(options, store_config_.path, cfs, &cf_handles, &db);
  if (!s.ok()) {
    return eraft::Error("failed to open raft db, error: " + s.ToString());
  }
  storage_ = std::make_shared<RaftLogStorage>(db, cf_handles[0], cf_handles[1]);
  auto applied = std::max(apply_id_, storage_->GetApplyIndex());
  auto nodes = storage_->GetNodeInfos();
  if (nodes.has_value()) {
    node_infos_.ParseFromString(nodes.value());
  }
  for (auto& [id, node] : node_infos_.nodes()) {
    auto client = std::make_shared<NodeClient>(client_service_, node.ip(),
                                               node.raft_poft());
    client->Connect();
    node_clients_.emplace(node.node_id(), std::move(client));
  }
  LOG_INFO("raft nodes info: {}", node_infos_.ShortDebugString());
  bool exist = storage_->Init();
  eraft::Error err;
  if (!node_infos_.nodes().empty()) {
    err = InferNodeIdFromNodeInfos(local_node_, node_infos_, &node_id_);
    if (err != nullptr) {
      return err;
    }
  } else {
    if (init_peers_.empty()) {
      if (exist) {
        return eraft::Error(
            fmt::format("failed to infer local node id from empty node infos "
                        "for [{}]",
                        LocalNodeIdentityString(local_node_)));
      }
      return eraft::Error(
          "initial peers are required for first raft bootstrap");
    }
    err = InferNodeIdFromInitPeers(local_node_, init_peers_, &node_id_);
    if (err != nullptr) {
      return err;
    }
  }
  id_generator_.Reset(node_id_, duration_cast<milliseconds>(
                                    steady_clock::now().time_since_epoch())
                                    .count());
  eraft::Config config;
  config.id_ = node_id_;
  config.applied_ = applied;
  config.electionTick_ = raft_config_.election_tick;
  config.heartbeatTick_ = raft_config_.heartbeat_tick;
  config.storage_ = storage_;
  config.maxSizePerMsg_ = 1024 * 1024;
  config.maxInflightMsgs_ = 256;
  config.maxUncommittedEntriesSize_ = 1 << 30;
  config.stepDownOnRemoval_ = true;
  config.preVote_ = true;
  config.checkQuorum_ = true;
  std::tie(rn_, err) = eraft::NewRawNode(config);
  if (err != nullptr) {
    return err;
  }
  if (!exist) {
    err = rn_->Bootstrap(init_peers_);
    if (err != nullptr) {
      return err;
    }
  }
  Tick();
  CheckAndCompactLog();

  threads_.emplace_back([this]() {
    pthread_setname_np(pthread_self(), "raft_service");
    boost::asio::io_service::work holder(raft_service_);
    raft_service_.run();
  });
  threads_.emplace_back([this]() {
    pthread_setname_np(pthread_self(), "timer_service");
    boost::asio::io_service::work holder(timer_service_);
    timer_service_.run();
  });
  threads_.emplace_back([this]() {
    pthread_setname_np(pthread_self(), "apply_service");
    boost::asio::io_service::work holder(apply_service_);
    apply_service_.run();
  });
  threads_.emplace_back([this]() {
    pthread_setname_np(pthread_self(), "client_service");
    boost::asio::io_service::work holder(client_service_);
    client_service_.run();
  });
  return {};
}

void RaftDriver::Stop() {
  client_service_.stop();
  timer_service_.stop();
  raft_service_.stop();
  apply_service_.stop();
  for (auto& t : threads_) {
    t.join();
  }
  threads_.clear();
  if (storage_) {
    storage_->Close();
  }
  LOG_INFO("bolt raft driver stopped");
}

void RaftDriver::Step(raftpb::Message msg) {
  raft_service_.post([this, msg = std::move(msg)]() mutable {
    auto err = rn_->Step(std::move(msg));
    if (err != nullptr) {
      LOG_WARN("failed to step message, err: {}", err.String());
      return;
    }
    CheckReady();
  });
}

std::shared_ptr<PromiseContext> RaftDriver::Propose(uint64_t uuid,
                                                    raftpb::Message msg) {
  auto context = std::make_shared<PromiseContext>();
  raft_service_.post([this, uuid, context, msg = std::move(msg)]() mutable {
    if (rn_->raft_->id_ != rn_->raft_->lead_) {
      context->proposed.set_value(eraft::Error("not leader"));
      return;
    }
    msg.set_from(rn_->raft_->id_);
    auto err = rn_->raft_->Step(std::move(msg));
    if (err != nullptr) {
      LOG_WARN("failed to step raft message, err: {}", err.String());
      return;
    }
    context->proposed.set_value(std::move(err));
    {
      std::lock_guard<std::mutex> guard(promise_mutex_);
      pending_promise_.emplace(uuid, std::move(context));
    }
    CheckReady();
  });
  return context;
}

void RaftDriver::Tick() {
  raft_service_.post([this]() mutable {
    rn_->Tick();
    CheckReady();
  });
  tick_timer_.expires_at(tick_timer_.expires_at() + tick_interval_);
  tick_timer_.async_wait([this](const boost::system::error_code& ec) {
    if (ec) {
      LOG_WARN("tick_timer async_wait error: {}", ec.message());
      return;
    }
    Tick();
  });
}

std::shared_ptr<PromiseContext> RaftDriver::ProposeConfChange(
    raftpb::ConfChange& cc) {
  cc.set_id(id_generator_.Next());
  raftpb::Message msg;
  auto entry = msg.add_entries();
  entry->set_type(raftpb::EntryType::EntryConfChange);
  entry->set_data(cc.SerializeAsString());
  msg.set_type(raftpb::MessageType::MsgProp);
  return Propose(cc.id(), std::move(msg));
}

std::shared_ptr<PromiseContext> RaftDriver::ProposeRaftRequest(
    meta::RaftRequest request) {
  request.set_id(id_generator_.Next());
  raftpb::Message msg;
  auto entry = msg.add_entries();
  entry->set_type(raftpb::EntryType::EntryNormal);
  entry->set_data(request.SerializeAsString());
  msg.set_type(raftpb::MessageType::MsgProp);
  return Propose(request.id(), std::move(msg));
}

meta::RaftNodeInfos RaftDriver::GetNodeInfosWithLeader() {
  std::promise<uint64_t> promise;
  auto future = promise.get_future();
  raft_service_.post(
      [this, &promise]() { promise.set_value(rn_->raft_->lead_); });
  auto leader = future.get();

  std::shared_lock<std::shared_mutex> lock(nodes_mutex_);
  meta::RaftNodeInfos ret = node_infos_;
  if (ret.nodes().count(leader)) {
    ret.mutable_nodes()->at(leader).set_is_leader(true);
  }
  return ret;
}

RaftStatus RaftDriver::GetRaftStatus() {
  std::promise<RaftStatus> promise;
  auto future = promise.get_future();
  raft_service_.post([this, &promise]() {
    RaftStatus rs;
    rs.s = rn_->GetStatus();
    rs.first_log = storage_->FirstIndex().first - 1;
    rs.last_log = storage_->LastIndex().first;
    promise.set_value(rs);
  });
  return future.get();
}

void RaftDriver::CheckAndCompactLog() {
  raft_service_.post([this]() mutable {
    auto first = storage_->FirstIndex().first;
    auto applied = rn_->raft_->raftLog_->applied_;
    if (applied > first) {
      if (applied - first >= store_config_.keep_logs + 100000) {
        auto compacted = applied - store_config_.keep_logs;
        storage_->Compact(compacted);
        LOG_INFO("compact raft log, compacted index:{}, applied index:{}",
                 compacted, applied);
      }
    }
  });
  compact_timer_.expires_at(compact_timer_.expires_at() + compact_interval_);
  compact_timer_.async_wait([this](const boost::system::error_code& ec) {
    if (ec) {
      LOG_WARN("compact_timer async_wait error: {}", ec.message());
      return;
    }
    CheckAndCompactLog();
  });
}

void RaftDriver::CheckReady() {
  if (!rn_->HasReady()) {
    return;
  }
  auto ready = rn_->GetReady();
  if (ready.softState_) {
    LOG_INFO("soft state change, state:{}, lead:{}",
             eraft::ToString(ready.softState_->raftState_),
             ready.softState_->lead_);
  }
  rocksdb::WriteBatch batch;
  if (!ready.entries_.empty()) {
    storage_->Append(std::move(ready.entries_), batch);
  }
  if (!eraft::IsEmptyHardState(ready.hardState_)) {
    storage_->SetHardState(ready.hardState_, batch);
  }
  if (!eraft::IsEmptySnap(ready.snapshot_)) {
    LOG_FATAL("snapshot should be empty");
  }
  storage_->WriteBatch(batch);
  {
    std::shared_lock<std::shared_mutex> lock(nodes_mutex_);
    for (const auto& msg : ready.messages_) {
      auto iter = node_clients_.find(msg.to());
      if (iter != node_clients_.end()) {
        if (iter->second->connected()) {
          iter->second->Send(MessageToNetString(msg));
          if (mark_unreachable_.count(msg.to())) {
            mark_unreachable_.erase(msg.to());
          }
        } else {
          if (!mark_unreachable_.count(msg.to())) {
            LOG_WARN("report raft node {} is unreachable, {}", msg.to(),
                     msg.ShortDebugString());
            rn_->ReportUnreachable(msg.to());
            mark_unreachable_.insert(msg.to());
          }
        }
      } else {
        LOG_WARN("send msg but peer client id {} not exists", msg.to());
      }
    }
  }
  if (!eraft::IsEmptySnap(ready.snapshot_)) {
    LOG_FATAL("snapshot should be empty");
  }
  if (!ready.committedEntries_.empty()) {
    bool has_confchange = false;
    for (auto& entry : ready.committedEntries_) {
      if (entry.type() == raftpb::EntryConfChange) {
        has_confchange = true;
        break;
      }
    }
    if (!has_confchange) {
      apply_service_.post(
          [this, committedEntries = std::move(ready.committedEntries_)]() {
            Apply(committedEntries);
          });
    } else {
      LOG_INFO("there is ConfChange in committed entries, entries size: {}",
               ready.committedEntries_.size());
      Apply(ready.committedEntries_);
    }
  }
  rn_->Advance({});
}

void RaftDriver::Apply(const std::vector<raftpb::Entry>& entries) {
  for (const auto& entry : entries) {
    switch (entry.type()) {
      case raftpb::EntryNormal: {
        if (entry.data().empty()) {
          continue;
        }
        meta::RaftRequest request;
        request.ParseFromString(entry.data());
        std::shared_ptr<raft::PromiseContext> context;
        {
          std::lock_guard<std::mutex> guard(promise_mutex_);
          auto iter = pending_promise_.find(request.id());
          if (iter != pending_promise_.end()) {
            context = iter->second;
            pending_promise_.erase(iter);
          }
        }
        if (context) {
          context->index = entry.index();
          context->commited.set_value();
          context->applied.get_future().wait();
        } else {
          apply_(entry.index(), request);
        }
        break;
      }
      case raftpb::EntryConfChange: {
        raftpb::ConfChange cc;
        if (!cc.ParseFromString(entry.data())) {
          LOG_FATAL("failed to parse ConfChange data");
        }
        auto confstate = rn_->ApplyConfChange(raftpb::ConfChangeWrap(cc));
        meta::RaftNodeInfo node_info;
        node_info.ParseFromString(cc.context());
        switch (cc.type()) {
          case raftpb::ConfChangeType::ConfChangeAddLearnerNode:
          case raftpb::ConfChangeType::ConfChangeAddNode: {
            if (cc.type() == raftpb::ConfChangeType::ConfChangeAddNode) {
              LOG_INFO("add node: {}", node_info.ShortDebugString());
            } else {
              LOG_INFO("add learner: {}", node_info.ShortDebugString());
            }
            std::unique_lock<std::shared_mutex> lock(nodes_mutex_);
            if (!node_infos_.nodes().count(node_info.node_id())) {
              node_infos_.mutable_nodes()->insert(
                  {node_info.node_id(), node_info});
              auto client = std::make_shared<NodeClient>(
                  client_service_, node_info.ip(), node_info.raft_poft());
              client->Connect();
              node_clients_.emplace(node_info.node_id(), std::move(client));
            } else {
              LOG_ERROR("node id {} has already existed", node_info.node_id());
            }
            break;
          }
          case raftpb::ConfChangeType::ConfChangeRemoveNode: {
            LOG_INFO("remove node: {}", node_info.node_id());
            std::unique_lock<std::shared_mutex> lock(nodes_mutex_);
            if (node_infos_.nodes().count(node_info.node_id())) {
              node_clients_.at(node_info.node_id())->Close();
              node_clients_.erase(node_info.node_id());
              node_infos_.mutable_nodes()->erase(node_info.node_id());
            } else {
              LOG_ERROR("no such node id {}", node_info.node_id());
            }
            break;
          }
          case raftpb::ConfChangeType::ConfChangeUpdateNode: {
            LOG_INFO("update node: {}", cc.ShortDebugString());
            break;
          }
          default: {
            break;
          }
        }

        LOG_INFO("new conf state: {}", confstate->ShortDebugString());
        rocksdb::WriteBatch wb;
        storage_->SetConfState(*confstate, wb);
        storage_->SetNodeInfos(node_infos_.SerializeAsString(), wb);
        storage_->SetApplyIndex(entry.index(), wb);
        storage_->WriteBatch(wb);

        std::shared_ptr<raft::PromiseContext> context;
        {
          std::lock_guard<std::mutex> guard(promise_mutex_);
          auto iter = pending_promise_.find(cc.id());
          if (iter != pending_promise_.end()) {
            context = iter->second;
            pending_promise_.erase(iter);
          }
        }
        if (context) {
          context->applied.set_value();
        }
        break;
      }
      default: {
        LOG_ERROR("unhandled entry : {}", entry.ShortDebugString());
      }
    }
  }
}

}  // namespace raft
