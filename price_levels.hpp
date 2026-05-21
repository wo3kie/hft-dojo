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
    return orders.front();
  }

  Index insert(OrderId orderId, Qty qty)
  {
    const Index index = orders.push_back(Order(orderId, qty));

    if(index != InvalidIndex) {
      balance += qty;
    }

    return index;
  }

  bool update(OrderId orderId, Index slot, Qty newQty)
  {
    Order& order = orders.at_slot(slot);

    if (UNLIKELY(order.id() != orderId)) {
      return false;
    }

    balance -= order.qty();
    order.update(newQty);
    balance += order.qty();

    return true;
  }

  bool cancel(OrderId orderId, Index slot)
  {
    Order& order = orders.at_slot(slot);

    if (UNLIKELY(order.id() != orderId)) {
      return false;
    }

    balance -= order.qty();
    order.clear();
    orders.remove(slot);

    return true;
  }

  OrderId expire() 
  {
    if (orders.empty()) {
      return Order::InvalidId;
    }

    Order& order = orders.front();
    const OrderId orderId = order.id();
    balance -= order.qty();
    order.clear();
    orders.pop_front();
    return orderId;
  }
  
  void trade(Qty qty)
  {
    Order& order = orders.front();

    balance -= qty;
    order.trade(qty);

    if(order.qty() == 0) {
      order.clear();
      orders.pop_front();
    }
  }

  bool empty() const
  {
    return orders.empty();
  }

  void reset() 
  {
    balance = 0;
    
    while(! orders.empty()) {
      orders.pop_back();
    }
  }

  int32_t balance{0};
  FlatList<Order, Orders> orders;
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
    Assert(centerPrice < MaxPrice - LevelsAbove);

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
    Assert(max_price() < MaxPrice);

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
    Assert(centerPrice < MaxPrice - LevelsAbove);

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
