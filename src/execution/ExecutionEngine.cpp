#include "mdlab/execution/ExecutionEngine.hpp"

#include "mdlab/logger/Logger.hpp"

namespace mdlab::execution {

using marketdata::Side;

ExecutionEngine::ExecutionEngine(risk::RiskManager& riskManager, risk::PositionTracker& positions,
                                   OrderGateway& gateway, core::StatsCollector& stats,
                                   core::Config config)
    : riskManager_(riskManager),
      positions_(positions),
      gateway_(gateway),
      stats_(stats),
      config_(std::move(config)) {}

std::uint64_t ExecutionEngine::nextOrderId() noexcept {
    return orderIdCounter_.fetch_add(1, std::memory_order_relaxed);
}

void ExecutionEngine::onBookSignal(const pipeline::BookSignal& signal) {
    if (!signal.bestBid || !signal.bestAsk) {
        return;
    }

    const double spread = *signal.bestAsk - *signal.bestBid;
    if (spread < 0.0 || spread > config_.tightSpreadThreshold) {
        return;
    }

    // Throttle: only act on every third qualifying (tight-spread) signal
    // per symbol, so the sample feed produces a small, deterministic
    // number of orders instead of one per tick.
    std::int64_t& count = qualifyingSignalCount_[signal.symbol];
    ++count;
    if (count % 3 != 0) {
        return;
    }

    const Side side = Side::Buy;
    const double price = *signal.bestAsk;
    const std::int64_t quantity = config_.clipSize;

    const risk::RiskDecision decision = riskManager_.evaluate(signal.symbol, side, price, quantity);
    if (!decision.approved) {
        stats_.recordOrderRejected();
        MDLAB_LOG_INFO("order rejected: symbol={} reason={}", signal.symbol, decision.reason);
        return;
    }

    const std::uint64_t orderId = nextOrderId();
    OrderRecord record{
        .orderId = orderId, .symbol = signal.symbol, .side = side, .price = price, .quantity = quantity};

    {
        std::lock_guard<std::mutex> lock(ordersMutex_);
        openOrders_.emplace(orderId, record);
    }

    stats_.recordOrderSubmitted();
    gateway_.submit(GatewayOrder{
        .orderId = orderId, .symbol = record.symbol, .side = side, .price = price, .quantity = quantity});
}

void ExecutionEngine::onFill(std::uint64_t orderId, double fillPrice, std::int64_t fillQuantity) {
    OrderRecord record;
    {
        std::lock_guard<std::mutex> lock(ordersMutex_);
        const auto it = openOrders_.find(orderId);
        if (it == openOrders_.end()) {
            return;
        }
        record = it->second;
        openOrders_.erase(it);
    }

    positions_.applyFill(record.symbol, record.side, fillPrice, fillQuantity);
    stats_.recordFill();
    MDLAB_LOG_INFO("fill: order={} symbol={} qty={} price={:.2f}", record.orderId, record.symbol,
                    fillQuantity, fillPrice);
}

std::size_t ExecutionEngine::openOrderCount() const {
    std::lock_guard<std::mutex> lock(ordersMutex_);
    return openOrders_.size();
}

double ExecutionEngine::totalExposure(const std::string& symbol) const {
    std::int64_t openQty = 0;
    {
        std::lock_guard<std::mutex> lock(ordersMutex_);
        for (const auto& [id, record] : openOrders_) {
            if (record.symbol == symbol) {
                openQty += record.quantity;
            }
        }
    }
    const risk::PositionTracker::Snapshot snap = positions_.snapshot(symbol);
    return static_cast<double>(snap.quantity + openQty);
}

}  // namespace mdlab::execution
