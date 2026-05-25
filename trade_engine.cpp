/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "trade_engine.hpp"
#include "assert.hpp"

template<Side side>
void test_insert() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, 10, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.insert_order<side>(2, 10, 100);
  Assert(engine.out().pop() == CreateAccepted(2, 1));

  engine.insert_order<side>(3, 11, 100);
  Assert(engine.out().pop() == CreateAccepted(3, 0));
}

template<Side side>
void test_insert_invalid_id() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(Order::InvalidId(), 100, 100);
  Assert(engine.out().pop() == CreateRejected(Order::InvalidId(), 100));
}

template<Side side>
void test_insert_invalid_price() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, engine.max_price() + 1, 100);
  Assert(engine.out().pop() == CreateRejected(1, 100));

  engine.insert_order<side>(2, engine.min_price() - 1, 100);
  Assert(engine.out().pop() == CreateRejected(2, 100));
}

template<Side side>
void test_insert_invalid_qty() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, 10, Order::MaxQty() + 1);
  Assert(engine.out().pop() == CreateRejected(1, Order::MaxQty() + 1));

  engine.insert_order<side>(2, 10, Order::MinQty() - 1);
  Assert(engine.out().pop() == CreateRejected(2, Order::MinQty() - 1));
}

template<Side side>
void test_insert_all() {
  QueueOut out;
  TradeEngine engine(out);

  for(int32_t price = engine.min_price(); price <= engine.max_price(); price++) {
    for(int32_t i = 0; i < engine.order_per_level(); i++) {
      engine.insert_order<side>(price * 100 + i, price, 10);
      Assert(engine.out().pop() == CreateAccepted(price * 100 + i, i));
    }
  }
}

template<Side side>
void test_update() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, 10, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.update_order<side>(1, 10, 0, 100 + 1);
  Assert(engine.out().pop() == UpdateAccepted(1));

  engine.update_order<side>(1, 10, 0, 100 - 1);
  Assert(engine.out().pop() == UpdateAccepted(1));
}

template<Side side>
void test_update_invalid_id() {
  QueueOut out;
  TradeEngine engine(out);

  engine.update_order<side>(Order::InvalidId(), 10, 0, 100 + 1);
  Assert(engine.out().pop() == UpdateRejected(Order::InvalidId(), 100 + 1));
}

template<Side side>
void test_update_invalid_qty() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, 10, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.update_order<side>(1, 10, 0, Order::MaxQty() + 1);
  Assert(engine.out().pop() == UpdateRejected(1, Order::MaxQty() + 1));
}

template<Side side>
void test_update_invalid_price() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, 10, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.update_order<side>(1, 10 + 1, 0, 100 + 1);
  Assert(engine.out().pop() == UpdateRejected(1, 100 + 1));
}

template<Side side>
void test_update_invalid_slot() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, 10, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.update_order<side>(1, 10, 0 + 1000000, 100 + 1);
  Assert(engine.out().pop() == UpdateRejected(1, 100 + 1));

  engine.update_order<side>(1, 10, 0 + 1, 100 + 1);
  Assert(engine.out().pop() == UpdateRejected(1, 100 + 1));
}

template<Side side>
void test_update_deleted() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, 10, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.cancel_order<side>(1, 10, 0);
  Assert(engine.out().pop() == CancelAccepted(1));

  engine.update_order<side>(1, 10, 0, 100 + 1);
  Assert(engine.out().pop() == UpdateRejected(1, 100 + 1));
}

template<Side side>
void test_delete() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, 10, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.cancel_order<side>(1, 10, 0);
  Assert(engine.out().pop() == CancelAccepted(1));
}

template<Side side>
void test_delete_invalid_id() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, 10, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.cancel_order<side>(Order::InvalidId(), 10, 0);
  Assert(engine.out().pop() == CancelRejected(Order::InvalidId()));
}

template<Side side>
void test_delete_invalid_price() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, 10, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.cancel_order<side>(1, 10 + 1, 0);
  Assert(engine.out().pop() == CancelRejected(1));
}

template<Side side>
void test_delete_invalid_slot() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, 10, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.cancel_order<side>(1, 10, 0 + 1000000);
  Assert(engine.out().pop() == CancelRejected(1));

  engine.cancel_order<side>(1, 10, 0 + 1);
  Assert(engine.out().pop() == CancelRejected(1));
}

template<Side side>
void test_double_delete() {
  QueueOut out;
  TradeEngine engine(out);

  engine.insert_order<side>(1, 10, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.cancel_order<side>(1, 10, 0);
  Assert(engine.out().pop() == CancelAccepted(1));

  engine.cancel_order<side>(1, 10, 0);
  Assert(engine.out().pop() == CancelRejected(1));
}

template<Side side>
void test_trade_level(int32_t price) {
  QueueOut out;
  TradeEngine engine(out);

  const int32_t qty = 100;
  const int32_t orders = engine.order_per_level();

  for(int32_t order = 1; order <= orders; order++) {
    engine.insert_order<side>(order, price, qty);
    Assert(engine.out().pop() == CreateAccepted(order, order - 1));
  }

  engine.insert_order<-side>(orders + 1, price, orders * qty);

  for(int32_t i = 0; i < engine.order_per_level(); i++) {
    Assert(engine.out().pop() == Trade(price, qty, orders + 1, i + 1));
  }
}

template<Side side>
void test_trade_level() {
  QueueOut out;
  TradeEngine engine(out);

  for(int32_t price = engine.min_price(); price <= engine.max_price(); price++) {
    test_trade_level<side>(price);
  }
}

int main() {
  {
    test_insert<Sell>();
    test_insert<Buy>();

    test_insert_invalid_id<Sell>();
    test_insert_invalid_id<Buy>();

    test_insert_invalid_price<Sell>();
    test_insert_invalid_price<Buy>();

    test_insert_invalid_qty<Sell>();
    test_insert_invalid_qty<Buy>();

    test_insert_all<Sell>();
    test_insert_all<Buy>();
  }

  {
    test_update<Sell>();
    test_update<Buy>();

    test_update_invalid_id<Sell>();
    test_update_invalid_id<Buy>();

    test_update_invalid_price<Sell>();
    test_update_invalid_price<Buy>();

    test_update_invalid_qty<Sell>();
    test_update_invalid_qty<Buy>();

    test_update_invalid_slot<Sell>();
    test_update_invalid_slot<Buy>();

    test_update_deleted<Sell>();
    test_update_deleted<Buy>();
  }

  {
    test_delete<Sell>();
    test_delete<Buy>();

    test_delete_invalid_id<Sell>();
    test_delete_invalid_id<Buy>();

    test_delete_invalid_price<Sell>();
    test_delete_invalid_price<Buy>();

    test_delete_invalid_slot<Sell>();
    test_delete_invalid_slot<Buy>();

    test_double_delete<Sell>();
    test_double_delete<Buy>();
  }

  {
    test_trade_level<Sell>();
    test_trade_level<Buy>();
  }

  return 0;
}