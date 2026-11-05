#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include "mdlab/marketdata/MarketDataTypes.hpp"

namespace mdlab::risk {

// Tracks net position and notional exposure per symbol. Written from the
// execution thread on every fill and read from the risk-check path (and
// potentially a monitoring thread). Quantity and notional are each their
// own atomic counter, updated independently, so neither update needs to
// wait on a lock held by the other.
class PositionTracker {
public:
    struct Snapshot {
        std::int64_t quantity = 0;
        double notional = 0.0;
    };

    void applyFill(const std::string& symbol, marketdata::Side side, double price,
                   std::int64_t quantity);

    [[nodiscard]] Snapshot snapshot(const std::string& symbol) const;

    // Invokes fn(symbol, snapshot) for every tracked symbol. Callbacks
    // should stay read-only and must not call back into PositionTracker.
    template <typename Fn>
    void forEachPosition(Fn&& fn) const {
        std::lock_guard<std::mutex> lock(mapMutex_);
        for (const auto& [symbol, pos] : positions_) {
            fn(symbol, Snapshot{.quantity = pos.quantity.load(std::memory_order_relaxed),
                                  .notional = pos.notional.load(std::memory_order_relaxed)});
        }
    }

private:
    struct AtomicPosition {
        std::atomic<std::int64_t> quantity{0};
        std::atomic<double> notional{0.0};
    };

    mutable std::mutex mapMutex_;
    std::unordered_map<std::string, AtomicPosition> positions_;
};

}  // namespace mdlab::risk
