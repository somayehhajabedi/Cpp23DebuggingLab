#include "mdlab/risk/RiskManager.hpp"

#include <gtest/gtest.h>

namespace mdlab::risk {

using marketdata::Side;

// These tests pin down the risk manager's boundary convention precisely:
// a value exactly *at* a configured limit is allowed; a value that
// *exceeds* the limit by even one unit is rejected. Each test isolates a
// single dimension (order quantity, notional, position) by setting the
// other two limits generously high.

TEST(RiskLimitBoundaryTest, OrderQtyExactlyAtLimitIsApproved) {
    PositionTracker positions;
    const RiskLimits limits{.maxPositionQty = 1'000'000, .maxOrderQty = 500, .maxNotionalExposure = 1e9};
    RiskManager manager(limits, positions);

    const auto decision = manager.evaluate("AAPL", Side::Buy, 1.0, 500);
    EXPECT_TRUE(decision.approved);
}

TEST(RiskLimitBoundaryTest, OrderQtyOneOverLimitIsRejected) {
    PositionTracker positions;
    const RiskLimits limits{.maxPositionQty = 1'000'000, .maxOrderQty = 500, .maxNotionalExposure = 1e9};
    RiskManager manager(limits, positions);

    const auto decision = manager.evaluate("AAPL", Side::Buy, 1.0, 501);
    EXPECT_FALSE(decision.approved);
}

TEST(RiskLimitBoundaryTest, NotionalExactlyAtLimitIsApproved) {
    PositionTracker positions;
    const RiskLimits limits{
        .maxPositionQty = 1'000'000, .maxOrderQty = 5'000, .maxNotionalExposure = 100'000.0};
    RiskManager manager(limits, positions);

    // 1000 * 100.0 == 100,000.0 exactly.
    const auto decision = manager.evaluate("AAPL", Side::Buy, 100.0, 1000);
    EXPECT_TRUE(decision.approved);
}

TEST(RiskLimitBoundaryTest, NotionalOneUnitOverLimitIsRejected) {
    PositionTracker positions;
    const RiskLimits limits{
        .maxPositionQty = 1'000'000, .maxOrderQty = 5'000, .maxNotionalExposure = 100'000.0};
    RiskManager manager(limits, positions);

    // 1001 * 100.0 == 100,100.0, just over the limit.
    const auto decision = manager.evaluate("AAPL", Side::Buy, 100.0, 1001);
    EXPECT_FALSE(decision.approved);
}

TEST(RiskLimitBoundaryTest, ProspectivePositionExactlyAtLimitIsApproved) {
    PositionTracker positions;
    const RiskLimits limits{.maxPositionQty = 1000, .maxOrderQty = 5'000, .maxNotionalExposure = 1e9};
    RiskManager manager(limits, positions);

    const auto decision = manager.evaluate("AAPL", Side::Buy, 1.0, 1000);
    EXPECT_TRUE(decision.approved);
}

TEST(RiskLimitBoundaryTest, ProspectivePositionOneOverLimitIsRejected) {
    PositionTracker positions;
    const RiskLimits limits{.maxPositionQty = 1000, .maxOrderQty = 5'000, .maxNotionalExposure = 1e9};
    RiskManager manager(limits, positions);

    const auto decision = manager.evaluate("AAPL", Side::Buy, 1.0, 1001);
    EXPECT_FALSE(decision.approved);
}

TEST(RiskLimitBoundaryTest, ExistingPositionPlusOrderExactlyAtLimitIsApproved) {
    PositionTracker positions;
    positions.applyFill("AAPL", Side::Buy, 1.0, 400);
    const RiskLimits limits{.maxPositionQty = 1000, .maxOrderQty = 5'000, .maxNotionalExposure = 1e9};
    RiskManager manager(limits, positions);

    const auto decision = manager.evaluate("AAPL", Side::Buy, 1.0, 600);
    EXPECT_TRUE(decision.approved);
}

}  // namespace mdlab::risk
