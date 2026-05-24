/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "assert.hpp"
#include "trade_engine.hpp"

namespace {

constexpr int32_t TestPrice = 10;

template<int TestSide>
constexpr Side RestingSide = (TestSide == 1) ? Sell : Buy;

template<int TestSide>
constexpr Side AggressiveSide = (TestSide == 1) ? Buy : Sell;

void expect_event(QueueOut& out, const Event& expected) {
  Assert(out.pop() == expected);
}

template<int TestSide>
void insert_resting_limit(TradeEngine& engine, int32_t orderId, int32_t price, int32_t qty) {
  engine.insert_order<RestingSide<TestSide>>(orderId, price, qty);
}

template<int TestSide>
void insert_aggressive_limit(TradeEngine& engine, int32_t orderId, int32_t price, int32_t qty) {
  engine.insert_order<AggressiveSide<TestSide>>(orderId, price, qty);
}

template<int TestSide>
void insert_aggressive_market(TradeEngine& engine, int32_t orderId, int32_t qty) {
  engine.insert_order<AggressiveSide<TestSide>>(orderId, qty);
}

template<int TestSide>
void update_resting(TradeEngine& engine, int32_t orderId, int32_t price, int32_t slot, int32_t qty) {
  engine.update_order<RestingSide<TestSide>>(orderId, price, slot, qty);
}

template<int TestSide>
void cancel_resting(TradeEngine& engine, int32_t orderId, int32_t price, int32_t slot) {
  engine.cancel_order<RestingSide<TestSide>>(orderId, price, slot);
}

template<int TestSide>
void test_insert_cancel_releases_book() {
  QueueOut out;
  TradeEngine engine(out);

  insert_resting_limit<TestSide>(engine, 1, TestPrice, 100);
  expect_event(out, CreateAccepted(1, 0));

  insert_resting_limit<TestSide>(engine, 2, TestPrice, 100);
  expect_event(out, CreateAccepted(2, 1));

  insert_resting_limit<TestSide>(engine, 3, TestPrice, 100);
  expect_event(out, CreateAccepted(3, 2));

  cancel_resting<TestSide>(engine, 2, TestPrice, 1);
  expect_event(out, CancelAccepted(2));

  cancel_resting<TestSide>(engine, 3, TestPrice, 2);
  expect_event(out, CancelAccepted(3));

  cancel_resting<TestSide>(engine, 1, TestPrice, 0);
  expect_event(out, CancelAccepted(1));

  insert_aggressive_limit<TestSide>(engine, 4, TestPrice, 100);
  expect_event(out, CreateAccepted(4, 0));
}

template<int TestSide>
void test_cancel_rejects_removed_order_and_book_stays_consistent() {
  QueueOut out;
  TradeEngine engine(out);

  insert_resting_limit<TestSide>(engine, 1, TestPrice, 100);
  expect_event(out, CreateAccepted(1, 0));

  insert_resting_limit<TestSide>(engine, 2, TestPrice, 100);
  expect_event(out, CreateAccepted(2, 1));

  insert_resting_limit<TestSide>(engine, 3, TestPrice, 100);
  expect_event(out, CreateAccepted(3, 2));

  cancel_resting<TestSide>(engine, 2, TestPrice, 1);
  expect_event(out, CancelAccepted(2));

  cancel_resting<TestSide>(engine, 2, TestPrice, 1);
  expect_event(out, CancelRejected(2));

  insert_aggressive_limit<TestSide>(engine, 4, TestPrice, 200);
  expect_event(out, Trade(TestPrice, 100, 4, 1));
  expect_event(out, Trade(TestPrice, 100, 4, 3));
}

template<int TestSide>
void test_update_changes_resting_qty_used_by_next_trade() {
  QueueOut out;
  TradeEngine engine(out);

  insert_resting_limit<TestSide>(engine, 1, TestPrice, 100);
  expect_event(out, CreateAccepted(1, 0));

  update_resting<TestSide>(engine, 1, TestPrice, 0, 120);
  expect_event(out, UpdateAccepted(1));

  insert_aggressive_limit<TestSide>(engine, 2, TestPrice, 120);
  expect_event(out, Trade(TestPrice, 120, 2, 1));
}

template<int TestSide>
void test_full_trade_preserves_fifo_on_single_level() {
  QueueOut out;
  TradeEngine engine(out);

  insert_resting_limit<TestSide>(engine, 1, TestPrice, 100);
  expect_event(out, CreateAccepted(1, 0));

  insert_resting_limit<TestSide>(engine, 2, TestPrice, 100);
  expect_event(out, CreateAccepted(2, 1));

  insert_resting_limit<TestSide>(engine, 3, TestPrice, 100);
  expect_event(out, CreateAccepted(3, 2));

  insert_aggressive_limit<TestSide>(engine, 4, TestPrice, 300);
  expect_event(out, Trade(TestPrice, 100, 4, 1));
  expect_event(out, Trade(TestPrice, 100, 4, 2));
  expect_event(out, Trade(TestPrice, 100, 4, 3));
}

template<int TestSide>
void test_market_order_trades_available_liquidity_and_rejects_rest() {
  QueueOut out;
  TradeEngine engine(out);

  insert_resting_limit<TestSide>(engine, 1, TestPrice, 100);
  expect_event(out, CreateAccepted(1, 0));

  insert_resting_limit<TestSide>(engine, 2, TestPrice + TestSide, 100);
  expect_event(out, CreateAccepted(2, 0));

  insert_aggressive_market<TestSide>(engine, 3, 250);
  expect_event(out, Trade(TestPrice, 100, 3, 1));
  expect_event(out, Trade(TestPrice + TestSide, 100, 3, 2));
  expect_event(out, CreateRejected(3, 50));
}

template<int TestSide>
void test_rejects_wrong_identity_and_wrong_cancel_price() {
  QueueOut out;
  TradeEngine engine(out);

  insert_resting_limit<TestSide>(engine, 2, TestPrice, 100);
  expect_event(out, CreateAccepted(2, 0));

  update_resting<TestSide>(engine, 99, TestPrice, 0, 50);
  expect_event(out, UpdateRejected(99, 50));

  cancel_resting<TestSide>(engine, 2, 0, 0);
  expect_event(out, CancelRejected(2));
}

}  // namespace

int main() {
  Assert(sizeof(OrderBook) <= 32 * 1024u);
  Assert(sizeof(TradeEngine) <= 32 * 1024u);

  test_insert_cancel_releases_book<1>();
  test_insert_cancel_releases_book<-1>();

  test_cancel_rejects_removed_order_and_book_stays_consistent<1>();
  test_cancel_rejects_removed_order_and_book_stays_consistent<-1>();

  test_update_changes_resting_qty_used_by_next_trade<1>();
  test_update_changes_resting_qty_used_by_next_trade<-1>();

  test_full_trade_preserves_fifo_on_single_level<1>();
  test_full_trade_preserves_fifo_on_single_level<-1>();

  test_market_order_trades_available_liquidity_and_rejects_rest<1>();
  test_market_order_trades_available_liquidity_and_rejects_rest<-1>();

  test_rejects_wrong_identity_and_wrong_cancel_price<1>();
  test_rejects_wrong_identity_and_wrong_cancel_price<-1>();

  return 0;
}