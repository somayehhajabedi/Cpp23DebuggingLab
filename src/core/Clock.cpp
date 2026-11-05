#include "mdlab/core/Clock.hpp"

namespace mdlab::core {

Clock::Clock() : start_(std::chrono::steady_clock::now()) {}

Nanos Clock::nowNanos() const noexcept {
    const auto elapsed = std::chrono::steady_clock::now() - start_;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
}

}  // namespace mdlab::core
