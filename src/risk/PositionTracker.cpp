#include "mdlab/risk/PositionTracker.hpp"

namespace mdlab::risk {

using marketdata::Side;

void PositionTracker::applyFill(const std::string& symbol, Side side, double price,
                                 std::int64_t quantity) {
    const std::int64_t signedQty = (side == Side::Buy) ? quantity : -quantity;

    std::lock_guard<std::mutex> lock(mutex_);
    Snapshot& snap = positions_[symbol];
    snap.quantity += signedQty;
    snap.notional += static_cast<double>(signedQty) * price;
}

PositionTracker::Snapshot PositionTracker::snapshot(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = positions_.find(symbol);
    if (it == positions_.end()) {
        return Snapshot{};
    }
    return it->second;
}

}  // namespace mdlab::risk
