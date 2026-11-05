#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

#include "mdlab/core/OrderedMap.hpp"
#include "mdlab/marketdata/MarketDataTypes.hpp"

namespace mdlab::orderbook {

// Single-symbol limit order book. Not thread-safe by design: exactly one
// thread (the book-building thread in the pipeline) ever touches a given
// OrderBook instance.
class OrderBook {
public:
    explicit OrderBook(std::string symbol);

    void addOrder(std::uint64_t orderId, marketdata::Side side, double price,
                  std::int64_t quantity);
    void cancelOrder(std::uint64_t orderId);
    void replaceOrder(std::uint64_t orderId, double newPrice, std::int64_t newQuantity);

    [[nodiscard]] std::optional<double> bestBid() const noexcept;
    [[nodiscard]] std::optional<double> bestAsk() const noexcept;
    [[nodiscard]] bool isCrossed() const noexcept;
    [[nodiscard]] const std::string& symbol() const noexcept { return symbol_; }
    [[nodiscard]] std::size_t openOrderCount() const noexcept { return orders_.size(); }

private:
    struct RestingOrder {
        marketdata::Side side;
        double price;
        std::int64_t quantity;
    };

    void addToLevel(marketdata::Side side, double price, std::int64_t quantity);
    void removeFromLevel(marketdata::Side side, double price, std::int64_t quantity);

    std::string symbol_;
    std::unordered_map<std::uint64_t, RestingOrder> orders_;
    core::OrderedMap<double, std::int64_t, std::greater<double>> bids_;
    core::OrderedMap<double, std::int64_t, std::less<double>> asks_;
};

}  // namespace mdlab::orderbook
