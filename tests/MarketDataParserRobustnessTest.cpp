#include "mdlab/marketdata/MarketDataParser.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <string>

namespace mdlab::marketdata {

// Feeds every line of the malformed-input fixture through the parser. A
// production feed handler must treat a bad line as data, not as a crash:
// every line here should come back as a well-formed std::expected, with no
// signal, no abort, no out-of-bounds access, regardless of how short or
// garbled the line is.
TEST(MarketDataParserRobustnessTest, NeverCrashesOnMalformedInput) {
    std::ifstream file(MDLAB_MALFORMED_FEED_PATH);
    ASSERT_TRUE(file.is_open()) << "fixture file not found: " << MDLAB_MALFORMED_FEED_PATH;

    std::string line;
    int linesSeen = 0;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        ++linesSeen;
        const auto result = MarketDataParser::parseLine(line);
        // We don't assert on has_value() here: some lines in the fixture
        // are intentionally malformed. The only hard requirement is that
        // reaching this point at all (i.e. not crashing) succeeded.
        (void)result;
    }
    EXPECT_GT(linesSeen, 0);
}

TEST(MarketDataParserRobustnessTest, TruncatedLineIsReportedAsAnError) {
    const auto result = MarketDataParser::parseLine("3000,AAPL,BUY,189.52");
    EXPECT_FALSE(result.has_value());
}

TEST(MarketDataParserRobustnessTest, GarbageLineIsReportedAsAnError) {
    const auto result = MarketDataParser::parseLine("not,a,valid,line,at,all,x");
    EXPECT_FALSE(result.has_value());
}

}  // namespace mdlab::marketdata
