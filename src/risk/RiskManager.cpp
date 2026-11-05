#include "mdlab/risk/RiskManager.hpp"

#include <cmath>
#include <format>

namespace mdlab::risk {

using marketdata::Side;

RiskManager::RiskManager(RiskLimits limits, PositionTracker& positions)
    : limits_(limits), positions_(positions) {}

RiskDecision RiskManager::evaluate(const std::string& symbol, Side side, double price,
                                    std::int64_t quantity) const {
    if (quantity > limits_.maxOrderQty) {
        return RiskDecision{.approved = false, .reason = "order quantity exceeds limit"};
    }

    const double notional = static_cast<double>(quantity) * price;
    if (notional > limits_.maxNotionalExposure) {
        return RiskDecision{.approved = false, .reason = "order notional exceeds limit"};
    }

    const PositionTracker::Snapshot current = positions_.snapshot(symbol);
    const std::int64_t signedQty = (side == Side::Buy) ? quantity : -quantity;
    const std::int64_t prospective = current.quantity + signedQty;

    if (std::abs(prospective) >= limits_.maxPositionQty) {
        return RiskDecision{.approved = false, .reason = "prospective position exceeds limit"};
    }

    return RiskDecision{.approved = true, .reason = "ok"};
}

}  // namespace mdlab::risk
