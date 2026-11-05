#include "mdlab/pipeline/MarketDataFeedHandler.hpp"

#include <chrono>
#include <fstream>
#include <string>
#include <thread>

#include "mdlab/logger/Logger.hpp"
#include "mdlab/marketdata/MarketDataParser.hpp"

namespace mdlab::pipeline {

MarketDataFeedHandler::MarketDataFeedHandler(std::string feedPath,
                                               execution::ExecutionEngine& executionEngine,
                                               core::StatsCollector& stats, core::Config config)
    : feedPath_(std::move(feedPath)),
      executionEngine_(executionEngine),
      stats_(stats),
      config_(config),
      tickQueue_(config_.tickQueueCapacity),
      signalQueue_(config_.signalQueueCapacity) {}

void MarketDataFeedHandler::feedLoop() {
    std::ifstream file(feedPath_);
    if (!file.is_open()) {
        MDLAB_LOG_ERROR("failed to open feed file: {}", feedPath_);
        feedDone_.store(true, std::memory_order_release);
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        const auto parsed = marketdata::MarketDataParser::parseLine(line);
        if (!parsed) {
            stats_.recordParseError();
            MDLAB_LOG_WARN("parse error on line: {}", line);
            continue;
        }
        stats_.recordTickParsed();
        while (!tickQueue_.tryPush(*parsed)) {
            std::this_thread::yield();
        }
    }
    feedDone_.store(true, std::memory_order_release);
}

void MarketDataFeedHandler::bookLoop() {
    using marketdata::MsgType;

    while (true) {
        auto tickOpt = tickQueue_.tryPop();
        if (!tickOpt) {
            if (feedDone_.load(std::memory_order_acquire) && tickQueue_.empty()) {
                break;
            }
            std::this_thread::yield();
            continue;
        }

        // Safety pacing: give downstream consumers a moment to keep up
        // with the book thread before applying the next tick.
        const auto paceUntil = std::chrono::steady_clock::now() + std::chrono::microseconds(100);
        while (std::chrono::steady_clock::now() < paceUntil) {
        }

        const marketdata::Tick& tick = *tickOpt;
        auto it = books_.find(tick.symbol);
        if (it == books_.end()) {
            it = books_.emplace(tick.symbol, orderbook::OrderBook(tick.symbol)).first;
        }
        orderbook::OrderBook& book = it->second;

        switch (tick.type) {
            case MsgType::Quote:
                book.addOrder(tick.orderId, tick.side, tick.price, tick.quantity);
                break;
            case MsgType::Cancel:
                book.cancelOrder(tick.orderId);
                break;
            case MsgType::Replace:
                book.replaceOrder(tick.orderId, tick.price, tick.quantity);
                break;
            case MsgType::Trade:
                break;
        }

        BookSignal signal{.symbol = tick.symbol,
                           .bestBid = book.bestBid(),
                           .bestAsk = book.bestAsk(),
                           .timestampNanos = tick.timestampNanos};
        stats_.recordSignalEmitted();
        while (!signalQueue_.tryPush(signal)) {
            std::this_thread::yield();
        }
    }
    bookStageDone_.store(true, std::memory_order_release);
}

void MarketDataFeedHandler::executionLoop() {
    while (true) {
        auto signalOpt = signalQueue_.tryPop();
        if (!signalOpt) {
            if (bookStageDone_.load(std::memory_order_acquire) && signalQueue_.empty()) {
                break;
            }
            std::this_thread::yield();
            continue;
        }
        executionEngine_.onBookSignal(*signalOpt);
    }
}

void MarketDataFeedHandler::run() {
    std::jthread feedThread([this] { feedLoop(); });
    std::jthread bookThread([this] { bookLoop(); });
    std::jthread executionThread([this] { executionLoop(); });
    // jthreads join automatically when they go out of scope, so by the
    // time run() returns every stage has fully drained.
}

}  // namespace mdlab::pipeline
