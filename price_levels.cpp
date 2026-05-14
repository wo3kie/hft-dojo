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
  {
    PriceLevels<3> pl(7);
    Assert(pl.centerPrice() == 7);
    Assert(pl.levels() == 3);
    Assert(pl.minPrice() == 4);
    Assert(pl.maxPrice() == 10);
    Assert(pl.checkPrice(0) == false);
    Assert(pl.checkPrice(1) == false);
    Assert(pl.checkPrice(4) == true);
    Assert(pl.checkPrice(7) == true);
    Assert(pl.checkPrice(10) == true);
    Assert(pl.checkPrice(11) == false);

    Assert(pl.priceToIndex(4) == 0);
    Assert(pl.priceToIndex(5) == 1);
    Assert(pl.priceToIndex(6) == 2);
    Assert(pl.priceToIndex(7) == 3);
    Assert(pl.priceToIndex(8) == 4);
    Assert(pl.priceToIndex(9) == 5);
    Assert(pl.priceToIndex(10) == 6);
  }

  {
    PriceLevels<31> pl(32);
    Assert(pl.centerPrice() == 32);
    Assert(pl.levels() == 31);
    Assert(pl.minPrice() == 1);
    Assert(pl.maxPrice() == 63);
    Assert(pl.checkPrice(0) == false);
    Assert(pl.checkPrice(1) == true);
    Assert(pl.checkPrice(31) == true);
    Assert(pl.checkPrice(32) == true);
    Assert(pl.checkPrice(63) == true);
    Assert(pl.checkPrice(64) == false);

    Assert(pl.priceToIndex(1) == 0);
    Assert(pl.priceToIndex(31) == 30);
    Assert(pl.priceToIndex(32) == 31);
    Assert(pl.priceToIndex(33) == 32);
    Assert(pl.priceToIndex(63) == 62);
  }
}

void test_price_levels_access_by_index()
{
  PriceLevels<31> pl(100);

  PriceLevel& level = pl.index(0);
  Assert(&pl.index(0) == &level);

  PriceLevel& level_pos = pl.index(5);
  Assert(&pl.index(5) == &level_pos);

  PriceLevel& level_neg = pl.index(-5);
  Assert(&pl.index(-5) == &level_neg);
}

void test_price_levels_access_by_price()
{
  PriceLevels<31> pl(100);

  PriceLevel& level100 = pl.price(100);
  Assert(&pl.price(100) == &level100);

  PriceLevel& level105 = pl.price(105);
  Assert(&pl.price(105) == &level105);

  PriceLevel& level95 = pl.price(95);
  Assert(&pl.price(95) == &level95);
}

void test_price_levels_price_to_index_conversion()
{
  PriceLevels<31> pl(100);

  Assert(pl.checkPrice(68) == false);
  Assert(pl.checkPrice(69) == true);

  Assert(pl.priceToIndex(69) == 0);
  Assert(pl.priceToIndex(99) == 30);
  Assert(pl.priceToIndex(100) == 31);
  Assert(pl.priceToIndex(101) == 32);
  Assert(pl.priceToIndex(131) == 62);

  Assert(pl.checkPrice(131) == true);
  Assert(pl.checkPrice(132) == false);
}

void test_price_levels_shift_up()
{
  PriceLevels<31> pl(100);
  Assert(pl.centerPrice() == 100);

  pl.shiftUp();
  Assert(pl.centerPrice() == 101);

  pl.shiftUp();
  Assert(pl.centerPrice() == 102);
}

void test_price_levels_shift_down()
{
  PriceLevels<31> pl(100);
  Assert(pl.centerPrice() == 100);

  pl.shiftDown();
  Assert(pl.centerPrice() == 99);

  pl.shiftDown();
  Assert(pl.centerPrice() == 98);
}

void test_price_levels_shift_with_price_access()
{
  PriceLevels<31> pl(100);

  pl.shiftUp();
  Assert(pl.centerPrice() == 101);
  Assert(pl.minPrice() == 70);
  Assert(pl.maxPrice() == 132);
}

int main()
{
  test_price_levels_constructor();
  test_price_levels_access_by_index();
  test_price_levels_access_by_price();
  test_price_levels_price_to_index_conversion();
  test_price_levels_shift_up();
  test_price_levels_shift_down();
  test_price_levels_shift_with_price_access();
}
