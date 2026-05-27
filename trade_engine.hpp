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

#include "assert.hpp"
#include "common.hpp"
#include "events.hpp"
#include "flat_queue.hpp"
#include "timer.hpp"


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
  static constexpr int32_t InvalidId = 0;
  
  static constexpr int32_t MinPrice = 1;
  static constexpr int32_t MaxPrice = 128 * 1024 * 1024;

  template<Side side>
  static constexpr int32_t AnyPrice = (side == Sell) ? MinPrice : MaxPrice;

  static constexpr int32_t MinQty = 1;
  static constexpr int32_t MaxQty = 32 * 1024 * 1024;
  
  int32_t id{Order::InvalidId};
  Qty qty{0};
};

/*
 * Level
 */

struct Level final: noncopyable, nonmovable {
  static constexpr Index MaxOrders = 8;

  Qty total{0};
  FlatQueue<Order, (int8_t)MaxOrders> orders;
};

/*
 * OrderBook
 */

struct OrderBook final: noncopyable, nonmovable {
public:
  static constexpr Index Size = 256;
  static constexpr Index Levels = ((Size / 2 - 1) / 4) * 4; // 124

public:
  explicit OrderBook(QueueOut& out, Price centerPrice = Levels + 1)
    : _out(out) 
  {
     centerPrice = (((centerPrice - 1) / 4) * 4) + 1;
    _minIndex = 0;
    _minPrice = std::max(centerPrice - Levels, Order::MinPrice);
    _maxPrice = std::min(_minPrice + Levels + Levels, Order::MaxPrice);

    _bestSellPrice = _maxPrice + 1;
    _bestBuyPrice = _minPrice - 1;

    Assert(_bestSellPrice > _bestBuyPrice);
    Assert(_bestSellPrice <= _maxPrice + 1);
    Assert(_bestBuyPrice >= _minPrice - 1);
  }

public:
  bool check_price(Price price) const noexcept {
    return price >= get_min_price() && price <= get_max_price();
  }

  bool check_qty(Qty qty) const noexcept {
    return (qty >= Order::MinQty) && (qty <= Order::MaxQty);
  }

  Price get_min_price() const noexcept {
    return _minPrice;
  }

  Price get_max_price() const noexcept {
    return _maxPrice;
  }

  Price get_center_price() const noexcept {
    return (_minPrice + _maxPrice) / 2;
  }

  template<Side side>
  Price price_limit(Price priceLimit) const noexcept {
    if constexpr(side == Sell) {
      const Price price = get_min_price();
      return std::max(priceLimit, price);
    } else {
      const Price price = get_max_price();
      return std::min(priceLimit, price);
    }
  }

  template<Side side>
  Price get_best_price() const noexcept {
    if constexpr(side == Sell) {
      return _bestSellPrice;
    } else {
      return _bestBuyPrice;
    }
  }

  template<Side side>
  void update_best_price(bool condition) noexcept {
    if constexpr(side == Sell) {
      _bestSellPrice += 1 * (Price)condition;
    } else {
      _bestBuyPrice -= 1 * (Price)condition;
    }

    Assert(_bestSellPrice > _bestBuyPrice);
    Assert(_bestSellPrice <= _maxPrice + 1);
    Assert(_bestBuyPrice >= _minPrice - 1);
  }

  Level& get_level_by_price(Price price) {
    Assert(check_price(price));
    return get_level_by_index(_minIndex + (price - _minPrice));
  }

  Level& get_level_by_index(Index index) {
    return _levels[index & (Size - 1)];
  }

  template<Side side>
  void insert_order(OrderId orderId, Price price, Qty qty) noexcept {
    if(check_price(price) == false) {
      return _out.push(CreateRejected(orderId, qty));
    }

    Level& level = get_level_by_price(price);

    if constexpr(side == Sell) {
      Assert(level.total <= 0);
    } else {
      Assert(level.total >= 0);
    }

    if(level.orders.full()) {
      return _out.push(CreateRejected(orderId, qty));
    }

    const Index index = level.orders.push(Order{orderId, qty});

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

    Assert(_bestSellPrice > _bestBuyPrice);
    Assert(_bestSellPrice <= _maxPrice + 1);
    Assert(_bestBuyPrice >= _minPrice - 1);
  }

  template<Side side>
  void update_order(OrderId orderId, Price price, Index slot, Qty newQty) noexcept {
    if(check_price(price) == false) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    Level& level = get_level_by_price(price);
    Order& order = level.orders.at(slot);

    if(order.id != orderId) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if constexpr(side == Sell) {
      level.total += order.qty;
      level.total -= newQty;
    } else {
      level.total -= order.qty;
      level.total += newQty;
    }

    order.qty = newQty;
    _out.push(UpdateAccepted(orderId));
  }

  template<Side side>
  void cancel_order(OrderId orderId, Price price, Index slot) noexcept {
    if(check_price(price) == false) {
      return _out.push(CancelRejected(orderId));
    }

    Level& level = get_level_by_price(price);
    Order& order = level.orders.at(slot);

    if(order.id != orderId) {
      return _out.push(CancelRejected(orderId));
    }

    if constexpr(side == Sell) {
      level.total += order.qty;
    } else {
      level.total -= order.qty;
    }

    order.id = 0;
    order.qty = 0;
    level.orders.remove(slot);
    _out.push(CancelAccepted(orderId));
  }

  void shiftUp() {
    Assert(_maxPrice != Order::MaxPrice);

    Level& level = get_level_by_price(_minPrice);
    _expire_level(level);

    _minIndex += 1;
    _minPrice += 1;
    _maxPrice += 1;

    _bestSellPrice = std::max(_bestSellPrice, _minPrice);
    _bestBuyPrice = std::max(_bestBuyPrice, _minPrice - 1);

    Assert(_bestSellPrice > _bestBuyPrice);
    Assert(_bestSellPrice <= _maxPrice + 1);
    Assert(_bestBuyPrice >= _minPrice - 1);
  }

  void shiftDown() {
    Assert(_minPrice != Order::MinPrice);

    Level& level = get_level_by_price(_maxPrice);
    _expire_level(level);

    _minIndex -= 1;
    _minPrice -= 1;
    _maxPrice -= 1;

    _bestSellPrice = std::min(_bestSellPrice, _maxPrice + 1);
    _bestBuyPrice = std::min(_bestBuyPrice, _maxPrice);

    Assert(_bestSellPrice > _bestBuyPrice);
    Assert(_bestSellPrice <= _maxPrice + 1);
    Assert(_bestBuyPrice >= _minPrice - 1);
  }

private:
  void _expire_level(Level& level) {
    while(level.orders.empty() == false) {
      const Order& order = level.orders.front();
      _out.push(OrderExpired(order.id));
      level.orders.pop();
    }

    level.total = 0;
  }

private:
  Index _minIndex;
  Price _minPrice;

  Price _bestSellPrice;
  Price _bestBuyPrice;

  Price _maxPrice;

  QueueOut& _out;
  Level _levels[Size];
};

/*
 * TradeEngine
 */

struct TradeEngine final: noncopyable, nonmovable {
  explicit TradeEngine(QueueOut& out, Price centerPrice = OrderBook::Levels + 1)
    : _out(out)
    , _orderBook(out, centerPrice) 
  {
  }

public:
  template<Side side>
  void insert_order(OrderId orderId, Price price, Qty qty) noexcept {
#ifndef NDEBUG
    if(orderId == Order::InvalidId) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(price < Order::MinPrice) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(price > Order::MaxPrice) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(qty < Order::MinQty) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(qty > Order::MaxQty) {
      return _out.push(CreateRejected(orderId, qty));
    }
#endif

    qty = _trade<side>(orderId, price, qty);

    if(qty != 0) {
      _orderBook.insert_order<side>(orderId, price, qty);
    }
  }

  template<Side side>
  void insert_order_ioc(OrderId orderId, Price price, Qty qty) noexcept {
#ifndef NDEBUG
    if(orderId == Order::InvalidId) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(price < Order::MinPrice) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(price > Order::MaxPrice) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(qty < Order::MinQty) {
      return _out.push(CreateRejected(orderId, qty));
    }

    if(qty > Order::MaxQty) {
      return _out.push(CreateRejected(orderId, qty));
    }
#endif

    qty = _trade<side>(orderId, price, qty);

    if(qty != 0) {
      _out.push(CreateRejected(orderId, qty));
    }
  }

  template<Side side>
  void insert_order_ioc(OrderId orderId, Qty qty) noexcept {
    const Price anyPrice = Order::AnyPrice<side>;
    insert_order_ioc<side>(orderId, anyPrice, qty);
  }

  template<Side side>
  void update_order(OrderId orderId, Price price, Index slot, Qty newQty) noexcept {
#ifndef NDEBUG
    if(orderId == Order::InvalidId) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if(price < Order::MinPrice) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if(price > Order::MaxPrice) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if(slot < 0) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if(slot >= Level::MaxOrders) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if(newQty < Order::MinQty) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if(newQty > Order::MaxQty) {
      return _out.push(UpdateRejected(orderId, newQty));
    }
#endif

    _orderBook.update_order<side>(orderId, price, slot, newQty);
  }

  template<Side side>
  void cancel_order(OrderId orderId, Price price, Index slot) noexcept {
#ifndef NDEBUG
    if(orderId == Order::InvalidId) {
      return _out.push(CancelRejected(orderId));
    }

    if(price < Order::MinPrice) {
      return _out.push(CancelRejected(orderId));
    }

    if(price > Order::MaxPrice) {
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

  Price min_price() const noexcept {
    return _orderBook.get_min_price();
  }

  Price center_price() const noexcept {
    return _orderBook.get_center_price();
  }

  Price max_price() const noexcept {
    return _orderBook.get_max_price();
  }  
  
  Index orders_per_level() const noexcept {
    return Level::MaxOrders;
  }
  
  QueueOut& out() noexcept {
    return _out;
  }

private:
  template<Side side>
  Qty _trade(OrderId orderId, Price priceLimit, Qty qty) noexcept {
    const auto check_price = [](Price price, Price priceLimit) -> bool {
      if constexpr(side == Sell) {
        return price >= priceLimit;
      } else {
        return price <= priceLimit;
      }
    };

    Price price = _orderBook.get_best_price<-side>();
    Price lastPrice = _orderBook.get_center_price();

    priceLimit = _orderBook.price_limit<side>(priceLimit);

    while((qty != 0) && check_price(price, priceLimit)) {
      Level& level = _orderBook.get_level_by_price(price);
      Assert((level.total * side) <= 0);

      while((qty != 0) && (level.total != 0)) {
        Order& order = level.orders.front();
        const Qty min = std::min(qty, order.qty);
        
        qty -= min;
        order.qty -= min;
        level.total += side * min;
        lastPrice = price;
        _out.push(Trade(price, min, orderId, order.id));

        if(UNLIKELY(order.qty != 0)) {
          continue;
        }

        order.id = 0;
        level.orders.pop();
        _orderBook.update_best_price<-side>(level.orders.empty());
      }

      price += side;
    }

    _shiftUp(lastPrice);
    _shiftDown(lastPrice);
    return qty;
  }

  void _shiftUp(Price lastPrice) {
    const Price offset = _orderBook.get_center_price() - lastPrice;

    if(offset <= 0) {
      return;
    }

    const Price space = Order::MaxPrice - _orderBook.get_max_price();
    const Price shift = std::min(offset, space);

    for(Price i = 0; i < shift; i++) {
      _orderBook.shiftUp();
    }
  }

  void _shiftDown(Price lastPrice) {
    const Price offset = lastPrice - _orderBook.get_center_price();

    if(offset <= 0) {
      return;
    }

    const Price space = _orderBook.get_min_price() - Order::MinPrice;
    const Price shift = std::min(offset, space);

    for(Price i = 0; i < shift; i++) {
      _orderBook.shiftDown();
    }
  }

private:
  QueueOut& _out;
  OrderBook _orderBook;
};
