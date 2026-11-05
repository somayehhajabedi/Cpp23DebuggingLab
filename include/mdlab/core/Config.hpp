#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace mdlab::core {

// Simulation-wide tunables. A single value object so every component is
// configured consistently from one place instead of scattered constants.
struct Config {
    std::size_t tickQueueCapacity = 4096;
    std::size_t signalQueueCapacity = 4096;
    std::size_t orderQueueCapacity = 1024;

    std::int64_t maxPositionQty = 10'000;
    std::int64_t maxOrderQty = 2'000;
    double maxNotionalExposure = 5'000'000.0;

    double tightSpreadThreshold = 0.05;
    std::int64_t clipSize = 100;

    std::string feedPath = "data/sample_ticks.csv";
};

}  // namespace mdlab::core
