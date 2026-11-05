#include "mdlab/marketdata/MarketDataParser.hpp"

#include <gtest/gtest.h>

namespace mdlab::marketdata {

TEST(MarketDataParserTest, ParsesValidQuoteLine) {
    const auto result = MarketDataParser::parseLine("1000,AAPL,BUY,189.50,100,QUOTE,1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->timestampNanos, 1000);
    EXPECT_EQ(result->symbol, "AAPL");
    EXPECT_EQ(result->side, Side::Buy);
    EXPECT_DOUBLE_EQ(result->price, 189.50);
    EXPECT_EQ(result->quantity, 100);
    EXPECT_EQ(result->type, MsgType::Quote);
    EXPECT_EQ(result->orderId, 1u);
}

TEST(MarketDataParserTest, ParsesSellSide) {
    const auto result = MarketDataParser::parseLine("2000,MSFT,SELL,410.00,50,TRADE,2");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->side, Side::Sell);
    EXPECT_EQ(result->type, MsgType::Trade);
}

TEST(MarketDataParserTest, RejectsEmptyLine) {
    const auto result = MarketDataParser::parseLine("");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ParseError::EmptyLine);
}

TEST(MarketDataParserTest, RejectsLineWithTooFewFields) {
    const auto result = MarketDataParser::parseLine("1000,AAPL,BUY,189.50");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ParseError::MissingField);
}

TEST(MarketDataParserTest, RejectsInvalidNumber) {
    const auto result = MarketDataParser::parseLine("1000,AAPL,BUY,not_a_price,100,QUOTE,1");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ParseError::InvalidNumber);
}

TEST(MarketDataParserTest, RejectsInvalidSide) {
    const auto result = MarketDataParser::parseLine("1000,AAPL,HOLD,189.50,100,QUOTE,1");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ParseError::InvalidSide);
}

TEST(MarketDataParserTest, RejectsInvalidMsgType) {
    const auto result = MarketDataParser::parseLine("1000,AAPL,BUY,189.50,100,SNAPSHOT,1");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ParseError::InvalidMsgType);
}

TEST(MarketDataParserTest, HandlesTrailingWhitespace) {
    const auto result = MarketDataParser::parseLine("1000,AAPL,BUY,189.50,100,QUOTE,1  \r");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->orderId, 1u);
}

}  // namespace mdlab::marketdata
