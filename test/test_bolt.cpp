#include <gtest/gtest.h>

#include <chrono>

#include "bolt/blocking_queue.h"

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
