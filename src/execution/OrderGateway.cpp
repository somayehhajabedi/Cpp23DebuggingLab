#include "mdlab/execution/OrderGateway.hpp"

namespace mdlab::execution {

OrderGateway::OrderGateway(FillCallback onFill) : onFill_(std::move(onFill)) {}

OrderGateway::~OrderGateway() { stop(); }

void OrderGateway::start() {
    worker_ = std::jthread([this](std::stop_token token) { workerLoop(token); });
}

void OrderGateway::stop() {
    if (worker_.joinable()) {
        worker_.request_stop();
        cv_.notify_all();
        worker_.join();
    }
}

void OrderGateway::submit(GatewayOrder order) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.push_back(std::move(order));
    cv_.notify_one();
}

void OrderGateway::workerLoop(std::stop_token token) {
    std::deque<GatewayOrder> batch;
    while (!token.stop_requested()) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, token, [this] { return !pending_.empty(); });
            batch = std::move(pending_);
            pending_.clear();
        }
        for (auto& order : batch) {
            if (onFill_) {
                onFill_(order.orderId, order.price, order.quantity);
            }
        }
        batch.clear();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& order : pending_) {
        if (onFill_) {
            onFill_(order.orderId, order.price, order.quantity);
        }
    }
    pending_.clear();
}

}  // namespace mdlab::execution
