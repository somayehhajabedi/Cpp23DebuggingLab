#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include "mdlab/marketdata/MarketDataTypes.hpp"

namespace mdlab::risk {

// Tracks net position and notional exposure per symbol. Written from the
// execution thread on every fill and read from the risk-check path (and
// potentially a monitoring thread), so every access goes through one mutex
// that guards quantity and notional together — callers always see a
// consistent pair, never a torn combination of the two.
class PositionTracker {
public:
    struct Snapshot {
        std::int64_t quantity = 0;
        double notional = 0.0;
    };

    void applyFill(const std::string& symbol, marketdata::Side side, double price,
                   std::int64_t quantity);

    [[nodiscard]] Snapshot snapshot(const std::string& symbol) const;

    // Invokes fn(symbol, snapshot) for every tracked symbol while holding
    // the internal lock, so a monitoring/reporting pass sees a consistent
    // set of snapshots. Callbacks should stay read-only and must not call
    // back into PositionTracker.
    template <typename Fn>
    void forEachPosition(Fn&& fn) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [symbol, snap] : positions_) {
            fn(symbol, snap);
        }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Snapshot> positions_;
};

}  // namespace mdlab::risk
