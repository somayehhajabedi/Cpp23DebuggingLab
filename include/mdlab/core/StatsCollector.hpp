#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace mdlab::core {

// Lock-free counters shared across every pipeline thread. Each counter is
// its own atomic, incremented independently, so there is no shared
// invariant between counters that concurrent updates could tear.
class StatsCollector {
public:
    void recordTickParsed() noexcept { ticksParsed_.fetch_add(1, std::memory_order_relaxed); }
    void recordParseError() noexcept { parseErrors_.fetch_add(1, std::memory_order_relaxed); }
    void recordSignalEmitted() noexcept { signalsEmitted_.fetch_add(1, std::memory_order_relaxed); }
    void recordOrderSubmitted() noexcept { ordersSubmitted_.fetch_add(1, std::memory_order_relaxed); }
    void recordOrderRejected() noexcept { ordersRejected_.fetch_add(1, std::memory_order_relaxed); }
    void recordFill() noexcept { fills_.fetch_add(1, std::memory_order_relaxed); }

    [[nodiscard]] std::uint64_t ticksParsed() const noexcept {
        return ticksParsed_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::uint64_t parseErrors() const noexcept {
        return parseErrors_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::uint64_t signalsEmitted() const noexcept {
        return signalsEmitted_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::uint64_t ordersSubmitted() const noexcept {
        return ordersSubmitted_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::uint64_t ordersRejected() const noexcept {
        return ordersRejected_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::uint64_t fills() const noexcept {
        return fills_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<std::uint64_t> ticksParsed_{0};
    std::atomic<std::uint64_t> parseErrors_{0};
    std::atomic<std::uint64_t> signalsEmitted_{0};
    std::atomic<std::uint64_t> ordersSubmitted_{0};
    std::atomic<std::uint64_t> ordersRejected_{0};
    std::atomic<std::uint64_t> fills_{0};
};

}  // namespace mdlab::core
