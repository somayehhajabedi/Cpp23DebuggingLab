#include "mdlab/util/SPSCQueue.hpp"

#include <gtest/gtest.h>

namespace mdlab::util {

TEST(SPSCQueueTest, EmptyQueuePopReturnsNullopt) {
    SPSCQueue<int> queue(4);
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.tryPop().has_value());
}

TEST(SPSCQueueTest, PushThenPopPreservesOrder) {
    SPSCQueue<int> queue(4);
    EXPECT_TRUE(queue.tryPush(1));
    EXPECT_TRUE(queue.tryPush(2));
    EXPECT_TRUE(queue.tryPush(3));

    EXPECT_EQ(queue.tryPop(), 1);
    EXPECT_EQ(queue.tryPop(), 2);
    EXPECT_EQ(queue.tryPop(), 3);
    EXPECT_FALSE(queue.tryPop().has_value());
}

TEST(SPSCQueueTest, FullQueueRejectsPush) {
    SPSCQueue<int> queue(2);
    EXPECT_TRUE(queue.tryPush(1));
    EXPECT_TRUE(queue.tryPush(2));
    EXPECT_FALSE(queue.tryPush(3));

    EXPECT_EQ(queue.tryPop(), 1);
    EXPECT_TRUE(queue.tryPush(3));
    EXPECT_EQ(queue.tryPop(), 2);
    EXPECT_EQ(queue.tryPop(), 3);
}

TEST(SPSCQueueTest, CapacityReflectsRequestedSize) {
    SPSCQueue<int> queue(16);
    EXPECT_EQ(queue.capacity(), 16u);
}

TEST(SPSCQueueTest, WrapsAroundCorrectly) {
    SPSCQueue<int> queue(3);
    for (int cycle = 0; cycle < 5; ++cycle) {
        EXPECT_TRUE(queue.tryPush(cycle));
        EXPECT_TRUE(queue.tryPush(cycle + 100));
        EXPECT_EQ(queue.tryPop(), cycle);
        EXPECT_EQ(queue.tryPop(), cycle + 100);
    }
}

}  // namespace mdlab::util
