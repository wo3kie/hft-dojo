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
#include "./events.hpp"
#include "./flat_list.hpp"
#include "./likely.hpp"
#include "./order.hpp"
#include "./ring_buffer_spsc.hpp"

template<uint32_t Orders>
struct PriceLevel
{
public:
  PriceLevel() = default;
  PriceLevel(PriceLevel&&) = delete;
  PriceLevel(const PriceLevel&) = delete;

  PriceLevel& operator=(PriceLevel&&) = delete;
  PriceLevel& operator=(const PriceLevel&) = delete;

  int32_t balance{0};
  FlatList<Order, Orders> orders;
};

template<uint32_t LevelsBelow, uint32_t LevelsAbove, uint32_t Orders = 32>
struct PriceLevels
{
  constexpr static uint32_t Mask = LevelsBelow + LevelsAbove;

public:
  PriceLevels(Price centerPrice)
    : _centerPrice(centerPrice)
    , _centerIndex(LevelsBelow)
  {
    Assert(centerPrice > LevelsBelow);
    Assert(centerPrice < MaxPrice - LevelsAbove);
  }

  PriceLevels(PriceLevels&&) = delete;
  PriceLevels(const PriceLevels&) = delete;

  PriceLevels& operator=(PriceLevels&&) = delete;
  PriceLevels& operator=(const PriceLevels&) = delete;

public:
  static constexpr uint32_t levels_below()
  {
    return LevelsBelow;
  }

  static constexpr uint32_t levels_above()
  {
    return LevelsAbove;
  }

  static constexpr uint32_t orders()
  {
    return Orders;
  }

  Price center_price() const
  {
    return _centerPrice;
  }

  Price min_price() const
  {
    return _centerPrice - LevelsBelow;
  }

  Price max_price() const
  {
    return _centerPrice + LevelsAbove;
  }

  bool check_price(Price price) const
  {
    const Price minPrice = this->min_price();
    const Price maxPrice = this->max_price();
    return bl::in_range(price, minPrice, maxPrice);
  }

  PriceLevel<Orders>& at_index(Index index)
  {
    return _levels[index];
  }

  const PriceLevel<Orders>& at_index(Index index) const
  {
    return _levels[index];
  }

  PriceLevel<Orders>& at_price(Price price)
  {
    const Index index = price_to_index(price);
    return _levels[index];
  }

  const PriceLevel<Orders>& at_price(Price price) const
  {
    const Index index = price_to_index(price);
    return _levels[index];
  }

  Index price_to_index(Price price) const
  {
    Assert(check_price(price));

    price -= _centerPrice;
    price += _centerIndex;
    price &= Mask;

    return ((Index)price);
  }

  void shift_up(QueueOut& out)
  {
    Assert(max_price() < MaxPrice);

    _centerPrice += 1;
    _centerIndex += 1;
    _centerIndex &= Mask;
  }

  void shift_down(QueueOut& out)
  {
    Assert(min_price() > 1);

    _centerPrice -= 1;
    _centerIndex -= 1;
    _centerIndex &= Mask;
  }

private:
  Price _centerPrice;
  Index _centerIndex;

  Array<PriceLevel<Orders>, LevelsBelow + LevelsAbove + 1> _levels;
};
