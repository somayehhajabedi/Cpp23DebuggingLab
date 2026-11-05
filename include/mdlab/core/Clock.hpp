#pragma once

#include <chrono>
#include <cstdint>

namespace mdlab::core {

using Nanos = std::int64_t;

// Monotonic clock wrapper. All timestamps in this codebase are nanoseconds
// since the Clock instance was constructed, not wall-clock time, so that
// simulation runs stay reproducible across machines.
class Clock {
public:
    Clock();

    [[nodiscard]] Nanos nowNanos() const noexcept;

private:
    std::chrono::steady_clock::time_point start_;
};

}  // namespace mdlab::core
