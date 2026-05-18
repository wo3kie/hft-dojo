#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstdint>
#include <immintrin.h>

#include "./events.hpp"
#include "./order_book.hpp"
#include "./price_levels.hpp"

template<uint32_t InsideLevels, uint32_t OutsideLevels, uint32_t Orders = 32>
struct MatchingEngine
{
  QueueOut _queueOut;
  OrderBook<InsideLevels, OutsideLevels, Orders> _orderBook;

  MatchingEngine(Price centerPrice)
    : _orderBook(_queueOut, centerPrice)
  {
  }

public:
  constexpr static uint32_t inside_levels() noexcept
  {
    return InsideLevels;
  }

  constexpr static uint32_t outside_levels() noexcept
  {
    return OutsideLevels;
  }

  constexpr static uint32_t orders() noexcept
  {
    return Orders;
  }

  void insert_sell_order_PL(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_orderBook.check_sell_price(price) == false)) {
      return _emit_events(CreateRejected(orderId, qty));
    }

    qty = trade_sell(orderId, price, qty);

    if(UNLIKELY(qty != 0)) {
      _orderBook.insert_sell_order(orderId, price, qty);
    }
  }

  void insert_sell_order_MKT(OrderId orderId, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(qty != 0);
    }

    qty = trade_sell(orderId, MinPrice, qty);

    if(UNLIKELY(qty != 0)) {
      _emit_events(CreateRejected(orderId, qty));
    }
  }

  void insert_buy_order_PL(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_orderBook.check_buy_price(price) == false)) {
      return _emit_events(CreateRejected(orderId, qty));
    }

    qty = tradeBuy(orderId, price, qty);

    if(UNLIKELY(qty != 0)) {
      _orderBook.insert_buy_order(orderId, price, qty);
    }
  }

  void insert_buy_order_MKT(OrderId orderId, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(qty != 0);
    }

    qty = tradeBuy(orderId, MaxPrice, qty);

    if(UNLIKELY(qty != 0)) {
      _emit_events(CreateRejected(orderId, qty));
    }
  }

  void update_buy_order(OrderId orderId, Index slot, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(slot != InvalidIndex);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_orderBook.check_buy_price(price) == false)) {
      return _emit_events(UpdateRejected(orderId, qty));
    }

    _orderBook.update_buy_order(orderId, slot, price, qty);
  }

  void update_sell_order(OrderId orderId, Index slot, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(slot != InvalidIndex);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_orderBook.check_sell_price(price) == false)) {
      return _emit_events(UpdateRejected(orderId, qty));
    }

    _orderBook.update_sell_order(orderId, slot, price, qty);
  }

  void cancel_buy_order(OrderId orderId, Index slot, Price price)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(slot != InvalidIndex);
      Assert(price != InvalidPrice);
    }

    if(UNLIKELY(_orderBook.check_buy_price(price) == false)) {
      return _emit_events(CancelRejected(orderId));
    }

    _orderBook.cancel_buy_order(orderId, slot, price);
  }

  void cancel_sell_order(OrderId orderId, Index slot, Price price)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(slot != InvalidIndex);
      Assert(price != InvalidPrice);
    }

    if(UNLIKELY(_orderBook.check_sell_price(price) == false)) {
      return _emit_events(CancelRejected(orderId));
    }

    _orderBook.cancel_sell_order(orderId, slot, price);
  }

  QueueOut& out()
  {
    return _queueOut;
  }

private:
  void _emit_events(Event event)
  {
    while(_queueOut.push(event) == false) {
      _mm_pause();
    }
  }

  void shift_order_book(Price price)
  {
    {
      Price centerPrice = _orderBook.center_price();
      Price minPrice = bl::min(centerPrice, price);
      Price maxPrice = bl::max(centerPrice, price);

      if(maxPrice - minPrice <= 4) {
        return;
      }
    }

    while(_orderBook.center_price() < price) {
      _orderBook.shift_up();
    }

    while(_orderBook.center_price() > price) {
      _orderBook.shift_down();
    }
  }

  Qty trade(OrderId orderId, Price price, Qty qty, PriceLevel<Orders>& level)
  {
    while((qty != 0) && (! level.orders.empty())) {
      Order& otherOrder = level.orders.front();
      Qty tradeQty = bl::min(qty, otherOrder.qty);

      qty -= tradeQty;
      otherOrder.qty -= tradeQty;

      _emit_events(Trade(price, tradeQty, orderId, otherOrder.id));

      if(otherOrder.qty == 0) {
        otherOrder.id = InvalidOrderId;
        level.orders.pop_front();
      }
    }

    return qty;
  }

  Qty trade_sell(OrderId orderId, Price priceLimit, Qty qty)
  {
    if(UNLIKELY(_orderBook.buy_price_from() == UINT32_MIN)) {
      return qty;
    }

    priceLimit = _orderBook.buy_price_to(priceLimit);

    Price price = _orderBook.buy_price_from();
    Index index = _orderBook.buy_index_from();

    while((qty != 0) && (price >= priceLimit)) {
      PriceLevel<Orders>& level = _orderBook.buy_levels().at_index(index);
      qty = trade(orderId, price, qty, level);

      const bool empty = level.orders.empty();
      _orderBook.dec_max_buy_price(empty);

      shift_order_book(price);

      price -= 1;
      index -= 1;
    }

    return qty;
  }

  Qty tradeBuy(OrderId orderId, Price priceLimit, Qty qty)
  {
    if(UNLIKELY(_orderBook.sell_price_from() == UINT32_MAX)) {
      return qty;
    }

    priceLimit = _orderBook.sell_price_to(priceLimit);

    Price price = _orderBook.sell_price_from();
    Index index = _orderBook.sell_index_from();

    while((qty != 0) && (price <= priceLimit)) {
      PriceLevel<Orders>& level = _orderBook.sell_levels().at_index(index);
      qty = trade(orderId, price, qty, level);

      const bool empty = level.orders.empty();
      _orderBook.inc_sell_min_price(empty);

      shift_order_book(price);

      price += 1;
      index += 1;
    }

    return qty;
  }
};
