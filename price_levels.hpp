#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <immintrin.h>

#include "./array.hpp"
#include "./assert.hpp"
#include "./branchless.hpp"
#include "./flat_list.hpp"
#include "./events.hpp"
#include "./likely.hpp"
#include "./order.hpp"
#include "./ring_buffer_spsc.hpp"

struct PriceLevel
{
public:
  PriceLevel() = default;
  PriceLevel(PriceLevel&&) = delete;
  PriceLevel(const PriceLevel&) = delete;

  PriceLevel& operator=(PriceLevel&&) = delete;
  PriceLevel& operator=(const PriceLevel&) = delete;

public:
  template<std::size_t N>
  void push_order(OrderId orderId, Qty qty, RingBufferSPSC<Event, N>& bufferOut)
  {
    const Index slot = _orders.push_back({orderId, qty});

    if(UNLIKELY(slot == InvalidIndex)) {
      emitEvent(CreateRejected(orderId, qty), bufferOut);
    } {
      emitEvent(CreateAccepted(orderId, slot), bufferOut);
    }
  }

  [[nodiscard]] Order& order() {
    return _orders.front();
  }

  [[nodiscard]] Order& at_slot(int32_t slot)
  {
    return _orders.at_slot(slot);
  }

  template<std::size_t N>
  void updateOrder(OrderId orderId, Index slot, Qty qty, RingBufferSPSC<Event, N>& bufferOut)
  {
    Order& order = _orders.at_slot(slot);

    if(UNLIKELY(order.id != orderId)) {
      return emitEvent(UpdateRejected(orderId, qty), bufferOut);
    }

    order.qty = qty;
    emitEvent(UpdateAccepted(orderId), bufferOut);
  }

  template<std::size_t N>
  void cancelOrder(OrderId orderId, int32_t slot, RingBufferSPSC<Event, N>& bufferOut)
  {
    Order& order = _orders.at_slot(slot);

    if(UNLIKELY(order.id != orderId)) {
      return emitEvent(CancelRejected(orderId), bufferOut);
    }

    order.id = InvalidOrderId;
    _orders.remove(slot);
    emitEvent(CancelAccepted(orderId), bufferOut);
  }

  void pop_order()
  {
    _orders.pop_front();
  }

  [[nodiscard]] bool empty() const
  {
    return _orders.empty();
  }

  template<std::size_t N>
  void expireOrders(RingBufferSPSC<Event, N>& bufferOut)
  {
    while(_orders.empty() == false) {
      Order& order = _orders.front();

      emitEvent(OrderExpired(order.id), bufferOut);

      order.id = InvalidOrderId;
      _orders.pop_front();
    }
  }

private:
  FlatList<Order, 32> _orders;

private:
  template<std::size_t N>
  void emitEvent(Event event, RingBufferSPSC<Event, N>& bufferOut)
  {
    while(bufferOut.push(event) == false) {
      _mm_pause();
    }

#ifdef HFT_DOJO_BENCH
    (void)bufferOut.pop();
#endif
  }
};

template<uint32_t Levels>
struct PriceLevels
{
  static_assert((Levels & (Levels + 1)) == 0);

  constexpr static uint32_t Size = Levels + 1 + Levels + 1;
  constexpr static uint32_t Mask = Size - 1;

public:
  PriceLevels(Price centerPrice)
    : _centerPrice(centerPrice)
    , _centerIndex(Levels)
  {
    Assert(centerPrice > Levels);
    Assert(centerPrice < MaxPrice - Levels);
  }

  PriceLevels(PriceLevels&&) = default;
  PriceLevels(const PriceLevels&) = delete;
  
  PriceLevels& operator=(PriceLevels&&) = default;
  PriceLevels& operator=(const PriceLevels&) = delete;

public:
  Price centerPrice() const
  {
    return _centerPrice;
  }

  uint32_t levels() const
  {
    return Levels;
  }

  Price minPrice() const
  {
    return _centerPrice - Levels;
  }

  Price maxPrice() const
  {
    return _centerPrice + Levels;
  }

  bool checkPrice(Price price) const
  {
    const Price minPrice = this->minPrice();
    const Price maxPrice = this->maxPrice();
    return bl::in_range(price, minPrice, maxPrice);
  }

  PriceLevel& at_index(Index index)
  {
    return _levels[index];
  }

  const PriceLevel& at_index(Index index) const
  {
    return _levels[index];
  }

  PriceLevel& at_price(Price price)
  {
    const Index index = priceToIndex(price);
    return _levels[index];
  }

  const PriceLevel& at_price(Price price) const
  {
    const Index index = priceToIndex(price);
    return _levels[index];
  }

  Index priceToIndex(Price price) const
  {
    Assert(checkPrice(price));
    
    price -= _centerPrice;
    price += _centerIndex;
    price &= Mask;

    return ((Index)price);
  }

  template<std::size_t N>
  void shiftUp(RingBufferSPSC<Event, N>& bufferOut)
  {
    Assert(maxPrice() < MaxPrice);
    
    _levels[_centerIndex - Levels].expireOrders(bufferOut);

    _centerPrice += 1;
    _centerIndex += 1;
    _centerIndex &= Mask;
  }

  template<std::size_t N>
  void shiftDown(RingBufferSPSC<Event, N>& bufferOut)
  {
    Assert(minPrice() > 1);
    
    PriceLevel& level = _levels[_centerIndex + Levels];
    level.expireOrders(bufferOut);

    _centerPrice -= 1;
    _centerIndex -= 1;
    _centerIndex &= Mask;
  }

private:
  Price _centerPrice;
  Index _centerIndex;

  Array<PriceLevel, Size> _levels;
};
