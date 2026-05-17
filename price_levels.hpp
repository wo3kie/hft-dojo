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
  FlatList<Order, 32>& orders()
  {
    return _orders;
  }

  const FlatList<Order, 32>& orders() const
  {
    return _orders;
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

#ifdef NDEBUG
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

  PriceLevel& index(Index index)
  {
    return _levels[index];
  }

  const PriceLevel& index(Index index) const
  {
    return _levels[index];
  }

  PriceLevel& price(Price price)
  {
    const Index index = priceToIndex(price);
    return _levels[index];
  }

  const PriceLevel& price(Price price) const
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
