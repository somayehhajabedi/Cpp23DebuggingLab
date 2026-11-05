#include <cstdint>
#include <format>
#include <print>
#include <string>

#include "mdlab/core/Config.hpp"
#include "mdlab/core/StatsCollector.hpp"
#include "mdlab/execution/ExecutionEngine.hpp"
#include "mdlab/execution/OrderGateway.hpp"
#include "mdlab/logger/Logger.hpp"
#include "mdlab/pipeline/MarketDataFeedHandler.hpp"
#include "mdlab/risk/PositionTracker.hpp"
#include "mdlab/risk/RiskLimits.hpp"
#include "mdlab/risk/RiskManager.hpp"

namespace {

void printSummary(const mdlab::core::StatsCollector& stats,
                    const mdlab::pipeline::MarketDataFeedHandler& handler,
                    const mdlab::risk::PositionTracker& positions) {
    std::println("");
    std::println("=== Simulation Summary ===");
    std::println("Ticks parsed:      {}", stats.ticksParsed());
    std::println("Parse errors:      {}", stats.parseErrors());
    std::println("Signals emitted:   {}", stats.signalsEmitted());
    std::println("Orders submitted:  {}", stats.ordersSubmitted());
    std::println("Orders rejected:   {}", stats.ordersRejected());
    std::println("Fills:             {}", stats.fills());

    std::println("");
    std::println("=== Final Order Books ===");
    for (const auto& [symbol, book] : handler.books()) {
        const std::string bidStr = book.bestBid() ? std::format("{:.2f}", *book.bestBid()) : "-";
        const std::string askStr = book.bestAsk() ? std::format("{:.2f}", *book.bestAsk()) : "-";
        std::println("{:<6} bid={:>10} ask={:>10} crossed={}", symbol, bidStr, askStr,
                      book.isCrossed() ? "YES" : "no");
    }

    std::println("");
    std::println("=== Final Positions ===");
    positions.forEachPosition([](const std::string& symbol,
                                   const mdlab::risk::PositionTracker::Snapshot& snap) {
        std::println("{:<6} qty={:>8} notional={:>14.2f}", symbol, snap.quantity, snap.notional);
    });
}

}  // namespace

int main(int argc, char** argv) {
    using namespace mdlab;

    core::Config config;
    if (argc > 1) {
        config.feedPath = argv[1];
    }

    core::StatsCollector stats;
    risk::PositionTracker positions;
    const risk::RiskLimits limits{.maxPositionQty = config.maxPositionQty,
                                    .maxOrderQty = config.maxOrderQty,
                                    .maxNotionalExposure = config.maxNotionalExposure};
    risk::RiskManager riskManager(limits, positions);

    execution::ExecutionEngine* enginePtr = nullptr;
    execution::OrderGateway gateway(
        [&enginePtr](std::uint64_t orderId, double price, std::int64_t qty) {
            enginePtr->onFill(orderId, price, qty);
        });
    execution::ExecutionEngine engine(riskManager, positions, gateway, stats, config);
    enginePtr = &engine;

    gateway.start();

    pipeline::MarketDataFeedHandler handler(config.feedPath, engine, stats, config);
    handler.run();

    gateway.stop();

    printSummary(stats, handler, positions);

    logger::Logger::instance().stop();
    return 0;
}
