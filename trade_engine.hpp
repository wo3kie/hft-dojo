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
#include "branchless.hpp"
#include "common.hpp"
#include "events.hpp"
#include "flat_queue_oa.hpp"

/*
 * PriceBits
 */

void set_price_bit(uint256_t& mask, uint8_t price) noexcept {
  mask.data[price >> 7] |= (uint128_t)1 << (price & 127);
}

void clear_price_bit(uint256_t& mask, uint8_t price) noexcept {
  mask.data[price >> 7] &= ~((uint128_t)1 << (price & 127));
}

uint8_t get_price_bit(uint256_t mask) noexcept {
  assert(mask != uint256_t(0));
  return 256 - 1 - clz256(mask);
}

/*
 * Order
 */

struct Order {
  OrderId id{InvalidOrderId};
  Qty qty{0};
};

/*
 * Orders
 */

class Orders: noncopyable, nonmovable {
public:
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

    if(UNLIKELY(order.id != 0)) {
      return false;
    }

    _buffer.insert(slot, Order{id, qty});
    return true;
  }

  bool update(int32_t id, int32_t newQty, int8_t slot) noexcept {
    Order& order = _buffer[slot];

    if(UNLIKELY(order.id != id)) {
      return false;
    }

    order.qty = newQty;

    return true;
  }

  bool cancel(int32_t id, int8_t slot) noexcept {
    Order& order = _buffer[slot];

    if(UNLIKELY(order.id != id)) {
      return false;
    }

    order.id = 0;
    _buffer.remove(slot);

    return true;
  }

  void pop() noexcept {
    Order& order = _buffer.front();
    assert(order.id != 0);

    order.id = 0;
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
    assert(_buffer.empty() == (get_total() == 0));
    return (get_total() == 0);
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
    for(int32_t iter = 0, slot = id & 7; iter < 8; iter += 1, slot = (slot + 3) & 7) {
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

    if(UNLIKELY(_buffer.update(id, newQty, slot) == false)) {
      return false;
    }
    
    if constexpr(side == Sell) {
      _total += oldQty;
      _total -= newQty;
    } else {
      _total -= oldQty;
      _total += newQty;
    }

    return true;
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

    if(UNLIKELY(_buffer.cancel(id, slot) == false)) {
      return false;
    }

    if constexpr(side == Sell) {
      _total += oldQty;
    } else {
      _total -= oldQty;
    }

    return true;
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
    _minPrice = bl::max(centerPrice - Levels, MinPrice);
    _maxPrice = bl::min(_minPrice + Levels + Levels, MaxPrice);

    _sellPricesMask = 0;
    _buyPricesMask = 0;
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
  Price get_bottom_price(Price price = (side == Sell ? MinPrice : MaxPrice)) const noexcept {
    if constexpr(side == Sell) {
      return bl::max(price, _minPrice);
    } else {
      return bl::min(price, _maxPrice);
    }
  }

  template<Side side>
  Price get_top_price() const noexcept {
    if constexpr(side == Sell) {
      if(_sellPricesMask != uint256_t(0)) {
        return _maxPrice - ::get_price_bit(_sellPricesMask);
      } else {
        return _maxPrice + 1;
      }
    } else {
      if(_buyPricesMask != uint256_t(0)) {
        return _minPrice + ::get_price_bit(_buyPricesMask);
      } else {
        return _minPrice - 1;
      }
    }
  }

  template<Side side>
  void clear_price_bit(Price price) noexcept {
    if constexpr(side == Sell) {
      ::clear_price_bit(_sellPricesMask, _maxPrice - price);
    } else {
      ::clear_price_bit(_buyPricesMask, price - _minPrice);
    }
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
    if(UNLIKELY(_check_price(price) == false)) {
      return _out.push(CreateRejected(id, qty, Reason::Price));
    }

    Level& level = get_level_by_price(price);
    assert(side * level.get_total() >= 0);

    if(UNLIKELY(level.full())) {
      return _out.push(CreateRejected(id, qty, Reason::Buffer));
    }

    const Index slot = level.push<side>(id, qty);

    if constexpr(side == Sell) {
      ::set_price_bit(_sellPricesMask, _maxPrice - price);
    } else {
      ::set_price_bit(_buyPricesMask, price - _minPrice);
    }

    _out.push(CreateAccepted(id, slot, qty));
  }

  template<Side side, Slot hasSlot = NoSlot>
  void update_order(OrderId id, Price price, Qty newQty, Index slot = -1) noexcept {
    if(UNLIKELY(_check_price(price) == false)) {
      return _out.push(UpdateRejected(id, newQty, Reason::Price));
    }

    Level& level = get_level_by_price(price);

    if(UNLIKELY(side * level.get_total() <= 0)) {
      return _out.push(UpdateRejected(id, newQty, Reason::Level));
    }

    const bool updated = (hasSlot == NoSlot)
                       ? level.update<side>(id, newQty) 
                       : level.update<side>(id, newQty, slot);

    if(UNLIKELY(updated == false)) {
      return _out.push(UpdateRejected(id, newQty, Reason::Slot));
    }

    _out.push(UpdateAccepted(id));
  }

  template<Side side, Slot hasSlot = NoSlot>
  void cancel_order(OrderId id, Price price, Index slot = -1) noexcept {
    if(UNLIKELY(_check_price(price) == false)) {
      return _out.push(CancelRejected(id, Reason::Price));
    }

    Level& level = get_level_by_price(price);

    if(UNLIKELY(side * level.get_total() <= 0)) {
      return _out.push(CancelRejected(id, Reason::Level));
    }

    const bool canceled = (hasSlot == NoSlot)
                        ? level.cancel<side>(id) 
                        : level.cancel<side>(id, slot);

    if(UNLIKELY(canceled == false)) {
      return _out.push(CancelRejected(id, Reason::Slot));
    }

    if(UNLIKELY(level.empty())) {
      if constexpr(side == Sell) {
        ::clear_price_bit(_sellPricesMask, _maxPrice - price);
      } else {
        ::clear_price_bit(_buyPricesMask, price - _minPrice);
      }
    }

    _out.push(CancelAccepted(id));
  }

  void shift(Price lastPrice) noexcept {
    const int32_t diff = lastPrice - get_center_price();

    if(diff > OrderBook::Shift) {
      if(_maxPrice + Shift <= MaxPrice) {
        return _shift_up();
      }
    }

    if(diff < -OrderBook::Shift) {
      if(_minPrice - Shift >= MinPrice) {
        return _shift_down();
      }
    }
  }

private:
  bool _check_price(Price price) const noexcept {
    return (price >= _minPrice) && (price <= _maxPrice);
  }

  bool _check_qty(Qty qty) const noexcept {
    return (qty >= MinQty) && (qty <= MaxQty);
  }

  template<Side side>
  void _expire_levels(Level& level, Price price) noexcept {
    for(/* empty */; level.empty() == false; level.pop()) {
      const Order& order = level.front();
      const int32_t id = order.id;
      const int32_t qty = order.qty;

      /*
       * Keep invariant: _buffer.empty() == (get_total() == 0)
       */

      level.set_total(level.get_total() - side * qty);
      _out.push(LevelExpired(price, id));
    }
  }

  template<Side side>
  void _expire_levels(Price fromPrice, Price toPrice) noexcept {
    for(Price price = fromPrice; price != toPrice + side; price += side) {
      _expire_levels<side>(get_level_by_price(price), price);
    }
  }

  void _create_levels(Price fromPrice, Price toPrice) noexcept {
    _out.push(LevelsCreated(fromPrice, toPrice));
  }

  void _shift_up() noexcept {
    _expire_levels<Buy>(_minPrice, _minPrice + (Shift - 1));
    _minIndex += Shift;
    _minPrice += Shift;
    _maxPrice += Shift;
    _sellPricesMask.shl<Shift>();
    _buyPricesMask.shr<Shift>();
    _create_levels(_maxPrice - (Shift - 1), _maxPrice);
  }

  void _shift_down() noexcept {
    _expire_levels<Sell>(_maxPrice, _maxPrice - (Shift - 1));
    _minIndex -= Shift;
    _minPrice -= Shift;
    _maxPrice -= Shift;
    _sellPricesMask.shr<Shift>();
    _buyPricesMask.shl<Shift>();
    _create_levels(_minPrice, _minPrice + (Shift - 1));
  }

private:
  Index _minIndex;
  Index ____padd0;

  Price _minPrice;
  Price _maxPrice;

  uint256_t _sellPricesMask;
  uint256_t _buyPricesMask;

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
      return _out.push(CreateRejected(id, qty, Reason::Request));
    }
#endif

    qty = _trade<side>(id, price, qty);

    if(UNLIKELY(qty != 0)) {
      _orderBook.insert_order<side>(id, price, qty);
    }
  }

  template<Side side>
  void insert_order_ioc(OrderId id, Price price, Qty qty) noexcept {
#ifndef NDEBUG
    if(UNLIKELY(! (_check_order_id(id) && _check_price(price) && _check_qty(qty)))) {
      return _out.push(CreateRejected(id, qty, Reason::Request));
    }
#endif

    qty = _trade<side>(id, price, qty);

    if(UNLIKELY(qty != 0)) {
      _out.push(CreateRejected(id, qty, Reason::IOC));
    }
  }

  template<Side side>
  void insert_order_fok(OrderId id, Price priceLimit, Qty qty) noexcept {
#ifndef NDEBUG
    if(UNLIKELY(! (_check_order_id(id) && _check_price(priceLimit) && _check_qty(qty)))) {
      return _out.push(CreateRejected(id, qty, Reason::Request));
    }
#endif

    if(UNLIKELY(_check_fok<side>(priceLimit, qty) == false)) {
      return _out.push(CreateRejected(id, qty, Reason::FOK));
    }

    qty = _trade<side>(id, priceLimit, qty);
    assert(qty == 0);
  }

  template<Side side>
  void insert_mkt_order_ioc(OrderId id, Qty qty) noexcept {
    insert_order_ioc<side>(id, (side == Sell) ? MinPrice : MaxPrice, qty);
  }

  template<Side side>
  void insert_mkt_order_fok(OrderId id, Qty qty) noexcept {
    insert_order_fok<side>(id, (side == Sell) ? MinPrice : MaxPrice, qty);
  }

  template<Side side>
  void update_order(OrderId id, Price price, Qty newQty) noexcept {
#ifndef NDEBUG
    if(UNLIKELY(! (_check_order_id(id) && _check_price(price) && _check_qty(newQty)))) {
      return _out.push(UpdateRejected(id, newQty, Reason::Request));
    }
#endif

    _orderBook.update_order<side, NoSlot>(id, price, newQty);
  }

  template<Side side>
  void update_order(OrderId id, Price price, Qty newQty, Index slot) noexcept {
#ifndef NDEBUG
    if(UNLIKELY(! (_check_order_id(id) && _check_price(price) && _check_slot(slot) && _check_qty(newQty)))) {
      return _out.push(UpdateRejected(id, newQty, Reason::Request));
    }
#endif

    _orderBook.update_order<side, HasSlot>(id, price, newQty, slot);
  }

  template<Side side>
  void cancel_order(OrderId id, Price price) noexcept {
#ifndef NDEBUG
    if(UNLIKELY(! (_check_order_id(id) && _check_price(price)))) {
      return _out.push(CancelRejected(id, Reason::Request));
    }
#endif

    _orderBook.cancel_order<side, NoSlot>(id, price);
  }

  template<Side side>
  void cancel_order(OrderId id, Price price, Index slot) noexcept {
#ifndef NDEBUG
    if(UNLIKELY(! (_check_order_id(id) && _check_price(price) && _check_slot(slot)))) {
      return _out.push(CancelRejected(id, Reason::Request));
    }
#endif

    _orderBook.cancel_order<side, HasSlot>(id, price, slot);
  }

  QueueOut& out() noexcept {
    return _out;
  }

private:
  bool _check_order_id(OrderId id) const noexcept {
    return id != InvalidOrderId;
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
  bool _check_fok(Price priceLimit, Qty qty) noexcept {
    const auto check_price = [](Price price, Price priceLimit) -> bool {
      if constexpr(side == Sell) {
        return price >= priceLimit;
      } else {
        return price <= priceLimit;
      }
    };
    
    priceLimit = _orderBook.get_bottom_price<side>(priceLimit);
    Price price = _orderBook.get_top_price<-side>();
    
    do {
      if(check_price(price, priceLimit) == false) {
        break;
      }

      Level& level = _orderBook.get_level_by_price(price);
      assert((side * level.get_total()) <= 0);
      
      qty += side * level.get_total();
      price += side;
    } while(qty > 0);
    
    return (qty <= 0);
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
    
    priceLimit = _orderBook.get_bottom_price<side>(priceLimit);
    Price lastPrice = _orderBook.get_center_price();

    do {
      const Price price = _orderBook.get_top_price<-side>();

      if(check_price(price, priceLimit) == false) {
        break;
      }

      Level& level = _orderBook.get_level_by_price(price);
      assert((side * level.get_total()) < 0);

      do {
        Order& order = level.front();
        const Qty min = bl::min(qty, order.qty);

        qty -= min;
        order.qty -= min;
        lastPrice = price;
        level.set_total(level.get_total() + side * min);
        _out.push(Trade(price, min, id, order.id));

        if(order.qty == 0) {
          level.pop();

          if (level.empty()){
            _orderBook.clear_price_bit<-side>(price);
            break;
          }
        }
      } while(qty != 0);
    } while(qty != 0);

    _orderBook.shift(lastPrice);
    return qty;
  }

private:
  QueueOut& _out;
  OrderBook _orderBook;
};
