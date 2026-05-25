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
void test_insert(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0, 100));

  engine.insert_order<side>(2, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(2, 1, 100));
}

template<Side side>
void test_insert_invalid_id(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(Order::InvalidId(), 100, 100);
  Assert(engine.out().pop() == CreateRejected(Order::InvalidId(), 100));
}

template<Side side>
void test_insert_invalid_price(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, engine.max_price() + 1, 100);
  Assert(engine.out().pop() == CreateRejected(1, 100));

  engine.insert_order<side>(2, engine.min_price() - 1, 100);
  Assert(engine.out().pop() == CreateRejected(2, 100));
}

template<Side side>
void test_insert_invalid_qty(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, 10, Order::MaxQty() + 1);
  Assert(engine.out().pop() == CreateRejected(1, Order::MaxQty() + 1));

  engine.insert_order<side>(2, 10, Order::MinQty() - 1);
  Assert(engine.out().pop() == CreateRejected(2, Order::MinQty() - 1));
}

template<Side side>
void test_insert_all(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  for(int32_t price = engine.min_price(); price <= engine.max_price(); price++) {
    for(int32_t i = 0; i < engine.order_per_level(); i++) {
      engine.insert_order<side>(price * 100 + i, price, 10);
      Assert(engine.out().pop() == CreateAccepted(price * 100 + i, i, 10));
    }
  }
}

template<Side side>
void test_update(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0, 100));

  engine.update_order<side>(1, centerPrice, 0, 100 + 1);
  Assert(engine.out().pop() == UpdateAccepted(1));

  engine.update_order<side>(1, centerPrice, 0, 100 - 1);
  Assert(engine.out().pop() == UpdateAccepted(1));
}

template<Side side>
void test_update_invalid_id(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.update_order<side>(Order::InvalidId(), centerPrice, 0, 100 + 1);
  Assert(engine.out().pop() == UpdateRejected(Order::InvalidId(), 100 + 1));
}

template<Side side>
void test_update_invalid_qty(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0, 100));

  engine.update_order<side>(1, centerPrice, 0, Order::MaxQty() + 1);
  Assert(engine.out().pop() == UpdateRejected(1, Order::MaxQty() + 1));
}

template<Side side>
void test_update_invalid_price(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0, 100));

  engine.update_order<side>(1, centerPrice + 1, 0, 100 + 1);
  Assert(engine.out().pop() == UpdateRejected(1, 100 + 1));
}

template<Side side>
void test_update_invalid_slot(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0, 100));

  engine.update_order<side>(1, centerPrice, 0 + 1000000, 100 + 1);
  Assert(engine.out().pop() == UpdateRejected(1, 100 + 1));

  engine.update_order<side>(1, centerPrice, 0 + 1, 100 + 1);
  Assert(engine.out().pop() == UpdateRejected(1, 100 + 1));
}

template<Side side>
void test_update_deleted(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0, 100));

  engine.cancel_order<side>(1, centerPrice, 0);
  Assert(engine.out().pop() == CancelAccepted(1));

  engine.update_order<side>(1, centerPrice, 0, 100 + 1);
  Assert(engine.out().pop() == UpdateRejected(1, 100 + 1));
}

template<Side side>
void test_delete(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0, 100));

  engine.cancel_order<side>(1, centerPrice, 0);
  Assert(engine.out().pop() == CancelAccepted(1));
}

template<Side side>
void test_delete_invalid_id(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0, 100));

  engine.cancel_order<side>(Order::InvalidId(), centerPrice, 0);
  Assert(engine.out().pop() == CancelRejected(Order::InvalidId()));
}

template<Side side>
void test_delete_invalid_price(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0, 100));

  engine.cancel_order<side>(1, centerPrice + 1, 0);
  Assert(engine.out().pop() == CancelRejected(1));
}

template<Side side>
void test_delete_invalid_slot(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0, 100));

  engine.cancel_order<side>(1, centerPrice, 0 + 1000000);
  Assert(engine.out().pop() == CancelRejected(1));

  engine.cancel_order<side>(1, centerPrice, 0 + 1);
  Assert(engine.out().pop() == CancelRejected(1));
}

template<Side side>
void test_double_delete(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0, 100));

  engine.cancel_order<side>(1, centerPrice, 0);
  Assert(engine.out().pop() == CancelAccepted(1));

  engine.cancel_order<side>(1, centerPrice, 0);
  Assert(engine.out().pop() == CancelRejected(1));
}

template<Side side>
void test_trade_level(int32_t centerPrice, int32_t tradePrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  const int32_t qty = 100;
  const int32_t orders = engine.order_per_level();

  for(int32_t order = 1; order <= orders; order++) {
    engine.insert_order<side>(order, tradePrice, qty);
    Assert(engine.out().pop() == CreateAccepted(order, order - 1, qty));
  }

  engine.insert_order<-side>(orders + 1, tradePrice, orders * qty);

  for(int32_t i = 0; i < engine.order_per_level(); i++) {
    Assert(engine.out().pop() == Trade(tradePrice, qty, orders + 1, i + 1));
  }
}

template<Side side>
void test_trade_level(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  for(int32_t price = engine.min_price(); price <= engine.max_price(); price++) {
    test_trade_level<side>(price, price);
  }
}

void test_random(int32_t centerPrice, int32_t iters = 100'000) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  const int32_t minPrice = engine.min_price();
  const int32_t maxPrice = engine.max_price();

  for(int32_t i = 1; i <= iters; i++) {
    const int32_t price = minPrice + rand() % (maxPrice - minPrice + 1);
    const int32_t size = rand() % 1000;

    if (i % 2) {
      engine.insert_order<Sell>(i, price, size);
      engine.out().log("insert sell: id:" + std::to_string(i) + " price:" + std::to_string(price) + " size:" + std::to_string(size) + " -> ");
    } else {
      engine.insert_order<Buy>(i, price, size);
      engine.out().log("insert buy : id:" + std::to_string(i) + " price:" + std::to_string(price) + " size:" + std::to_string(size) + " -> ");
    }
  }

  engine.insert_order<Buy>(iters + 1, OrderBook::MaxLevels * Level::MaxOrders * 1000);
  engine.out().log("insert buy : id:" + std::to_string(iters + 1) + " price:any " + " size:" + std::to_string(Level::MaxOrders * 1000) + " -> ");

  engine.insert_order<Sell>(iters + 2, OrderBook::MaxLevels * Level::MaxOrders * 1000);
  engine.out().log("insert sell: id:" + std::to_string(iters + 2) + " price:any " + " size:" + std::to_string(Level::MaxOrders * 1000) + " -> ");
}

int main() {
  constexpr int32_t MinPrice = Order::MinPrice();
  constexpr int32_t MaxPrice = Order::MaxPrice();
  constexpr int32_t CenterPrices[] = {MinPrice, MinPrice + 32, 1000000, MaxPrice - 64, MaxPrice};

  for(int32_t centerPrice : {100}) {
    test_random(centerPrice, 10'000);
  }

  for(int32_t centerPrice : CenterPrices) {
    test_insert<Sell>(centerPrice);
    test_insert<Buy>(centerPrice);

    test_insert_invalid_id<Sell>(centerPrice);
    test_insert_invalid_id<Buy>(centerPrice);

    test_insert_invalid_price<Sell>(centerPrice);
    test_insert_invalid_price<Buy>(centerPrice);

    test_insert_invalid_qty<Sell>(centerPrice);
    test_insert_invalid_qty<Buy>(centerPrice);

    test_insert_all<Sell>(centerPrice);
    test_insert_all<Buy>(centerPrice);
  }

  for(int32_t centerPrice : CenterPrices) {
    test_update<Sell>(centerPrice);
    test_update<Buy>(centerPrice);

    test_update_invalid_id<Sell>(centerPrice);
    test_update_invalid_id<Buy>(centerPrice);

    test_update_invalid_price<Sell>(centerPrice);
    test_update_invalid_price<Buy>(centerPrice);

    test_update_invalid_qty<Sell>(centerPrice);
    test_update_invalid_qty<Buy>(centerPrice);

    test_update_invalid_slot<Sell>(centerPrice);
    test_update_invalid_slot<Buy>(centerPrice);

    test_update_deleted<Sell>(centerPrice);
    test_update_deleted<Buy>(centerPrice);
  }

  for(int32_t centerPrice : CenterPrices) {
    test_delete<Sell>(centerPrice);
    test_delete<Buy>(centerPrice);

    test_delete_invalid_id<Sell>(centerPrice);
    test_delete_invalid_id<Buy>(centerPrice);

    test_delete_invalid_price<Sell>(centerPrice);
    test_delete_invalid_price<Buy>(centerPrice);

    test_delete_invalid_slot<Sell>(centerPrice);
    test_delete_invalid_slot<Buy>(centerPrice);

    test_double_delete<Sell>(centerPrice);
    test_double_delete<Buy>(centerPrice);
  }

  for(int32_t centerPrice : CenterPrices) {
    test_trade_level<Sell>(centerPrice);
    test_trade_level<Buy>(centerPrice);
  }

  return 0;
}