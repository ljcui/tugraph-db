#include <gflags/gflags.h>
#include <filesystem>
#include <shared_mutex>
#include <boost/endian/conversion.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include "raft_driver.h"
#include "tools/json.hpp"
#include "tools/lgraph_log.h"
#include "fma-common/string_formatter.h"
#include "fma-common/assert.h"
#include "bolt_raft.pb.h"

using boost::asio::async_write;
using boost::asio::ip::tcp;

namespace bolt_raft {
void NodeClient::reconnect() {
    if (has_closed_) {
        return;
    }
    boost::system::error_code ec;
    socket_.close(ec);
    if (ec) {
        LOG_WARN() << FMA_FMT("socket close error: {}", ec.message());
    }
    connected_ = false;
    send_buffers_.clear();
    msg_queue_.clear();
    timer_.expires_from_now(interval_);
    timer_.async_wait([this, self = shared_from_this()](const boost::system::error_code &ec) {
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
        LOG_DEBUG() << FMA_FMT("connection[{}:{}] is closed, drop this message", endpoint_.address().to_string(), endpoint_.port());
        return;
    }
    io_service_.post([this, self = shared_from_this(), msg = std::move(str)]() mutable  {
        if (!connected_) {
            LOG_DEBUG() << FMA_FMT("connection[{}:{}] is not available, drop this message", endpoint_.address().to_string(), endpoint_.port());
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
        [this, self = shared_from_this()](const boost::system::error_code &ec, size_t) {
            if (ec) {
                LOG_WARN() << FMA_FMT("async write error {}", ec.message().c_str());
                if (ec == boost::asio::error::operation_aborted) {
                    return;
                }
                reconnect();
                return;
            }
            connected_ = true;
            do_read_some();
        }
    );
}

void NodeClient::do_send() {
    for (size_t i = 0; i < msg_queue_.size(); i++) {
        send_buffers_.emplace_back(boost::asio::buffer(msg_queue_[i]));
        if (send_buffers_.size() >= 5) {
            break;
        }
    }
    async_write(socket_, send_buffers_,
        [this, self = shared_from_this()](const boost::system::error_code &ec, std::size_t) {
            if (ec) {
                LOG_WARN() << FMA_FMT("async write error {}", ec.message().c_str());
                if (ec == boost::asio::error::operation_aborted) {
                    return;
                }
                reconnect();
                return;
            }
            assert(msg_queue_.size() >= send_buffers_.size());
            msg_queue_.erase(msg_queue_.begin(), msg_queue_.begin() + send_buffers_.size());
            send_buffers_.clear();
            if (!msg_queue_.empty()) {
                do_send();
            }
        }
    );
}

void NodeClient::do_read_some() {
    socket_.async_read_some(boost::asio::buffer(buffer4_),
        [this, self = shared_from_this()](const boost::system::error_code &ec, size_t) {
            if (ec) {
                LOG_WARN() << FMA_FMT("async_read_some error: {}", ec.message().c_str());
                if (ec == boost::asio::error::operation_aborted) {
                    return;
                }
                reconnect();
            } else {
                LOG_ERROR() <<"unexpected read";
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
        LOG_ERROR() << FMA_FMT("socket_set_options error: {}", e.what());
    }
}

void NodeClient::Connect() {
    if (has_closed_) {
        return;
    }
    boost::system::error_code ec;
    socket_.open(tcp::v4(), ec);
    if (ec) {
        LOG_WARN() << FMA_FMT("socket open error: {}", ec.message().c_str());
    }
    socket_.async_connect(endpoint_,
        [this, self = shared_from_this()](const boost::system::error_code &ec) {
            if (ec) {
                if (ec == boost::asio::error::operation_aborted) {
                    return;
                }
                LOG_WARN() << FMA_FMT("async connect {} error: {}", boost::lexical_cast<std::string>(endpoint_).c_str(), ec.message().c_str());
                reconnect();
            } else {
                LOG_INFO() << FMA_FMT("connect {} success", boost::lexical_cast<std::string>(endpoint_).c_str());
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

RaftDriver::RaftDriver(std::function<void (uint64_t index, const RaftRequest&)> apply,
           uint64_t apply_id,
           int64_t node_id,
           std::vector<eraft::Peer> init_peers,
           std::string log_path)
    : apply_(std::move(apply)),
    apply_id_(apply_id),
    node_id_(node_id),
    init_peers_(std::move(init_peers)),
    log_path_(std::move(log_path)),
    tick_interval_(100),
    tick_timer_(tick_service_, tick_interval_) {
    id_generator_ = std::make_shared<Generator>(
        node_id,std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::steady_clock::now().time_since_epoch()).count());
    threads_.emplace_back([this]() {
        pthread_setname_np(pthread_self(), "raft_service");
        boost::asio::io_service::work holder(raft_service_);
        raft_service_.run();
    });
    threads_.emplace_back([this]() {
        pthread_setname_np(pthread_self(), "tick_service");
        boost::asio::io_service::work holder(tick_service_);
        tick_service_.run();
    });
    threads_.emplace_back([this]() {
        pthread_setname_np(pthread_self(), "ready_service");
        boost::asio::io_service::work holder(apply_service_);
        apply_service_.run();
    });
    threads_.emplace_back([this]() {
        pthread_setname_np(pthread_self(), "client_service");
        boost::asio::io_service::work holder(client_service_);
        client_service_.run();
    });
}

eraft::Error RaftDriver::Run() {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.create_missing_column_families = true;
    std::vector<rocksdb::ColumnFamilyDescriptor> cfs;
    cfs.emplace_back(rocksdb::kDefaultColumnFamilyName, options);
    cfs.emplace_back("meta", options);
    std::vector<rocksdb::ColumnFamilyHandle*> cf_handles;
    rocksdb::DB* db;
    std::filesystem::create_directories(log_path_);
    auto s = rocksdb::DB::Open(options, log_path_, cfs, &cf_handles, &db);
    if (!s.ok()) {
        return eraft::Error("Open raft db error: " + s.ToString());
    }
    storage_ = std::make_shared<RaftLogStorage>(db, cf_handles[0], cf_handles[1]);
    auto applied = std::max(apply_id_, storage_->GetApplyIndex());
    auto nodes = storage_->GetNodeInfos();
    if (nodes.has_value()) {
        node_infos_.ParseFromString(nodes.value());
    }
    for (auto& [id, node] : node_infos_.nodes()) {
        auto client = std::make_shared<NodeClient>(
            client_service_, node.ip(), node.bolt_raft_port());
        client->Connect();
        node_clients_.emplace(node.node_id(), std::move(client));
    }
    LOG_INFO() << FMA_FMT("Raft nodes info: {}", node_infos_.ShortDebugString());
    bool exist = storage_->Init();
    eraft::Config config;
    config.id_ = node_id_;
    config.applied_ = applied;
    config.electionTick_ = 10;
    config.heartbeatTick_ = 1;
    config.storage_ = storage_;
    config.maxSizePerMsg_ = 1024 * 1024;
    config.maxInflightMsgs_ = 256;
    config.maxUncommittedEntriesSize_ = 1 << 30;
    config.stepDownOnRemoval_ = true;
    config.preVote_ = true;
    config.checkQuorum_ = true;
    eraft::Error err;
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
    return {};
}

void RaftDriver::Message(raftpb::Message msg) {
    // Ignore unexpected local messages receiving over network.
    if (eraft::IsLocalMsg(msg.type()) && !eraft::IsLocalMsgTarget(msg.from())) {
        LOG_WARN() << "ignore unexpected local messages receiving over network : " << msg.ShortDebugString();
        return;
    }
    raft_service_.post([this, msg = std::move(msg)]() mutable {
        if (eraft::IsResponseMsg(msg.type()) &&
            !eraft::IsLocalMsgTarget(msg.from()) &&
            rn_->raft_->trk_.progress_.data().count(msg.from()) == 0) {
            // Filter out response message from unknown From.
            return;
        }
        auto err = rn_->Step(std::move(msg));
        if (err != nullptr) {
            LOG_WARN() << FMA_FMT("failed to step message, err: {}",err.String().c_str());
            return;
        }
        CheckReady();
    });
}

std::shared_ptr<PromiseContext> RaftDriver::PostMessage(uint64_t uuid, raftpb::Message msg) {
    auto context = std::make_shared<PromiseContext>();
    raft_service_.post([this, uuid, context, msg = std::move(msg)]() mutable {
        if (rn_->raft_->id_ != rn_->raft_->lead_) {
            context->proposed.set_value(eraft::Error("not leader"));
            return;
        }
        msg.set_from(rn_->raft_->id_);
        auto err = rn_->Step(std::move(msg));
        if (err != nullptr) {
            LOG_WARN() << FMA_FMT("failed to step raft message, err: {}",err.String().c_str());
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
    tick_timer_.async_wait([this](const boost::system::error_code& ec){
        if (ec) {
            LOG_WARN() << "tick_timer async_wait error: " << ec.message();
            return;
        }
        Tick();
    });
}

std::shared_ptr<PromiseContext> RaftDriver::ProposeConfChange(const raftpb::ConfChange& cc) {
    raftpb::Message msg;
    auto entry = msg.add_entries();
    entry->set_type(raftpb::EntryType::EntryConfChange);
    entry->set_data(cc.SerializeAsString());
    msg.set_type(raftpb::MessageType::MsgProp);
    return PostMessage(id_generator_->Next(), std::move(msg));
}

std::shared_ptr<PromiseContext> RaftDriver::Propose(bolt_raft::RaftRequest request) {
    request.set_id(id_generator_->Next());
    raftpb::Message msg;
    auto entry = msg.add_entries();
    entry->set_type(raftpb::EntryType::EntryNormal);
    entry->set_data(request.SerializeAsString());
    msg.set_type(raftpb::MessageType::MsgProp);
    return PostMessage(request.id(), std::move(msg));
}

NodeInfos RaftDriver::GetNodeInfosWithLeader() {
    std::promise<uint64_t> promise;
    auto future = promise.get_future();
    raft_service_.post([this, &promise]() {
        promise.set_value(rn_->raft_->lead_);
    });
    auto leader = future.get();

    std::shared_lock<std::shared_mutex> lock(node_infos_mutex_);
    NodeInfos ret = node_infos_;
    if (ret.nodes().count(leader)) {
        ret.mutable_nodes()->at(leader).set_is_leader(true);
    }
    return ret;
}

void RaftDriver::CheckReady() {
    if (!rn_->HasReady()) {
        return;
    }
    auto ready = rn_->GetReady();
    if (ready.softState_) {
        LOG_INFO() << FMA_FMT("soft state change, state:{}, lead:{}",
                              eraft::ToString(ready.softState_->raftState_), ready.softState_->lead_);
    }
    rocksdb::WriteBatch batch;
    if (!ready.entries_.empty()) {
        storage_->Append(std::move(ready.entries_), batch);
    }
    if (!eraft::IsEmptyHardState(ready.hardState_)) {
        storage_->SetHardState(ready.hardState_, batch);
    }
    if (!eraft::IsEmptySnap(ready.snapshot_)) {
        LOG_FATAL() << "Snapshot should be empty";
    }
    storage_->WriteBatch(batch);
    for (const auto& msg : ready.messages_) {
        auto iter = node_clients_.find(msg.to());
        if (iter != node_clients_.end()) {
            iter->second->Send(MessageToNetString(msg));
        } else {
            LOG_WARN() << FMA_FMT("send msg, but peer client id {} not exists", msg.to());
        }
    }
    if (!eraft::IsEmptySnap(ready.snapshot_)) {
        LOG_FATAL() << "Snapshot should be empty";
    }
    if (!ready.committedEntries_.empty()) {
        apply_service_.post([this, committedEntries = std::move(ready.committedEntries_)]() mutable {
            Apply(std::move(committedEntries));
        });
    }
    rn_->Advance({});
}

void RaftDriver::Apply(std::vector<raftpb::Entry> entries) {
    for (const auto& entry : entries) {
        switch (entry.type()) {
        case raftpb::EntryNormal: {
                if (entry.data().empty()) {
                    continue;
                }
                RaftRequest request;
                auto ret = request.ParseFromString(entry.data());
                FMA_ASSERT(ret);
                std::shared_ptr<bolt_raft::PromiseContext> context;
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
                    LOG_FATAL() << "failed to parse ConfChange";
                }
                auto confstate = rn_->ApplyConfChange(raftpb::ConfChangeWrap(cc));
                NodeInfo node_info;
                node_info.ParseFromString(cc.context());
                switch (cc.type()) {
                case  raftpb::ConfChangeType::ConfChangeAddLearnerNode:
                case  raftpb::ConfChangeType::ConfChangeAddNode: {
                        if (cc.type() == raftpb::ConfChangeType::ConfChangeAddNode) {
                            LOG_INFO() << FMA_FMT("Add node: {}", node_info.ShortDebugString());
                        } else {
                            LOG_INFO() << FMA_FMT("Add learner: {}", node_info.ShortDebugString());
                        }
                        std::unique_lock<std::shared_mutex> lock(node_infos_mutex_);
                        if (!node_infos_.nodes().count(node_info.node_id())) {
                            node_infos_.mutable_nodes()->insert({node_info.node_id(), node_info});
                            auto client = std::make_shared<NodeClient>(client_service_, node_info.ip(), node_info.bolt_raft_port());
                            client->Connect();
                            node_clients_.emplace(node_info.node_id(), std::move(client));
                        } else {
                            LOG_ERROR() << FMA_FMT("node id %d has already existed", node_info.node_id());
                        }
                        break;
                    }
                case  raftpb::ConfChangeType::ConfChangeRemoveNode: {
                        LOG_INFO() << FMA_FMT("Remove node: {}", node_info.node_id());
                        std::unique_lock<std::shared_mutex> lock(node_infos_mutex_);
                        if (node_infos_.nodes().count(node_info.node_id())) {
                            node_clients_.at(node_info.node_id())->Close();
                            node_clients_.erase(node_info.node_id());
                            node_infos_.mutable_nodes()->erase(node_info.node_id());
                        } else {
                            LOG_ERROR() << FMA_FMT("No such node id {}", node_info.node_id());
                        }
                        break;
                    }
                case  raftpb::ConfChangeType::ConfChangeUpdateNode: {
                        LOG_INFO() << FMA_FMT("Update node: %s", cc.ShortDebugString());
                        break;
                    }
                default: {
                        break;
                    }}

                LOG_INFO() << FMA_FMT("New ConfState: {}", confstate->ShortDebugString());
                rocksdb::WriteBatch wb;
                storage_->SetConfState(*confstate, wb);
                storage_->SetNodeInfos(node_infos_.SerializeAsString(), wb);
                storage_->SetApplyIndex(entry.index(), wb);
                storage_->WriteBatch(wb);
                /*if (cc.id() > 0) {
                    std::shared_lock lock(promise_mutex);
                    auto iter = pending_promise.find(cc.id());
                    if (iter != pending_promise.end()) {
                        iter->second.set_value(cc.context());
                    }
                }*/
                break;
            }
        default: {
                LOG_ERROR() << FMA_FMT("Unhandled entry : {}", entry.ShortDebugString());
            }
        }
    }
}

}
