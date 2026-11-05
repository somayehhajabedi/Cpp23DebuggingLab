#pragma once

#include <cstdint>

namespace mdlab::risk {

struct RiskLimits {
    std::int64_t maxPositionQty = 10'000;
    std::int64_t maxOrderQty = 2'000;
    double maxNotionalExposure = 5'000'000.0;
};

}  // namespace mdlab::risk
