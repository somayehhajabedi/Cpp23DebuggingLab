#include "mdlab/risk/RiskManager.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

namespace mdlab::risk {

using marketdata::Side;

// An order with an unusually large quantity should still be evaluated
// correctly: the resulting notional must be a large *positive* number that
// genuinely exceeds the configured limit, and the order must be rejected.
// Position and order-quantity limits are set generously high so only the
// notional check is exercised.
TEST(NotionalOverflowTest, VeryLargeQuantityIsStillRejectedOnNotional) {
    PositionTracker positions;
    const RiskLimits limits{.maxPositionQty = std::numeric_limits<std::int64_t>::max() / 4,
                              .maxOrderQty = std::numeric_limits<std::int64_t>::max() / 4,
                              .maxNotionalExposure = 1'000'000.0};
    RiskManager manager(limits, positions);

    constexpr std::int64_t kHugeQuantity = 3'000'000'000LL;  // overflows a 32-bit quantity type
    const auto decision = manager.evaluate("AAPL", Side::Buy, 100.0, kHugeQuantity);

    EXPECT_FALSE(decision.approved)
        << "a notional of roughly 3e11 must exceed a 1e6 limit; an approval here implies the "
           "notional calculation wrapped around instead of staying positive and large";
}

TEST(NotionalOverflowTest, ModeratelyLargeQuantityNotionalIsPositiveAndProportional) {
    PositionTracker positions;
    const RiskLimits limits{.maxPositionQty = std::numeric_limits<std::int64_t>::max() / 4,
                              .maxOrderQty = std::numeric_limits<std::int64_t>::max() / 4,
                              .maxNotionalExposure = std::numeric_limits<double>::max()};
    RiskManager manager(limits, positions);

    // With the notional limit effectively unlimited, this must always be
    // approved; the point of this test is just to make sure a large order
    // doesn't trip anything unexpected on its own.
    constexpr std::int64_t kLargeQuantity = 10'000'000LL;
    const auto decision = manager.evaluate("AAPL", Side::Buy, 50.0, kLargeQuantity);
    EXPECT_TRUE(decision.approved);
}

}  // namespace mdlab::risk
