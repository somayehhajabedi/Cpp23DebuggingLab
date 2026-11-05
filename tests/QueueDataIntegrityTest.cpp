#include "mdlab/util/SPSCQueue.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <thread>

namespace mdlab::util {

namespace {

// Each element carries a sequence number and a checksum derived from it.
// A consumer that ever sees sequence N must see checksum == N ^ kMagic;
// anything else means a slot got corrupted or the producer/consumer raced
// past each other.
constexpr std::uint64_t kMagic = 0x9E3779B97F4A7C15ULL;

struct Payload {
    std::uint64_t sequence;
    std::uint64_t checksum;
};

}  // namespace

TEST(QueueDataIntegrityTest, SustainedLoadPreservesOrderAndContent) {
    constexpr std::uint64_t kCount = 500'000;
    SPSCQueue<Payload> queue(1024);

    std::thread producer([&queue] {
        for (std::uint64_t seq = 0; seq < kCount; ++seq) {
            Payload payload{.sequence = seq, .checksum = seq ^ kMagic};
            while (!queue.tryPush(payload)) {
                std::this_thread::yield();
            }
        }
    });

    std::uint64_t expectedNext = 0;
    std::uint64_t received = 0;
    while (received < kCount) {
        auto item = queue.tryPop();
        if (!item) {
            std::this_thread::yield();
            continue;
        }
        ASSERT_EQ(item->sequence, expectedNext) << "received out-of-order or corrupted sequence";
        ASSERT_EQ(item->checksum, item->sequence ^ kMagic) << "payload checksum mismatch";
        ++expectedNext;
        ++received;
    }

    producer.join();
    EXPECT_EQ(received, kCount);
}

}  // namespace mdlab::util
