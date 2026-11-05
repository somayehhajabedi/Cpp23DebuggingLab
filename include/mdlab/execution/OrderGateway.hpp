#pragma once

#include <cstdint>
#include <deque>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "mdlab/marketdata/MarketDataTypes.hpp"

namespace mdlab::execution {

struct GatewayOrder {
    std::uint64_t orderId = 0;
    std::string symbol;
    marketdata::Side side = marketdata::Side::Buy;
    double price = 0.0;
    std::int64_t quantity = 0;
};

using FillCallback = std::function<void(std::uint64_t orderId, double fillPrice,
                                         std::int64_t fillQuantity)>;

// Simulated exchange: accepts orders on the caller's thread, processes them
// on its own worker thread (so fills genuinely arrive asynchronously, as
// they would from a real venue), and always fully fills at the submitted
// price — deterministic by design, so runs are reproducible.
class OrderGateway {
public:
    explicit OrderGateway(FillCallback onFill);
    ~OrderGateway();

    void start();
    void stop();

    void submit(GatewayOrder order);

private:
    void workerLoop(std::stop_token token);

    FillCallback onFill_;

    std::mutex mutex_;
    std::condition_variable_any cv_;
    std::deque<GatewayOrder> pending_;
    std::jthread worker_;
};

}  // namespace mdlab::execution
