#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "./flat_list.hpp"
#include "./assert.hpp"
#include "./ring_buffer_spsc.hpp"
#include "./branchless.hpp"
#include "./likely.hpp"
#include "./array.hpp"
#include "./price_levels.hpp"
#include "./events.hpp"

#include <cassert>
#include <array>
#include <chrono>
#include <iostream>
#include <limits>
#include <functional>
#include <utility>
#include <cassert>
#include <iostream>
#include <limits>

template<int32_t Levels>
struct OrderBook
{
  static_assert((Levels & (Levels + 1)) == 0);

  int32_t _sellTopPrice;
  int32_t _buyTopPrice;

  PriceLevels<Levels> _sellLevels;
  PriceLevels<Levels> _buyLevels;

  RingBufferSPSC<Event, 1024>& _bufferOut;

  OrderBook(RingBufferSPSC<Event, 1024>& bufferOut, int32_t centerPrice)
    : _sellTopPrice{-1}
    , _buyTopPrice{-1}
    , _buyLevels(centerPrice)
    , _sellLevels(centerPrice)
    , _bufferOut(bufferOut)
  {
  }

  int32_t centerPrice() const
  {
    return _buyLevels.centerPrice();
  }

  void emitEvent(Event event)
  {
    while(_bufferOut.push(event) == false) {
    }
  }

  int32_t buyPriceFrom() const
  {
    return _buyTopPrice;
  }

  int32_t buyPriceTo(int32_t price) const
  {
    int32_t min_price = _buyLevels.minPrice();
    int32_t max_price = bl::max(price, min_price);
    return max_price;
  }

  int32_t buyIndexFrom() const
  {
    int32_t index = _buyLevels.priceToIndex(_buyTopPrice);
    return index;
  }

  int32_t sellPriceFrom() const
  {
    return _sellTopPrice;
  }

  int32_t sellPriceTo(int32_t price) const
  {
    int32_t maxPrice = _sellLevels.maxPrice();
    int32_t minPrice = bl::min(price, maxPrice);
    return minPrice;
  }

  int32_t sellIndexFrom() const
  {
    int32_t index = _sellLevels.priceToIndex(_sellTopPrice);
    return index;
  }

  bool checkSellPrice(int32_t price) const
  {
    int32_t minPrice = _sellLevels.minPrice();
    int32_t maxPrice = _sellLevels.maxPrice();
    
    return bl::in_range(price, minPrice, maxPrice);
  }

  bool checkBuyPrice(int32_t price) const
  {
    int32_t minPrice = _buyLevels.minPrice();
    int32_t maxPrice = _buyLevels.maxPrice();
    
    return bl::in_range(price, minPrice, maxPrice);
  }

  void insertSellOrder(OrderId orderId, int32_t price, int32_t qty) {
    PriceLevel& level = _sellLevels.price(price);
    int32_t slot = level.orders.push_back({orderId, qty});

    if(UNLIKELY(slot == -1)) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    emitEvent(CreateAccepted(orderId, slot));

    auto clearSignBit = [](int32_t i) -> int32_t {
      return i & 0x7FFFFFFF;
    };

    _sellTopPrice = bl::min(clearSignBit(_sellTopPrice), price);
  }

  void insertBuyOrder(OrderId orderId, int32_t price, int32_t qty) {
    PriceLevel& level = _buyLevels.price(price);
    int32_t slot = level.orders.push_back({orderId, qty});

    if(UNLIKELY(slot == -1)) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    emitEvent(CreateAccepted(orderId, slot));

    _buyTopPrice = bl::max(_buyTopPrice, price);
  }

  void updateSellOrder(OrderId orderId, int32_t slot, int32_t price, int32_t qty) {
    PriceLevel& level = _sellLevels.price(price);
    Order& order = level.orders.at(slot);

    if(UNLIKELY(order.id != orderId)) {
      return emitEvent(UpdateRejected(orderId, qty));
    }

    order.qty = qty;
    emitEvent(UpdateAccepted(orderId));
  }

  void updateBuyOrder(OrderId orderId, int32_t slot, int32_t price, int32_t qty) {
    PriceLevel& level = _buyLevels.price(price);
    Order& order = level.orders.at(slot);

    if(UNLIKELY(order.id != orderId)) {
      return emitEvent(UpdateRejected(orderId, qty));
    }

    order.qty = qty;
    emitEvent(UpdateAccepted(orderId));
  }

  void cancelSellOrder(OrderId orderId, int32_t slot, int32_t price) {
    PriceLevel& level = _sellLevels.price(price);
    Order& order = level.orders.at(slot);

    if(UNLIKELY(order.id != orderId)) {
      return emitEvent(CancelRejected(orderId));
    }

    order.id = InvalidOrderId;
    level.orders.pop_back();
    emitEvent(CancelAccepted(orderId));
  }

  void cancelBuyOrder(OrderId orderId, int32_t slot, int32_t price) {
    PriceLevel& level = _buyLevels.price(price);
    Order& order = level.orders.at(slot);

    if(UNLIKELY(order.id != orderId)) {
      return emitEvent(CancelRejected(orderId));
    }

    order.id = InvalidOrderId;
    level.orders.pop_back();
    emitEvent(CancelAccepted(orderId));
  }

  void shiftUp() {
    {
      int32_t minPrice = _buyLevels.minPrice();
      PriceLevel& level = _buyLevels.price(minPrice);
      expireOrders(level);
      _buyLevels.shiftUp();
    }

    {
      int32_t minPrice = _sellLevels.minPrice();
      PriceLevel& level = _sellLevels.price(minPrice);
      expireOrders(level);
      _sellLevels.shiftUp();
    }
  }

  void shiftDown() {
    {
      int32_t maxPrice = _buyLevels.maxPrice();
      PriceLevel& level = _buyLevels.price(maxPrice);
      expireOrders(level);
      _buyLevels.shiftDown();
    }

    {
      int32_t maxPrice = _sellLevels.maxPrice();
      PriceLevel& level = _sellLevels.price(maxPrice);
      expireOrders(level);
      _sellLevels.shiftDown();
    }
  }

  void expireOrders(PriceLevel& level) {
    while(level.orders.empty() == false) {
      Order& order = level.orders.front();
      
      emitEvent(OrderExpired(order.id));
      
      order.id = InvalidOrderId;
      level.orders.pop_front();
    }
  }
};
