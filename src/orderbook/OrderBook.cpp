#include "mdlab/orderbook/OrderBook.hpp"

namespace mdlab::orderbook {

using marketdata::Side;

OrderBook::OrderBook(std::string symbol) : symbol_(std::move(symbol)) {}

void OrderBook::addToLevel(Side side, double price, std::int64_t quantity) {
    if (side == Side::Buy) {
        bids_[price] += quantity;
    } else {
        asks_[price] += quantity;
    }
}

void OrderBook::removeFromLevel(Side side, double price, std::int64_t quantity) {
    if (side == Side::Buy) {
        auto it = bids_.find(price);
        if (it == bids_.end()) {
            return;
        }
	it->second -= quantity;

	if (it->second == 0) {
    	   bids_.erase(it);
	}
    } else {
        auto it = asks_.find(price);
        if (it == asks_.end()) {
            return;
        }

        it->second -= quantity;
	
	if (it->second == 0) {
    	   asks_.erase(it);
	}
    }
}

void OrderBook::addOrder(std::uint64_t orderId, Side side, double price, std::int64_t quantity) {
    orders_[orderId] = RestingOrder{.side = side, .price = price, .quantity = quantity};
    addToLevel(side, price, quantity);
}

void OrderBook::cancelOrder(std::uint64_t orderId) {
    const auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        return;
    }
    const RestingOrder order = it->second;
    orders_.erase(it);
    removeFromLevel(order.side, order.price, order.quantity);
}

void OrderBook::replaceOrder(std::uint64_t orderId, double newPrice, std::int64_t newQuantity) {
    const auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        return;
    }
    const RestingOrder oldOrder = it->second;

    removeFromLevel(oldOrder.side, oldOrder.price, oldOrder.quantity);
    it->second = RestingOrder{.side = oldOrder.side, .price = newPrice, .quantity = newQuantity};
    addToLevel(oldOrder.side, newPrice, newQuantity);
}

std::optional<double> OrderBook::bestBid() const noexcept {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<double> OrderBook::bestAsk() const noexcept {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

bool OrderBook::isCrossed() const noexcept {
    const auto bid = bestBid();
    const auto ask = bestAsk();
    if (!bid || !ask) {
        return false;
    }
    return *bid > *ask;
}

}  // namespace mdlab::orderbook
