#pragma once

#include <cstdint>

namespace mdlab::orderbook {

struct PriceLevel {
    double price = 0.0;
    std::int64_t totalQuantity = 0;
};

}  // namespace mdlab::orderbook
