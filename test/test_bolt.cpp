#include <gtest/gtest.h>
#include <sys/wait.h>

#include <any>
#include <array>
#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bolt/blocking_queue.h"
#include "bolt/errors.h"
#include "bolt/hydrator.h"
#include "bolt/pack_stream.h"
#include "common/exceptions.h"
#include "proto/meta.pb.h"
#include "proxy/proxy_server.h"
#include "server/lgraph_server.h"
#include "test_util.h"

namespace {

namespace fs = std::filesystem;

int32_t AllocateFreePort() {
  boost::asio::io_service service;
  boost::asio::ip::tcp::acceptor acceptor(
      service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));
  return static_cast<int32_t>(acceptor.local_endpoint().port());
}

int RunShellCommand(const std::string& command) {
  int status = std::system(command.c_str());
  if (status == -1) {
    return -1;
  }
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return status;
}

std::string ChunkPayload(const std::string& payload) {
  auto size =
      boost::endian::native_to_big(static_cast<uint16_t>(payload.size()));
  std::string message(reinterpret_cast<const char*>(&size), sizeof(size));
  message.append(payload);
  message.append(2, '\0');
  return message;
}

std::string MakeStructMessage(bolt::BoltMsg type) {
  std::string payload;
  payload.push_back(static_cast<char>(0xb0));
  payload.push_back(static_cast<char>(type));
  return ChunkPayload(payload);
}

std::string MakeRouteMessage() {
  std::string payload;
  payload.push_back(static_cast<char>(0xb3));
  payload.push_back(static_cast<char>(bolt::BoltMsg::Route));
  payload.push_back(static_cast<char>(0xa0));
  payload.push_back(static_cast<char>(0x90));
  payload.push_back(static_cast<char>(0xc0));
  return ChunkPayload(payload);
}

struct BoltClientResponse {
  bolt::BoltMsg tag = bolt::BoltMsg::Ignored;
  std::string failure_code;
  std::string failure_message;
  std::string server;
};

class RawBoltClient {
 public:
  explicit RawBoltClient(int32_t port) : socket_(io_context_) {
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), port);
    socket_.connect(endpoint);
    Handshake();
  }

  ~RawBoltClient() {
    boost::system::error_code ec;
    socket_.close(ec);
  }

  void SendHello() {
    bolt::PackStream ps;
    ps.AppendHello({{"user_agent", std::string("tugraph-test")},
                    {"principal", std::string("admin")},
                    {"credentials", std::string("73@TuGraph")}});
    Send(ps.ConstBuffer());
  }

  void SendReset() {
    bolt::PackStream ps;
    ps.AppendReset();
    Send(ps.ConstBuffer());
  }

  void SendPull(int64_t n) {
    bolt::PackStream ps;
    ps.AppendPullN(n);
    Send(ps.ConstBuffer());
  }

  void SendRun(const std::string& cypher,
               const std::unordered_map<std::string, std::any>& params,
               const std::unordered_map<std::string, std::any>& extra) {
    bolt::PackStream ps;
    ps.AppendRun(cypher, params, extra);
    Send(ps.ConstBuffer());
  }

  void SendRunWithExtraField() {
    std::string payload;
    payload.push_back(static_cast<char>(0xb4));
    payload.push_back(static_cast<char>(bolt::BoltMsg::Run));
    payload.push_back(static_cast<char>(0x80));
    payload.push_back(static_cast<char>(0xa0));
    payload.push_back(static_cast<char>(0xa0));
    payload.push_back(static_cast<char>(0xc0));
    Send(ChunkPayload(payload));
  }

  void SendRoute() { Send(MakeRouteMessage()); }

  void SendBegin() { Send(MakeStructMessage(bolt::BoltMsg::Begin)); }

  BoltClientResponse ReadResponse() {
    auto payload = ReadPayload();

    bolt::Unpacker unpacker;
    unpacker.Reset(payload);
    unpacker.Next();
    if (unpacker.CurrentType() != bolt::PackType::Structure) {
      throw std::runtime_error("Bolt response should be a struct");
    }
    unpacker.Len();
    BoltClientResponse response;
    response.tag = static_cast<bolt::BoltMsg>(unpacker.StructTag());

    bolt::Hydrator hydrator;
    auto parsed = hydrator.Hydrate(payload);
    if (parsed.second) {
      throw std::runtime_error(parsed.second.value());
    }
    if (response.tag == bolt::BoltMsg::Failure) {
      auto error = std::any_cast<std::optional<bolt::Neo4jError>>(parsed.first);
      if (error.has_value()) {
        response.failure_code = error->code;
        response.failure_message = error->msg;
      }
    } else if (response.tag == bolt::BoltMsg::Success) {
      auto* success = std::any_cast<bolt::Success*>(parsed.first);
      if (success != nullptr) {
        response.server = success->server;
      }
    }
    return response;
  }

  bool WaitForClose(std::chrono::milliseconds timeout) {
    boost::system::error_code ec;
    socket_.non_blocking(true, ec);
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
      char byte = 0;
      auto n = socket_.read_some(boost::asio::buffer(&byte, 1), ec);
      if (!ec && n > 0) {
        return false;
      }
      if (ec == boost::asio::error::eof ||
          ec == boost::asio::error::connection_reset ||
          ec == boost::asio::error::bad_descriptor ||
          ec == boost::asio::error::not_connected) {
        return true;
      }
      if (ec != boost::asio::error::would_block &&
          ec != boost::asio::error::try_again) {
        return true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
  }

 private:
  void Send(const std::string& message) {
    boost::asio::write(socket_, boost::asio::buffer(message));
  }

  void Handshake() {
    constexpr std::array<uint8_t, 20> handshake = {
        0x60, 0x60, 0xb0, 0x17, 0x00, 0x04, 0x04, 0x04, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    boost::asio::write(socket_, boost::asio::buffer(handshake));
    std::array<uint8_t, 4> selected = {0};
    boost::asio::read(socket_, boost::asio::buffer(selected));
    if (selected[2] != 4 || selected[3] != 4) {
      throw std::runtime_error("server did not select Bolt v4.4");
    }
  }

  std::string ReadPayload() {
    std::string payload;
    while (true) {
      uint16_t size = 0;
      boost::asio::read(socket_, boost::asio::buffer(&size, sizeof(size)));
      size = boost::endian::big_to_native(size);
      if (size == 0) {
        if (!payload.empty()) {
          return payload;
        }
        continue;
      }
      std::string chunk(size, '\0');
      boost::asio::read(socket_, boost::asio::buffer(chunk));
      payload.append(chunk);
    }
  }

  boost::asio::io_context io_context_;
  boost::asio::ip::tcp::socket socket_;
};

class BoltProxyTestServer {
 public:
  BoltProxyTestServer() : bolt_port_(AllocateFreePort()) {}
  ~BoltProxyTestServer() { Stop(); }

  bool Start() {
    Stop();
    proxy::ProxyServerOptions options;
    options.listen_port = bolt_port_;
    options.bolt_io_thread_num = 1;
    options.worker_thread_num = 1;
    options.max_pending_messages_per_connection = 8;
    options.shard_map = proxy::ShardMap::FromConfig("default", "default_s", 1,
                                                    2, "0-0=1@127.0.0.1:1:2");
    server_ = std::make_unique<proxy::ProxyServer>(std::move(options));
    return server_->Start();
  }

  void Stop() {
    if (server_) {
      server_->Stop();
      server_.reset();
    }
  }

  int32_t bolt_port() const { return bolt_port_; }

 private:
  int32_t bolt_port_ = 0;
  std::unique_ptr<proxy::ProxyServer> server_;
};

class BoltProxyBackendTestServer {
 public:
  BoltProxyBackendTestServer()
      : backend_bolt_port_(AllocateFreePort()),
        backend_raft_port_(AllocateFreePort()),
        proxy_bolt_port_(AllocateFreePort()) {
    data_path_ =
        "test_bolt_proxy_backend_" + std::to_string(backend_bolt_port_);
  }

  ~BoltProxyBackendTestServer() { Stop(); }

  bool Start() {
    Stop();
    fs::remove_all(data_path_);

    server::LGraphServerOptions backend_options;
    backend_options.data_path = data_path_;
    backend_options.local_node_options.host = "127.0.0.1";
    backend_options.local_node_options.bolt_port = backend_bolt_port_;
    backend_options.local_node_options.raft_port = backend_raft_port_;
    backend_options.bolt_io_thread_num = 1;
    backend_options.bolt_worker_thread_num = 2;
    backend_options.galaxy_options.block_cache_size = 8 * 1024 * 1024;
    backend_options.galaxy_options.row_cache_size = 4 * 1024 * 1024;
    backend_options.galaxy_options.raft_log_block_cache_size = 8 * 1024 * 1024;
    backend_options.galaxy_options.raft_scheduler_shards = 1;
    backend_options.galaxy_options.assistant_thread_num = 1;

    backend_ =
        std::make_unique<server::LGraphServer>(std::move(backend_options));
    if (!backend_->Start()) {
      return false;
    }

    meta::RaftNodeInfo node_info;
    node_info.set_node_id(1);
    node_info.set_graph("default_s00");
    node_info.set_ip("127.0.0.1");
    node_info.set_bolt_port(backend_bolt_port_);
    node_info.set_raft_poft(backend_raft_port_);
    meta::RaftNodeInfos node_infos;
    (*node_infos.mutable_nodes())[1] = node_info;
    backend_->galaxy()->CreateGraphWithRaft("default_s00", node_infos);

    proxy::ProxyServerOptions proxy_options;
    proxy_options.listen_port = proxy_bolt_port_;
    proxy_options.bolt_io_thread_num = 1;
    proxy_options.worker_thread_num = 1;
    proxy_options.max_pending_messages_per_connection = 8;
    proxy_options.shard_map = proxy::ShardMap::FromConfig(
        "default", "default_s", 1, 2,
        "0-0=1@127.0.0.1:" + std::to_string(backend_bolt_port_) + ":" +
            std::to_string(backend_raft_port_));
    proxy_ = std::make_unique<proxy::ProxyServer>(std::move(proxy_options));
    return proxy_->Start() &&
           testutil::WaitUntilRaftLeader(
               backend_->galaxy()->OpenGraph("default_s00")->raft_driver(),
               std::chrono::seconds(10));
  }

  void Stop() {
    if (proxy_) {
      proxy_->Stop();
      proxy_.reset();
    }
    if (backend_) {
      backend_->Stop();
      backend_.reset();
    }
    if (!data_path_.empty()) {
      fs::remove_all(data_path_);
    }
  }

  int32_t proxy_bolt_port() const { return proxy_bolt_port_; }

 private:
  int32_t backend_bolt_port_ = 0;
  int32_t backend_raft_port_ = 0;
  int32_t proxy_bolt_port_ = 0;
  std::string data_path_;
  std::unique_ptr<server::LGraphServer> backend_;
  std::unique_ptr<proxy::ProxyServer> proxy_;
};

class BoltNeo4jDriverServer {
 public:
  BoltNeo4jDriverServer()
      : bolt_port_(AllocateFreePort()), raft_port_(AllocateFreePort()) {
    data_path_ = "test_bolt_neo4j_driver_" + std::to_string(bolt_port_);
  }

  ~BoltNeo4jDriverServer() { Stop(); }

  bool Start() {
    Stop();
    fs::remove_all(data_path_);

    server::LGraphServerOptions options;
    options.data_path = data_path_;
    options.local_node_options.host = "127.0.0.1";
    options.local_node_options.bolt_port = bolt_port_;
    options.local_node_options.raft_port = raft_port_;
    options.bolt_io_thread_num = 1;
    options.bolt_worker_thread_num = 2;
    options.galaxy_options.block_cache_size = 8 * 1024 * 1024;
    options.galaxy_options.row_cache_size = 4 * 1024 * 1024;
    options.galaxy_options.raft_log_block_cache_size = 8 * 1024 * 1024;
    options.galaxy_options.raft_scheduler_shards = 1;
    options.galaxy_options.assistant_thread_num = 1;

    server_ = std::make_unique<server::LGraphServer>(std::move(options));
    return server_->Start();
  }

  void Stop() {
    if (server_) {
      server_->Stop();
      server_.reset();
    }
    if (!data_path_.empty()) {
      fs::remove_all(data_path_);
    }
  }

  int32_t bolt_port() const { return bolt_port_; }

 private:
  int32_t bolt_port_ = 0;
  int32_t raft_port_ = 0;
  std::string data_path_;
  std::unique_ptr<server::LGraphServer> server_;
};

}  // namespace

TEST(BoltBlockingQueue, EnforcesCapacity) {
  bolt::BlockingQueue<int> queue(2);

  EXPECT_TRUE(queue.Push(1));
  EXPECT_TRUE(queue.Push(2));
  EXPECT_FALSE(queue.Push(3));
  EXPECT_EQ(queue.Size(), 2u);

  auto first = queue.TryPop();
  ASSERT_TRUE(first.has_value());
  EXPECT_EQ(first.value(), 1);

  EXPECT_TRUE(queue.Push(3));

  auto second = queue.Pop(std::chrono::milliseconds(1));
  ASSERT_TRUE(second.has_value());
  EXPECT_EQ(second.value(), 2);

  auto third = queue.Pop(std::chrono::milliseconds(1));
  ASSERT_TRUE(third.has_value());
  EXPECT_EQ(third.value(), 3);

  EXPECT_TRUE(queue.Empty());
}

TEST(BoltBlockingQueue, ZeroCapacityMeansUnlimited) {
  bolt::BlockingQueue<int> queue(0);

  for (int i = 0; i < 8; ++i) {
    EXPECT_TRUE(queue.Push(i));
  }

  EXPECT_EQ(queue.Size(), 8u);
  for (int i = 0; i < 8; ++i) {
    auto value = queue.TryPop();
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), i);
  }
  EXPECT_TRUE(queue.Empty());
}

TEST(ProxyShardMap, RejectsInvalidShardRange) {
  EXPECT_THROW(proxy::ShardMap::FromConfig("default", "default_s", 2, 2,
                                           "bad=1@127.0.0.1:7687:7688"),
               LgraphException);
}

TEST(ProxyShardMap, RejectsInvalidBackendPort) {
  EXPECT_THROW(proxy::ShardMap::FromConfig("default", "default_s", 1, 2,
                                           "0=1@127.0.0.1:70000:7688"),
               LgraphException);
}

TEST(ProxyBoltProtocol, DuplicateHelloClosesConnection) {
  BoltProxyTestServer server;
  ASSERT_TRUE(server.Start());

  RawBoltClient client(server.bolt_port());
  client.SendHello();
  auto hello = client.ReadResponse();
  EXPECT_EQ(hello.tag, bolt::BoltMsg::Success);
  EXPECT_EQ(hello.server, "Neo4j/tugraph-db-proxy");

  client.SendHello();
  EXPECT_TRUE(client.WaitForClose(std::chrono::milliseconds(2000)));
}

TEST(ProxyBoltProtocol, RouteFailureIsRecoverableWithReset) {
  BoltProxyTestServer server;
  ASSERT_TRUE(server.Start());

  RawBoltClient client(server.bolt_port());
  client.SendHello();
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Success);

  client.SendRoute();
  auto route_failure = client.ReadResponse();
  EXPECT_EQ(route_failure.tag, bolt::BoltMsg::Failure);
  EXPECT_EQ(route_failure.failure_code, "Neo.ClientError.Request.Invalid");
  EXPECT_NE(route_failure.failure_message.find("routing"), std::string::npos);

  client.SendRoute();
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Ignored);

  client.SendReset();
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Success);

  client.SendRoute();
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Failure);
}

TEST(ProxyBoltProtocol, ExplicitTransactionFailureIsRecoverableWithReset) {
  BoltProxyTestServer server;
  ASSERT_TRUE(server.Start());

  RawBoltClient client(server.bolt_port());
  client.SendHello();
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Success);

  client.SendBegin();
  auto begin_failure = client.ReadResponse();
  EXPECT_EQ(begin_failure.tag, bolt::BoltMsg::Failure);
  EXPECT_EQ(begin_failure.failure_code, "Neo.ClientError.Request.Invalid");
  EXPECT_NE(begin_failure.failure_message.find("transactions"),
            std::string::npos);

  client.SendReset();
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Success);

  client.SendBegin();
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Failure);
}

TEST(ProxyBoltProtocol, PullWithoutStreamClosesConnection) {
  BoltProxyTestServer server;
  ASSERT_TRUE(server.Start());

  RawBoltClient client(server.bolt_port());
  client.SendHello();
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Success);

  client.SendPull(-1);
  EXPECT_TRUE(client.WaitForClose(std::chrono::milliseconds(2000)));
}

TEST(ProxyBoltProtocol, RejectsRunWithUnexpectedFieldCount) {
  BoltProxyTestServer server;
  ASSERT_TRUE(server.Start());

  RawBoltClient client(server.bolt_port());
  client.SendHello();
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Success);

  client.SendRunWithExtraField();
  auto run_failure = client.ReadResponse();
  EXPECT_EQ(run_failure.tag, bolt::BoltMsg::Failure);
  EXPECT_EQ(run_failure.failure_code,
            "Neo.ClientError.Statement.ArgumentError");
  EXPECT_NE(run_failure.failure_message.find("fields size"), std::string::npos);
}

TEST(ProxyBoltProtocol, ResetInterruptsActiveBackendStream) {
  BoltProxyBackendTestServer server;
  ASSERT_TRUE(server.Start());

  RawBoltClient client(server.proxy_bolt_port());
  client.SendHello();
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Success);

  client.SendRun("UNWIND range(0, 10000) AS n RETURN n",
                 {{"_shard_key_", std::string("user-1")}}, {{"db", "default"}});
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Success);

  client.SendReset();
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Success);

  client.SendRun(
      "RETURN $value AS n",
      {{"_shard_key_", std::string("user-1")}, {"value", int64_t{7}}},
      {{"db", "default"}});
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Success);
  client.SendPull(-1);
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Record);
  EXPECT_EQ(client.ReadResponse().tag, bolt::BoltMsg::Success);
}

TEST(BoltNeo4jDriver, HandlesBolt4PlaceholdersAndRecovery) {
  if (RunShellCommand("python3 -c 'import neo4j'") != 0) {
    GTEST_SKIP() << "neo4j Python driver is not installed";
  }

  BoltNeo4jDriverServer server;
  ASSERT_TRUE(server.Start());

  std::string command = "python3 ../test/scripts/test_bolt_neo4j_driver.py " +
                        std::to_string(server.bolt_port());
  EXPECT_EQ(RunShellCommand(command), 0);
}
