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
  OrderBook(QueueOut& out, Price centerPrice)
    : _minSellPrice{UINT32_MAX}
    , _maxBuyPrice{UINT32_MIN}
    , _buyLevels(centerPrice)
    , _sellLevels(centerPrice)
    , _queueOut(out)
  {
  }

  OrderBook(OrderBook&&) = delete;
  OrderBook(const OrderBook&) = delete;

  OrderBook& operator=(OrderBook&&) = delete;
  OrderBook& operator=(const OrderBook&) = delete;

public:
  static constexpr uint32_t inside_levels() noexcept
  {
    return InsideLevels;
  }

  static constexpr uint32_t outside_levels() noexcept
  {
    return OutsideLevels;
  }

  static constexpr uint32_t orders() noexcept
  {
    return Orders;
  }

  Price center_price() const
  {
    Assert(_sellLevels.center_price() == _buyLevels.center_price());
    return _sellLevels.center_price();
  }

  Price sell_price_from() const
  {
    return _minSellPrice;
  }

  Price sell_price_to(Price price = MaxPrice) const
  {
    const Price maxPrice = _sellLevels.max_price();
    return bl::min(price, maxPrice);
  }

  Index sell_index_from() const
  {
    return _sellLevels.price_to_index(_minSellPrice);
  }

  Index sell_index_to(Price price = MaxPrice) const
  {
    price = sell_price_to(price);
    return _sellLevels.price_to_index(price);
  }

  void inc_sell_min_price(bool empty = true)
  {
    _minSellPrice += (Price)empty;
  }

  Price buy_price_from() const
  {
    return _maxBuyPrice;
  }

  Price buy_price_to(Price price = MinPrice) const
  {
    const Price min_price = _buyLevels.min_price();
    return bl::max(price, min_price);
  }

  Index buy_index_from() const
  {
    return _buyLevels.price_to_index(_maxBuyPrice);
  }

  Index buy_index_to(Price price = MinPrice) const
  {
    price = buy_price_to(price);
    return _buyLevels.price_to_index(price);
  }

  void dec_max_buy_price(bool empty = true)
  {
    _maxBuyPrice -= (Price)empty;
  }

  bool check_sell_price(Price price) const
  {
    const Price minPrice = _sellLevels.min_price();
    const Price maxPrice = _sellLevels.max_price();

    return bl::in_range(price, minPrice, maxPrice);
  }

  bool check_buy_price(Price price) const
  {
    const Price minPrice = _buyLevels.min_price();
    const Price maxPrice = _buyLevels.max_price();

    return bl::in_range(price, minPrice, maxPrice);
  }

  PriceLevels<InsideLevels, OutsideLevels>& sell_levels()
  {
    return _sellLevels;
  }

  const PriceLevels<InsideLevels, OutsideLevels>& sell_levels() const
  {
    return _sellLevels;
  }

  PriceLevels<OutsideLevels, InsideLevels, Orders>& buy_levels()
  {
    return _buyLevels;
  }

  const PriceLevels<OutsideLevels, InsideLevels, Orders>& buy_levels() const
  {
    return _buyLevels;
  }

  void insert_sell_order(OrderId orderId, Price price, Qty qty)
  {
    PriceLevel<Orders>& level = _sellLevels.at_price(price);
    _insert_order(level, orderId, price, qty);
    _minSellPrice = bl::min(_minSellPrice, price);
  }

  void insert_buy_order(OrderId orderId, Price price, Qty qty)
  {
    PriceLevel<Orders>& level = _buyLevels.at_price(price);
    _insert_order(level, orderId, price, qty);
    _maxBuyPrice = bl::max(_maxBuyPrice, price);
  }

  void update_sell_order(OrderId orderId, Index slot, Price price, Qty qty)
  {
    PriceLevel<Orders>& level = _sellLevels.at_price(price);
    _update_order(level, orderId, slot, price, qty);
  }

  void update_buy_order(OrderId orderId, Index slot, Price price, Qty qty)
  {
    PriceLevel<Orders>& level = _buyLevels.at_price(price);
    _update_order(level, orderId, slot, price, qty);
  }

  void cancel_sell_order(OrderId orderId, Index slot, Price price)
  {
    PriceLevel<Orders>& level = _sellLevels.at_price(price);
    _cancel_order(level, orderId, slot);
  }

  void cancel_buy_order(OrderId orderId, Index slot, Price price)
  {
    PriceLevel<Orders>& level = _buyLevels.at_price(price);
    _cancel_order(level, orderId, slot);
  }

  void shift_up()
  {
    _expire_level(_buyLevels.at_price(_buyLevels.min_price()));
    _buyLevels.shift_up(_queueOut);

    _expire_level(_sellLevels.at_price(_sellLevels.min_price()));
    _sellLevels.shift_up(_queueOut);
  }

  void shift_down()
  {
    _expire_level(_buyLevels.at_price(_buyLevels.max_price()));
    _buyLevels.shift_down(_queueOut);

    _expire_level(_sellLevels.at_price(_sellLevels.max_price()));
    _sellLevels.shift_down(_queueOut);
  }

  QueueOut& out()
  {
    return _queueOut;
  }

private:
  void _insert_order(PriceLevel<Orders>& level, OrderId orderId, Price price, Qty qty)
  {
    const Index slot = level.orders.push_back({orderId, qty});

    if(UNLIKELY(slot == InvalidIndex) ) {
       return _emit_events(CreateRejected(orderId, qty));
    }

    _emit_events(CreateAccepted(orderId, slot));
  }

  void _update_order(PriceLevel<Orders>& level, OrderId orderId, Index slot, Price price, Qty qty)
  {
    Order& order = level.orders.at_slot(slot);

    if(UNLIKELY(order.id != orderId)) {
      return _emit_events(UpdateRejected(orderId, qty));
    }

    order.qty = qty;
    _emit_events(UpdateAccepted(orderId));
  }

  void _cancel_order(PriceLevel<Orders>& level, OrderId orderId, Index slot)
  {
    Order& order = level.orders.at_slot(slot);

    if(UNLIKELY(order.id != orderId)) {
      return _emit_events(CancelRejected(orderId));
    }

    _emit_events(CancelAccepted(orderId));
    order.id = InvalidOrderId;
    level.orders.remove(slot);
  }

  void _expire_order(Order& order)
  {
    _emit_events(OrderExpired(order.id));
    order.id = InvalidOrderId;
  }

  void _expire_level(PriceLevel<Orders>& level)
  {
    while(level.orders.empty() == false) {
      Order& order = level.orders.front();
      _expire_order(order);
      level.orders.pop_front();
    }
  }

  void _emit_events(Event event)
  {
    while(_queueOut.push(event) == false) {
      _mm_pause();
    }
  }

private:
  Price _minSellPrice;
  Price _maxBuyPrice;

  PriceLevels<InsideLevels, OutsideLevels, Orders> _sellLevels;
  PriceLevels<OutsideLevels, InsideLevels, Orders> _buyLevels;

  QueueOut& _queueOut;
};
