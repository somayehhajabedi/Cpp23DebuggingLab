#pragma once

#include <atomic>
#include <string>
#include <unordered_map>

#include "mdlab/core/Config.hpp"
#include "mdlab/core/StatsCollector.hpp"
#include "mdlab/execution/ExecutionEngine.hpp"
#include "mdlab/marketdata/MarketDataTypes.hpp"
#include "mdlab/orderbook/OrderBook.hpp"
#include "mdlab/pipeline/PipelineTypes.hpp"
#include "mdlab/util/SPSCQueue.hpp"

namespace mdlab::pipeline {

// Wires the three pipeline stages together on their own threads:
//   feed thread   -> tickQueue_   -> book thread -> signalQueue_ -> execution thread
// Each hop is a genuine single-producer/single-consumer handoff, so
// SPSCQueue is the right fit for both queues. run() blocks until the feed
// file is fully consumed and every downstream stage has drained.
class MarketDataFeedHandler {
public:
    MarketDataFeedHandler(std::string feedPath, execution::ExecutionEngine& executionEngine,
                           core::StatsCollector& stats, core::Config config);

    void run();

    [[nodiscard]] const std::unordered_map<std::string, orderbook::OrderBook>& books() const {
        return books_;
    }

private:
    void feedLoop();
    void bookLoop();
    void executionLoop();

    std::string feedPath_;
    execution::ExecutionEngine& executionEngine_;
    core::StatsCollector& stats_;
    core::Config config_;

    util::SPSCQueue<marketdata::Tick> tickQueue_;
    util::SPSCQueue<BookSignal> signalQueue_;

    std::atomic<bool> feedDone_{false};
    std::atomic<bool> bookStageDone_{false};

    std::unordered_map<std::string, orderbook::OrderBook> books_;
};

}  // namespace mdlab::pipeline
