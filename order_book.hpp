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
#include "./common.hpp"
#include "./events.hpp"
#include "./flat_list.hpp"
#include "./likely.hpp"
#include "./price_levels.hpp"
#include "./ring_buffer_spsc.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <functional>
#include <immintrin.h>
#include <iostream>
#include <limits>

template<uint32_t InsideLevels, uint32_t OutsideLevels, uint32_t Orders = 32>
struct OrderBook
{
public:
  OrderBook(RingBufferSPSC<Event, 1024>& bufferOut, Price centerPrice)
    : _topSellPrice{UINT32_MAX}
    , _topBuyPrice{UINT32_MIN}
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
  static constexpr uint32_t insideLevels() noexcept
  {
    return InsideLevels;
  }

  static constexpr uint32_t outsideLevels() noexcept
  {
    return OutsideLevels;
  }

  static constexpr uint32_t orders() noexcept
  {
    return Orders;
  }

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
    _topBuyPrice -= (Price)empty;
  }

  void incTopSellPrice(bool empty = true)
  {
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

    return bl::in_range(price, minPrice, maxPrice);
  }

  bool checkBuyPrice(Price price) const
  {
    const Price minPrice = _buyLevels.minPrice();
    const Price maxPrice = _buyLevels.maxPrice();

    return bl::in_range(price, minPrice, maxPrice);
  }

  PriceLevels<InsideLevels, OutsideLevels, Orders>& sellLevels() {
    return _sellLevels;
  }

  const PriceLevels<InsideLevels, OutsideLevels, Orders>& sellLevels() const {
    return _sellLevels;
  }

  PriceLevels<OutsideLevels, InsideLevels, Orders>& buyLevels() {
    return _buyLevels;
  }

  const PriceLevels<OutsideLevels, InsideLevels, Orders>& buyLevels() const {
    return _buyLevels;
  }

  void insertSellOrder(OrderId orderId, Price price, Qty qty)
  {
    PriceLevel<Orders>& level = _sellLevels.at_price(price);
    level.push_order(orderId, qty, _bufferOut);
    _topSellPrice = bl::min(_topSellPrice, price);
  }

  void insertBuyOrder(OrderId orderId, Price price, Qty qty)
  {
    PriceLevel<Orders>& level = _buyLevels.at_price(price);
    level.push_order(orderId, qty, _bufferOut);
    _topBuyPrice = bl::max(_topBuyPrice, price);
  }

  void updateSellOrder(OrderId orderId, Index slot, Price price, Qty qty)
  {
    PriceLevel<Orders>& level = _sellLevels.at_price(price);
    level.updateOrder(orderId, slot, qty, _bufferOut);
  }

  void updateBuyOrder(OrderId orderId, Index slot, Price price, Qty qty)
  {
    PriceLevel<Orders>& level = _buyLevels.at_price(price);
    level.updateOrder(orderId, slot, qty, _bufferOut);
  }

  void cancelSellOrder(OrderId orderId, Index slot, Price price)
  {
    PriceLevel<Orders>& level = _sellLevels.at_price(price);
    level.cancelOrder(orderId, slot, _bufferOut);
  }

  void cancelBuyOrder(OrderId orderId, Index slot, Price price)
  {
    PriceLevel<Orders>& level = _buyLevels.at_price(price);
    level.cancelOrder(orderId, slot, _bufferOut);
  }

  void shiftUp()
  {
    _buyLevels.shiftUp(_bufferOut);
    _sellLevels.shiftUp(_bufferOut);
  }

  void shiftDown()
  {
    _buyLevels.shiftDown(_bufferOut);
    _sellLevels.shiftDown(_bufferOut);
  }

  RingBufferSPSC<Event, 1024>& bufferOut()
  {
    return _bufferOut;
  }

private:
  Price _topSellPrice;
  Price _topBuyPrice;

  PriceLevels</* below */ InsideLevels, /* above */ OutsideLevels, Orders> _sellLevels;
  PriceLevels</* below */ OutsideLevels, /* above */ InsideLevels, Orders> _buyLevels;

  RingBufferSPSC<Event, 1024>& _bufferOut;
};
