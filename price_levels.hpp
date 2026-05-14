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
  constexpr static uint32_t Size = Levels + 1 + Levels + 1;
  constexpr static uint32_t Mask = Size - 1;

  static_assert((Levels & (Levels + 1)) == 0);

  PriceLevels(Price centerPrice)
    : _centerPrice(centerPrice)
    , _centerIndex(Levels)
  {
    Assert(centerPrice > Levels);
    Assert(centerPrice < MaxPrice - Levels);
  }

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
    Price minPrice = this->minPrice();
    Price maxPrice = this->maxPrice();
    bool result = bl::in_range(price, minPrice, maxPrice);
    return result;
  }

  PriceLevel& index(Index index)
  {
    index &= Mask;
    PriceLevel& level = _levels[index];
    return level;
  }

  const PriceLevel& index(Index index) const
  {
    index &= Mask;
    const PriceLevel& level = _levels[index];
    return level;
  }

  PriceLevel& price(Price price)
  {
    Index index = priceToIndex(price);
    PriceLevel& level = _levels[index];
    return level;
  }

  const PriceLevel& price(Price price) const
  {
    Index index = priceToIndex(price);
    const PriceLevel& level = _levels[index];
    return level;
  }

  Index priceToIndex(Price price) const
  {
    {
      Assert(checkPrice(price));
    }
    
    Index index = (Index)price;
    index += _centerIndex;
    index -= _centerPrice;
    index &= Mask;
    return index;
  }

  void shiftUp()
  {
    {
      Assert(_centerPrice <= MaxPrice - Levels);
    }

    _centerPrice += 1;
    _centerIndex += 1;
    _centerIndex &= Mask;
  }

  void shiftDown()
  {
    {
      Assert(_centerPrice >= Levels);
    }
    
    _centerPrice -= 1;
    _centerIndex -= 1;
    _centerIndex &= Mask;
  }

private:
  Price _centerPrice;
  Index _centerIndex;

  Array<PriceLevel, Size> _levels;
};
