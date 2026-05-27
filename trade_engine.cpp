/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "trade_engine.hpp"
#include "assert.hpp"
#include "timer.hpp"

#ifdef NDEBUG
  constexpr auto PROFILE = "Release";
#else
  constexpr auto PROFILE = "Debug";
#endif

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

  engine.insert_order<side>(Order::InvalidId, 100, 100);
  Assert(engine.out().pop() == CreateRejected(Order::InvalidId, 100));
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

  engine.insert_order<side>(1, 10, Order::MaxQty + 1);
  Assert(engine.out().pop() == CreateRejected(1, Order::MaxQty + 1));

  engine.insert_order<side>(2, 10, Order::MinQty - 1);
  Assert(engine.out().pop() == CreateRejected(2, Order::MinQty - 1));
}

template<Side side>
void test_insert_all(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  for(int32_t price = engine.min_price(); price <= engine.max_price(); price++) {
    for(int32_t i = 0; i < engine.orders_per_level(); i++) {
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

  engine.update_order<side>(Order::InvalidId, centerPrice, 0, 100 + 1);
  Assert(engine.out().pop() == UpdateRejected(Order::InvalidId, 100 + 1));
}

template<Side side>
void test_update_invalid_qty(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0, 100));

  engine.update_order<side>(1, centerPrice, 0, Order::MaxQty + 1);
  Assert(engine.out().pop() == UpdateRejected(1, Order::MaxQty + 1));
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

  engine.cancel_order<side>(Order::InvalidId, centerPrice, 0);
  Assert(engine.out().pop() == CancelRejected(Order::InvalidId));
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
  const int32_t orders = engine.orders_per_level();

  for(int32_t order = 1; order <= orders; order++) {
    engine.insert_order<side>(order, tradePrice, qty);
    Assert(engine.out().pop() == CreateAccepted(order, order - 1, qty));
  }

  engine.insert_order<-side>(orders + 1, tradePrice, orders * qty);

  for(int32_t i = 0; i < engine.orders_per_level(); i++) {
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

void test_trend(int32_t centerPrice, int32_t trend = 1) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  int32_t price = centerPrice;
  const int32_t minPrice = engine.min_price();
  const int32_t maxPrice = engine.max_price();

  const auto update_price = [&](int32_t price, int32_t trend) -> int32_t {
    price += trend;

    price = std::max(price, Order::MinPrice);
    price = std::min(price, Order::MaxPrice);

    return price;
  };

  for(int32_t iter = 0; iter != 3 * (maxPrice - minPrice); iter += 1, price = update_price(price, trend)) {
    engine.insert_order<Sell>(1000000 + price, price, 100);
    engine.out().clear();
    // Assert(engine.out().pop() == CreateAccepted(1000000 + price, 0, 100));

    engine.insert_order<Buy>(2000000 + price, price, 100);
    engine.out().clear();
    // Assert(engine.out().pop() == Trade(price, 100, 2000000 + price, 1000000 + price));
  }
}

void test() {
  constexpr int32_t MinPrice = Order::MinPrice;
  constexpr int32_t MaxPrice = Order::MaxPrice;

  constexpr int32_t Price1 = MinPrice;
  constexpr int32_t Price2 = MinPrice + 32;
  constexpr int32_t Price3 = 1'000;
  constexpr int32_t Price4 = MaxPrice - 32;
  constexpr int32_t Price5 = MaxPrice;

  {
    test_insert<Sell>(Price1);
    test_insert<Buy>(Price1);

    test_insert<Sell>(Price2);
    test_insert<Buy>(Price2);

    test_insert<Sell>(Price3);
    test_insert<Buy>(Price3);

    test_insert<Sell>(Price4);
    test_insert<Buy>(Price4);

    test_insert<Sell>(Price5);
    test_insert<Buy>(Price5);
  }

  {
    test_insert_invalid_id<Sell>(Price1);
    test_insert_invalid_id<Buy>(Price1);

    test_insert_invalid_id<Sell>(Price2);
    test_insert_invalid_id<Buy>(Price2);

    test_insert_invalid_id<Sell>(Price3);
    test_insert_invalid_id<Buy>(Price3);

    test_insert_invalid_id<Sell>(Price4);
    test_insert_invalid_id<Buy>(Price4);

    test_insert_invalid_id<Sell>(Price5);
    test_insert_invalid_id<Buy>(Price5);
  }

  {
    test_insert_invalid_price<Sell>(Price1);
    test_insert_invalid_price<Buy>(Price1);

    test_insert_invalid_price<Sell>(Price2);
    test_insert_invalid_price<Buy>(Price2);

    test_insert_invalid_price<Sell>(Price3);
    test_insert_invalid_price<Buy>(Price3);

    test_insert_invalid_price<Sell>(Price4);
    test_insert_invalid_price<Buy>(Price4);

    test_insert_invalid_price<Sell>(Price5);
    test_insert_invalid_price<Buy>(Price5);
  }

  {
    test_insert_invalid_qty<Sell>(Price1);
    test_insert_invalid_qty<Buy>(Price1);

    test_insert_invalid_qty<Sell>(Price2);
    test_insert_invalid_qty<Buy>(Price2);

    test_insert_invalid_qty<Sell>(Price3);
    test_insert_invalid_qty<Buy>(Price3);

    test_insert_invalid_qty<Sell>(Price4);
    test_insert_invalid_qty<Buy>(Price4);

    test_insert_invalid_qty<Sell>(Price5);
    test_insert_invalid_qty<Buy>(Price5);
  }

  {
    test_insert_all<Sell>(Price1);
    test_insert_all<Buy>(Price1);

    test_insert_all<Sell>(Price2);
    test_insert_all<Buy>(Price2);

    test_insert_all<Sell>(Price3);
    test_insert_all<Buy>(Price3);

    test_insert_all<Sell>(Price4);
    test_insert_all<Buy>(Price4);

    test_insert_all<Sell>(Price5);
    test_insert_all<Buy>(Price5);
  }

  {
    test_update<Sell>(Price1);
    test_update<Buy>(Price1);

    test_update<Sell>(Price2);
    test_update<Buy>(Price2);

    test_update<Sell>(Price3);
    test_update<Buy>(Price3);

    test_update<Sell>(Price4);
    test_update<Buy>(Price4);

    test_update<Sell>(Price5);
    test_update<Buy>(Price5);
  }

  {
    test_update_invalid_id<Sell>(Price1);
    test_update_invalid_id<Buy>(Price1);

    test_update_invalid_id<Sell>(Price2);
    test_update_invalid_id<Buy>(Price2);

    test_update_invalid_id<Sell>(Price3);
    test_update_invalid_id<Buy>(Price3);

    test_update_invalid_id<Sell>(Price4);
    test_update_invalid_id<Buy>(Price4);

    test_update_invalid_id<Sell>(Price5);
    test_update_invalid_id<Buy>(Price5);
  }

  {
    test_update_invalid_price<Sell>(Price1);
    test_update_invalid_price<Buy>(Price1);

    test_update_invalid_price<Sell>(Price2);
    test_update_invalid_price<Buy>(Price2);

    test_update_invalid_price<Sell>(Price3);
    test_update_invalid_price<Buy>(Price3);

    test_update_invalid_price<Sell>(Price4);
    test_update_invalid_price<Buy>(Price4);

    test_update_invalid_price<Sell>(Price5);
    test_update_invalid_price<Buy>(Price5);
  }

  {
    test_update_invalid_qty<Sell>(Price1);
    test_update_invalid_qty<Buy>(Price1);

    test_update_invalid_qty<Sell>(Price2);
    test_update_invalid_qty<Buy>(Price2);

    test_update_invalid_qty<Sell>(Price3);
    test_update_invalid_qty<Buy>(Price3);

    test_update_invalid_qty<Sell>(Price4);
    test_update_invalid_qty<Buy>(Price4);

    test_update_invalid_qty<Sell>(Price5);
    test_update_invalid_qty<Buy>(Price5);
  }

  {
    test_update_invalid_slot<Sell>(Price1);
    test_update_invalid_slot<Buy>(Price1);

    test_update_invalid_slot<Sell>(Price2);
    test_update_invalid_slot<Buy>(Price2);

    test_update_invalid_slot<Sell>(Price3);
    test_update_invalid_slot<Buy>(Price3);

    test_update_invalid_slot<Sell>(Price4);
    test_update_invalid_slot<Buy>(Price4);

    test_update_invalid_slot<Sell>(Price5);
    test_update_invalid_slot<Buy>(Price5);
  }

  {
    test_update_deleted<Sell>(Price1);
    test_update_deleted<Buy>(Price1);

    test_update_deleted<Sell>(Price2);
    test_update_deleted<Buy>(Price2);

    test_update_deleted<Sell>(Price3);
    test_update_deleted<Buy>(Price3);

    test_update_deleted<Sell>(Price4);
    test_update_deleted<Buy>(Price4);

    test_update_deleted<Sell>(Price5);
    test_update_deleted<Buy>(Price5);
  }

  {
    test_delete<Sell>(Price1);
    test_delete<Buy>(Price1);

    test_delete<Sell>(Price2);
    test_delete<Buy>(Price2);

    test_delete<Sell>(Price3);
    test_delete<Buy>(Price3);

    test_delete<Sell>(Price4);
    test_delete<Buy>(Price4);

    test_delete<Sell>(Price5);
    test_delete<Buy>(Price5);
  }

  {
    test_delete_invalid_id<Sell>(Price1);
    test_delete_invalid_id<Buy>(Price1);

    test_delete_invalid_id<Sell>(Price2);
    test_delete_invalid_id<Buy>(Price2);

    test_delete_invalid_id<Sell>(Price3);
    test_delete_invalid_id<Buy>(Price3);

    test_delete_invalid_id<Sell>(Price4);
    test_delete_invalid_id<Buy>(Price4);

    test_delete_invalid_id<Sell>(Price5);
    test_delete_invalid_id<Buy>(Price5);
  }

  {
    test_delete_invalid_price<Sell>(Price1);
    test_delete_invalid_price<Buy>(Price1);

    test_delete_invalid_price<Sell>(Price2);
    test_delete_invalid_price<Buy>(Price2);

    test_delete_invalid_price<Sell>(Price3);
    test_delete_invalid_price<Buy>(Price3);

    test_delete_invalid_price<Sell>(Price4);
    test_delete_invalid_price<Buy>(Price4);

    test_delete_invalid_price<Sell>(Price5);
    test_delete_invalid_price<Buy>(Price5);
  }

  {
    test_delete_invalid_slot<Sell>(Price1);
    test_delete_invalid_slot<Buy>(Price1);

    test_delete_invalid_slot<Sell>(Price2);
    test_delete_invalid_slot<Buy>(Price2);

    test_delete_invalid_slot<Sell>(Price3);
    test_delete_invalid_slot<Buy>(Price3);

    test_delete_invalid_slot<Sell>(Price4);
    test_delete_invalid_slot<Buy>(Price4);

    test_delete_invalid_slot<Sell>(Price5);
    test_delete_invalid_slot<Buy>(Price5);
  }

  {
    test_double_delete<Sell>(Price1);
    test_double_delete<Buy>(Price1);

    test_double_delete<Sell>(Price2);
    test_double_delete<Buy>(Price2);

    test_double_delete<Sell>(Price3);
    test_double_delete<Buy>(Price3);

    test_double_delete<Sell>(Price4);
    test_double_delete<Buy>(Price4);

    test_double_delete<Sell>(Price5);
    test_double_delete<Buy>(Price5);
  }

  {
    test_trade_level<Sell>(Price1);
    test_trade_level<Buy>(Price1);

    test_trade_level<Sell>(Price2);
    test_trade_level<Buy>(Price2);

    test_trade_level<Sell>(Price3);
    test_trade_level<Buy>(Price3);

    test_trade_level<Sell>(Price4);
    test_trade_level<Buy>(Price4);

    test_trade_level<Sell>(Price5);
    test_trade_level<Buy>(Price5);
  }

  {
    test_trend(Price1, +1);
    test_trend(Price1, -1);

    test_trend(Price2, +1);
    test_trend(Price2, -1);

    test_trend(Price3, +1);
    test_trend(Price3, -1);

    test_trend(Price4, +1);
    test_trend(Price4, -1);

    test_trend(Price5, +1);
    test_trend(Price5, -1);
  }
}

void test_random(int32_t centerPrice, int32_t iters) {
  const auto str = [](int32_t i) -> std::string {
    return std::to_string(i);
  };

  QueueOut out;
  TradeEngine engine(out, centerPrice);

  const int32_t minPrice = engine.min_price();
  const int32_t maxPrice = engine.max_price();

  for(int32_t i = 1; i <= iters; i++) {
    const int32_t price = minPrice + rand() % (maxPrice - minPrice + 1);
    const int32_t size = rand() % 1000;

    if(i % 2) {
      engine.insert_order<Sell>(i, price, size);
      engine.out().clear();
      //engine.out().log("insert sell: id:" + str(i) + " price:" + str(price) + " size:" + str(size) + " -> ");
    } else {
      engine.insert_order<Buy>(i, price, size);
      engine.out().clear();
      //engine.out().log("insert buy : id:" + str(i) + " price:" + str(price) + " size:" + str(size) + " -> ");
    }
  }

  engine.insert_order_ioc<Buy>(iters + 1, (2 * OrderBook::Levels + 1) * Level::MaxOrders * 1000);
  engine.out().clear();
  //engine.out().log("insert buy : id:" + str(iters + 1) + " price:any " + " size:" + str(Level::MaxOrders * 1000) + " -> ");

  engine.insert_order_ioc<Sell>(iters + 2, (2 * OrderBook::Levels + 1) * Level::MaxOrders * 1000);
  engine.out().clear();
  //engine.out().log("insert sell: id:" + str(iters + 2) + " price:any " + " size:" + str(Level::MaxOrders * 1000) + " -> ");
}

void test_random(int32_t iters) {
  constexpr int32_t MinPrice = Order::MinPrice;
  constexpr int32_t MaxPrice = Order::MaxPrice;
  constexpr int32_t CenterPrices[] = {MinPrice, MinPrice + 32, 1000000, MaxPrice - 64, MaxPrice};

  for(int32_t centerPrice : CenterPrices) {
    test_random(centerPrice, iters);
  }
}

void test_micro_bench_insert() {
  QueueOut out;
  TradeEngine engine(out);

  struct Insert {
    Insert(TradeEngine& engine)
      : engine(engine) {
    }

    TradeEngine& engine;

    void setup() {
    }

    void run() {
      engine.insert_order<Sell>(engine.center_price(), engine.center_price(), 100);
      engine.insert_order<Sell>(engine.center_price() + 1, engine.center_price() + 1, 100);
      engine.insert_order<Sell>(engine.center_price() + 2, engine.center_price() + 2, 100);
      engine.insert_order<Sell>(engine.center_price() + 3, engine.center_price() + 3, 100);
      engine.insert_order<Sell>(engine.center_price() + 4, engine.center_price() + 4, 100);
      engine.insert_order<Sell>(engine.center_price() + 5, engine.center_price() + 5, 100);
      engine.insert_order<Sell>(engine.center_price() + 6, engine.center_price() + 6, 100);
      engine.insert_order<Sell>(engine.center_price() + 7, engine.center_price() + 7, 100);
    }

    void teardown() {
      engine.cancel_order<Sell>(engine.center_price(), engine.center_price(), 0);
      engine.cancel_order<Sell>(engine.center_price() + 1, engine.center_price() + 1, 0);
      engine.cancel_order<Sell>(engine.center_price() + 2, engine.center_price() + 2, 0);
      engine.cancel_order<Sell>(engine.center_price() + 3, engine.center_price() + 3, 0);
      engine.cancel_order<Sell>(engine.center_price() + 4, engine.center_price() + 4, 0);
      engine.cancel_order<Sell>(engine.center_price() + 5, engine.center_price() + 5, 0);
      engine.cancel_order<Sell>(engine.center_price() + 6, engine.center_price() + 6, 0);
      engine.cancel_order<Sell>(engine.center_price() + 7, engine.center_price() + 7, 0);
      engine.out().clear();
    }
  } insert(engine);

  std::cout << "Micro benchmark (" << PROFILE << "): insert: " << Timer<32>(insert) / 8 << "ns" << std::endl;
}

void test_micro_bench_trade() {
  QueueOut out;
  TradeEngine engine(out);

  struct Trade {
    Trade(TradeEngine& engine)
      : engine(engine) {
    }

    TradeEngine& engine;

    void setup() {
      engine.insert_order<Sell>(1, engine.center_price(), 100);
      engine.insert_order<Sell>(2, engine.center_price(), 100);
      engine.insert_order<Sell>(3, engine.center_price(), 100);
      engine.insert_order<Sell>(4, engine.center_price(), 100);
      engine.insert_order<Sell>(5, engine.center_price(), 100);
      engine.insert_order<Sell>(6, engine.center_price(), 100);
      engine.insert_order<Sell>(7, engine.center_price(), 100);
      engine.insert_order<Sell>(8, engine.center_price(), 100);
    }

    void run() {
      engine.insert_order<Buy>(1, engine.center_price(), 100);
      engine.insert_order<Buy>(2, engine.center_price(), 100);
      engine.insert_order<Buy>(3, engine.center_price(), 100);
      engine.insert_order<Buy>(4, engine.center_price(), 100);
      engine.insert_order<Buy>(5, engine.center_price(), 100);
      engine.insert_order<Buy>(6, engine.center_price(), 100);
      engine.insert_order<Buy>(7, engine.center_price(), 100);
      engine.insert_order<Buy>(8, engine.center_price(), 100);
    }

    void teardown() {
      engine.out().clear();
    }
  } trade(engine);

  std::cout << "Micro benchmark (" << PROFILE << "): trade: " << Timer<32>(trade) / 8 << "ns" << std::endl;
}

int main() {

#ifndef NDEBUG
  test();
#endif

  test_micro_bench_insert();
  test_micro_bench_trade();
  test_random(100'000);

  return 0;
}