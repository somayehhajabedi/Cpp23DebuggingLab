#include "mdlab/marketdata/MarketDataParser.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>

namespace mdlab::marketdata {

namespace {

constexpr std::size_t kExpectedFieldCount = 7;

[[nodiscard]] std::string_view trim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

[[nodiscard]] std::expected<Side, ParseError> parseSide(std::string_view token) {
    if (token == "BUY" || token == "Buy" || token == "buy") {
        return Side::Buy;
    }
    if (token == "SELL" || token == "Sell" || token == "sell") {
        return Side::Sell;
    }
    return std::unexpected(ParseError::InvalidSide);
}

[[nodiscard]] std::expected<MsgType, ParseError> parseMsgType(std::string_view token) {
    if (token == "QUOTE") return MsgType::Quote;
    if (token == "TRADE") return MsgType::Trade;
    if (token == "CANCEL") return MsgType::Cancel;
    if (token == "REPLACE") return MsgType::Replace;
    return std::unexpected(ParseError::InvalidMsgType);
}

template <typename T>
[[nodiscard]] std::expected<T, ParseError> parseNumber(std::string_view token) {
    T value{};
    const auto* begin = token.data();
    const auto* end = token.data() + token.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::unexpected(ParseError::InvalidNumber);
    }
    return value;
}

}  // namespace

std::vector<std::string_view> MarketDataParser::splitFields(std::string_view line) {
    std::vector<std::string_view> fields;
    fields.reserve(kExpectedFieldCount);

    std::size_t start = 0;
    while (start <= line.size()) {
        const std::size_t comma = line.find(',', start);
        if (comma == std::string_view::npos) {
            fields.push_back(trim(line.substr(start)));
            break;
        }
        fields.push_back(trim(line.substr(start, comma - start)));
        start = comma + 1;
    }
    return fields;
}

std::expected<Tick, ParseError> MarketDataParser::parseLine(std::string_view line) {
    line = trim(line);
    if (line.empty()) {
        return std::unexpected(ParseError::EmptyLine);
    }

    const std::vector<std::string_view> fields = splitFields(line);
    // Bounds check happens before any indexing below: a short or truncated
    // line is reported as a normal parse error instead of touching memory
    // past the vector.
    if (fields.size() < kExpectedFieldCount) {
        return std::unexpected(ParseError::MissingField);
    }

    const auto timestamp = parseNumber<std::int64_t>(fields[0]);
    if (!timestamp) {
        return std::unexpected(timestamp.error());
    }

    const std::string_view symbol = fields[1];
    if (symbol.empty()) {
        return std::unexpected(ParseError::MissingField);
    }

    const auto side = parseSide(fields[2]);
    if (!side) {
        return std::unexpected(side.error());
    }

    const auto price = parseNumber<double>(fields[3]);
    if (!price) {
        return std::unexpected(price.error());
    }

    const auto quantity = parseNumber<std::int64_t>(fields[4]);
    if (!quantity) {
        return std::unexpected(quantity.error());
    }

    const auto msgType = parseMsgType(fields[5]);
    if (!msgType) {
        return std::unexpected(msgType.error());
    }

    const auto orderId = parseNumber<std::uint64_t>(fields[6]);
    if (!orderId) {
        return std::unexpected(orderId.error());
    }

    return Tick{.timestampNanos = *timestamp,
                .symbol = std::string(symbol),
                .side = *side,
                .price = *price,
                .quantity = *quantity,
                .type = *msgType,
                .orderId = *orderId};
}

}  // namespace mdlab::marketdata
