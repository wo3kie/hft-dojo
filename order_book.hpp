#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "./array.hpp"
#include "./assert.hpp"
#include "./branchless.hpp"
#include "./events.hpp"
#include "./flat_list.hpp"
#include "./likely.hpp"
#include "./price_levels.hpp"
#include "./ring_buffer_spsc.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <limits>
#include <utility>

template<uint32_t Levels>
struct OrderBook
{
  static_assert((Levels & (Levels + 1)) == 0);

  Price _sellTopPrice;
  Price _buyTopPrice;

  PriceLevels<Levels> _sellLevels;
  PriceLevels<Levels> _buyLevels;

  RingBufferSPSC<Event, 1024>& _bufferOut;

  OrderBook(RingBufferSPSC<Event, 1024>& bufferOut, Price centerPrice)
    : _sellTopPrice{InvalidPrice}
    , _buyTopPrice{InvalidPrice}
    , _buyLevels(centerPrice)
    , _sellLevels(centerPrice)
    , _bufferOut(bufferOut)
  {
  }

  Price centerPrice() const
  {
    return _buyLevels.centerPrice();
  }

  void emitEvent(Event event)
  {
    while(_bufferOut.push(event) == false) {
    }
  }

  Price buyPriceFrom() const
  {
    return _buyTopPrice;
  }

  Price buyPriceTo(Price price) const
  {
    Price min_price = _buyLevels.minPrice();
    Price max_price = bl::max(price, min_price);
    return max_price;
  }

  Index buyIndexFrom() const
  {
    Index index = _buyLevels.priceToIndex(_buyTopPrice);
    return index;
  }

  Price sellPriceFrom() const
  {
    return _sellTopPrice;
  }

  Price sellPriceTo(Price price) const
  {
    Price maxPrice = _sellLevels.maxPrice();
    Price minPrice = bl::min(price, maxPrice);
    return minPrice;
  }

  Index sellIndexFrom() const
  {
    Index index = _sellLevels.priceToIndex(_sellTopPrice);
    return index;
  }

  bool checkSellPrice(Price price) const
  {
    Price minPrice = _sellLevels.minPrice();
    Price maxPrice = _sellLevels.maxPrice();

    return bl::in_range<Price>(price, minPrice, maxPrice);
  }

  bool checkBuyPrice(Price price) const
  {
    Price minPrice = _buyLevels.minPrice();
    Price maxPrice = _buyLevels.maxPrice();

    return bl::in_range(price, minPrice, maxPrice);
  }

  void insertSellOrder(OrderId orderId, Price price, Qty qty)
  {
    PriceLevel& level = _sellLevels.price(price);
    Index slot = level.orders.push_back({orderId, qty});

    if(UNLIKELY(slot == InvalidIndex)) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    emitEvent(CreateAccepted(orderId, slot));

    auto zero_to_max = [](Price x) -> Price {
      return x | -Price(x == 0);
      ;
    };

    _sellTopPrice = bl::min(zero_to_max(_sellTopPrice), price);
  }

  void insertBuyOrder(OrderId orderId, Price price, Qty qty)
  {
    PriceLevel& level = _buyLevels.price(price);
    Index slot = level.orders.push_back({orderId, qty});

    if(UNLIKELY(slot == InvalidIndex)) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    emitEvent(CreateAccepted(orderId, slot));

    _buyTopPrice = bl::max(_buyTopPrice, price);
  }

  void updateSellOrder(OrderId orderId, Index slot, Price price, Qty qty)
  {
    PriceLevel& level = _sellLevels.price(price);
    Order& order = level.orders.at(slot);

    if(UNLIKELY(order.id != orderId)) {
      return emitEvent(UpdateRejected(orderId, qty));
    }

    order.qty = qty;
    emitEvent(UpdateAccepted(orderId));
  }

  void updateBuyOrder(OrderId orderId, Index slot, Price price, Qty qty)
  {
    PriceLevel& level = _buyLevels.price(price);
    Order& order = level.orders.at(slot);

    if(UNLIKELY(order.id != orderId)) {
      return emitEvent(UpdateRejected(orderId, qty));
    }

    order.qty = qty;
    emitEvent(UpdateAccepted(orderId));
  }

  void cancelSellOrder(OrderId orderId, Index slot, Price price)
  {
    PriceLevel& level = _sellLevels.price(price);
    Order& order = level.orders.at(slot);

    if(UNLIKELY(order.id != orderId)) {
      return emitEvent(CancelRejected(orderId));
    }

    order.id = InvalidOrderId;
    level.orders.pop_back();
    emitEvent(CancelAccepted(orderId));
  }

  void cancelBuyOrder(OrderId orderId, Index slot, Price price)
  {
    PriceLevel& level = _buyLevels.price(price);
    Order& order = level.orders.at(slot);

    if(UNLIKELY(order.id != orderId)) {
      return emitEvent(CancelRejected(orderId));
    }

    order.id = InvalidOrderId;
    level.orders.pop_back();
    emitEvent(CancelAccepted(orderId));
  }

  void shiftUp()
  {
    {
      Price minPrice = _buyLevels.minPrice();
      PriceLevel& level = _buyLevels.price(minPrice);
      expireOrders(level);
      _buyLevels.shiftUp();
    }

    {
      Price minPrice = _sellLevels.minPrice();
      PriceLevel& level = _sellLevels.price(minPrice);
      expireOrders(level);
      _sellLevels.shiftUp();
    }
  }

  void shiftDown()
  {
    {
      Price maxPrice = _buyLevels.maxPrice();
      PriceLevel& level = _buyLevels.price(maxPrice);
      expireOrders(level);
      _buyLevels.shiftDown();
    }

    {
      Price maxPrice = _sellLevels.maxPrice();
      PriceLevel& level = _sellLevels.price(maxPrice);
      expireOrders(level);
      _sellLevels.shiftDown();
    }
  }

  void expireOrders(PriceLevel& level)
  {
    while(level.orders.empty() == false) {
      Order& order = level.orders.front();

      emitEvent(OrderExpired(order.id));

      order.id = InvalidOrderId;
      level.orders.pop_front();
    }
  }
};
