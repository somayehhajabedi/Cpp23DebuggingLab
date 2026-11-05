#include "mdlab/orderbook/OrderBook.hpp"

#include <gtest/gtest.h>

namespace mdlab::orderbook {

using marketdata::Side;

TEST(OrderBookTest, EmptyBookHasNoBestPrices) {
    OrderBook book("AAPL");
    EXPECT_FALSE(book.bestBid().has_value());
    EXPECT_FALSE(book.bestAsk().has_value());
    EXPECT_FALSE(book.isCrossed());
}

TEST(OrderBookTest, AddOrderSetsBestPrice) {
    OrderBook book("AAPL");
    book.addOrder(1, Side::Buy, 100.00, 10);
    book.addOrder(2, Side::Sell, 100.50, 10);

    ASSERT_TRUE(book.bestBid().has_value());
    ASSERT_TRUE(book.bestAsk().has_value());
    EXPECT_DOUBLE_EQ(*book.bestBid(), 100.00);
    EXPECT_DOUBLE_EQ(*book.bestAsk(), 100.50);
    EXPECT_FALSE(book.isCrossed());
}

TEST(OrderBookTest, BestBidIsHighestAndBestAskIsLowest) {
    OrderBook book("AAPL");
    book.addOrder(1, Side::Buy, 100.00, 10);
    book.addOrder(2, Side::Buy, 100.10, 10);
    book.addOrder(3, Side::Sell, 101.00, 10);
    book.addOrder(4, Side::Sell, 100.90, 10);

    EXPECT_DOUBLE_EQ(*book.bestBid(), 100.10);
    EXPECT_DOUBLE_EQ(*book.bestAsk(), 100.90);
}

TEST(OrderBookTest, CancelRemovesOrderAndEmptyLevel) {
    OrderBook book("AAPL");
    book.addOrder(1, Side::Buy, 100.00, 10);
    book.cancelOrder(1);
    EXPECT_FALSE(book.bestBid().has_value());
    EXPECT_EQ(book.openOrderCount(), 0u);
}

TEST(OrderBookTest, CancelOnlyRemovesTargetedQuantity) {
    OrderBook book("AAPL");
    book.addOrder(1, Side::Buy, 100.00, 10);
    book.addOrder(2, Side::Buy, 100.00, 5);
    book.cancelOrder(1);

    ASSERT_TRUE(book.bestBid().has_value());
    EXPECT_DOUBLE_EQ(*book.bestBid(), 100.00);
}

TEST(OrderBookTest, CancelUnknownOrderIsNoop) {
    OrderBook book("AAPL");
    book.addOrder(1, Side::Buy, 100.00, 10);
    book.cancelOrder(999);
    EXPECT_TRUE(book.bestBid().has_value());
}

TEST(OrderBookTest, ReplaceMovesOrderToNewPrice) {
    OrderBook book("AAPL");
    book.addOrder(1, Side::Buy, 100.00, 10);
    book.replaceOrder(1, 100.25, 7);

    ASSERT_TRUE(book.bestBid().has_value());
    EXPECT_DOUBLE_EQ(*book.bestBid(), 100.25);
}

TEST(OrderBookTest, ReplaceClearsOldPriceLevelWhenEmptied) {
    OrderBook book("AAPL");
    book.addOrder(1, Side::Buy, 100.00, 10);
    book.addOrder(2, Side::Buy, 99.50, 10);
    book.replaceOrder(1, 99.00, 10);

    // Old level (100.00) should no longer be the best bid; 99.50 should be.
    ASSERT_TRUE(book.bestBid().has_value());
    EXPECT_DOUBLE_EQ(*book.bestBid(), 99.50);
}

TEST(OrderBookTest, ReplaceUnknownOrderIsNoop) {
    OrderBook book("AAPL");
    book.addOrder(1, Side::Buy, 100.00, 10);
    book.replaceOrder(999, 50.00, 1);
    EXPECT_DOUBLE_EQ(*book.bestBid(), 100.00);
}

}  // namespace mdlab::orderbook
