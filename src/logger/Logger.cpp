#include "mdlab/logger/Logger.hpp"

#include <chrono>
#include <print>
#include <vector>

#ifdef MDLAB_HAVE_STACKTRACE
#include <stacktrace>
#endif

namespace mdlab::logger {

namespace {
void printRecord(const LogRecord& record) {
    std::println("[{:12}] [{:<5}] [tid={}] {}", record.timestampNanos, toString(record.level),
                 std::hash<std::thread::id>{}(record.threadId), record.message);
}
}  // namespace

std::string_view toString(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Fatal:
            return "FATAL";
    }
    return "UNKNOWN";
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() { start(); }

Logger::~Logger() { stop(); }

void Logger::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
        return;
    }
    running_ = true;
    worker_ = std::jthread([this](std::stop_token token) { workerLoop(token); });
}

void Logger::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) {
            return;
        }
        running_ = false;
    }
    if (worker_.joinable()) {
        worker_.request_stop();
        cv_.notify_all();
        worker_.join();
    }
}

void Logger::log(LogLevel level, std::string message, std::source_location loc) {
    const auto nowNanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();

#ifdef MDLAB_HAVE_STACKTRACE
    if (level == LogLevel::Fatal) {
        message += std::format("\n{}", std::stacktrace::current());
    }
#endif

    LogRecord record{.level = level,
                      .timestampNanos = nowNanos,
                      .threadId = std::this_thread::get_id(),
                      .message = std::move(message),
                      .file = loc.file_name(),
                      .line = static_cast<int>(loc.line())};

    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() >= kMaxQueueSize) {
        ++dropped_;
        return;
    }
    queue_.push_back(std::move(record));
    cv_.notify_one();
}

std::size_t Logger::droppedCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return dropped_;
}

void Logger::workerLoop(std::stop_token token) {
    std::vector<LogRecord> batch;
    while (!token.stop_requested()) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, token, [this] { return !queue_.empty(); });
            while (!queue_.empty()) {
                batch.push_back(std::move(queue_.front()));
                queue_.pop_front();
            }
        }
        for (auto& record : batch) {
            printRecord(record);
        }
        batch.clear();
    }

    // Drain whatever is left so the final summary isn't missing records.
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        printRecord(queue_.front());
        queue_.pop_front();
    }
}

}  // namespace mdlab::logger
