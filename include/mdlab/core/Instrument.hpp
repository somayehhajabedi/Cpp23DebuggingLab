#pragma once

#include <cstdint>
#include <string>

namespace mdlab::core {

struct Instrument {
    std::string symbol;
    double tickSize = 0.01;
    std::int64_t lotSize = 1;
};

}  // namespace mdlab::core
