#include "mdlab/risk/PositionTracker.hpp"

#include <gtest/gtest.h>

namespace mdlab::risk {

using marketdata::Side;

TEST(PositionTrackerTest, StartsFlat) {
    PositionTracker tracker;
    const auto snap = tracker.snapshot("AAPL");
    EXPECT_EQ(snap.quantity, 0);
    EXPECT_DOUBLE_EQ(snap.notional, 0.0);
}

TEST(PositionTrackerTest, BuyIncreasesPositionAndNotional) {
    PositionTracker tracker;
    tracker.applyFill("AAPL", Side::Buy, 100.0, 10);
    const auto snap = tracker.snapshot("AAPL");
    EXPECT_EQ(snap.quantity, 10);
    EXPECT_DOUBLE_EQ(snap.notional, 1000.0);
}

TEST(PositionTrackerTest, SellDecreasesPosition) {
    PositionTracker tracker;
    tracker.applyFill("AAPL", Side::Buy, 100.0, 10);
    tracker.applyFill("AAPL", Side::Sell, 105.0, 4);
    const auto snap = tracker.snapshot("AAPL");
    EXPECT_EQ(snap.quantity, 6);
    EXPECT_DOUBLE_EQ(snap.notional, 1000.0 - 420.0);
}

TEST(PositionTrackerTest, SymbolsAreIndependent) {
    PositionTracker tracker;
    tracker.applyFill("AAPL", Side::Buy, 100.0, 10);
    tracker.applyFill("MSFT", Side::Sell, 50.0, 3);

    EXPECT_EQ(tracker.snapshot("AAPL").quantity, 10);
    EXPECT_EQ(tracker.snapshot("MSFT").quantity, -3);
}

TEST(PositionTrackerTest, ForEachPositionVisitsEverySymbol) {
    PositionTracker tracker;
    tracker.applyFill("AAPL", Side::Buy, 100.0, 10);
    tracker.applyFill("MSFT", Side::Buy, 50.0, 3);

    int visited = 0;
    tracker.forEachPosition([&visited](const std::string&, const PositionTracker::Snapshot&) {
        ++visited;
    });
    EXPECT_EQ(visited, 2);
}

}  // namespace mdlab::risk
