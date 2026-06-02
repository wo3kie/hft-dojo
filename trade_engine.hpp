#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cassert>
#include <cstdint>

#include "assert.hpp"
#include "common.hpp"
#include "events.hpp"
#include "flat_queue_oa.hpp"

/*
 * Side
 */

typedef int8_t Side;
constexpr Side Sell = -1;
constexpr Side Buy = 1;

/*
 * Slot
 */

typedef bool Slot;
constexpr Slot NoSlot = false;
constexpr Slot HasSlot = true;

/*
 * Constants
 */

static constexpr Price MinPrice = 1;
static constexpr Price MaxPrice = 128 * 1024 * 1024;

static constexpr Qty MinQty = 1;
static constexpr Qty MaxQty = 64 * 1024 * 1024;

/*
 * Order
 */

struct Order {
  static constexpr OrderId InvalidId = 0;

  OrderId id{Order::InvalidId};
  Qty qty{0};
};

/*
 * Orders
 */

class Orders : noncopyable, nonmovable {
public:
  static constexpr int8_t SENTINEL = 8;

  Orders() {
    for(int8_t i = 0; i < 8; i += 1) {
      Order& order = _buffer[i];
      order.id = 0;
      order.qty = 0;
    }
  }

  bool empty() const noexcept {
    return _buffer.empty();
  }

  bool full() const noexcept {
    return _buffer.full();
  }

  Order& front() noexcept {
    return _buffer.front();
  }

  bool insert(int32_t id, int32_t qty, int8_t slot) noexcept {
    Order& order = _buffer[slot];

    if(order.id != 0) {
      return false;
    }

    _buffer.insert(slot, Order{id, qty});
    return true;
  }

  bool update(int32_t id, int32_t newQty, int8_t slot) noexcept {
    Order& order = _buffer[slot];

    if(order.id != id) {
      return false;
    }

    order.qty = newQty;

    return true;
  }

  bool cancel(int32_t id, int8_t slot) noexcept {
    Order& order = _buffer[slot];

    if(order.id != id) {
      return false;
    }

    order.id = 0;
    order.qty = 0;
    _buffer.remove(slot);

    return true;
  }

  void pop() noexcept {
    Order& order = _buffer.front();
    assert(order.id != 0);

    order.id = 0;
    order.qty = 0;
    _buffer.pop();
  }

  const Order& operator[](int32_t slot) const noexcept {
    return _buffer[slot];
  }

private:
  FlatQueue_OA<Order, 8> _buffer;
};

/*
 * Level
 */

struct Level final: noncopyable, nonmovable {
  static constexpr Index MaxOrders = 8;

  Level() {
  }

  bool empty() const noexcept {
    return _buffer.empty();
  }

  bool full() const noexcept {
    return _buffer.full();
  }

  Qty get_total() const noexcept {
    return _total;
  }

  void set_total(Qty total) noexcept {
    _total = total;
  }

  Order& front() noexcept {
    return _buffer.front();
  }

  template<Side side>
  bool push(int32_t id, int32_t qty, int32_t slot) noexcept {
    if(_buffer.insert(id, qty, slot) == false) {
      return false;
    }

    if constexpr(side == Sell) {
      _total -= qty;
    } else {
      _total += qty;
    }

    return true;
  }

  template<Side side>
  int32_t push(int32_t id, int32_t qty) noexcept {
    for(int32_t iter = 0,slot = id & 7; iter < 8; iter += 1, slot = (slot + 3) & 7) {
      if(push<side>(id, qty, slot)) {
        return slot;
      }
    }

    assert(false);
    return -1;
  }

  template<Side side>
  bool update(int32_t id, int32_t newQty, int32_t slot) noexcept {
    const int32_t oldQty = _buffer[slot].qty;

    if(_buffer.update(id, newQty, slot)) {
      if constexpr(side == Sell) {
        _total += oldQty;
        _total -= newQty;
      } else {
        _total -= oldQty;
        _total += newQty;
      }

      return true;
    }

    return false;
  }

  template<Side side>
  bool update(int32_t id, int32_t newQty) noexcept {
    for(int32_t iter = 0, slot = id & 7; iter < 8; iter += 1, slot = (slot + 3) & 7) {
      if(update<side>(id, newQty, slot)) {
        return true;
      }
    }

    return false;
  }

  template<Side side>
  bool cancel(int32_t id, int32_t slot) noexcept {
    const int32_t oldQty = _buffer[slot].qty;

    if(_buffer.cancel(id, slot)) {
      if constexpr(side == Sell) {
        _total += oldQty;
      } else {
        _total -= oldQty;
      }

      return true;
    }

    return false;
  }

  template<Side side>
  bool cancel(int32_t id) noexcept {
    for(int32_t iter = 0, slot = id & 7; iter < 8; iter += 1, slot = (slot + 3) & 7) {
      if(cancel<side>(id, slot)) {
        return true;
      }
    }

    return false;
  }

  void pop() noexcept {
    _buffer.pop();
  }

private:
  Qty _total{0};
  Orders _buffer;
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
    _minPrice = std::max(centerPrice - Levels, MinPrice);
    _maxPrice = std::min(_minPrice + Levels + Levels, MaxPrice);
    _centerPrice = (_minPrice + _maxPrice) / 2;

    _bestSellPrice = _maxPrice + 1;
    _bestBuyPrice = _minPrice - 1;

    assert(_bestSellPrice > _bestBuyPrice);
    assert(_bestSellPrice <= _maxPrice + 1);
    assert(_bestBuyPrice >= _minPrice - 1);
  }

  Price get_min_price() const noexcept {
    return _minPrice;
  }

  Price get_max_price() const noexcept {
    return _maxPrice;
  }

  Price get_center_price() const noexcept {
    return _centerPrice;
  }

  template<Side side>
  Price get_worst_price(Price price = (side == Sell ? MinPrice : MaxPrice)) const noexcept {
    if constexpr(side == Sell) {
      return std::max(price, _minPrice);
    } else {
      return std::min(price, _maxPrice);
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

    assert(_bestSellPrice > _bestBuyPrice);
    assert(_bestSellPrice <= _maxPrice + 1);
    assert(_bestBuyPrice >= _minPrice - 1);
  }

  Level& get_level_by_price(Price price) noexcept {
    assert(_check_price(price));
    return get_level_by_index(_minIndex + (price - _minPrice));
  }

  Level& get_level_by_index(Index index) noexcept {
    return _levels[index & (Size - 1)];
  }

  template<Side side>
  void insert_order(OrderId id, Price price, Qty qty) noexcept {
    if(_check_price(price) == false) {
      return _out.push(CreateRejected(id, qty));
    }

    Level& level = get_level_by_price(price);

    if constexpr(side == Sell) {
      assert(level.get_total() <= 0);
    } else {
      assert(level.get_total() >= 0);
    }

    if(level.full()) {
      return _out.push(CreateRejected(id, qty));
    }

    const Index slot = level.push<side>(id, qty);

    if constexpr(side == Sell) {
      _bestSellPrice = std::min(_bestSellPrice, price);
      _bestBuyPrice = std::min(_bestBuyPrice, _bestSellPrice - 1);
    } else {
      _bestBuyPrice = std::max(_bestBuyPrice, price);
      _bestSellPrice = std::max(_bestSellPrice, _bestBuyPrice + 1);
    }

    _out.push(CreateAccepted(id, slot, qty));

    assert(_bestSellPrice > _bestBuyPrice);
    assert(_bestSellPrice <= _maxPrice + 1);
    assert(_bestBuyPrice >= _minPrice - 1);
  }

  template<Side side, Slot hasSlot = NoSlot>
  void update_order(OrderId id, Price price, Qty newQty, Index slot = -1) noexcept {
    if(_check_price(price) == false) {
      return _out.push(UpdateRejected(id, newQty));
    }

    Level& level = get_level_by_price(price);
    
    if (side * level.get_total() <= 0) {
      return _out.push(UpdateRejected(id, newQty));
    }

    const bool updated = (hasSlot == NoSlot) 
                        ? level.update<side>(id, newQty) 
                        : level.update<side>(id, newQty, slot);

    if(UNLIKELY(updated == false)) {
      return _out.push(UpdateRejected(id, newQty));
    }

    _out.push(UpdateAccepted(id));
  }

  template<Side side, Slot hasSlot = NoSlot>
  void cancel_order(OrderId id, Price price, Index slot = -1) noexcept {
    if(_check_price(price) == false) {
      return _out.push(CancelRejected(id));
    }

    Level& level = get_level_by_price(price);
    
    if (side * level.get_total() <= 0) {
      return _out.push(CancelRejected(id));
    }

    const bool canceled = (hasSlot == NoSlot) 
                        ? level.cancel<side>(id) 
                        : level.cancel<side>(id, slot);

    if(UNLIKELY(canceled == false)) {
      return _out.push(CancelRejected(id));
    }

    if constexpr(side == Sell) {
      const bool empty = level.empty();
      const bool top = price == _bestSellPrice;
      _bestSellPrice += 1 * empty * top;
    } else {
      const bool empty = level.empty();
      const bool top = price == _bestBuyPrice;
      _bestBuyPrice -= 1 * empty * top;
    }

    _out.push(CancelAccepted(id));
  }

  void shift(Price lastPrice) noexcept {
    const int32_t diff = lastPrice - _centerPrice;

    if(diff > OrderBook::Shift) {
      if(_maxPrice + Shift <= MaxPrice) {
        _shiftUp();
      }
    }

    if(diff < -OrderBook::Shift) {
      if(_minPrice - Shift >= MinPrice) {
        _shiftDown();
      }
    }

    assert(_bestSellPrice > _bestBuyPrice);
    assert(_bestSellPrice <= _maxPrice + 1);
    assert(_bestBuyPrice >= _minPrice - 1);
    assert(_minPrice >= MinPrice);
    assert(_maxPrice <= MaxPrice);
  }

private:
  bool _check_price(Price price) const noexcept {
    return (price >= _minPrice) && (price <= _maxPrice);
  }

  bool _check_qty(Qty qty) const noexcept {
    return (qty >= MinQty) && (qty <= MaxQty);
  }

  void _expire_levels(Level& level, Price price) noexcept {
    for(level.set_total(0); level.empty() == false; level.pop()) {
      _out.push(LevelExpired(price, level.front().id));
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
    _centerPrice += Shift;
    _create_levels(_maxPrice - (Shift - 1), _maxPrice);

    _bestSellPrice = std::max(_bestSellPrice, _minPrice);
    _bestBuyPrice = std::max(_bestBuyPrice, _minPrice - 1);
  }

  void _shiftDown() noexcept {
    _expire_levels<Sell>(_maxPrice, _maxPrice - (Shift - 1));
    _minIndex -= Shift;
    _minPrice -= Shift;
    _maxPrice -= Shift;
    _centerPrice -= Shift;
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
  Price _centerPrice;

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
    , _orderBook(out, centerPrice) {
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

  template<Side side>
  void insert_order(OrderId id, Price price, Qty qty) noexcept {
#ifndef NDEBUG
    if(UNLIKELY(! (_check_order_id(id) && _check_price(price) && _check_qty(qty)))) {
      return _out.push(CreateRejected(id, qty));
    }
#endif

    qty = _trade<side>(id, price, qty);

    if(qty != 0) {
      _orderBook.insert_order<side>(id, price, qty);
    }
  }

  template<Side side>
  void insert_order_ioc(OrderId id, Price price, Qty qty) noexcept {
#ifndef NDEBUG
    if(UNLIKELY(! (_check_order_id(id) && _check_price(price) && _check_qty(qty)))) {
      return _out.push(CreateRejected(id, qty));
    }
#endif

    qty = _trade<side>(id, price, qty);

    if(UNLIKELY(qty != 0)) {
      _out.push(CreateRejected(id, qty));
    }
  }

  template<Side side>
  void insert_mkt_order_ioc(OrderId id, Qty qty) noexcept {
    insert_order_ioc<side>(id, (side == Sell) ? MinPrice : MaxPrice, qty);
  }

  template<Side side>
  void update_order(OrderId id, Price price, Qty newQty) noexcept {
#ifndef NDEBUG
    if(UNLIKELY(! (_check_order_id(id) && _check_price(price) && _check_qty(newQty)))) {
      return _out.push(UpdateRejected(id, newQty));
    }
#endif

    _orderBook.update_order<side, NoSlot>(id, price, newQty);
  }

  template<Side side>
  void update_order(OrderId id, Price price, Qty newQty, Index slot) noexcept {
#ifndef NDEBUG
    if(UNLIKELY(! (_check_order_id(id) && _check_price(price) && _check_slot(slot) && _check_qty(newQty)))) {
      return _out.push(UpdateRejected(id, newQty));
    }
#endif

    _orderBook.update_order<side, HasSlot>(id, price, newQty, slot);
  }

  template<Side side>
  void cancel_order(OrderId id, Price price) noexcept {
#ifndef NDEBUG
    if(UNLIKELY(! (_check_order_id(id) && _check_price(price)))) {
      return _out.push(CancelRejected(id));
    }
#endif

    _orderBook.cancel_order<side, NoSlot>(id, price);
  }

  template<Side side>
  void cancel_order(OrderId id, Price price, Index slot) noexcept {
#ifndef NDEBUG
    if(UNLIKELY(! (_check_order_id(id) && _check_price(price) && _check_slot(slot)))) {
      return _out.push(CancelRejected(id));
    }
#endif

    _orderBook.cancel_order<side, HasSlot>(id, price, slot);
  }

  QueueOut& out() noexcept {
    return _out;
  }

private:
  bool _check_order_id(OrderId id) const noexcept {
    return id != Order::InvalidId;
  }

  bool _check_price(Price price) const noexcept {
    return (price >= MinPrice) && (price <= MaxPrice);
  }

  bool _check_qty(Qty qty) const noexcept {
    return (qty >= MinQty) && (qty <= MaxQty);
  }

  bool _check_slot(Index slot) const noexcept {
    return (slot >= 0) && (slot < OrdersPerLevel);
  }

  template<Side side>
  Qty _trade(OrderId id, Price priceLimit, Qty qty) noexcept {
    const auto check_price = [](Price price, Price priceLimit) -> bool {
      if constexpr(side == Sell) {
        return price >= priceLimit;
      } else {
        return price <= priceLimit;
      }
    };

    Price price = _orderBook.get_best_price<-side>();
    Price centerPrice = _orderBook.get_center_price();

    priceLimit = _orderBook.get_worst_price<side>(priceLimit);

    while(check_price(price, priceLimit)) {
      Level& level = _orderBook.get_level_by_price(price);
      assert((level.get_total() * side) <= 0);

      while(level.get_total() != 0) {
        Order& order = level.front();
        const Qty min = std::min(qty, order.qty);

        qty -= min;
        order.qty -= min;
        level.set_total(level.get_total() + side * min);

        centerPrice = price;
        _out.push(Trade(price, min, id, order.id));

        if(UNLIKELY(order.qty == 0)) {
          level.pop();
          _orderBook.update_best_price<-side>(level.empty());
        }

        if(UNLIKELY(qty == 0)) {
          _orderBook.shift(centerPrice);
          return qty;
        }
      }

      price += side;
    }

    return qty;
  }

private:
  QueueOut& _out;
  OrderBook _orderBook;
};
