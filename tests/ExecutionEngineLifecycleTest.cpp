#include "mdlab/execution/ExecutionEngine.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <format>
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
        : riskManager(risk::RiskLimits{.maxPositionQty = 1'000'000,
                                         .maxOrderQty = 10'000,
                                         .maxNotionalExposure = 1e9},
                       positions),
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

// Drives a sustained stream of orders and asynchronous fills across several
// symbols, so order records are created on the signal-handling path and
// erased on the gateway's fill-callback path repeatedly and continuously.
// Every surviving order record must be gone by the time fills settle, and
// nothing in that create/fill/erase cycle should corrupt state used by
// later ticks for the same symbol.
TEST(ExecutionEngineLifecycleTest, SustainedOrderFillCyclesLeaveNoStaleState) {
    Harness h;
    constexpr int kSymbolCount = 5;
    constexpr int kSignalsPerSymbol = 300;

    for (int round = 0; round < kSignalsPerSymbol; ++round) {
        for (int s = 0; s < kSymbolCount; ++s) {
            const std::string symbol = std::format("SYM{}", s);
            const double bid = 100.0 + (round % 7) * 0.01;
            const pipeline::BookSignal signal{
                .symbol = symbol, .bestBid = bid, .bestAsk = bid + 0.01, .timestampNanos = round};
            h.engine.onBookSignal(signal);
        }
    }

    // Drain: wait until every submitted order has produced a fill.
    for (int i = 0; i < 2000 && h.stats.fills() < h.stats.ordersSubmitted(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    EXPECT_EQ(h.engine.openOrderCount(), 0u);
    EXPECT_EQ(h.stats.fills(), h.stats.ordersSubmitted());
    EXPECT_GT(h.stats.ordersSubmitted(), 0u);

    for (int s = 0; s < kSymbolCount; ++s) {
        const std::string symbol = std::format("SYM{}", s);
        EXPECT_GE(h.positions.snapshot(symbol).quantity, 0);
    }
}

}  // namespace mdlab::execution
