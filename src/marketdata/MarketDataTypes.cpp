#include "mdlab/marketdata/MarketDataTypes.hpp"

namespace mdlab::marketdata {

std::string_view toString(ParseError err) noexcept {
    switch (err) {
        case ParseError::EmptyLine:
            return "EmptyLine";
        case ParseError::MissingField:
            return "MissingField";
        case ParseError::InvalidNumber:
            return "InvalidNumber";
        case ParseError::InvalidSide:
            return "InvalidSide";
        case ParseError::InvalidMsgType:
            return "InvalidMsgType";
    }
    return "Unknown";
}

std::string_view toString(Side side) noexcept {
    switch (side) {
        case Side::Buy:
            return "Buy";
        case Side::Sell:
            return "Sell";
    }
    return "Unknown";
}

std::string_view toString(MsgType type) noexcept {
    switch (type) {
        case MsgType::Quote:
            return "Quote";
        case MsgType::Trade:
            return "Trade";
        case MsgType::Cancel:
            return "Cancel";
        case MsgType::Replace:
            return "Replace";
    }
    return "Unknown";
}

}  // namespace mdlab::marketdata
