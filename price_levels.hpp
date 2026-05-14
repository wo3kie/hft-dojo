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

  void shiftUp()
  {
    Assert(maxPrice() < MaxPrice);
    
    _centerPrice += 1;
    _centerIndex += 1;
    _centerIndex &= Mask;
  }

  void shiftDown()
  {
    Assert(minPrice() > 1);
    
    _centerPrice -= 1;
    _centerIndex -= 1;
    _centerIndex &= Mask;
  }

private:
  Price _centerPrice;
  Index _centerIndex;

  Array<PriceLevel, Size> _levels;
};
