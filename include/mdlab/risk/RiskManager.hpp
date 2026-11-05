#pragma once

#include <cstdint>
#include <string>

#include "mdlab/marketdata/MarketDataTypes.hpp"
#include "mdlab/risk/PositionTracker.hpp"
#include "mdlab/risk/RiskLimits.hpp"

namespace mdlab::risk {

struct RiskDecision {
    bool approved = false;
    std::string reason;
};

// Pre-trade risk check. Boundary convention: a limit is only breached when
// the relevant quantity *exceeds* it; a value exactly at the limit is
// allowed. This is applied consistently across all three checks below.
class RiskManager {
public:
    RiskManager(RiskLimits limits, PositionTracker& positions);

    [[nodiscard]] RiskDecision evaluate(const std::string& symbol, marketdata::Side side,
                                         double price, std::int64_t quantity) const;

private:
    RiskLimits limits_;
    PositionTracker& positions_;
};

}  // namespace mdlab::risk
