#include "mdlab/orderbook/OrderBook.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <random>
#include <unordered_map>
#include <vector>

namespace mdlab::orderbook {

namespace {

using marketdata::Side;

double chooseBuyPrice(std::optional<double> currentAsk, std::mt19937& rng) {
    std::uniform_real_distribution<double> offset(0.01, 0.50);
    double candidate = 99.00 - offset(rng);
    if (currentAsk) {
        candidate = std::min(candidate, *currentAsk - offset(rng));
    }
    return std::clamp(candidate, 50.0, 149.99);
}

double chooseSellPrice(std::optional<double> currentBid, std::mt19937& rng) {
    std::uniform_real_distribution<double> offset(0.01, 0.50);
    double candidate = 101.00 + offset(rng);
    if (currentBid) {
        candidate = std::max(candidate, *currentBid + offset(rng));
    }
    return std::clamp(candidate, 50.01, 150.0);
}

}  // namespace

// Drives a long, seeded (reproducible) sequence of add/cancel/replace
// operations where every new or replaced price is chosen to be safely on
// its own side of the book's *current* best opposite price at the moment
// it's placed. A correct book should therefore never report a crossed
// market afterwards, no matter how the add/cancel/replace calls interleave.
TEST(OrderBookInvariantTest, NeverReportsCrossedMarketUnderSustainedActivity) {
    OrderBook book("AAPL");
    std::mt19937 rng(20240101);
    std::uniform_int_distribution<int> actionDist(0, 99);

    struct TrackedOrder {
        Side side;
    };
    std::unordered_map<std::uint64_t, TrackedOrder> openOrders;
    std::vector<std::uint64_t> openOrderIds;
    std::uint64_t nextOrderId = 1;

    constexpr int kIterations = 3000;
    for (int i = 0; i < kIterations; ++i) {
        const int action = actionDist(rng);

        if (openOrderIds.empty() || action < 50) {
            // Add a new order.
            const Side side = (actionDist(rng) % 2 == 0) ? Side::Buy : Side::Sell;
            const double price = (side == Side::Buy) ? chooseBuyPrice(book.bestAsk(), rng)
                                                        : chooseSellPrice(book.bestBid(), rng);
            const std::int64_t quantity = 1 + (actionDist(rng) % 100);

            const std::uint64_t orderId = nextOrderId++;
            book.addOrder(orderId, side, price, quantity);
            openOrders[orderId] = TrackedOrder{.side = side};
            openOrderIds.push_back(orderId);
        } else if (action < 75) {
            // Cancel a random open order.
            const std::size_t idx = actionDist(rng) % openOrderIds.size();
            const std::uint64_t orderId = openOrderIds[idx];
            book.cancelOrder(orderId);
            openOrders.erase(orderId);
            openOrderIds.erase(openOrderIds.begin() + static_cast<std::ptrdiff_t>(idx));
        } else {
            // Replace a random open order, keeping its side but moving its
            // price/quantity.
            const std::size_t idx = actionDist(rng) % openOrderIds.size();
            const std::uint64_t orderId = openOrderIds[idx];
            const Side side = openOrders.at(orderId).side;
            const double newPrice = (side == Side::Buy) ? chooseBuyPrice(book.bestAsk(), rng)
                                                           : chooseSellPrice(book.bestBid(), rng);
            const std::int64_t newQuantity = 1 + (actionDist(rng) % 100);
            book.replaceOrder(orderId, newPrice, newQuantity);
        }

        ASSERT_FALSE(book.isCrossed())
            << "crossed market detected at iteration " << i << ": bid="
            << (book.bestBid() ? *book.bestBid() : -1.0)
            << " ask=" << (book.bestAsk() ? *book.bestAsk() : -1.0);
    }
}

}  // namespace mdlab::orderbook
