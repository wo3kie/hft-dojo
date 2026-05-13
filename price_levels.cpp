/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "./price_levels.hpp"
#include "./assert.hpp"

void test_price_levels_constructor()
{
  PriceLevels<31> pl(100, 0);
  Assert(pl.centerPrice() == 100);
  Assert(pl.levels() == 31);
}

void test_price_levels_constructor_default()
{
  PriceLevels<63> pl;
  Assert(pl.centerPrice() == 0);
  Assert(pl.levels() == 63);
}

void test_price_levels_min_max_price()
{
  PriceLevels<31> pl(100, 0);
  Assert(pl.minPrice() == 69);
  Assert(pl.maxPrice() == 131);
}

void test_price_levels_min_price_below_zero()
{
  PriceLevels<31> pl(16, 0);
  Assert(pl.minPrice() == 0);
  Assert(pl.maxPrice() == 47);
}

void test_price_levels_access_by_index()
{
  PriceLevels<31> pl(100, 0);
  
  PriceLevel& level = pl.index(0);
  Assert(&pl.index(0) == &level);
  
  PriceLevel& level_pos = pl.index(5);
  Assert(&pl.index(5) == &level_pos);
  
  PriceLevel& level_neg = pl.index(-5);
  Assert(&pl.index(-5) == &level_neg);
}

void test_price_levels_access_by_price()
{
  PriceLevels<31> pl(100, 0);
  
  PriceLevel& level100 = pl.price(100);
  Assert(&pl.price(100) == &level100);
  
  PriceLevel& level105 = pl.price(105);
  Assert(&pl.price(105) == &level105);
  
  PriceLevel& level95 = pl.price(95);
  Assert(&pl.price(95) == &level95);
}

void test_price_levels_price_to_index_conversion()
{
  PriceLevels<31> pl(100, 0);
  
  Assert(pl.priceToIndex(100) == 0);
  Assert(pl.priceToIndex(105) == 5);
  Assert(pl.priceToIndex(95) == -5);
}

void test_price_levels_shift_up()
{
  PriceLevels<31> pl(100, 0);
  Assert(pl.centerPrice() == 100);
  
  pl.shiftUp();
  Assert(pl.centerPrice() == 101);
  
  pl.shiftUp();
  Assert(pl.centerPrice() == 102);
}

void test_price_levels_shift_down()
{
  PriceLevels<31> pl(100, 0);
  Assert(pl.centerPrice() == 100);
  
  pl.shiftDown();
  Assert(pl.centerPrice() == 99);
  
  pl.shiftDown();
  Assert(pl.centerPrice() == 98);
}

void test_price_levels_shift_with_price_access()
{
  PriceLevels<31> pl(100, 0);
  
  pl.shiftUp();
  Assert(pl.centerPrice() == 101);
  Assert(pl.minPrice() == 70);
  Assert(pl.maxPrice() == 132);
}

void test_price_levels_access_consistency()
{
  PriceLevels<31> pl(100, 0);
  
  PriceLevel& by_price = pl.price(105);
  PriceLevel& by_index = pl.index(5);
  Assert(&by_price == &by_index);
}

int main()
{
  test_price_levels_constructor();
  test_price_levels_constructor_default();
  test_price_levels_min_max_price();
  test_price_levels_min_price_below_zero();
  test_price_levels_access_by_index();
  test_price_levels_access_by_price();
  test_price_levels_price_to_index_conversion();
  test_price_levels_shift_up();
  test_price_levels_shift_down();
  test_price_levels_shift_with_price_access();
  test_price_levels_access_consistency();
}
