#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <format>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <thread>

namespace mdlab::logger {

enum class LogLevel : std::uint8_t { Debug, Info, Warn, Error, Fatal };

std::string_view toString(LogLevel level) noexcept;

struct LogRecord {
    LogLevel level;
    std::int64_t timestampNanos;
    std::thread::id threadId;
    std::string message;
    std::string file;
    int line;
};

// Asynchronous, multi-producer structured logger. Every pipeline component
// may call into this from its own thread; a single dedicated worker thread
// drains the queue and writes to stdout so logging never blocks hot-path
// producers on I/O. The internal queue is bounded: once full, new records
// are dropped and counted rather than growing without limit.
class Logger {
public:
    static Logger& instance();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void start();
    void stop();

    void log(LogLevel level, std::string message,
              std::source_location loc = std::source_location::current());

    template <typename... Args>
    void logf(LogLevel level, std::source_location loc, std::format_string<Args...> fmt,
              Args&&... args) {
        log(level, std::format(fmt, std::forward<Args>(args)...), loc);
    }

    [[nodiscard]] std::size_t droppedCount() const noexcept;

private:
    Logger();
    ~Logger();

    void workerLoop(std::stop_token token);

    static constexpr std::size_t kMaxQueueSize = 8192;

    mutable std::mutex mutex_;
    std::condition_variable_any cv_;
    std::deque<LogRecord> queue_;
    std::size_t dropped_ = 0;
    std::jthread worker_;
    bool running_ = false;
};

}  // namespace mdlab::logger

#define MDLAB_LOG(level, ...) \
    ::mdlab::logger::Logger::instance().logf(level, std::source_location::current(), __VA_ARGS__)

#define MDLAB_LOG_DEBUG(...) MDLAB_LOG(::mdlab::logger::LogLevel::Debug, __VA_ARGS__)
#define MDLAB_LOG_INFO(...) MDLAB_LOG(::mdlab::logger::LogLevel::Info, __VA_ARGS__)
#define MDLAB_LOG_WARN(...) MDLAB_LOG(::mdlab::logger::LogLevel::Warn, __VA_ARGS__)
#define MDLAB_LOG_ERROR(...) MDLAB_LOG(::mdlab::logger::LogLevel::Error, __VA_ARGS__)
