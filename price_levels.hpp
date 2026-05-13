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


template<int32_t Levels>
struct PriceLevels
{
  static_assert((Levels & (Levels + 1)) == 0);

  PriceLevels(int32_t centerPrice = 0, int32_t centerIndex = 0)
    : _centerPrice(centerPrice)
    , _centerIndex(centerIndex)
    
  {
  }

  int32_t centerPrice() const
  {
    return _centerPrice;
  }

  int32_t levels() const {
    return Levels;
  }

  int32_t minPrice() const
  {
    return bl::max0(_centerPrice - Levels);
  }

  int32_t maxPrice() const
  {
    return _centerPrice + Levels;
  }

  PriceLevel& index(int32_t index)
  {
    index += _centerIndex;
    return _levels[index];
  }

  const PriceLevel& index(int32_t index) const
  {
    index += _centerIndex;
    return _levels[index];
  }

  PriceLevel& price(int32_t price)
  {
    price -= _centerPrice;
    price += _centerIndex;
    return _levels[price];
  }

  const PriceLevel& price(int32_t price) const
  {
    price -= _centerPrice;
    price += _centerIndex;
    return _levels[price];
  }

  int32_t priceToIndex(int32_t price) const
  {
    price -= _centerPrice;
    return price;
  }

  void shiftUp() {
    _centerPrice += 1;
    _centerIndex += 1;
    _centerIndex &= (Levels - 1);
  }

  void shiftDown() {
    _centerPrice -= 1;
    _centerIndex -= 1;
    _centerIndex &= (Levels - 1);
  }

private:
  int32_t _centerPrice;
  int32_t _centerIndex;

  Array<PriceLevel, Levels + 1 + Levels + 1> _levels;
};
