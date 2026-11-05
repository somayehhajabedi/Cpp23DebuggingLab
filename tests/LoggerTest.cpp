#include "mdlab/logger/Logger.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

namespace mdlab::logger {

TEST(LoggerTest, LogsFromMultipleThreadsWithoutCrashing) {
    Logger::instance().start();

    constexpr int kThreads = 8;
    constexpr int kMessagesPerThread = 200;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t] {
            for (int i = 0; i < kMessagesPerThread; ++i) {
                MDLAB_LOG_INFO("thread {} message {}", t, i);
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    // Give the worker a moment to drain before the test ends.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    SUCCEED();
}

}  // namespace mdlab::logger
