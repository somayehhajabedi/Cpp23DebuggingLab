#include "mdlab/pipeline/MarketDataFeedHandler.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

#include "mdlab/core/Config.hpp"
#include "mdlab/core/StatsCollector.hpp"
#include "mdlab/execution/ExecutionEngine.hpp"
#include "mdlab/execution/OrderGateway.hpp"
#include "mdlab/risk/PositionTracker.hpp"
#include "mdlab/risk/RiskManager.hpp"

namespace mdlab::pipeline {

namespace {

std::filesystem::path generateSyntheticFeed(int tickCount) {
    const auto path = std::filesystem::temp_directory_path() / "mdlab_throughput_feed.csv";
    std::ofstream out(path, std::ios::trunc);

    static constexpr const char* kSymbols[] = {"SYM0", "SYM1", "SYM2"};
    std::uint64_t orderId = 1;
    for (int i = 0; i < tickCount; ++i) {
        const char* symbol = kSymbols[i % 3];
        const char* side = (i % 2 == 0) ? "BUY" : "SELL";
        const double price = 100.00 + static_cast<double>(i % 50) * 0.01;
        out << (1000 + i) << ',' << symbol << ',' << side << ',' << price << ",10,QUOTE," << orderId++
            << '\n';
    }
    return path;
}

}  // namespace

// A generous wall-clock budget for the whole pipeline to chew through a
// sizeable synthetic feed. This is not a tight micro-benchmark — it's a
// smoke test meant to catch a gross regression (the feed used to finish in
// well under a second; a budget blown by multiple seconds is a real
// problem, not noise).
TEST(SimulationThroughputSmokeTest, ProcessesALargeFeedWithinBudget) {
    constexpr int kTickCount = 100'000;
    const auto feedPath = generateSyntheticFeed(kTickCount);

    core::Config config;
    config.feedPath = feedPath.string();

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

    const auto start = std::chrono::steady_clock::now();
    handler.run();
    const auto elapsed = std::chrono::steady_clock::now() - start;

    gateway.stop();

    EXPECT_EQ(stats.ticksParsed(), static_cast<std::uint64_t>(kTickCount));

    const auto elapsedSeconds = std::chrono::duration<double>(elapsed).count();
    EXPECT_LT(elapsedSeconds, 5.0) << "pipeline took " << elapsedSeconds
                                     << "s to process " << kTickCount << " ticks";

    std::filesystem::remove(feedPath);
}

}  // namespace mdlab::pipeline
