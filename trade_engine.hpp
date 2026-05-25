#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <immintrin.h>
#include <iostream>

#include "assert.hpp"
#include "common.hpp"
#include "events.hpp"
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

  RingBufferSPSC<Event, 1024> _queue;
};


/*
 * Side
 */

typedef int8_t Side;
constexpr Side Sell = -1;
constexpr Side Buy = 1;

/*
 * Order
 */ 

struct Order {
  static constexpr int32_t MinPrice() noexcept{
    return 1;
  }

  static constexpr int32_t MaxPrice() noexcept{
    return 100;
  }
  
  template<Side side>
  static constexpr int32_t AnyPrice() noexcept {
    if constexpr(side == Sell) {
      return Order::MinPrice();
    } else {
      return Order::MaxPrice();
    }
  }


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
public:
  Level() = default;
  Level(const Level&) = delete;
  Level(Level&&) = delete;

  Level& operator=(const Level&) = delete;
  Level& operator=(Level&&) = delete;

public:
  static constexpr int8_t MaxOrders = 16;

  int32_t total{0};
  FlatQueue<Order, MaxOrders> orders;
};

/*
 * OrderBook
 */

struct OrderBook {
public:
  static bool check_price(int32_t price) noexcept {
    return price >= Order::MinPrice() && price <= Order::MaxPrice();
  }
  
public:
  explicit OrderBook(QueueOut& out)
    : _bestSellPrice{Order::MaxPrice()}
    , _bestBuyPrice{Order::MinPrice()}
    , _out(out) {
  }

  OrderBook(OrderBook&&) = delete;
  OrderBook(const OrderBook&) = delete;

  OrderBook& operator=(OrderBook&&) = delete;
  OrderBook& operator=(const OrderBook&) = delete;

public:  
  int32_t get_min_price() const noexcept {
    return Order::MinPrice();
  }

  int32_t get_max_price() const noexcept {
    return Order::MaxPrice();
  }

  template<Side side>
  int32_t price_limit(int32_t priceLimit) const noexcept {
    if constexpr (side == Sell) {
      const int32_t price = get_min_price();
      return std::max(priceLimit, price);
    } else {
      const int32_t price = get_max_price();
      return std::min(priceLimit, price);
    }
  }

  Level& get_level(int32_t price) {
    Assert(check_price(price));
    return _levels[price];
  }

  template<Side side>
  int32_t get_best_price() const noexcept {
    if constexpr(side == Sell) {
      return _bestSellPrice;
    } else {
      return _bestBuyPrice;
    }
  }

  template<Side side>
  void set_best_price(int32_t price) noexcept {
    if constexpr(side == Sell) {
      _bestSellPrice = price;
    } else {
      _bestBuyPrice = price;
    }
  }

  template<Side side>
  void insert_order(int32_t orderId, int32_t price, int32_t qty) noexcept {
    if(check_price(price) == false) {
      return _out.push(CreateRejected(orderId, qty));
    }

    Level& get_level = _levels[price];

    if constexpr(side == Sell) {
      assert(get_level.total <= 0);
    } else {
      assert(get_level.total >= 0);
    }

    if (get_level.orders.full()) {
      return _out.push(CreateRejected(orderId, qty));
    }

    const int32_t index = get_level.orders.push(Order{orderId, qty});

    if constexpr(side == Sell) {
      get_level.total -= qty;
      _bestSellPrice = std::min(_bestSellPrice, price);
    } else {
      get_level.total += qty;
      _bestBuyPrice = std::max(_bestBuyPrice, price);
    }

    _out.push(CreateAccepted(orderId, index));
  }

  template<Side side>
  void update_order(int32_t orderId, int32_t price, int32_t slot, int32_t newQty) noexcept {
    if(check_price(price) == false) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    Level& get_level = _levels[price];
    Order& order = get_level.orders.at(slot);

    if(order.id != orderId) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    const int32_t qty = order.qty;

    if constexpr(side == Sell) {
      get_level.total += qty;
      get_level.total -= newQty;
    } else {
      get_level.total -= qty;
      get_level.total += newQty;
    }

    order.qty = newQty;
    _out.push(UpdateAccepted(orderId));
  }

  template<Side side>
  void cancel_order(int32_t orderId, int32_t price, int32_t slot) noexcept {
    if(check_price(price) == false) {
      return _out.push(CancelRejected(orderId));
    }

    Level& get_level = _levels[price];
    Order& order = get_level.orders.at(slot);

    if(order.id != orderId) {
      return _out.push(CancelRejected(orderId));
    }

    const int32_t qty = order.qty;

    if constexpr(side == Sell) {
      get_level.total += qty;
    } else {
      get_level.total -= qty;
    }

    order.id = 0;
    order.qty = 0;
    get_level.orders.remove(slot);

    _out.push(CancelAccepted(orderId));
  }

private:
  int32_t _bestSellPrice;
  int32_t _bestBuyPrice;

  QueueOut& _out;
  Level _levels[128];
};

/*
 * TradeEngine
 */

struct TradeEngine {
  explicit TradeEngine(QueueOut& out)
    : _out(out)
    , _orderBook(out) 
  {
  }

  TradeEngine(TradeEngine&&) = delete;
  TradeEngine(const TradeEngine&) = delete;

  TradeEngine& operator=(TradeEngine&&) = delete;
  TradeEngine& operator=(const TradeEngine&) = delete;

public:
  template<Side side>
  void insert_order(int32_t orderId, int32_t price, int32_t qty) noexcept {
    qty = _trade<side>(orderId, price, qty);

    if(qty != 0) {
      _orderBook.insert_order<side>(orderId, price, qty);
    }
  }

  template<Side side>
  void insert_order(int32_t orderId, int32_t qty) noexcept {
    qty = _trade<side>(orderId, Order::AnyPrice<side>(), qty);

    if(qty != 0) {
      _out.push(CreateRejected(orderId, qty));
    }
  }

  template<Side side>
  void update_order(int32_t orderId, int32_t price, int32_t slot, int32_t newQty) noexcept {
    _orderBook.update_order<side>(orderId, price, slot, newQty);
  }

  template<Side side>
  void cancel_order(int32_t orderId, int32_t price, int32_t slot) noexcept {
    _orderBook.cancel_order<side>(orderId, price, slot);
  }

  int32_t min_price() const noexcept {
    return _orderBook.get_min_price();
  }

  int32_t max_price() const noexcept {
    return _orderBook.get_max_price();
  }

  int32_t order_per_level() const noexcept {
    return (int32_t)Level::MaxOrders;
  }

  QueueOut& out() noexcept {
    return _out;
  }

private:
 template<Side side>
  int32_t _trade(int32_t orderId, int32_t priceLimit, int32_t qty) noexcept {
    const auto check_qty = [](int32_t qty) -> bool {
      return qty != 0;
    };

    const auto check_price = [](int32_t price, int32_t priceLimit) -> bool {
      if constexpr(side == Sell) {
        return price >= priceLimit;
      } else {
        return price <= priceLimit;
      }
    };

    const auto check_total = [](int32_t total) -> bool {
      if constexpr(side == Sell) {
        return total > 0;
      } else {
        return total < 0;
      }
    };

    int32_t price = _orderBook.get_best_price<-side>();
    priceLimit = _orderBook.price_limit<side>(priceLimit);

    while(check_qty(qty) && check_price(price, priceLimit)) {
      Level& get_level = _orderBook.get_level(price);
      _orderBook.set_best_price<side>(price);

      while(check_qty(qty) && check_total(get_level.total)) {
        Order& order = get_level.orders.front();
        const int32_t min = std::min(qty, order.qty);

        qty -= min;
        order.qty -= min;
        get_level.total += side * min;

        _out.push(Trade(price, min, orderId, order.id));

        if(order.qty == 0) {
          order.id = 0;
          get_level.orders.pop();
        }
      }

      price += side * 1;
    }

    return qty;
  }

private:
  QueueOut& _out;
  OrderBook _orderBook;
};
