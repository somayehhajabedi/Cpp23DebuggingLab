#include "mdlab/risk/RiskManager.hpp"

#include <gtest/gtest.h>

namespace mdlab::risk {

using marketdata::Side;

namespace {
RiskLimits defaultLimits() {
    return RiskLimits{.maxPositionQty = 1000, .maxOrderQty = 500, .maxNotionalExposure = 100'000.0};
}
}  // namespace

TEST(RiskManagerTest, ApprovesOrderWithinAllLimits) {
    PositionTracker positions;
    RiskManager manager(defaultLimits(), positions);
    const auto decision = manager.evaluate("AAPL", Side::Buy, 100.0, 50);
    EXPECT_TRUE(decision.approved);
}

TEST(RiskManagerTest, RejectsOrderExceedingOrderQtyLimit) {
    PositionTracker positions;
    RiskManager manager(defaultLimits(), positions);
    const auto decision = manager.evaluate("AAPL", Side::Buy, 100.0, 501);
    EXPECT_FALSE(decision.approved);
}

TEST(RiskManagerTest, RejectsOrderExceedingNotionalLimit) {
    PositionTracker positions;
    RiskManager manager(defaultLimits(), positions);
    // 400 * 300 = 120,000 > 100,000 limit, while staying under the order
    // quantity limit on its own.
    const auto decision = manager.evaluate("AAPL", Side::Buy, 300.0, 400);
    EXPECT_FALSE(decision.approved);
}

TEST(RiskManagerTest, RejectsOrderThatWouldBreachPositionLimit) {
    PositionTracker positions;
    positions.applyFill("AAPL", Side::Buy, 100.0, 900);
    RiskManager manager(defaultLimits(), positions);

    const auto decision = manager.evaluate("AAPL", Side::Buy, 100.0, 200);
    EXPECT_FALSE(decision.approved);
}

TEST(RiskManagerTest, ApprovesSellThatReducesExistingLongPosition) {
    PositionTracker positions;
    positions.applyFill("AAPL", Side::Buy, 100.0, 900);
    RiskManager manager(defaultLimits(), positions);

    const auto decision = manager.evaluate("AAPL", Side::Sell, 100.0, 200);
    EXPECT_TRUE(decision.approved);
}

TEST(RiskManagerTest, TracksPositionLimitForShortPositionsToo) {
    PositionTracker positions;
    positions.applyFill("AAPL", Side::Sell, 100.0, 900);
    RiskManager manager(defaultLimits(), positions);

    const auto decision = manager.evaluate("AAPL", Side::Sell, 100.0, 200);
    EXPECT_FALSE(decision.approved);
}

}  // namespace mdlab::risk
