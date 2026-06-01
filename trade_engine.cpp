/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "assert.hpp"
#include "trade_engine.hpp"
#include "timer.hpp"

#ifdef NDEBUG
  constexpr auto PROFILE = "Release";
#else
  constexpr auto PROFILE = "Debug";
#endif

constexpr int32_t ANY = 0;

template<Side side>
void test_insert(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, ANY, 100));

  engine.insert_order<side>(2, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(2, ANY, 100));
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

  engine.insert_order<side>(1, 10, MaxQty + 1);
  Assert(engine.out().pop() == CreateRejected(1, MaxQty + 1));

  engine.insert_order<side>(2, 10, MinQty - 1);
  Assert(engine.out().pop() == CreateRejected(2, MinQty - 1));
}

template<Side side>
void test_insert_all(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  for(int32_t price = engine.min_price(); price <= engine.max_price(); price++) {
    for(int32_t i = 0; i < TradeEngine::OrdersPerLevel; i++) {
      engine.insert_order<side>(price * 100 + i, price, 10);
      Assert(engine.out().pop() == CreateAccepted(price * 100 + i, ANY, 10));
    }
  }
}

template<Side side>
void test_update(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  const Event event = engine.out().pop();
  Assert(event == CreateAccepted(1, ANY, 100));

  engine.update_order<side>(1, centerPrice, 100 + 1, event.m3);
  Assert(engine.out().pop() == UpdateAccepted(1));

  engine.update_order<side>(1, centerPrice, 100 - 1, event.m3);
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
  const Event event = engine.out().pop();
  Assert(event == CreateAccepted(1, 0, 100));

  engine.update_order<side>(1, centerPrice, MaxQty + 1, event.m3);
  Assert(engine.out().pop() == UpdateRejected(1, MaxQty + 1));
}

template<Side side>
void test_update_invalid_price(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  const Event event = engine.out().pop();
  Assert(event == CreateAccepted(1, ANY, 100));

  engine.update_order<side>(1, centerPrice + 1, 100 + 1, event.m3);
  Assert(engine.out().pop() == UpdateRejected(1, 100 + 1));
}

template<Side side>
void test_update_invalid_slot(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  const Event event = engine.out().pop();
  Assert(event == CreateAccepted(1, ANY, 100));

  engine.update_order<side>(1, centerPrice, 100 + 1, event.m3 + 1000000);
  Assert(engine.out().pop() == UpdateRejected(1, 100 + 1));

  engine.update_order<side>(1, centerPrice, 100 + 1, event.m3 + 1);
  Assert(engine.out().pop() == UpdateRejected(1, 100 + 1));
}

template<Side side>
void test_update_deleted(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  const Event event = engine.out().pop();
  Assert(event == CreateAccepted(1, ANY, 100));

  engine.cancel_order<side>(1, centerPrice, event.m3);
  Assert(engine.out().pop() == CancelAccepted(1));

  engine.update_order<side>(1, centerPrice, 100 + 1, event.m3);
  Assert(engine.out().pop() == UpdateRejected(1, 100 + 1));
}

template<Side side>
void test_delete(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  const Event event = engine.out().pop();
  Assert(event == CreateAccepted(1, ANY, 100));

  engine.cancel_order<side>(1, centerPrice, event.m3);
  Assert(engine.out().pop() == CancelAccepted(1));
}

template<Side side>
void test_delete_invalid_id(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  Assert(engine.out().pop() == CreateAccepted(1, ANY, 100));

  engine.cancel_order<side>(Order::InvalidId, centerPrice, 0);
  Assert(engine.out().pop() == CancelRejected(Order::InvalidId));
}

template<Side side>
void test_delete_invalid_price(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  const Event event = engine.out().pop();
  Assert(event == CreateAccepted(1, ANY, 100));

  engine.cancel_order<side>(1, centerPrice + 1, event.m3);
  Assert(engine.out().pop() == CancelRejected(1));
}

template<Side side>
void test_delete_invalid_slot(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  const Event event = engine.out().pop();
  Assert(event == CreateAccepted(1, ANY, 100));

  engine.cancel_order<side>(1, centerPrice, event.m3 + 1000000);
  Assert(engine.out().pop() == CancelRejected(1));

  engine.cancel_order<side>(1, centerPrice, event.m3 + 1);
  Assert(engine.out().pop() == CancelRejected(1));
}

template<Side side>
void test_double_delete(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice, 100);
  const Event event = engine.out().pop();
  Assert(event == CreateAccepted(1, ANY, 100));

  engine.cancel_order<side>(1, centerPrice, event.m3);
  Assert(engine.out().pop() == CancelAccepted(1));

  engine.cancel_order<side>(1, centerPrice, event.m3);
  Assert(engine.out().pop() == CancelRejected(1));
}

template<Side side>
void test_trade_level(int32_t centerPrice, int32_t tradePrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  const int32_t qty = 100;
  const int32_t orders = TradeEngine::OrdersPerLevel;

  for(int32_t order = 1; order <= orders; order++) {
    engine.insert_order<side>(order, tradePrice, qty);
    Assert(engine.out().pop() == CreateAccepted(order, ANY, qty));
  }

  engine.insert_order<-side>(orders + 1, tradePrice, orders * qty);

  for(int32_t i = 0; i < TradeEngine::OrdersPerLevel; i++) {
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

    price = std::max(price, MinPrice);
    price = std::min(price, MaxPrice);

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

void test(int32_t price) {
    test_insert<Sell>(price);
    test_insert<Buy>(price);

    test_insert_invalid_id<Sell>(price);
    test_insert_invalid_id<Buy>(price);

    test_insert_invalid_price<Sell>(price);
    test_insert_invalid_price<Buy>(price);

    test_insert_invalid_qty<Sell>(price);
    test_insert_invalid_qty<Buy>(price);

    test_insert_all<Sell>(price);
    test_insert_all<Buy>(price);

    test_update<Sell>(price);
    test_update<Buy>(price);

    test_update_invalid_id<Sell>(price);
    test_update_invalid_id<Buy>(price);

    test_update_invalid_price<Sell>(price);
    test_update_invalid_price<Buy>(price);

    test_update_invalid_qty<Sell>(price);
    test_update_invalid_qty<Buy>(price);

    test_update_deleted<Sell>(price);
    test_update_deleted<Buy>(price);

    test_delete<Sell>(price);
    test_delete<Buy>(price);

    test_delete_invalid_id<Sell>(price);
    test_delete_invalid_id<Buy>(price);

    test_delete_invalid_price<Sell>(price);
    test_delete_invalid_price<Buy>(price);

    test_double_delete<Sell>(price);
    test_double_delete<Buy>(price);

    test_trade_level<Sell>(price);
    test_trade_level<Buy>(price);

    test_trend(price, +1);
    test_trend(price, -1);

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
      engine.insert_order<Sell>(1, engine.center_price() + 1, 100);
      engine.insert_order<Sell>(2, engine.center_price() + 2, 100);
      engine.insert_order<Sell>(3, engine.center_price() + 3, 100);
      engine.insert_order<Sell>(4, engine.center_price() + 4, 100);
      engine.insert_order<Sell>(5, engine.center_price() + 5, 100);
      engine.insert_order<Sell>(6, engine.center_price() + 6, 100);
      engine.insert_order<Sell>(7, engine.center_price() + 7, 100);
      engine.insert_order<Sell>(8, engine.center_price() + 8, 100);
    }

    void teardown() {
      engine.cancel_order<Sell>(1, engine.center_price() + 1, 1&7);
      engine.cancel_order<Sell>(2, engine.center_price() + 2, 2&7);
      engine.cancel_order<Sell>(3, engine.center_price() + 3, 3&7);
      engine.cancel_order<Sell>(4, engine.center_price() + 4, 4&7);
      engine.cancel_order<Sell>(5, engine.center_price() + 5, 5&7);
      engine.cancel_order<Sell>(6, engine.center_price() + 6, 6&7);
      engine.cancel_order<Sell>(7, engine.center_price() + 7, 7&7);
      engine.cancel_order<Sell>(8, engine.center_price() + 8, 8&7);
      engine.out().clear();
    }
  } insert(engine);

  std::cout << "Micro benchmark (" << PROFILE << "): insert: " << Timer<32>(insert) / 8 << " ns" << std::endl;
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

  std::cout << "Micro benchmark (" << PROFILE << "): trade: " << Timer<32>(trade) / 8 << " ns" << std::endl;
}

void benchmark(int32_t iters) {
  QueueOut out;
  int32_t events = 0;
  TradeEngine engine(out);

  struct Benchmark {
    Benchmark(TradeEngine& engine, int32_t iters, int32_t& events)
      : _iters(iters)
      , _engine(engine)
      , _events(events)
    {
    }

    int32_t _iters;
    int32_t& _events;
    TradeEngine& _engine;
    std::vector<Request> _requests;

    void setup() {
      RequestGenerator gen(
        /* centerPrice     */ 100,
        /* windowHalfSize  */ 124,
        /* laplaceScale b  */ 5.0,
        /* marketProb      */ 0.05,
        /* cancelProb      */ 0.25,
        /* seed            */ 123
      );

      _requests = gen.generate(_iters);
    }

    void run() {
      _events = 0;

      for(const auto& e : _requests) {
        if (e.qty > 0) {
          if (e.price == 0) {
            _engine.insert_mkt_order_ioc<Buy>(e.id, e.qty);
          } else {
            _engine.insert_order<Buy>(e.id, e.price, e.qty);
          }
        } else if (e.qty < 0) {
          if (e.price == 0) {
            _engine.insert_mkt_order_ioc<Sell>(e.id, -e.qty);
          } else {
            _engine.insert_order<Sell>(e.id, e.price, -e.qty);
          }
        } else {
          if (e.price > 0) {
            _engine.cancel_order<Buy>(e.id, e.price, e.id&7);
          } else if (e.price < 0) {
            _engine.cancel_order<Sell>(e.id, -e.price, e.id&7);
          }
        }

#ifndef NDEBUG
        _events += _engine.out().log(to_string(e) + " => ");
#else
        _events += _engine.out().clear();
#endif
      }
    }

    void teardown() {
    }

  } bench(engine, iters, events);

  Timer<1>(bench).log([iters, events](int ns, const std::string& msg) { 
    std::cout << "Benchmark (" << PROFILE << ")(events=" << events << "): " 
              << ns/1000000 << " ms :: " 
              << (ns/events) << " ns/event :: " 
              << (int)(1e9 * events/ns) << " events/s"
              << std::endl; 
  });
}

int main() {
#ifndef NDEBUG
  test(MinPrice + 32);
  test(1'000);
  test(MaxPrice - 32);
#endif
  
  test_micro_bench_insert();
  test_micro_bench_trade();

#ifndef NDEBUG
  benchmark(10'000);
#else
  benchmark(10'000'000);
#endif

  return 0;
}