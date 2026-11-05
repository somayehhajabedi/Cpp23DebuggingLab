#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace mdlab::pipeline {

// Top-of-book snapshot emitted by the order-book stage after each tick it
// applies, consumed by the execution stage.
struct BookSignal {
    std::string symbol;
    std::optional<double> bestBid;
    std::optional<double> bestAsk;
    std::int64_t timestampNanos = 0;
};

}  // namespace mdlab::pipeline
