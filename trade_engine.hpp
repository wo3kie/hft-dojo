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
#include <thread>

#include "array.hpp"
#include "assert.hpp"
#include "common.hpp"
#include "events.hpp"
#include "flat_queue.hpp"
#include "timer.hpp"

/*
 * QueueOut
 */

struct QueueOut {
  void push(const Event& event) noexcept {
    for(int i = 0; i < 8; i++) {
      if(_queue.push(event)) {
        return;
      } else {
        _mm_pause();
      }
    }

    for(;;) {
      if(_queue.push(event)) {
        return;
      } else {
        std::this_thread::yield();
      }
    }
  }

  Event pop() noexcept {
    return _queue.pop();
  }

  bool empty() const noexcept {
    return _queue.empty_approx();
  }

  void clear() noexcept {
    while(_queue.empty_approx() == false) {
      _queue.pop();
    }
  }

  void log(const std::string& prefix = "") {
    while(_queue.empty_approx() == false) {
      const Event event = _queue.pop();
      std::cout << prefix << event << std::endl;
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
  static constexpr int32_t InvalidId() noexcept {
    return 0;
  }

  static constexpr int32_t MinPrice() noexcept {
    return 1;
  }

  static constexpr int32_t MaxPrice() noexcept {
    return 128 * 1024 * 1024;
  }

  template<Side side>
  static constexpr int32_t AnyPrice() noexcept {
    if constexpr(side == Sell) {
      return Order::MinPrice();
    } else {
      return Order::MaxPrice();
    }
  }

  static constexpr int32_t MinQty() noexcept {
    return 1;
  }

  static constexpr int32_t MaxQty() noexcept {
    return 32 * 1024 * 1024;
  }

  int32_t id{Order::InvalidId()};
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

struct Level final: noncopyable, nonmovable {
  static constexpr int8_t MaxOrders = 8;

  int32_t total{0};
  FlatQueue<Order, MaxOrders> orders;
};

/*
 * OrderBook
 */

struct OrderBook final: noncopyable, nonmovable {
public:
  static constexpr int32_t MaxLevels = 256;
  static_assert((MaxLevels & (MaxLevels - 1)) == 0, "MaxLevels must be a power of 2");

public:
  explicit OrderBook(QueueOut& out, int32_t centerPrice = MaxLevels / 2)
    : _out(out) {
    _minIndex = 0;
    _minPrice = std::max(centerPrice - (MaxLevels / 2) - 1, Order::MinPrice());
    _maxPrice = std::min(_minPrice + (MaxLevels - 1), Order::MaxPrice());

    _bestSellPrice = _maxPrice;
    _bestBuyPrice = _minPrice;
  }

public:
  void _expire_level(Level& level) {
    while(level.orders.empty() == false) {
      const Order& order = level.orders.front();
      _out.push(OrderExpired(order.id));
      level.orders.pop();
    }

    level.total = 0;
  }

  bool shiftUp() {
    assert(_maxPrice != Order::MaxPrice());

    Level& level = get_level_by_price(_minPrice);
    _expire_level(level);

    _minIndex += 1;
    _minPrice += 1;
    _maxPrice += 1;

    _bestSellPrice = std::max(_bestSellPrice, _minPrice);
    _bestBuyPrice = std::max(_bestBuyPrice, _minPrice);

    Assert(check_price(_bestSellPrice));
    Assert(check_price(_bestBuyPrice));

    return true;
  }

  bool shiftDown() {
    assert(_minPrice != Order::MinPrice());

    Level& level = get_level_by_price(_maxPrice);
    _expire_level(level);

    _minIndex -= 1;
    _minPrice -= 1;
    _maxPrice -= 1;

    _bestSellPrice = std::min(_bestSellPrice, _maxPrice);
    _bestBuyPrice = std::min(_bestBuyPrice, _maxPrice);

    Assert(check_price(_bestSellPrice));
    Assert(check_price(_bestBuyPrice));

    return true;
  }

  bool check_price(int32_t price) noexcept {
    return price >= get_min_price() && price <= get_max_price();
  }

  bool check_qty(int32_t qty) noexcept {
    return qty >= Order::MinQty() && qty <= Order::MaxQty();
  }

  int32_t get_min_price() const noexcept {
    return _minPrice;
  }

  int32_t get_max_price() const noexcept {
    return _maxPrice;
  }

  int32_t get_center_price() const noexcept {
    return (_minPrice + _maxPrice) / 2;
  }

  template<Side side>
  int32_t price_limit(int32_t priceLimit) const noexcept {
    if constexpr(side == Sell) {
      const int32_t price = get_min_price();
      return std::max(priceLimit, price);
    } else {
      const int32_t price = get_max_price();
      return std::min(priceLimit, price);
    }
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
  int32_t get_best_index() const noexcept {
    return (_minIndex + (get_best_price<side>() - _minPrice)) & (MaxLevels - 1);
  }

  template<Side side>
  void set_best_price(int32_t price) noexcept {
    if constexpr(side == Sell) {
      _bestSellPrice = price;
    } else {
      _bestBuyPrice = price;
    }

    Assert(_bestSellPrice > _bestBuyPrice);
  }

  Level& get_level_by_price(int32_t price) {
    Assert(check_price(price));
    return _levels[(_minIndex + (price - _minPrice))];
  }

  Level& get_level_by_index(int32_t index) {
    return _levels[index];
  }

  template<Side side>
  void insert_order(int32_t orderId, int32_t price, int32_t qty) noexcept {
    if(check_price(price) == false) {
      return _out.push(CreateRejected(orderId, qty));
    }

    Level& level = get_level_by_price(price);

    if constexpr(side == Sell) {
      assert(level.total <= 0);
    } else {
      assert(level.total >= 0);
    }

    if(level.orders.full()) {
      return _out.push(CreateRejected(orderId, qty));
    }

    const int32_t index = level.orders.push(Order{orderId, qty});

    if constexpr(side == Sell) {
      level.total -= qty;
      _bestSellPrice = std::min(_bestSellPrice, price);
      _bestBuyPrice = std::min(_bestBuyPrice, _bestSellPrice - 1);
    } else {
      level.total += qty;
      _bestBuyPrice = std::max(_bestBuyPrice, price);
      _bestSellPrice = std::max(_bestSellPrice, _bestBuyPrice + 1);
    }

    _out.push(CreateAccepted(orderId, index, qty));

    assert(_bestSellPrice > _bestBuyPrice);
  }

  template<Side side>
  void update_order(int32_t orderId, int32_t price, int32_t slot, int32_t newQty) noexcept {
    if(check_price(price) == false) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    Level& level = get_level_by_price(price);
    Order& order = level.orders.at(slot);

    if(order.id != orderId) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    const int32_t qty = order.qty;

    if constexpr(side == Sell) {
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

    Level& level = get_level_by_price(price);
    Order& order = level.orders.at(slot);

    if(order.id != orderId) {
      return _out.push(CancelRejected(orderId));
    }

    const int32_t qty = order.qty;

    if constexpr(side == Sell) {
      level.total += qty;
    } else {
      level.total -= qty;
    }

    order.id = 0;
    order.qty = 0;
    level.orders.remove(slot);

    _out.push(CancelAccepted(orderId));
  }

private:
  int32_t _minIndex;
  int32_t _minPrice;

  int32_t _bestSellPrice;
  int32_t _bestBuyPrice;

  int32_t _maxPrice;

  QueueOut& _out;
  Array<Level, MaxLevels> _levels;
};

/*
 * TradeEngine
 */

struct TradeEngine final: noncopyable, nonmovable {
  explicit TradeEngine(QueueOut& out, int32_t centerPrice = OrderBook::MaxLevels / 2)
    : _out(out)
    , _orderBook(out, centerPrice) {
  }

public:
  template<Side side>
  void insert_order(int32_t orderId, int32_t price, int32_t qty) noexcept {
#ifndef NDEBUG
    if(orderId == Order::InvalidId()) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(price < Order::MinPrice()) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(price > Order::MaxPrice()) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(qty < Order::MinQty()) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(qty > Order::MaxQty()) {
      return _out.push(CreateRejected(orderId, qty));
    }
#endif

    qty = _trade<side>(orderId, price, qty);

    if(qty != 0) {
      _orderBook.insert_order<side>(orderId, price, qty);
    }
  }

  template<Side side>
  void insert_order(int32_t orderId, int32_t qty) noexcept {
#ifndef NDEBUG
    if(orderId == Order::InvalidId()) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(qty < Order::MinQty()) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(qty > Order::MaxQty()) {
      return _out.push(CreateRejected(orderId, qty));
    }
#endif

    const int32_t anyPrice = Order::AnyPrice<side>();
    qty = _trade<side>(orderId, anyPrice, qty);

    if(qty != 0) {
      _out.push(CreateRejected(orderId, qty));
    }
  }

  template<Side side>
  void update_order(int32_t orderId, int32_t price, int32_t slot, int32_t newQty) noexcept {
#ifndef NDEBUG
    if(orderId == Order::InvalidId()) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if(price < Order::MinPrice()) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if(price > Order::MaxPrice()) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if(slot < 0) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if(slot >= Level::MaxOrders) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if(newQty < Order::MinQty()) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if(newQty > Order::MaxQty()) {
      return _out.push(UpdateRejected(orderId, newQty));
    }
#endif

    _orderBook.update_order<side>(orderId, price, slot, newQty);
  }

  template<Side side>
  void cancel_order(int32_t orderId, int32_t price, int32_t slot) noexcept {
#ifndef NDEBUG
    if(orderId == Order::InvalidId()) {
      return _out.push(CancelRejected(orderId));
    }

    if(price < Order::MinPrice()) {
      return _out.push(CancelRejected(orderId));
    }

    if(price > Order::MaxPrice()) {
      return _out.push(CancelRejected(orderId));
    }

    if(slot < 0) {
      return _out.push(CancelRejected(orderId));
    }

    if(slot >= Level::MaxOrders) {
      return _out.push(CancelRejected(orderId));
    }
#endif

    _orderBook.cancel_order<side>(orderId, price, slot);
  }

  int32_t min_price() const noexcept {
    return _orderBook.get_min_price();
  }

  int32_t max_price() const noexcept {
    return _orderBook.get_max_price();
  }

  int32_t center_price() const noexcept {
    return _orderBook.get_center_price();
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
    const auto check_price = [](int32_t price, int32_t priceLimit) -> bool {
      if constexpr(side == Sell) {
        return price >= priceLimit;
      } else {
        return price <= priceLimit;
      }
    };

    int32_t price = _orderBook.get_best_price<-side>();
    int32_t index = _orderBook.get_best_index<-side>();
    int32_t centerPrice = _orderBook.get_center_price();
    priceLimit = _orderBook.price_limit<side>(priceLimit);

    while((qty != 0) && check_price(price, priceLimit)) {
      Level& level = _orderBook.get_level_by_index(index);
      assert(level.total * side <= 0);

      while((qty != 0) && (level.total != 0)) {
        Order& order = level.orders.front();
        const int32_t min = std::min(qty, order.qty);

        qty -= min;
        order.qty -= min;
        level.total += side * min;
        centerPrice = price;

        _out.push(Trade(price, min, orderId, order.id));

        if(order.qty != 0) {
          continue;
        }

        order.id = 0;
        level.orders.pop();

        if(level.orders.empty()) {
          _orderBook.set_best_price<-side>(price + side);
        }
      }

      price += side * 1;
      index += side * 1;
    }

    _shiftUp(centerPrice);
    _shiftDown(centerPrice);

    return qty;
  }

  void _shiftUp(int32_t centerPrice) {
    const int32_t offset = _orderBook.get_center_price() - centerPrice;

    if(offset <= 0) {
      return;
    }

    const int32_t space = Order::MaxPrice() - _orderBook.get_max_price();
    const int32_t shift = std::min(offset, space);

    for(int32_t i = 0; i < shift; i++) {
      _orderBook.shiftUp();
    }
  }

  void _shiftDown(int32_t centerPrice) {
    const int32_t offset = centerPrice - _orderBook.get_center_price();

    if(offset <= 0) {
      return;
    }

    const int32_t space = _orderBook.get_min_price() - Order::MinPrice();
    const int32_t shift = std::min(offset, space);

    for(int32_t i = 0; i < shift; i++) {
      _orderBook.shiftDown();
    }
  }

private:
  QueueOut& _out;
  OrderBook _orderBook;
};
