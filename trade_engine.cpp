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

constexpr int32_t ANY = 0;

struct Request {
  uint32_t id;
  int32_t price; // {price!=0 : limit}, {price=0 : market}
  int32_t qty;   // {qty>0 : buy}, {qty<0 : sell}, {qty=0, price>0 : cancel buy}, {qty=0, price<0 : cancel sell}
};

std::string to_string(const Request& request) {
  std::ostringstream os;

  std::string type;

  if(request.price == 0 && request.qty > 0) {
    type = "Market side:tBuy";
  } else if(request.price == 0 && request.qty < 0) {
    type = "Market side:Sell";
  } else if(request.price > 0 && request.qty > 0) {
    type = "Limit side:Buy";
  } else if(request.price > 0 && request.qty < 0) {
    type = "Limit side:Sell";
  } else if(request.price < 0 && request.qty == 0) {
    type = "Cancel side:Sell";
  } else if(request.price > 0 && request.qty == 0) {
    type = "Cancel side:Buy";
  } else {
    type = "Unknown";
  }

  os << "Request: type=" << type << " id=" << request.id 
      << " price=" << std::abs(request.price) 
      << " qty=" << std::abs(request.qty);

  return os.str();
}

std::ostream& operator<<(std::ostream& os, const Request& request) {
  return os << to_string(request);
}

class RequestGenerator {
public:
  RequestGenerator(
      int32_t centerPrice, //
      int32_t levels,
      double laplaceScale,
      double marketProb,
      double cancelProb,
      double trend = 0.1,
      uint32_t seed = 12345)
    : _center(centerPrice)
    , _levels(levels)
    , _laplaceScale(laplaceScale)
    , _p_Markets(marketProb)
    , _p_Cancels(cancelProb)
    , _rng(seed)
    , _uni(0.0, 1.0)
    , _trend(trend)
    , _uniSign(-0.5, 0.5) 
  {
  }

  std::vector<Request> generate(int32_t count) {
    std::vector<Request> out(count);
    std::unordered_map<int32_t, int32_t> idPrice;
    std::unordered_map<int32_t, int32_t> idSide;

    for(int32_t i = 0; i < count; i++) {
      if (i == count / 2) {
        _trend = -_trend;
      }

      auto& e = out[i];
      e.id = int32_t(i + 1);
      double u = _uni(_rng);

      if(u < _p_Cancels) {
        e.id = std::max(1, i - int32_t(_uni(_rng) * 20));
        e.price = idPrice[e.id] * idSide[e.id];
        e.qty = 0;
        continue;
      }

      if(u < _p_Cancels + _p_Markets) {
        e.price = 0;
        e.qty = 1 + int32_t(_uni(_rng) * 10);

        if(_uni(_rng) > 0.5) {
          e.qty = -e.qty;
        }

        continue;
      }

      e.qty = 1 + int32_t(_uni(_rng) * 10);

      if(_uni(_rng) > 0.5) {
        e.qty = -e.qty;
      }

      double s = _uniSign(_rng);
      double x = -_laplaceScale * ((s < 0.0) ? -1.0 : 1.0) * std::log(1.0 - 2.0 * std::abs(s));
      double price = _center + int32_t(std::round(x));

      e.price = std::max(_center - _levels, std::min(price, _center + _levels));

      idPrice[e.id] = e.price;
      idSide[e.id] = (e.qty > 0) ? 1 : -1;

      _center += _trend;
    }

    return out;
  }

private:
  double _center;
  double _levels;
  double _laplaceScale;
  double _p_Markets;
  double _p_Cancels;
  double _trend;

  std::mt19937 _rng;
  std::uniform_real_distribution<double> _uni;
  std::uniform_real_distribution<double> _uniSign;
};

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

  engine.insert_order<side>(InvalidOrderId, 100, 100);
  Assert(engine.out().pop() == CreateRejected(InvalidOrderId, 100));
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

  engine.update_order<side>(InvalidOrderId, centerPrice, 0, 100 + 1);
  Assert(engine.out().pop() == UpdateRejected(InvalidOrderId, 100 + 1));
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

  engine.cancel_order<side>(InvalidOrderId, centerPrice, 0);
  Assert(engine.out().pop() == CancelRejected(InvalidOrderId));
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

    engine.insert_order<Buy>(2000000 + price, price, 100);
    engine.out().clear();
  }
}

template<Side side>
void test_fok_trade(int32_t centerPrice) {
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<side>(1, centerPrice + side * 1, 100);
  Assert(engine.out().pop() == CreateAccepted(1, ANY, 100));

  engine.insert_order<side>(2, centerPrice + side * 2, 100);
  Assert(engine.out().pop() == CreateAccepted(2, ANY, 100));

  engine.insert_order<side>(3, centerPrice + side * 3, 100);
  Assert(engine.out().pop() == CreateAccepted(3, ANY, 100));

  engine.insert_order_fok<-side>(4, centerPrice, 400);
  Assert(engine.out().pop() == CreateRejected(4, 400, Reason::FOK));

  engine.insert_order<side>(5, centerPrice + side * 5, 100);
  Assert(engine.out().pop() == CreateAccepted(5, ANY, 100));

  engine.insert_order_fok<-side>(6, centerPrice, 400);
  Assert(engine.out().pop() == Trade(ANY, 100, 6, ANY));
  Assert(engine.out().pop() == Trade(ANY, 100, 6, ANY));
  Assert(engine.out().pop() == Trade(ANY, 100, 6, ANY));
  Assert(engine.out().pop() == Trade(ANY, 100, 6, ANY));
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

  test_fok_trade<Sell>(price);
  test_fok_trade<Buy>(price);
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
      engine.cancel_order<Sell>(1, engine.center_price() + 1, 1 & 7);
      engine.cancel_order<Sell>(2, engine.center_price() + 2, 2 & 7);
      engine.cancel_order<Sell>(3, engine.center_price() + 3, 3 & 7);
      engine.cancel_order<Sell>(4, engine.center_price() + 4, 4 & 7);
      engine.cancel_order<Sell>(5, engine.center_price() + 5, 5 & 7);
      engine.cancel_order<Sell>(6, engine.center_price() + 6, 6 & 7);
      engine.cancel_order<Sell>(7, engine.center_price() + 7, 7 & 7);
      engine.cancel_order<Sell>(8, engine.center_price() + 8, 8 & 7);
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
      , _events(events) {
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
          /* trend           */ 0.10,
          /* seed            */ 123);

      _requests = gen.generate(_iters);
    }

    void run() {
      _events = 0;

      for(const auto& e : _requests) {
        if(e.qty > 0) {
          if(e.price == 0) {
            _engine.insert_mkt_order_ioc<Buy>(e.id, e.qty);
          } else {
            _engine.insert_order<Buy>(e.id, e.price, e.qty);
          }
        } else if(e.qty < 0) {
          if(e.price == 0) {
            _engine.insert_mkt_order_ioc<Sell>(e.id, -e.qty);
          } else {
            _engine.insert_order<Sell>(e.id, e.price, -e.qty);
          }
        } else {
          if(e.price > 0) {
            _engine.cancel_order<Buy>(e.id, e.price, e.id & 7);
          } else if(e.price < 0) {
            _engine.cancel_order<Sell>(e.id, -e.price, e.id & 7);
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
    std::cout << "Benchmark (" << PROFILE << ")(events=" << events << "): " << ns / 1000000 << " ms :: " << (ns / events)
              << " ns/event :: " << (int)(1e9 * events / ns) << " events/s" << std::endl;
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