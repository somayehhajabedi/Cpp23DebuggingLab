#pragma once

#include <expected>
#include <string_view>
#include <vector>

#include "mdlab/marketdata/MarketDataTypes.hpp"

namespace mdlab::marketdata {

// Parses lines of the form:
//   timestamp_ns,symbol,side,price,quantity,msg_type,order_id
// e.g. 1000,AAPL,BUY,189.52,100,QUOTE,42
class MarketDataParser {
public:
    [[nodiscard]] static std::expected<Tick, ParseError> parseLine(std::string_view line);

private:
    [[nodiscard]] static std::vector<std::string_view> splitFields(std::string_view line);
};

}  // namespace mdlab::marketdata
