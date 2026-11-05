#include "mdlab/execution/ExecutionEngine.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <thread>

#include "mdlab/core/Config.hpp"
#include "mdlab/core/StatsCollector.hpp"
#include "mdlab/execution/OrderGateway.hpp"
#include "mdlab/pipeline/PipelineTypes.hpp"
#include "mdlab/risk/PositionTracker.hpp"
#include "mdlab/risk/RiskManager.hpp"

namespace mdlab::execution {

namespace {

struct Harness {
    risk::PositionTracker positions;
    risk::RiskManager riskManager;
    core::StatsCollector stats;
    core::Config config;
    ExecutionEngine* enginePtr = nullptr;
    OrderGateway gateway;
    ExecutionEngine engine;

    Harness()
        : riskManager(risk::RiskLimits{}, positions),
          gateway([this](std::uint64_t orderId, double price, std::int64_t qty) {
              enginePtr->onFill(orderId, price, qty);
          }),
          engine(riskManager, positions, gateway, stats, config) {
        enginePtr = &engine;
        gateway.start();
    }

    ~Harness() { gateway.stop(); }
};

}  // namespace

TEST(ExecutionEngineTest, TightSpreadSignalsEventuallyProduceAFill) {
    Harness h;
    const pipeline::BookSignal signal{
        .symbol = "AAPL", .bestBid = 100.00, .bestAsk = 100.01, .timestampNanos = 0};

    // Throttle fires on every 3rd qualifying signal.
    h.engine.onBookSignal(signal);
    h.engine.onBookSignal(signal);
    h.engine.onBookSignal(signal);

    for (int i = 0; i < 200 && h.engine.openOrderCount() > 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    EXPECT_EQ(h.engine.openOrderCount(), 0u);
    EXPECT_EQ(h.stats.fills(), 1u);
    EXPECT_EQ(h.positions.snapshot("AAPL").quantity, 100);
}

TEST(ExecutionEngineTest, WideSpreadSignalNeverProducesAnOrder) {
    Harness h;
    const pipeline::BookSignal signal{
        .symbol = "AAPL", .bestBid = 100.00, .bestAsk = 105.00, .timestampNanos = 0};
    for (int i = 0; i < 10; ++i) {
        h.engine.onBookSignal(signal);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(h.stats.ordersSubmitted(), 0u);
}

TEST(ExecutionEngineTest, OneSidedSignalIsIgnored) {
    Harness h;
    const pipeline::BookSignal signal{
        .symbol = "AAPL", .bestBid = std::nullopt, .bestAsk = 100.01, .timestampNanos = 0};
    h.engine.onBookSignal(signal);
    EXPECT_EQ(h.stats.ordersSubmitted(), 0u);
}

}  // namespace mdlab::execution
