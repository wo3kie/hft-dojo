#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "assert.hpp"
#include "common.hpp"
#include "events.hpp"
#include "flat_queue.hpp"

#include "orders.hpp"

/*
 * Level
 */

struct Level final: noncopyable, nonmovable {
  static constexpr Index MaxOrders = 8;

  Qty total{0};
  Orders8 orders;
};

/*
 * OrderBook
 */

struct OrderBook final: noncopyable, nonmovable {
private:
  static constexpr Index Size = 256;

public:  
  static constexpr Index Shift = 8;
  static constexpr Index Levels = ((Size / 2 - 1) / Shift) * Shift; // 124
    
  explicit OrderBook(QueueOut& out, Price centerPrice = Levels + 1)
    : _out(out) 
  {
     centerPrice = (((centerPrice - 1) / Shift) * Shift) + 1;
    _minIndex = 0;
    _minPrice = std::max(centerPrice - Levels, Order::MinPrice);
    _maxPrice = std::min(_minPrice + Levels + Levels, Order::MaxPrice);

    _bestSellPrice = _maxPrice + 1;
    _bestBuyPrice = _minPrice - 1;

    Assert(_bestSellPrice > _bestBuyPrice);
    Assert(_bestSellPrice <= _maxPrice + 1);
    Assert(_bestBuyPrice >= _minPrice - 1);
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
      return std::max(priceLimit, _minPrice);
    } else {
      return std::min(priceLimit, _maxPrice);
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
  void update_best_price(bool condition = true) noexcept {
    if constexpr(side == Sell) {
      _bestSellPrice += 1 * (Price)condition;
    } else {
      _bestBuyPrice -= 1 * (Price)condition;
    }

    Assert(_bestSellPrice > _bestBuyPrice);
    Assert(_bestSellPrice <= _maxPrice + 1);
    Assert(_bestBuyPrice >= _minPrice - 1);
  }

  Level& get_level_by_price(Price price) noexcept {
    Assert(_check_price(price));
    return get_level_by_index(_minIndex + (price - _minPrice));
  }

  Level& get_level_by_index(Index index) noexcept {
    return _levels[index & (Size - 1)];
  }

  template<Side side>
  void insert_order(OrderId orderId, Price price, Qty qty) noexcept {
    if(_check_price(price) == false) {
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

    const Index index = level.orders.push(orderId & 7, orderId, qty);

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
    if(_check_price(price) == false) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    int32_t oldQty = 0;
    Level& level = get_level_by_price(price);

    if (level.orders.update(slot, orderId, oldQty, newQty) == false) {
      return _out.push(UpdateRejected(orderId, newQty));
    }

    if constexpr(side == Sell) {
      level.total += oldQty;
      level.total -= newQty;
    } else {
      level.total -= oldQty;
      level.total += newQty;
    }

    _out.push(UpdateAccepted(orderId));
  }

  template<Side side>
  void cancel_order(OrderId orderId, Price price, Index slot) noexcept {
    if(_check_price(price) == false) {
      return _out.push(CancelRejected(orderId));
    }

    int32_t oldQty = 0;
    Level& level = get_level_by_price(price);
    
    if (level.orders.cancel(slot, orderId, oldQty) == false) {
      return _out.push(CancelRejected(orderId));
    }

    if constexpr(side == Sell) {
      level.total += oldQty;
    } else {
      level.total -= oldQty;
    }

    _out.push(CancelAccepted(orderId));
  }

  void shift(Price lastPrice) noexcept {
    const int32_t diff = lastPrice - get_center_price();

    if(diff > OrderBook::Shift) {
      if (_maxPrice + Shift <= Order::MaxPrice) {
        _shiftUp();
      }
    }

    if(diff < -OrderBook::Shift) {
      if (_minPrice - Shift >= Order::MinPrice) {
        _shiftDown();
      }
    }  

    Assert(_bestSellPrice > _bestBuyPrice);
    Assert(_bestSellPrice <= _maxPrice + 1);
    Assert(_bestBuyPrice >= _minPrice - 1);
    Assert(_minPrice >= Order::MinPrice);
    Assert(_maxPrice <= Order::MaxPrice);
  }

private:
  bool _check_price(Price price) const noexcept {
    return (price >= _minPrice) && (price <= _maxPrice);
  }

  bool _check_qty(Qty qty) const noexcept {
    return (qty >= Order::MinQty) && (qty <= Order::MaxQty);
  }

  void _expire_levels(Level& level, Price price) noexcept {
    for(level.total = 0; level.orders.empty() == false; level.orders.pop()) {
      _out.push(LevelExpired(price, level.orders.front().id));
    }
  }

  template<Side side>
  void _expire_levels(Price fromPrice, Price toPrice) noexcept {
    for(Price price = fromPrice; price != toPrice + side; price += side) {
      _expire_levels(get_level_by_price(price), price);
    }
  }

  void _create_levels(Price fromPrice, Price toPrice) noexcept {
    _out.push(LevelsCreated(fromPrice, toPrice));
  }

  void _shiftUp() noexcept {
    _expire_levels<Buy>(_minPrice, _minPrice + (Shift - 1));
    _minIndex += Shift;
    _minPrice += Shift;
    _maxPrice += Shift;
    _create_levels(_maxPrice - (Shift - 1), _maxPrice);

    _bestSellPrice = std::max(_bestSellPrice, _minPrice);
    _bestBuyPrice = std::max(_bestBuyPrice, _minPrice - 1);
  }

  void _shiftDown() noexcept {
    _expire_levels<Sell>(_maxPrice, _maxPrice - (Shift - 1));
    _minIndex -= Shift;
    _minPrice -= Shift;
    _maxPrice -= Shift;
    _create_levels(_minPrice + (Shift - 1), _minPrice);

    _bestSellPrice = std::min(_bestSellPrice, _maxPrice + 1);
    _bestBuyPrice = std::min(_bestBuyPrice, _maxPrice);
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
  static constexpr Index Levels = OrderBook::Levels;
  static constexpr Index OrdersPerLevel = Level::MaxOrders; 

  explicit TradeEngine(QueueOut& out, Price centerPrice = OrderBook::Levels + 1)
    : _out(out)
    , _orderBook(out, centerPrice) 
  {
  }

  bool _check_order_id(OrderId orderId) const noexcept {
    return orderId != Order::InvalidId;
  }

  bool _check_price(Price price) const noexcept {
    return (price >= Order::MinPrice) && (price <= Order::MaxPrice);
  }

  bool _check_qty(Qty qty) const noexcept {
    return (qty >= Order::MinQty) && (qty <= Order::MaxQty);
  }

  bool _check_slot(Index slot) const noexcept {
    return (slot >= 0) && (slot < OrdersPerLevel);
  }

  template<Side side>
  void insert_order(OrderId orderId, Price price, Qty qty) noexcept {
#ifndef NDEBUG
    if(!(_check_order_id(orderId) && _check_price(price) && _check_qty(qty))) {
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
    if(!(_check_order_id(orderId) && _check_price(price) && _check_qty(qty))) {
      return _out.push(CreateRejected(orderId, qty));
    }
#endif

    qty = _trade<side>(orderId, price, qty);

    if(UNLIKELY(qty != 0)) {
      _out.push(CreateRejected(orderId, qty));
    }
  }

  template<Side side>
  void insert_mkt_order_ioc(OrderId orderId, Qty qty) noexcept {
    insert_order_ioc<side>(orderId, (side == Sell) ? Order::MinPrice : Order::MaxPrice, qty);
  }

  template<Side side>
  void update_order(OrderId orderId, Price price, Index slot, Qty newQty) noexcept {
#ifndef NDEBUG
    if (!(_check_order_id(orderId) && _check_price(price) && _check_slot(slot) && _check_qty(newQty))) {
      return _out.push(UpdateRejected(orderId, newQty));
    }
  #endif

    _orderBook.update_order<side>(orderId, price, slot, newQty);
  }

  template<Side side>
  void cancel_order(OrderId orderId, Price price, Index slot) noexcept {
#ifndef NDEBUG
    if (!(_check_order_id(orderId) && _check_price(price) && _check_slot(slot))) {
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
    Price centerPrice = _orderBook.get_center_price();

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
        
        centerPrice = price;
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

    _orderBook.shift(centerPrice);
    return qty;
  }

private:
  QueueOut& _out;
  OrderBook _orderBook;
};
