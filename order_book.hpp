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
#include <xmmintrin.h>

template<uint32_t Levels>
struct OrderBook
{
public:
  static_assert((Levels & (Levels + 1)) == 0);

public:
  OrderBook(RingBufferSPSC<Event, 1024>& bufferOut, Price centerPrice)
    : _topSellPrice{InvalidPrice}
    , _topBuyPrice{InvalidPrice}
    , _buyLevels(centerPrice)
    , _sellLevels(centerPrice)
    , _bufferOut(bufferOut)
  {
  }

  OrderBook(OrderBook&&) = delete;
  OrderBook(const OrderBook&) = delete;

  OrderBook& operator=(OrderBook&&) = delete;
  OrderBook& operator=(const OrderBook&) = delete;

public:
  Price centerPrice() const
  {
    Assert(_sellLevels.centerPrice() == _buyLevels.centerPrice());
    return _sellLevels.centerPrice();
  }

  void emitEvent(Event event)
  {
    while(_bufferOut.push(event) == false) {
      _mm_pause();
    }

#ifdef NDEBUG
    (void)_bufferOut.pop();
#endif
  }

  Price topBuyPrice() const
  {
    return _topBuyPrice;
  }

  Price buyPriceLimit(Price price = MinPrice) const
  {
    const Price min_price = _buyLevels.minPrice();
    return bl::max(price, min_price);
  }

  void decTopBuyPrice(bool empty = true)
  {
    /*
     * branchless version for conditional decrement:
     *
     * if (buyOrders.empty()) {
     *  _topBuyPrice -= 1;
     * }
     * 
     */

    _topBuyPrice -= (Price)empty;
  }

  void incTopSellPrice(bool empty = true)
  {
    /*
     * branchless version for conditional increment:
     *
     * if (sellOrders.empty()) {
     *  _topSellPrice += 1;
     * }
     * 
     */

    _topSellPrice += (Price)empty;
  }

  Index topBuyIndex() const
  {
    return _buyLevels.priceToIndex(_topBuyPrice);
  }

  Price topSellPrice() const
  {
    return _topSellPrice;
  }

  Price sellPriceLimit(Price price = MaxPrice) const
  {
    const Price maxPrice = _sellLevels.maxPrice();
    return bl::min(price, maxPrice);
  }

  Index topSellIndex() const
  {
    return _sellLevels.priceToIndex(_topSellPrice);
  }

  bool checkSellPrice(Price price) const
  {
    const Price minPrice = _sellLevels.minPrice();
    const Price maxPrice = _sellLevels.maxPrice();

    return bl::in_range<Price>(price, minPrice, maxPrice);
  }

  bool checkBuyPrice(Price price) const
  {
    const Price minPrice = _buyLevels.minPrice();
    const Price maxPrice = _buyLevels.maxPrice();

    return bl::in_range(price, minPrice, maxPrice);
  }

  PriceLevels<Levels>& sellLevels() {
    return _sellLevels;
  }

  const PriceLevels<Levels>& sellLevels() const {
    return _sellLevels;
  }

  PriceLevels<Levels>& buyLevels() {
    return _buyLevels;
  }

  const PriceLevels<Levels>& buyLevels() const {
    return _buyLevels;
  }

  void insertSellOrder(OrderId orderId, Price price, Qty qty)
  {
    PriceLevel& level = _sellLevels.price(price);
    const Index slot = level.orders.push_back({orderId, qty});

    if(UNLIKELY(slot == InvalidIndex)) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    emitEvent(CreateAccepted(orderId, slot));

    {
      /*
       * if (_topSellPrice == uint32_t(0)) {
       *   _topSellPrice = price;
       * } else {
       *   _topSellPrice = bl::min(_topSellPrice, price);
       * }
       */

      _topSellPrice = bl::min(_topSellPrice - 1, price - 1) + 1;
    }
  }

  void insertBuyOrder(OrderId orderId, Price price, Qty qty)
  {
    PriceLevel& level = _buyLevels.price(price);
    const Index slot = level.orders.push_back({orderId, qty});

    if(UNLIKELY(slot == InvalidIndex)) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    emitEvent(CreateAccepted(orderId, slot));

    _topBuyPrice = bl::max(_topBuyPrice, price);
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
      const Price minPrice = _buyLevels.minPrice();
      PriceLevel& level = _buyLevels.price(minPrice);
      expireOrders(level);
      _buyLevels.shiftUp();
    }

    {
      const Price minPrice = _sellLevels.minPrice();
      PriceLevel& level = _sellLevels.price(minPrice);
      expireOrders(level);
      _sellLevels.shiftUp();
    }
  }

  void shiftDown()
  {
    {
      const Price maxPrice = _buyLevels.maxPrice();
      PriceLevel& level = _buyLevels.price(maxPrice);
      expireOrders(level);
      _buyLevels.shiftDown();
    }

    {
      const Price maxPrice = _sellLevels.maxPrice();
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

private:
  Price _topSellPrice;
  Price _topBuyPrice;

  PriceLevels<Levels> _sellLevels;
  PriceLevels<Levels> _buyLevels;

  RingBufferSPSC<Event, 1024>& _bufferOut;
};
