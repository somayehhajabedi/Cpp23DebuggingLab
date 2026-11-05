#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include "mdlab/core/Config.hpp"
#include "mdlab/core/StatsCollector.hpp"
#include "mdlab/execution/OrderGateway.hpp"
#include "mdlab/pipeline/PipelineTypes.hpp"
#include "mdlab/risk/PositionTracker.hpp"
#include "mdlab/risk/RiskManager.hpp"

namespace mdlab::execution {

struct OrderRecord {
    std::uint64_t orderId = 0;
    std::string symbol;
    marketdata::Side side = marketdata::Side::Buy;
    double price = 0.0;
    std::int64_t quantity = 0;
};

// Consumes book signals, applies a minimal liquidity-taking rule, checks
// risk, and routes approved orders to the gateway. onBookSignal runs only
// on the execution-stage thread; onFill runs on the gateway's thread —
// openOrders_ is the only state shared between those two call paths and is
// always accessed under ordersMutex_.
class ExecutionEngine {
public:
    ExecutionEngine(risk::RiskManager& riskManager, risk::PositionTracker& positions,
                     OrderGateway& gateway, core::StatsCollector& stats, core::Config config);

    void onBookSignal(const pipeline::BookSignal& signal);
    void onFill(std::uint64_t orderId, double fillPrice, std::int64_t fillQuantity);

    [[nodiscard]] std::size_t openOrderCount() const;
    [[nodiscard]] double totalExposure(const std::string& symbol) const;

private:
    [[nodiscard]] std::uint64_t nextOrderId() noexcept;

    risk::RiskManager& riskManager_;
    risk::PositionTracker& positions_;
    OrderGateway& gateway_;
    core::StatsCollector& stats_;
    core::Config config_;

    std::atomic<std::uint64_t> orderIdCounter_{1};

    mutable std::mutex ordersMutex_;
    std::unordered_map<std::uint64_t, OrderRecord> openOrders_;

    std::unordered_map<std::string, std::int64_t> qualifyingSignalCount_;
};

}  // namespace mdlab::execution
