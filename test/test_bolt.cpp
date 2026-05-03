#include <gtest/gtest.h>
#include <sys/wait.h>

#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

#include "bolt/blocking_queue.h"
#include "server/lgraph_server.h"

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
    options.bolt_connection_options.handshake_timeout_seconds = 5;
    options.bolt_connection_options.login_timeout_seconds = 5;
    options.bolt_connection_options.idle_timeout_seconds = 30;
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
