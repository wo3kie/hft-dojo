#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstdint>

#include "./order_book.hpp"
#include "./price_levels.hpp"
#include "./events.hpp"

template<int32_t Levels, int32_t LevelsTolerance = Levels + 1>
struct MatchingEngine 
{
  RingBufferSPSC<Event, 1024> _bufferOut;
  OrderBook<Levels> _orderBook;

  MatchingEngine(int32_t centerPrice)
    : _bufferOut()
    , _orderBook(std::ref(_bufferOut), centerPrice)
  {
  }

public:
  void insertSellOrder_PL(int32_t orderId, int32_t price, int32_t qty)
  {
    if (UNLIKELY(_orderBook.checkSellPrice(price) == false)) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    qty = tradeSell(orderId, price, qty);
    
    if (UNLIKELY(qty != 0)) {
      _orderBook.insertSellOrder(orderId, price, qty);
    }
  }

  void insertSellOrder_MKT(int32_t orderId, int32_t qty)
  {
    qty = tradeSell(orderId, 0, qty);
    
    if (UNLIKELY(qty != 0)) {
      emitEvent(CreateRejected(orderId, qty));
    }
  }

  void insertBuyOrder_PL(int32_t orderId, int32_t price, int32_t qty)
  {
    if (UNLIKELY(_orderBook.checkBuyPrice(price) == false)) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    qty = tradeBuy(orderId, price, qty);
    
    if (UNLIKELY(qty != 0)) {
      _orderBook.insertBuyOrder(orderId, price, qty);
    }
  }

  void insertBuyOrder_MKT(int32_t orderId, int32_t qty)
  {
    qty = tradeBuy(orderId, 9999, qty);
    
    if (UNLIKELY(qty != 0)) {
      emitEvent(CreateRejected(orderId, qty));
    }
  }

  void updateBuy(int32_t orderId, int32_t slot, int32_t price, int32_t qty)
  {
    if (UNLIKELY(_orderBook.checkBuyPrice(price) == false)) {
      return emitEvent(UpdateRejected(orderId, qty));
    }

    _orderBook.updateBuyOrder(orderId, slot, price, qty);
  }

  void updateSell(int32_t orderId, int32_t slot, int32_t price, int32_t qty)
  {
    if (UNLIKELY(_orderBook.checkSellPrice(price) == false)) {
      return emitEvent(UpdateRejected(orderId, qty));
    }

    _orderBook.updateSellOrder(orderId, slot, price, qty);
  }

  void cancelBuy(int32_t orderId, int32_t slot, int32_t price)
  {
    if (UNLIKELY(_orderBook.checkBuyOrder(price) == false)) {
      return emitEvent(CancelRejected(orderId));
    }

    _orderBook.cancelBuyOrder(orderId, slot, price);
  }

  void cancelSell(int32_t orderId, int32_t slot, int32_t price)
  {
    if (UNLIKELY(_orderBook.checkSellOrder(price) == false)) {
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
    }
  }

  void shiftOrderBook(int32_t price) {
    int32_t diff = price - _orderBook.centerPrice();

    if (bl::abs(diff) <= LevelsTolerance) {
      return;
    }

    if (diff < 0) {
      while(_orderBook.centerPrice() != price) {
        _orderBook.shiftDown();
      }
    } else {
      while(_orderBook.centerPrice() != price) {
        _orderBook.shiftUp();
      }
    }
  }

  int32_t tradeAtPriceLevel(int32_t orderId, int32_t price, int32_t qty, PriceLevel& level)
  {
    while(qty > 0 && ! level.orders.empty()) {
      Order& otherOrder = level.orders.front();
      int32_t tradeQty = std::min(qty, otherOrder.qty);
      
      qty -= tradeQty;
      otherOrder.qty -= tradeQty;
      
      emitEvent(Trade(price, tradeQty, orderId, otherOrder.id));
      
      if(otherOrder.qty == 0) {
        level.orders.front().id = -1;
        level.orders.pop_front();
      }
    }

    return qty;
  }

  int32_t tradeSell(int32_t orderId, int32_t priceTo, int32_t qty)
  {
    if (UNLIKELY(_orderBook._buyTopPrice == -1)) {
      return qty;
    }

    {
      priceTo = _orderBook.buyPriceTo(priceTo);

      int32_t price = _orderBook.buyPriceFrom();
      int32_t index = _orderBook.buyIndexFrom();
      
      while (price >= priceTo) {       
        if (qty == 0) {
          break;
        }
        
        PriceLevel& level = _orderBook._buyLevels.index(index);
        qty = tradeAtPriceLevel(orderId, price, qty, level);
        
        int32_t empty = (int32_t)level.orders.empty();
        _orderBook._buyTopPrice -= empty;
        
        shiftOrderBook(price);

        price -= 1;
        index -= 1;
      }
      
      return qty;
    }
  }

  int32_t tradeBuy(int32_t orderId, int32_t priceTo, int32_t qty)
  {
    if (UNLIKELY(_orderBook._sellTopPrice == -1)) {
      return qty;
    }

    {
      priceTo = _orderBook.sellPriceTo(priceTo);

      int32_t price = _orderBook.sellPriceFrom();
      int32_t index = _orderBook.sellIndexFrom();
      
      while (price <= priceTo) {
        if (qty == 0) {
          break;
        }
        
        PriceLevel& level = _orderBook._sellLevels.index(index);
        qty = tradeAtPriceLevel(orderId, price, qty, level);
        
        int32_t empty = (int32_t)level.orders.empty();
        _orderBook._sellTopPrice += empty;
        
        shiftOrderBook(price);

        price += 1;
        index += 1;
      }
      
      return qty;
    }
  }
};
