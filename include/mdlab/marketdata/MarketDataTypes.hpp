#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace mdlab::marketdata {

enum class Side : std::uint8_t { Buy, Sell };
enum class MsgType : std::uint8_t { Quote, Trade, Cancel, Replace };

struct Tick {
    std::int64_t timestampNanos = 0;
    std::string symbol;
    Side side = Side::Buy;
    double price = 0.0;
    std::int64_t quantity = 0;
    MsgType type = MsgType::Quote;
    std::uint64_t orderId = 0;
};

enum class ParseError { EmptyLine, MissingField, InvalidNumber, InvalidSide, InvalidMsgType };

std::string_view toString(ParseError err) noexcept;
std::string_view toString(Side side) noexcept;
std::string_view toString(MsgType type) noexcept;

}  // namespace mdlab::marketdata
