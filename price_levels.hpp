#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <immintrin.h>

#include "./array.hpp"
#include "./assert.hpp"
#include "./branchless.hpp"
#include "./events.hpp"
#include "./flat_list.hpp"
#include "./likely.hpp"
#include "./order.hpp"
#include "./ring_buffer_spsc.hpp"

template<uint32_t _Orders>
struct PriceLevel
{
  constexpr static uint32_t Orders = _Orders;

public:
  PriceLevel() = default;
  PriceLevel(PriceLevel&&) = delete;
  PriceLevel(const PriceLevel&) = delete;

  PriceLevel& operator=(PriceLevel&&) = delete;
  PriceLevel& operator=(const PriceLevel&) = delete;

public:
  const Order& order() const
  {
    return _orders.front();
  }

  Index insert(OrderId orderId, Qty qty)
  {
    const Index index = _orders.push_back(Order(orderId, qty));

    if(index != InvalidIndex) {
      _balance += qty;
    }

    return index;
  }

  bool update(OrderId orderId, Index slot, Qty newQty)
  {
    Order& order = _orders.at_slot(slot);

    if (UNLIKELY(order.id() != orderId)) {
      return false;
    }

    _balance -= order.qty();
    order.update(newQty);
    _balance += order.qty();

    return true;
  }

  bool cancel(OrderId orderId, Index slot)
  {
    Order& order = _orders.at_slot(slot);

    if (UNLIKELY(order.id() != orderId)) {
      return false;
    }

    _balance -= order.qty();
    order.clear();
    _orders.remove(slot);

    return true;
  }

  void expire_front() 
  {
    Order& order = _orders.front();
    _balance -= order.qty();
    order.clear();
    _orders.pop_front();
  }
  
  void trade_front(Qty qty)
  {
    Order& order = _orders.front();
    order.trade(qty);
    _balance -= qty;

    if(order.qty() == 0) {
      order.clear();
      _orders.pop_front();
    }
  }

  bool empty() const
  {
    return _orders.empty();
  }

  void reset() 
  {
    _balance = 0;
    
    while(! _orders.empty()) {
      _orders.pop_back();
    }
  }

  Qty balance() const
  {
    return _balance;
  }

private:
  Qty _balance{0};
  FlatList<Order, Orders> _orders;
};

template<uint32_t _LevelsBelow, uint32_t _LevelsAbove, uint32_t _Orders = 32>
struct PriceLevels
{
  constexpr static uint32_t LevelsBelow = _LevelsBelow;
  constexpr static uint32_t LevelsAbove = _LevelsAbove;
  constexpr static uint32_t Orders = _Orders;
  constexpr static uint32_t Mask = LevelsBelow + LevelsAbove;

public:
  PriceLevels(Price centerPrice)
    : _centerIndex(LevelsBelow)
  {
    Assert(centerPrice > LevelsBelow);
    Assert(centerPrice < Order::MaxPrice - LevelsAbove);

    _minPrice = centerPrice - LevelsBelow;
    _maxPrice = centerPrice + LevelsAbove;
  }

  PriceLevels(PriceLevels&&) = delete;
  PriceLevels(const PriceLevels&) = delete;

  PriceLevels& operator=(PriceLevels&&) = delete;
  PriceLevels& operator=(const PriceLevels&) = delete;

public:
  Price center_price() const
  {
    return _minPrice + LevelsBelow;
  }

  Price min_price() const
  {
    return _minPrice;
  }

  Price max_price() const
  {
    return _maxPrice;
  }

  bool check_price(Price price) const
  {
    return bl::in_range(price, _minPrice, _maxPrice);
  }

  PriceLevel<Orders>& at_index(Index index)
  {
    return _levels[index];
  }

  const PriceLevel<Orders>& at_index(Index index) const
  {
    return _levels[index];
  }

  PriceLevel<Orders>& at_price(Price price)
  {
    const Index index = price_to_index(price);
    return _levels[index];
  }

  const PriceLevel<Orders>& at_price(Price price) const
  {
    const Index index = price_to_index(price);
    return _levels[index];
  }

  Index price_to_index(Price price) const
  {
    Assert(check_price(price));

    price -= (_minPrice + LevelsBelow);
    price += _centerIndex;
    price &= Mask;

    return ((Index)price);
  }

  void shift_up(QueueOut& out)
  {
    Assert(max_price() < Order::MaxPrice);

    _minPrice += 1;
    _maxPrice += 1;
    _centerIndex += 1;
    _centerIndex &= Mask;
  }

  void shift_down(QueueOut& out)
  {
    Assert(min_price() > 1);

    _minPrice -= 1;
    _maxPrice -= 1;
    _centerIndex -= 1;
    _centerIndex &= Mask;
  }

  void reset(Price centerPrice)
  {
    for(auto& level : _levels.data()) {
      level.reset();
    }

    Assert(centerPrice > LevelsBelow);
    Assert(centerPrice < Order::MaxPrice - LevelsAbove);

    _centerIndex = LevelsBelow;
    _minPrice = centerPrice - LevelsBelow;
    _maxPrice = centerPrice + LevelsAbove;
  }

private:
  Price _minPrice;
  Price _maxPrice;
  Index _centerIndex;
  Index __padding;

  Array<PriceLevel<Orders>, LevelsBelow + LevelsAbove + 1> _levels;
};
