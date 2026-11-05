#include "mdlab/pipeline/MarketDataFeedHandler.hpp"

#include <gtest/gtest.h>

#include <cstdint>

#include "mdlab/core/Config.hpp"
#include "mdlab/core/StatsCollector.hpp"
#include "mdlab/execution/ExecutionEngine.hpp"
#include "mdlab/execution/OrderGateway.hpp"
#include "mdlab/risk/PositionTracker.hpp"
#include "mdlab/risk/RiskManager.hpp"

namespace mdlab::pipeline {

namespace {

void runOnce() {
    core::Config config;
    config.feedPath = MDLAB_SAMPLE_FEED_PATH;

    core::StatsCollector stats;
    risk::PositionTracker positions;
    risk::RiskManager riskManager(risk::RiskLimits{}, positions);

    execution::ExecutionEngine* enginePtr = nullptr;
    execution::OrderGateway gateway([&enginePtr](std::uint64_t orderId, double price, std::int64_t qty) {
        enginePtr->onFill(orderId, price, qty);
    });
    execution::ExecutionEngine engine(riskManager, positions, gateway, stats, config);
    enginePtr = &engine;
    gateway.start();

    MarketDataFeedHandler handler(config.feedPath, engine, stats, config);
    handler.run();

    gateway.stop();
}

}  // namespace

// Relies on ctest's per-test TIMEOUT (set globally for this binary) to fail
// loudly if the pipeline ever stops making progress and hangs, rather than
// letting the test runner hang forever. Running several iterations back to
// back increases the chance of hitting any timing-dependent stall.
TEST(PipelineLivenessTest, PipelineAlwaysRunsToCompletion) {
    constexpr int kIterations = 20;
    for (int i = 0; i < kIterations; ++i) {
        runOnce();
    }
    SUCCEED();
}

}  // namespace mdlab::pipeline
