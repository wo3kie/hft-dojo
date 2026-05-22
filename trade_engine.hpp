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
#include <thread>

#include "./events.hpp"
#include "./order_book.hpp"
#include "./price_levels.hpp"

template<uint32_t _InsideLevels, uint32_t _OutsideLevels, uint32_t _Orders = 32>
struct TradeEngine
{
  constexpr static uint32_t InsideLevels = _InsideLevels;
  constexpr static uint32_t OutsideLevels = _OutsideLevels;
  constexpr static uint32_t Orders = _Orders;

public:
  TradeEngine(Price centerPrice)
    : _orderBook(_queueOut, centerPrice)
  {
  }

  TradeEngine(TradeEngine&&) = delete;
  TradeEngine(const TradeEngine&) = delete;

  TradeEngine& operator=(TradeEngine&&) = delete;
  TradeEngine& operator=(const TradeEngine&) = delete;

public:
  void insert_sell_order_PL(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    qty = _trade_sell(orderId, price, qty);
    
    if(qty == 0) {
      return;
    }

    if(_orderBook.check_sell_price(price)) {
      return _orderBook.insert_sell_order(orderId, price, qty);
    }
      
    return _emit_event(CreateRejected(orderId, qty));
  }

  void insert_buy_order_PL(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    qty = _trade_buy(orderId, price, qty);

    if(qty == 0) {
      return;
    }

    if(_orderBook.check_buy_price(price)) {
      return _orderBook.insert_buy_order(orderId, price, qty);
    }
      
    return _emit_event(CreateRejected(orderId, qty));
  }

  void insert_sell_order_MKT(OrderId orderId, Qty qty)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(qty != 0);
    }

    qty = _trade_sell(orderId, Order::MinPrice, qty);

    if(UNLIKELY(qty != 0)) {
      _emit_event(CreateRejected(orderId, qty));
    }
  }

  void insert_buy_order_MKT(OrderId orderId, Qty qty)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(qty != 0);
    }

    qty = _trade_buy(orderId, Order::MaxPrice, qty);

    if(UNLIKELY(qty != 0)) {
      _emit_event(CreateRejected(orderId, qty));
    }
  }

  void insert_sell_order_IOC(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    qty = _trade_sell(orderId, price, qty);

    if(UNLIKELY(qty != 0)) {
      _emit_event(CreateRejected(orderId, qty));
    }
  }

  void insert_buy_order_IOC(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    qty = _trade_buy(orderId, price, qty);

    if(UNLIKELY(qty != 0)) {
      _emit_event(CreateRejected(orderId, qty));
    }
  }

  void insert_sell_order_PO(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_check_sell_PO(price) == false)) {
      return _emit_event(CreateRejected(orderId, qty));
    }
    
    if(UNLIKELY(_orderBook.check_sell_price(price) == false)) {
      return _emit_event(CreateRejected(orderId, qty));
    }

    _orderBook.insert_sell_order(orderId, price, qty);
  }
  
  void insert_buy_order_PO(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_check_buy_PO(price) == false)) {
      return _emit_event(CreateRejected(orderId, qty));
    }
    
    if(UNLIKELY(_orderBook.check_buy_price(price) == false)) {
      return _emit_event(CreateRejected(orderId, qty));
    }

    _orderBook.insert_buy_order(orderId, price, qty);
  }

  void insert_sell_order_FOK(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_check_sell_FOK(price, qty) == false)) {
      return _emit_event(CreateRejected(orderId, qty));
    }

    qty = _trade_sell(orderId, price, qty);
    Assert(qty == 0);
  }
  
  void insert_buy_order_FOK(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_check_buy_FOK(price, qty) == false)) {
      return _emit_event(CreateRejected(orderId, qty));
    }

    qty = _trade_buy(orderId, price, qty);
    Assert(qty == 0);
  }

  void update_buy_order(OrderId orderId, Price price, Index slot, Qty newQty)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(slot != InvalidIndex);
      Assert(price != InvalidPrice);
      Assert(newQty != 0);
    }

    if(UNLIKELY(_orderBook.check_buy_price(price) == false)) {
      return _emit_event(UpdateRejected(orderId, newQty));
    }

    _orderBook.update_buy_order(orderId, slot, price, newQty);
  }

  void update_sell_order(OrderId orderId, Price price, Index slot, Qty newQty)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(slot != InvalidIndex);
      Assert(price != InvalidPrice);
      Assert(newQty != 0);
    }

    if(UNLIKELY(_orderBook.check_sell_price(price) == false)) {
      return _emit_event(UpdateRejected(orderId, newQty));
    }

    _orderBook.update_sell_order(orderId, slot, price, newQty);
  }

  void cancel_buy_order(OrderId orderId, Price price, Index slot)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(slot != InvalidIndex);
      Assert(price != InvalidPrice);
    }

    if(UNLIKELY(_orderBook.check_buy_price(price) == false)) {
      return _emit_event(CancelRejected(orderId));
    }

    _orderBook.cancel_buy_order(orderId, slot, price);
  }

  void cancel_sell_order(OrderId orderId, Price price, Index slot)
  {
    {
      Assert(orderId != Order::InvalidId);
      Assert(slot != InvalidIndex);
      Assert(price != InvalidPrice);
    }

    if(UNLIKELY(_orderBook.check_sell_price(price) == false)) {
      return _emit_event(CancelRejected(orderId));
    }

    _orderBook.cancel_sell_order(orderId, slot, price);
  }

  void reset(Price centerPrice)
  {
    _orderBook.reset(centerPrice);
  }

  QueueOut& out()
  {
    return _queueOut;
  }

private:
  void _emit_event(Event event)
  {
    int spins = 0;

    while(_queueOut.push(event) == false) {
      if(spins++ < 16) {
        _mm_pause();
      } else {
        std::this_thread::yield();
      }
    }
  }

  void _shift_order_book(Price price)
  {
    {
      Price centerPrice = _orderBook.center_price();
      Price minPrice = std::min(centerPrice, price);
      Price maxPrice = std::max(centerPrice, price);

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

  Qty _trade_level(OrderId id, Price price, Qty qty, PriceLevel<Orders>& level)
  {
    while((qty != 0) && (! level.empty())) {
      const Order& order = level.order();
      const OrderId orderId = order.id();
      const Qty orderQty = order.qty();
      const Qty min = std::min(qty, orderQty);

      qty -= min;
      level.trade_front(min);
      
      _emit_event(Trade(price, min, id, orderId));
    }

    return qty;
  }

  Qty _trade_sell(OrderId orderId, Price priceLimit, Qty qty)
  {
    priceLimit = _orderBook.buy_price_to(priceLimit);

    Price price = _orderBook.buy_price_from();
    Index index = _orderBook.buy_index_from();

    while((qty != 0) && (price >= priceLimit)) {
      PriceLevel<Orders>& buyLevel = _orderBook.buy_levels().at_index(index);
      qty = _trade_level(orderId, price, qty, buyLevel);
      
      _orderBook.dec_max_buy_price(buyLevel.empty());
      _shift_order_book(price);

      price -= 1;
      index -= 1;
    }

    return qty;
  }

  Qty _trade_buy(OrderId orderId, Price priceLimit, Qty qty)
  {
    priceLimit = _orderBook.sell_price_to(priceLimit);

    Price price = _orderBook.sell_price_from();
    Index index = _orderBook.sell_index_from();

    while((qty != 0) && (price <= priceLimit)) {
      PriceLevel<Orders>& sellLevel = _orderBook.sell_levels().at_index(index);
      qty = _trade_level(orderId, price, qty, sellLevel);

      _orderBook.inc_sell_min_price(sellLevel.empty());
      _shift_order_book(price);

      price += 1;
      index += 1;
    }

    return qty;
  }

  bool _check_sell_PO(Price priceLimit) const
  {
    priceLimit = _orderBook.buy_price_to(priceLimit);

    Price price = _orderBook.buy_price_from();
    Price index = _orderBook.buy_index_from();

    while(price >= priceLimit) {
      const PriceLevel<Orders>& buyLevel = _orderBook.buy_levels().at_index(index);

      if(UNLIKELY(buyLevel.balance != 0)) {
        return false;
      }

      price -= 1;
      index -= 1;
    }

    return true;
  }

  bool _check_buy_PO(Price priceLimit) const
  {
    priceLimit = _orderBook.sell_price_to(priceLimit);

    Price price = _orderBook.sell_price_from();
    Price index = _orderBook.sell_index_from();

    while(price <= priceLimit) {
      const PriceLevel<Orders>& sellLevel = _orderBook.sell_levels().at_index(index);

      if(UNLIKELY(sellLevel.balance != 0)) {
        return false;
      }

      price += 1;
      index += 1;
    }

    return true;
  }
  
  bool _check_sell_FOK(Price priceLimit, Qty qty) const
  {
    priceLimit = _orderBook.buy_price_to(priceLimit);

    Price price = _orderBook.buy_price_from();
    Price index = _orderBook.buy_index_from();

    while((qty != 0) && (price >= priceLimit)) {
      const PriceLevel<Orders>& buyLevel = _orderBook.buy_levels().at_index(index);
      qty -= std::min<Qty>(qty, buyLevel.balance());

      price -= 1;
      index -= 1;
    }

    return (qty == 0);
  }

  bool _check_buy_FOK(Price priceLimit, Qty qty) const
  {
    priceLimit = _orderBook.sell_price_to(priceLimit);

    Price price = _orderBook.sell_price_from();
    Price index = _orderBook.sell_index_from();

    while((qty != 0) && (price <= priceLimit)) {
      const PriceLevel<Orders>& sellLevel = _orderBook.sell_levels().at_index(index);
      qty -= std::min<Qty>(qty, sellLevel.balance());

      price += 1;
      index += 1;
    }

    return (qty == 0);
  }

private:
  QueueOut _queueOut;
  OrderBook<InsideLevels, OutsideLevels, Orders> _orderBook;
};
