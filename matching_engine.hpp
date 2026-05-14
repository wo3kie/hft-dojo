#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <xmmintrin.h>
#include <cstdint>

#include "./events.hpp"
#include "./order_book.hpp"
#include "./price_levels.hpp"

template<uint32_t Levels, uint32_t LevelsTolerance = Levels + 1>
struct MatchingEngine
{
  RingBufferSPSC<Event, 1024> _bufferOut;
  OrderBook<Levels> _orderBook;

  MatchingEngine(Price centerPrice)
    : _bufferOut()
    , _orderBook(std::ref(_bufferOut), centerPrice)
  {
  }

public:
  void insertSellOrder_PL(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_orderBook.checkSellPrice(price) == false)) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    qty = tradeSell(orderId, price, qty);

    if(UNLIKELY(qty != 0)) {
      _orderBook.insertSellOrder(orderId, price, qty);
    }
  }

  void insertSellOrder_MKT(OrderId orderId, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(qty != 0);
    }

    qty = tradeSell(orderId, MinPrice, qty);

    if(UNLIKELY(qty != 0)) {
      emitEvent(CreateRejected(orderId, qty));
    }
  }

  void insertBuyOrder_PL(OrderId orderId, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_orderBook.checkBuyPrice(price) == false)) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    qty = tradeBuy(orderId, price, qty);

    if(UNLIKELY(qty != 0)) {
      _orderBook.insertBuyOrder(orderId, price, qty);
    }
  }

  void insertBuyOrder_MKT(OrderId orderId, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(qty != 0);
    }

    qty = tradeBuy(orderId, MaxPrice, qty);

    if(UNLIKELY(qty != 0)) {
      emitEvent(CreateRejected(orderId, qty));
    }
  }

  void updateBuy(OrderId orderId, Index slot, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(slot != InvalidIndex);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_orderBook.checkBuyPrice(price) == false)) {
      return emitEvent(UpdateRejected(orderId, qty));
    }

    _orderBook.updateBuyOrder(orderId, slot, price, qty);
  }

  void updateSell(OrderId orderId, Index slot, Price price, Qty qty)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(slot != InvalidIndex);
      Assert(price != InvalidPrice);
      Assert(qty != 0);
    }

    if(UNLIKELY(_orderBook.checkSellPrice(price) == false)) {
      return emitEvent(UpdateRejected(orderId, qty));
    }

    _orderBook.updateSellOrder(orderId, slot, price, qty);
  }

  void cancelBuy(OrderId orderId, Index slot, Price price)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(slot != InvalidIndex);
      Assert(price != InvalidPrice);
    }

    if(UNLIKELY(_orderBook.checkBuyOrder(price) == false)) {
      return emitEvent(CancelRejected(orderId));
    }

    _orderBook.cancelBuyOrder(orderId, slot, price);
  }

  void cancelSell(OrderId orderId, Index slot, Price price)
  {
    {
      Assert(orderId != InvalidOrderId);
      Assert(slot != InvalidIndex);
      Assert(price != InvalidPrice);
    }

    if(UNLIKELY(_orderBook.checkSellOrder(price) == false)) {
      return emitEvent(CancelRejected(orderId));
    }

    _orderBook.cancelSellOrder(orderId, slot, price);
  }

  RingBufferSPSC<Event, 1024>& bufferOut()
  {
    return _bufferOut;
  }

private:
  void emitEvent(Event event)
  {
    while(_bufferOut.push(event) == false) {
      _mm_pause();
    }
  }

  void shiftOrderBook(Price price)
  {
    Price centerPrice = _orderBook.centerPrice();
    Price minPrice = bl::min(centerPrice, price);
    Price maxPrice = bl::max(centerPrice, price);

    if(maxPrice - minPrice <= LevelsTolerance) {
      return;
    }

    while(_orderBook.centerPrice() < price) {
      _orderBook.shiftUp();
    }

    while(_orderBook.centerPrice() > price) {
      _orderBook.shiftDown();
    }
  }

  Qty tradeAtPriceLevel(OrderId orderId, Price price, Qty qty, PriceLevel& level)
  {
    Assert(bl::in_range(price, MinPrice, MaxPrice));

    while((qty != 0) && (! level.orders.empty())) {
      Order& otherOrder = level.orders.front();
      Qty tradeQty = bl::min(qty, otherOrder.qty);

      qty -= tradeQty;
      otherOrder.qty -= tradeQty;

      emitEvent(Trade(price, tradeQty, orderId, otherOrder.id));

      if(otherOrder.qty == 0) {
        level.orders.front().id = InvalidOrderId;
        level.orders.pop_front();
      }
    }

    return qty;
  }

  Qty tradeSell(OrderId orderId, Price priceTo, Qty qty)
  {
    Assert(bl::in_range(priceTo, MinPrice, MaxPrice));

    if(UNLIKELY(_orderBook._buyTopPrice == InvalidPrice)) {
      return qty;
    }

    {
      priceTo = _orderBook.buyPriceTo(priceTo);

      Price price = _orderBook.buyPriceFrom();
      Index index = _orderBook.buyIndexFrom();

      while((qty != 0) && (price >= priceTo)) {
        PriceLevel& level = _orderBook._buyLevels.index(index);
        qty = tradeAtPriceLevel(orderId, price, qty, level);

        Index empty = (Index)level.orders.empty();
        _orderBook._buyTopPrice -= empty;

        shiftOrderBook(price);

        price -= 1;
        index -= 1;
      }

      return qty;
    }
  }

  Qty tradeBuy(OrderId orderId, Price priceTo, Qty qty)
  {
    Assert(bl::in_range(priceTo, MinPrice, MaxPrice));

    if(UNLIKELY(_orderBook._sellTopPrice == InvalidPrice)) {
      return qty;
    }

    {
      priceTo = _orderBook.sellPriceTo(priceTo);

      Price price = _orderBook.sellPriceFrom();
      Index index = _orderBook.sellIndexFrom();

      while((qty != 0) && (price <= priceTo)) {
        PriceLevel& level = _orderBook._sellLevels.index(index);
        qty = tradeAtPriceLevel(orderId, price, qty, level);

        Index empty = (Index)level.orders.empty();
        _orderBook._sellTopPrice += empty;

        shiftOrderBook(price);

        price += 1;
        index += 1;
      }

      return qty;
    }
  }
};
