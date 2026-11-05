#include "mdlab/risk/PositionTracker.hpp"

namespace mdlab::risk {

using marketdata::Side;

void PositionTracker::applyFill(const std::string& symbol, Side side, double price,
                                 std::int64_t quantity) {
    const std::int64_t signedQty = (side == Side::Buy) ? quantity : -quantity;

    AtomicPosition* pos = nullptr;
    {
        std::lock_guard<std::mutex> lock(mapMutex_);
        pos = &positions_[symbol];
    }

    pos->quantity.fetch_add(signedQty, std::memory_order_relaxed);
    pos->notional.fetch_add(static_cast<double>(signedQty) * price, std::memory_order_relaxed);
}

PositionTracker::Snapshot PositionTracker::snapshot(const std::string& symbol) const {
    const AtomicPosition* pos = nullptr;
    {
        std::lock_guard<std::mutex> lock(mapMutex_);
        const auto it = positions_.find(symbol);
        if (it == positions_.end()) {
            return Snapshot{};
        }
        pos = &it->second;
    }
    return Snapshot{.quantity = pos->quantity.load(std::memory_order_relaxed),
                     .notional = pos->notional.load(std::memory_order_relaxed)};
}

}  // namespace mdlab::risk
