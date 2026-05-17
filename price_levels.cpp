/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "./assert.hpp"
#include "./events.hpp"
#include "./price_levels.hpp"
#include "./ring_buffer_spsc.hpp"

void test_price_levels_basic_1()
{
  PriceLevels<3, 4> pl(7);

  Assert(pl.centerPrice() == 7);
  Assert(pl.minPrice() == 4);
  Assert(pl.maxPrice() == 11);

  Assert(pl.priceToIndex(4) == 0);
  Assert(pl.priceToIndex(5) == 1);
  Assert(pl.priceToIndex(6) == 2);
  Assert(pl.priceToIndex(7) == 3);
  Assert(pl.priceToIndex(8) == 4);
  Assert(pl.priceToIndex(9) == 5);
  Assert(pl.priceToIndex(10) == 6);
  Assert(pl.priceToIndex(11) == 7);
}

void test_price_levels_basic_2()
{
  PriceLevels<3, 60> pl(32);

  Assert(pl.centerPrice() == 32);
  Assert(pl.minPrice() == 32 - 3);
  Assert(pl.maxPrice() == 32 + 60);

  Assert(pl.priceToIndex(32 - 3) == 0);
  Assert(pl.priceToIndex(32 - 2) == 1);
  Assert(pl.priceToIndex(32 - 1) == 2);
  Assert(pl.priceToIndex(32) == 3);
  Assert(pl.priceToIndex(33) == 4);
  Assert(pl.priceToIndex(34) == 5);
  Assert(pl.priceToIndex(92) == 63);
}

void test_price_levels_shift_1()
{
  PriceLevels<3, 60> pl(100);
  RingBufferSPSC<Event, 1024> buffer;


  for(std::size_t i = 0; i < 68; ++i) {
    pl.shiftDown(buffer);
  }

  Assert(pl.centerPrice() == 100 - 68);
  Assert(pl.minPrice() == 100 - 3 - 68);
  Assert(pl.maxPrice() == 100 + 60 - 68);

  for(std::size_t i = 0; i < 68; ++i) {
    pl.shiftUp(buffer);
  }

  Assert(pl.centerPrice() == 100);
  Assert(pl.minPrice() == 100 - 3);
  Assert(pl.maxPrice() == 100 + 60);

  Assert(pl.priceToIndex(100 - 3) == 0);
  Assert(pl.priceToIndex(100 - 2) == 1);
  Assert(pl.priceToIndex(100 - 1) == 2);
  Assert(pl.priceToIndex(100) == 3);
  Assert(pl.priceToIndex(101) == 4);
  Assert(pl.priceToIndex(102) == 5);
  Assert(pl.priceToIndex(160) == 63);
}

void test_price_levels_shift_2()
{
  PriceLevels<3, 60> pl(100);
  RingBufferSPSC<Event, 1024> buffer;

  for(std::size_t i = 0; i < 68; ++i) {
    pl.shiftUp(buffer);
  }
  
  Assert(pl.centerPrice() == 100 + 68);
  Assert(pl.minPrice() == 100 - 3 + 68);
  Assert(pl.maxPrice() == 100 + 60 + 68);

  for(std::size_t i = 0; i < 68; ++i) {
    pl.shiftDown(buffer);
  }

  Assert(pl.centerPrice() == 100);
  Assert(pl.minPrice() == 100 - 3);
  Assert(pl.maxPrice() == 100 + 60);

  Assert(pl.priceToIndex(100 - 3) == 0);
  Assert(pl.priceToIndex(100 - 2) == 1);
  Assert(pl.priceToIndex(100 - 1) == 2);
  Assert(pl.priceToIndex(100) == 3);
  Assert(pl.priceToIndex(101) == 4);
  Assert(pl.priceToIndex(160) == 63);
}

void test_price_levels_storage_access()
{
  PriceLevels<3, 4> pl(6);
  RingBufferSPSC<Event, 1024> buffer;

  pl.at_price(3).push_order(3, 33, buffer);
  pl.at_price(4).push_order(4, 44, buffer);
  pl.at_price(5).push_order(5, 55, buffer);
  pl.at_price(6).push_order(6, 66, buffer);
  pl.at_price(7).push_order(7, 77, buffer);
  pl.at_price(8).push_order(8, 88, buffer);
  pl.at_price(9).push_order(9, 99, buffer);

  Assert(pl.at_price(3).order() == Order(3, 33));
  Assert(pl.at_price(4).order() == Order(4, 44));
  Assert(pl.at_price(5).order() == Order(5, 55));
  Assert(pl.at_price(6).order() == Order(6, 66));
  Assert(pl.at_price(7).order() == Order(7, 77));
  Assert(pl.at_price(8).order() == Order(8, 88));
  Assert(pl.at_price(9).order() == Order(9, 99));
}

int main()
{
  test_price_levels_basic_1();
  test_price_levels_basic_2();

  test_price_levels_shift_1();
  test_price_levels_shift_2();

  test_price_levels_storage_access();
}
