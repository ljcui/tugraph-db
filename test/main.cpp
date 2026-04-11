#include <gtest/gtest.h>

#include "common/logger.h"
#include "test_util.h"

namespace {

class TestDataCleanupListener : public testing::EmptyTestEventListener {
 public:
  void OnTestStart(const testing::TestInfo &) override {
    testutil::CleanupTestDataDirectories();
  }

  void OnTestEnd(const testing::TestInfo &) override {
    testutil::CleanupTestDataDirectories();
  }
};

}  // namespace

int main(int argc, char **argv) {
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e %t %l %s:%#] %v");
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  testutil::CleanupTestDataDirectories();
  testing::UnitTest::GetInstance()->listeners().Append(
      new TestDataCleanupListener());
  int ret = RUN_ALL_TESTS();
  testutil::CleanupTestDataDirectories();
  return ret;
}
