#include "mdlab/core/Clock.hpp"

#include <gtest/gtest.h>

#include <thread>

namespace mdlab::core {

TEST(ClockTest, StartsNearZero) {
    Clock clock;
    EXPECT_GE(clock.nowNanos(), 0);
}

TEST(ClockTest, IsMonotonicallyNonDecreasing) {
    Clock clock;
    const Nanos first = clock.nowNanos();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const Nanos second = clock.nowNanos();
    EXPECT_GT(second, first);
}

}  // namespace mdlab::core
