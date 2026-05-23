#include <immintrin.h>
#include <iostream>

#include "assert.hpp"
#include "common.hpp"
#include "events.hpp"
#include "flat_list.hpp"
#include "flat_queue.hpp"
#include "likely.hpp"
#include "timer.hpp"

/*
 * QueueOut
 */

struct QueueOut {
  void push(const Event& event) noexcept {
    while(_queue.push(event) == false) {
      _mm_pause();
    }
  }

  Event pop() noexcept {
    return _queue.pop();
  }

  void log() {
    while(_queue.empty_approx() == false) {
      const Event event = _queue.pop();
      std::cout << event << std::endl;
    }
  }

  RingBufferSPSC<Event, 64> _queue;
};

/*
 * Order
 */ 

struct Order {
  int32_t id{0};
  int32_t qty{0};
};

inline bool operator==(const Order& lhs, const Order& rhs) noexcept {
  return lhs.id == rhs.id && lhs.qty == rhs.qty;
}

inline bool operator!=(const Order& lhs, const Order& rhs) noexcept {
  return ! (lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& os, const Order& order) {
  return os << "Order{id=" << order.id << ", qty=" << order.qty << "}";
}

/*
 * Level
 */

struct Level {
  int32_t total{0};
  FlatQueue<Order, 16> orders;
};

/*
 * Side
 */

enum class Side : uint8_t { Sell = 0, Buy = 1 };

/*
 * OrderBook
 */

struct OrderBook {
  OrderBook(QueueOut& out)
    : _bestSellPrice{MaxPrice}
    , _bestBuyPrice{MinPrice}
    , _out(out) {
  }

  static constexpr int32_t MinPrice = 1;
  static constexpr int32_t MaxPrice = 100;

  bool check_price(int32_t price) const noexcept {
    return price >= MinPrice && price <= MaxPrice;
  }

  template<Side side>
  void insert_order(int32_t orderId, int32_t price, int32_t qty) noexcept {
    if(check_price(price) == false) {
      return _out.push(CreateRejected(orderId, qty));
    }

    Level& level = _levels[price];

    if constexpr(side == Side::Sell) {
      assert(level.total <= 0);
    } else {
      assert(level.total >= 0);
    }

    if (level.orders.full()) {
      return _out.push(CreateRejected(orderId, qty));
    }

    const int32_t index = level.orders.push(Order{orderId, qty});

    if constexpr(side == Side::Sell) {
      level.total -= qty;
      _bestSellPrice = std::min(_bestSellPrice, price);
    } else {
      level.total += qty;
      _bestBuyPrice = std::max(_bestBuyPrice, price);
    }

    _out.push(CreateAccepted(orderId, index));
  }

  template<Side side>
  void update_order(int32_t orderId, int32_t price, int32_t slot, int32_t newQty) noexcept {
    if(check_price(price) == false) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    Level& level = _levels[price];
    Order& order = level.orders.at(slot);

    if(order.id != orderId) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    const int32_t qty = order.qty;

    if constexpr(side == Side::Sell) {
      level.total += qty;
      level.total -= newQty;
    } else {
      level.total -= qty;
      level.total += newQty;
    }

    order.qty = newQty;
    _out.push(UpdateAccepted(orderId));
  }

  template<Side side>
  void cancel_order(int32_t orderId, int32_t price, int32_t slot) noexcept {
    if(check_price(price) == false) {
      return _out.push(CancelRejected(orderId));
    }

    Level& level = _levels[price];
    Order& order = level.orders.at(slot);

    if(order.id != orderId) {
      return _out.push(CancelRejected(orderId));
    }

    const int32_t qty = order.qty;

    if constexpr(side == Side::Sell) {
      level.total += qty;
    } else {
      level.total -= qty;
    }

    order.id = 0;
    order.qty = 0;
    level.orders.remove(slot);

    _out.push(CancelAccepted(orderId));
  }

  Level& level(int32_t price) {
    Assert(check_price(price));
    return _levels[price];
  }
  
  int32_t _bestSellPrice;
  int32_t _bestBuyPrice;

private:
  Level _levels[128];
  QueueOut& _out;
};

/*
 * TradeEngine
 */

struct TradeEngine {
  TradeEngine(QueueOut& out)
    : _out(out)
    , _orderBook(out) 
  {
  }

  TradeEngine(TradeEngine&&) = delete;
  TradeEngine(const TradeEngine&) = delete;

  TradeEngine& operator=(TradeEngine&&) = delete;
  TradeEngine& operator=(const TradeEngine&) = delete;

public:
  void insert_sell_order(int32_t orderId, int32_t price, int32_t qty) noexcept {
    qty = _trade<Side::Sell>(orderId, price, qty);

    if(qty != 0) {
      _orderBook.insert_order<Side::Sell>(orderId, price, qty);
    }
  }

  void insert_buy_order(int32_t orderId, int32_t price, int32_t qty) noexcept {
    qty = _trade<Side::Buy>(orderId, price, qty);

    if(qty != 0) {
      _orderBook.insert_order<Side::Buy>(orderId, price, qty);
    }
  }

  void insert_sell_order(int32_t orderId, int32_t qty) noexcept {
    qty = _trade<Side::Sell>(orderId, OrderBook::MinPrice, qty);

    if(qty != 0) {
      _out.push(CreateRejected(orderId, qty));
    }
  }

  void insert_buy_order(int32_t orderId, int32_t qty) noexcept {
    qty = _trade<Side::Buy>(orderId, OrderBook::MaxPrice, qty);

    if(qty != 0) {
      _out.push(CreateRejected(orderId, qty));
    }
  }

  void update_sell_order(int32_t orderId, int32_t price, int32_t slot, int32_t newQty) noexcept {
    _orderBook.update_order<Side::Sell>(orderId, price, slot, newQty);
  }

  void update_buy_order(int32_t orderId, int32_t price, int32_t slot, int32_t newQty) noexcept {
    _orderBook.update_order<Side::Buy>(orderId, price, slot, newQty);
  }

  void cancel_sell_order(int32_t orderId, int32_t price, int32_t slot) noexcept {
    _orderBook.cancel_order<Side::Sell>(orderId, price, slot);
  }

  void cancel_buy_order(int32_t orderId, int32_t price, int32_t slot) noexcept {
    _orderBook.cancel_order<Side::Buy>(orderId, price, slot);
  }

  QueueOut& out() noexcept {
    return _out;
  }

private:
  int32_t _trade_sell(int32_t orderId, int32_t priceLimit, int32_t qty) noexcept {
    int32_t price = _orderBook._bestBuyPrice;

    while((qty != 0) && (price >= priceLimit)) {
      Level& level = _orderBook.level(price);

      while((qty != 0) && (level.total > 0)) {
        Order& order = level.orders.front();
        const int32_t min = std::min(qty, order.qty);

        qty -= min;
        order.qty -= min;
        level.total -= min;

        _out.push(Trade(price, min, orderId, order.id));

        if(UNLIKELY(order.qty != 0)) {
          continue;
        }

        order.id = 0;
        level.orders.pop();

        if(level.orders.empty()) {
          _orderBook._bestBuyPrice = price - 1;
        }
      }

      price -= 1;
    }

    return qty;
  }

  int32_t _trade_buy(int32_t orderId, int32_t priceLimit, int32_t qty) noexcept{
    int32_t price = _orderBook._bestSellPrice;

    while((qty != 0) && (price <= priceLimit)) {
      Level& level = _orderBook.level(price);

      while((qty != 0) && (level.total < 0)) {
        Order& order = level.orders.front();
        const int32_t min = std::min(qty, order.qty);

        qty -= min;
        order.qty -= min;
        level.total += min;

        _out.push(Trade(price, min, orderId, order.id));

        if(UNLIKELY(order.qty != 0)) {
          continue;
        }

        order.id = 0;
        level.orders.pop();

        if(level.orders.empty()) {
          _orderBook._bestSellPrice = price + 1;
        }
      }

      price += 1;
    }

    return qty;
  }

  template<Side side>
  int32_t _trade(int32_t orderId, int32_t priceLimit, int32_t qty) {
    if constexpr(side == Side::Sell) {
      return _trade_sell(orderId, priceLimit, qty);
    } else {
      return _trade_buy(orderId, priceLimit, qty);
    }
  }

private:
  QueueOut& _out;
  OrderBook _orderBook;
};

/*
 * main
 */

int main() {
  std::cout << sizeof(OrderBook) << std::endl;   // 25616 bytes
  std::cout << sizeof(TradeEngine) << std::endl; // 26880 bytes

  std::cout << "-----------------" << std::endl;

  QueueOut out;
  TradeEngine engine(out);
  
  {
    engine.insert_sell_order(1, 10, 100);
    engine.insert_buy_order(2, 10, 50);
    engine.insert_buy_order(3, 10, 75);
    engine.insert_sell_order(3, 10, 25);
    engine.out().log();
  }

  std::cout << "-----------------" << std::endl;

  {
    engine.insert_sell_order(1, 10, 100);
    engine.insert_sell_order(2, 11, 100);
    engine.insert_sell_order(3, 12, 100);
    engine.insert_sell_order(4, 13, 100);
    engine.insert_buy_order(5, 12, 500);
    engine.insert_buy_order(6, 13, 100);
    engine.insert_sell_order(7, 12, 200);
    engine.out().log();
  }

  std::cout << "-----------------" << std::endl;

  {
    engine.insert_sell_order(1, 10, 100);
    engine.insert_sell_order(2, 11, 100);
    engine.insert_sell_order(3, 12, 100);
    engine.insert_sell_order(4, 13, 100);
    engine.insert_buy_order(5, 500);
    engine.out().log();
  }

  std::cout << "-----------------" << std::endl;

  {
    engine.insert_buy_order(1, 10, 100);
    engine.insert_buy_order(2, 11, 100);
    engine.insert_buy_order(3, 12, 100);
    engine.insert_buy_order(4, 13, 100);
    engine.insert_sell_order(5, 500);
    engine.out().log();
  }

  return 0;
}
