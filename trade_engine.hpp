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

template<uint32_t InsideLevels, uint32_t OutsideLevels, uint32_t Orders = 32>
struct TradeEngine
{
  QueueOut _queueOut;
  OrderBook<InsideLevels, OutsideLevels, Orders> _orderBook;

  TradeEngine(Price centerPrice)
    : _orderBook(_queueOut, centerPrice)
  {
  }

public:
  constexpr static uint32_t inside_levels()
  {
    return InsideLevels;
  }

  constexpr static uint32_t outside_levels()
  {
    return OutsideLevels;
  }

  constexpr static uint32_t orders()
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

    const Qty traded = _trade_sell(orderId, price, qty);
    
    if(UNLIKELY(traded != qty)) {
      if(UNLIKELY(_orderBook.check_sell_price(price) == false)) {
        return _emit_event(CreateRejected(orderId, qty));
      }

      _orderBook.insert_sell_order(orderId, price, (qty - traded));
    }
  }

  void insert_buy_order_PL(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    const Qty traded = _trade_buy(orderId, price, qty);

    if(UNLIKELY(traded != qty)) {
      if(UNLIKELY(_orderBook.check_buy_price(price) == false)) {
        return _emit_event(CreateRejected(orderId, qty));
      }

      _orderBook.insert_buy_order(orderId, price, (qty - traded));
    }
  }

  void insert_sell_order_MKT(OrderId orderId, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(qty != 0);
    }

    const Qty traded = _trade_sell(orderId, MinPrice, qty);

    if(UNLIKELY(traded != qty)) {
      _emit_event(CreateRejected(orderId, (qty - traded)));
    }
  }

  void insert_buy_order_MKT(OrderId orderId, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(qty != 0);
    }

    const Qty traded = _trade_buy(orderId, MaxPrice, qty);

    if(UNLIKELY(traded != qty)) {
      _emit_event(CreateRejected(orderId, (qty - traded)));
    }
  }

  void insert_sell_order_IOC(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    const Qty traded = _trade_sell(orderId, price, qty);

    if(UNLIKELY(traded != qty)) {
      _emit_event(CreateRejected(orderId, (qty - traded)));
    }
  }

  void insert_buy_order_IOC(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    const Qty traded = _trade_buy(orderId, price, qty);

    if(UNLIKELY(traded != qty)) {
      _emit_event(CreateRejected(orderId, (qty - traded)));
    }
  }

  void insert_sell_order_PO(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
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
      Assert(orderId != InvalidOrderId);
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
      Assert(orderId != InvalidOrderId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_check_sell_FOK(price, qty) == false)) {
      return _emit_event(CreateRejected(orderId, qty));
    }

    const Qty traded = _trade_sell(orderId, price, qty);
    Assert(traded == qty);
  }
  
  void insert_buy_order_FOK(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_check_buy_FOK(price, qty) == false)) {
      return _emit_event(CreateRejected(orderId, qty));
    }

    const Qty traded = _trade_buy(orderId, price, qty);
    Assert(traded == qty);
  }

  void update_buy_order(OrderId orderId, Price price, Index slot, Qty newQty)
  {
    {
      Assert(orderId != InvalidOrderId);
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
      Assert(orderId != InvalidOrderId);
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
      Assert(orderId != InvalidOrderId);
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
      Assert(orderId != InvalidOrderId);
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

  Qty _trade_level(OrderId orderId, Price price, Qty qty, PriceLevel<Orders>& level)
  {
    const Qty oldQty = qty;

    while((qty != 0) && (! level.orders.empty())) {
      Order& otherOrder = level.orders.front();
      Qty tradeQty = std::min(qty, otherOrder.qty);

      qty -= tradeQty;
      otherOrder.qty -= tradeQty;

      _emit_event(Trade(price, tradeQty, orderId, otherOrder.id));

      const Qty mask = (otherOrder.qty == 0);
      otherOrder.id = otherOrder.id & ~mask;

      if(mask != 0) {
        level.orders.pop_front();
      }
    }

    return oldQty - qty;
  }

  Qty _trade_sell(OrderId orderId, Price priceLimit, Qty qty)
  {
    const Qty oldQty = qty;

    {
      priceLimit = _orderBook.buy_price_to(priceLimit);

      Price price = _orderBook.buy_price_from();
      Index index = _orderBook.buy_index_from();

      while((qty != 0) && (price >= priceLimit)) {
        PriceLevel<Orders>& buyLevel = _orderBook.buy_levels().at_index(index);
        
        const Qty traded = _trade_level(orderId, price, qty, buyLevel);

        buyLevel.balance -= traded;
        qty -= traded;

        const bool empty = buyLevel.orders.empty();
        assert((!empty) || (buyLevel.balance == 0));
        
        _orderBook.dec_max_buy_price(empty);
        _shift_order_book(price);

        price -= 1;
        index -= 1;
      }
    }

    return (oldQty - qty);
  }

  Qty _trade_buy(OrderId orderId, Price priceLimit, Qty qty)
  {
    const Qty oldQty = qty;

    {
      priceLimit = _orderBook.sell_price_to(priceLimit);

      Price price = _orderBook.sell_price_from();
      Index index = _orderBook.sell_index_from();

      while((qty != 0) && (price <= priceLimit)) {
        PriceLevel<Orders>& sellLevel = _orderBook.sell_levels().at_index(index);
        const Qty traded = _trade_level(orderId, price, qty, sellLevel);

        sellLevel.balance -= traded;
        qty -= traded;

        const bool empty = sellLevel.orders.empty();
        assert((!empty) || (sellLevel.balance == 0));
        
        _orderBook.inc_sell_min_price(empty);
        _shift_order_book(price);

        price += 1;
        index += 1;
      }
    }

    return (oldQty - qty);
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
      qty -= std::min<Qty>(qty, buyLevel.balance);

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
      qty -= std::min<Qty>(qty, sellLevel.balance);

      price += 1;
      index += 1;
    }

    return (qty == 0);
  }
};
