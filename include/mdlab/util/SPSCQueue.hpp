#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <vector>

namespace mdlab::util {

template <typename T>
concept QueueElement = std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>;

// Bounded single-producer/single-consumer ring buffer. Exactly one thread
// may call tryPush, and exactly one (possibly different) thread may call
// tryPop; mixing that up is undefined behavior by design, same as any real
// SPSC queue. Capacity is rounded up internally by one slot so a full queue
// never aliases with an empty one.
template <QueueElement T>
class SPSCQueue {
public:
    explicit SPSCQueue(std::size_t capacity) : capacity_(capacity + 1), buffer_(capacity_) {}

    bool tryPush(T value) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t nextHead = increment(head);
        if (nextHead == tail_.load(std::memory_order_acquire)) {
            return false;  // full
        }
        buffer_[head] = std::move(value);
        head_.store(nextHead, std::memory_order_release);
        return true;
    }

    std::optional<T> tryPop() {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt;  // empty
        }
        T value = std::move(buffer_[tail]);
        tail_.store(increment(tail), std::memory_order_release);
        return value;
    }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_ - 1; }

private:
    [[nodiscard]] std::size_t increment(std::size_t idx) const noexcept {
        return (idx + 1) % capacity_;
    }

    std::size_t capacity_;
    std::vector<T> buffer_;
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};

}  // namespace mdlab::util
