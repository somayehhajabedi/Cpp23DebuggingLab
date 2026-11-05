#include "mdlab/risk/RiskManager.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cmath>
#include <thread>
#include <vector>

namespace mdlab::risk {

using marketdata::Side;

// Several threads fill the same symbol concurrently while a reader thread
// continuously pulls snapshots (and runs them through the risk manager)
// in the background. Every fill in this test uses the same fixed price, so
// quantity and notional must stay exactly proportional to each other in
// *every* snapshot a reader can observe — any snapshot where they drift
// apart means a reader saw a partially-updated (torn) position, which is
// exactly the kind of inconsistency that would make a concurrent risk
// decision wrong.
TEST(RiskManagerConcurrencyTest, SnapshotsStayConsistentUnderConcurrentFills) {
    constexpr double kPrice = 50.0;
    constexpr std::int64_t kQtyPerFill = 10;
    constexpr int kWriterThreads = 8;
    constexpr int kFillsPerThread = 2000;

    PositionTracker positions;
    const RiskLimits limits{
        .maxPositionQty = 10'000'000, .maxOrderQty = 10'000'000, .maxNotionalExposure = 1e12};
    RiskManager manager(limits, positions);

    std::atomic<bool> stopReader{false};
    std::atomic<bool> inconsistencyFound{false};

    std::thread reader([&] {
        while (!stopReader.load(std::memory_order_relaxed)) {
            const auto snap = positions.snapshot("AAPL");
            const double expectedNotional = static_cast<double>(snap.quantity) * kPrice;
            if (std::abs(snap.notional - expectedNotional) > 1e-6) {
                inconsistencyFound.store(true, std::memory_order_relaxed);
            }
            // Exercise the risk-decision path too, under concurrent writers.
            (void)manager.evaluate("AAPL", Side::Buy, kPrice, kQtyPerFill);
        }
    });

    std::vector<std::thread> writers;
    writers.reserve(kWriterThreads);
    for (int w = 0; w < kWriterThreads; ++w) {
        writers.emplace_back([&] {
            for (int i = 0; i < kFillsPerThread; ++i) {
                positions.applyFill("AAPL", Side::Buy, kPrice, kQtyPerFill);
            }
        });
    }
    for (auto& t : writers) {
        t.join();
    }
    stopReader.store(true, std::memory_order_relaxed);
    reader.join();

    EXPECT_FALSE(inconsistencyFound) << "a reader observed a torn (inconsistent) position snapshot";

    const auto finalSnap = positions.snapshot("AAPL");
    const std::int64_t expectedQty =
        static_cast<std::int64_t>(kWriterThreads) * kFillsPerThread * kQtyPerFill;
    EXPECT_EQ(finalSnap.quantity, expectedQty);
    EXPECT_NEAR(finalSnap.notional, static_cast<double>(expectedQty) * kPrice, 1e-3);
}

}  // namespace mdlab::risk
