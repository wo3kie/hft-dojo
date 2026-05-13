#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "./array.hpp"
#include "./branchless.hpp"
#include "./flat_list.hpp"
#include "./order.hpp"


struct PriceLevel
{
  FlatList<32, Order> orders;
};


template<uint32_t Levels>
struct PriceLevels
{
  static_assert((Levels & (Levels + 1)) == 0);

  PriceLevels(Price centerPrice = 0, uint32_t centerIndex = 0)
    : _centerPrice(centerPrice)
    , _centerIndex(centerIndex)
    
  {
  }

  Price centerPrice() const
  {
    return _centerPrice;
  }

  uint32_t levels() const {
    return Levels;
  }

  Price minPrice() const
  {
    if (_centerPrice + 1 < Levels) {
      return 1;
    } else {
      return _centerPrice - Levels;
    }
  }

  Price maxPrice() const
  {
    return _centerPrice + Levels;
  }

  PriceLevel& index(Index index)
  {
    index += _centerIndex;
    return _levels[index];
  }

  const PriceLevel& index(Index index) const
  {
    index += _centerIndex;
    index &= (Levels + 1 + Levels + 1) - 1;
    return _levels[index];
  }

  PriceLevel& price(Price price)
  {
    price += _centerIndex;
    price -= _centerPrice;
    price &= (Levels + 1 + Levels + 1) - 1;
    return _levels[price];
  }

  const PriceLevel& price(Price price) const
  {
    price += _centerIndex;
    price -= _centerPrice;
    price &= (Levels + 1 + Levels + 1) - 1;
    return _levels[price];
  }

  Index priceToIndex(Price price) const
  {
    price -= _centerPrice;
    price &= (Levels + 1 + Levels + 1) - 1;
    return price;
  }

  void shiftUp() {
    _centerPrice += 1;
    _centerIndex += 1;
    _centerIndex &= (Levels + 1 + Levels + 1) - 1;
  }

  void shiftDown() {
    _centerPrice -= 1;
    _centerIndex -= 1;
    _centerIndex &= (Levels + 1 + Levels + 1) - 1;
  }

private:
  Price _centerPrice;
  Price _centerIndex;

  Array<PriceLevel, Levels + 1 + Levels + 1> _levels;
};
