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

class Orders {
public:
  static constexpr int8_t SENTINEL = 8;

  Orders() {
    for(int8_t i = 0; i < 8 + 1; i += 1) {
      _buffer[i]._value.id = 0;
      _buffer[i]._value.qty = 0;
      _buffer[i]._next = SENTINEL;
      _buffer[i]._prev = SENTINEL;
    }
  }

  bool empty() const noexcept {
    return _size == 0;
  }

  bool full() const noexcept {
    return _size == 8;
  }

  Order& front() noexcept {
    return _buffer[_buffer[SENTINEL]._next]._value;
  }

  bool insert(int32_t id, int32_t qty, int8_t slot) noexcept {
    if(_buffer[slot]._value.id != 0) {
      return false;
    }

    _buffer[slot]._value.id = id;
    _buffer[slot]._value.qty = qty;

    const int8_t tail = _buffer[SENTINEL]._prev;

    _buffer[slot]._next = SENTINEL;
    _buffer[slot]._prev = tail;

    _buffer[tail]._next = slot;
    _buffer[SENTINEL]._prev = slot;

    _size += 1;
    return true;
  }

  bool update(int32_t id, int32_t& oldQty, int32_t newQty, int8_t slot) noexcept {
    if(_buffer[slot]._value.id != id) {
      return false;
    }

    oldQty = _buffer[slot]._value.qty;
    _buffer[slot]._value.qty = newQty;

    return true;
  }

  bool cancel(int32_t id, int32_t& oldQty, int8_t slot) noexcept {
    if(_buffer[slot]._value.id != id) {
      return false;
    }

    oldQty = _buffer[slot]._value.qty;

    const int8_t prev = _buffer[slot]._prev;
    const int8_t next = _buffer[slot]._next;

    _buffer[prev]._next = next;
    _buffer[next]._prev = prev;

    _buffer[slot]._value.id = 0;
    _buffer[slot]._value.qty = 0;
    _buffer[slot]._next = SENTINEL;
    _buffer[slot]._prev = SENTINEL;

    _size -= 1;
    return true;
  }

  void pop() noexcept {
    const int8_t slot = _buffer[SENTINEL]._next;
    const int8_t next = _buffer[slot]._next;

    _buffer[SENTINEL]._next = next;
    _buffer[next]._prev = SENTINEL;

    _buffer[slot]._value.id = 0;
    _buffer[slot]._value.qty = 0;
    _buffer[slot]._next = SENTINEL;
    _buffer[slot]._prev = SENTINEL;

    _size -= 1;
  }

  int8_t head() const noexcept {
    return _buffer[SENTINEL]._next;
  }

  int8_t tail() const noexcept {
    return _buffer[SENTINEL]._prev;
  }

private:
  struct _Node {
    Order _value;
    int8_t _next;
    int8_t _prev;
  };

  int8_t _size{0};
  _Node _buffer[8 + /* sentinel */ 1]; 
};

/*
 * Level
 */

struct Level final: noncopyable, nonmovable {
  Level() {
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

  template<Side side>
  bool push(int32_t id, int32_t qty, int32_t slot) noexcept {
    if(_buffer.insert(id, qty, slot) == false) {
      return false;
    }

    if constexpr(side == Sell) {
      total -= qty;
    } else {
      total += qty;
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
  bool update(int32_t id, int32_t& oldQty, int32_t newQty, int32_t slot) noexcept {
    if(_buffer.update(id, oldQty, newQty, slot)) {
      if constexpr(side == Sell) {
        total += oldQty;
        total -= newQty;
      } else {
        total -= oldQty;
        total += newQty;
      }

      return true;
    }

    return false;
  }

  template<Side side>
  bool update(int32_t id, int32_t& oldQty, int32_t newQty) noexcept {
    for(int32_t iter = 0, slot = id & 7; iter < 8; iter += 1, slot = (slot + 3) & 7) {
      if(update<side>(id, oldQty, newQty, slot)) {
        return true;
      }
    }

    return false;
  }

  template<Side side>
  bool cancel(int32_t id, int32_t& oldQty, int32_t slot) noexcept {
    if(_buffer.cancel(id, oldQty, slot)) {
      if constexpr(side == Sell) {
        total += oldQty;
      } else {
        total -= oldQty;
      }

      return true;
    }

    return false;
  }

  template<Side side>
  bool cancel(int32_t id, int32_t& oldQty) noexcept {
    for(int32_t iter = 0, slot = id & 7; iter < 8; iter += 1, slot = (slot + 3) & 7) {
      if(cancel<side>(id, oldQty, slot)) {
        return true;
      }
    }

    return false;
  }

  void pop() noexcept {
    _buffer.pop();
  }

  static constexpr Index MaxOrders = 8;

  Qty total{0};
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
      assert(level.total <= 0);
    } else {
      assert(level.total >= 0);
    }

    if(level.full()) {
      return _out.push(CreateRejected(id, qty));
    }

    const Index index = level.push<side>(id, qty);

    if constexpr(side == Sell) {
      _bestSellPrice = std::min(_bestSellPrice, price);
      _bestBuyPrice = std::min(_bestBuyPrice, _bestSellPrice - 1);
    } else {
      _bestBuyPrice = std::max(_bestBuyPrice, price);
      _bestSellPrice = std::max(_bestSellPrice, _bestBuyPrice + 1);
    }

    _out.push(CreateAccepted(id, index, qty));

    assert(_bestSellPrice > _bestBuyPrice);
    assert(_bestSellPrice <= _maxPrice + 1);
    assert(_bestBuyPrice >= _minPrice - 1);
  }

  template<Side side, Slot hasSlot = NoSlot>
  void update_order(OrderId id, Price price, Qty newQty, Index slot = -1) noexcept {
    if(_check_price(price) == false) {
      return _out.push(UpdateRejected(id, newQty));
    }

    bool updated;
    int32_t oldQty;
    Level& level = get_level_by_price(price);

    if (side * level.total <= 0) {
      return _out.push(UpdateRejected(id, newQty));
    }

    if constexpr(hasSlot == NoSlot) {
      updated = level.update<side>(id, oldQty, newQty);
    } else {
      updated = level.update<side>(id, oldQty, newQty, slot);
    }

    if(updated == false) {
      return _out.push(UpdateRejected(id, newQty));
    }

    _out.push(UpdateAccepted(id));
  }

  template<Side side, Slot hasSlot = NoSlot>
  void cancel_order(OrderId id, Price price, Index slot = -1) noexcept {
    if(_check_price(price) == false) {
      return _out.push(CancelRejected(id));
    }

    bool canceled;
    int32_t oldQty;
    Level& level = get_level_by_price(price);

    if (side * level.total <= 0) {
      return _out.push(CancelRejected(id));
    }

    if constexpr(hasSlot == NoSlot) {
      canceled = level.cancel<side>(id, oldQty);
    } else {
      canceled = level.cancel<side>(id, oldQty, slot);
    }

    if(canceled == false) {
      return _out.push(CancelRejected(id));
    }

    _out.push(CancelAccepted(id));
  }

  void shift(Price lastPrice) noexcept {
    const int32_t diff = lastPrice - get_center_price();

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
    for(level.total = 0; level.empty() == false; level.pop()) {
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
    , _orderBook(out, centerPrice) {
  }

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
  void insert_order(OrderId id, Price price, Qty qty) noexcept {
#ifndef NDEBUG
    if(! (_check_order_id(id) && _check_price(price) && _check_qty(qty))) {
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
    if(! (_check_order_id(id) && _check_price(price) && _check_qty(qty))) {
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
  void update_order(OrderId id, Price price, Qty newQty, Index slot) noexcept {
#ifndef NDEBUG
    if(! (_check_order_id(id) && _check_price(price) && _check_slot(slot) && _check_qty(newQty))) {
      return _out.push(UpdateRejected(id, newQty));
    }
#endif

    _orderBook.update_order<side, HasSlot>(id, price, newQty, slot);
  }

  template<Side side>
  void update_order(OrderId id, Price price, Qty newQty) noexcept {
#ifndef NDEBUG
    if(! (_check_order_id(id) && _check_price(price) && _check_qty(newQty))) {
      return _out.push(UpdateRejected(id, newQty));
    }
#endif

    _orderBook.update_order<side, NoSlot>(id, price, newQty);
  }

  template<Side side>
  void cancel_order(OrderId id, Price price, Index slot) noexcept {
#ifndef NDEBUG
    if(! (_check_order_id(id) && _check_price(price) && _check_slot(slot))) {
      return _out.push(CancelRejected(id));
    }
#endif

    _orderBook.cancel_order<side, HasSlot>(id, price, slot);
  }

  template<Side side>
  void cancel_order(OrderId id, Price price) noexcept {
#ifndef NDEBUG
    if(! (_check_order_id(id) && _check_price(price))) {
      return _out.push(CancelRejected(id));
    }
#endif

    _orderBook.cancel_order<side, NoSlot>(id, price);
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

    priceLimit = _orderBook.price_limit<side>(priceLimit);

    while((qty != 0) && check_price(price, priceLimit)) {
      Level& level = _orderBook.get_level_by_price(price);
      assert((level.total * side) <= 0);

      while((qty != 0) && (level.total != 0)) {
        Order& order = level.front();
        const Qty min = std::min(qty, order.qty);

        qty -= min;
        order.qty -= min;
        level.total += side * min;

        centerPrice = price;
        _out.push(Trade(price, min, id, order.id));

        if(UNLIKELY(order.qty != 0)) {
          continue;
        }

        order.id = 0;
        level.pop();
        _orderBook.update_best_price<-side>(level.empty());
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
