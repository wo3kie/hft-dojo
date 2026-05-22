/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "assert.hpp"
#include "events.hpp"
#include "price_levels.hpp"
#include "ring_buffer_spsc.hpp"

void test_price_levels_basic()
{
  PriceLevels<3, 60> pl(32);

  Assert(pl.center_price() == 32);
  Assert(pl.min_price() == 32 - 3);
  Assert(pl.max_price() == 32 + 60);

  Assert(pl.price_to_index(32 - 3) == 0);
  Assert(pl.price_to_index(32 - 2) == 1);
  Assert(pl.price_to_index(32 - 1) == 2);
  Assert(pl.price_to_index(32 + 0) == 3);
  Assert(pl.price_to_index(32 + 1) == 4);
  Assert(pl.price_to_index(32 + 2) == 5);
  Assert(pl.price_to_index(32 + 60) == 63);
}

void test_price_levels_shift_1()
{
  PriceLevels<3, 60> pl(100);
  QueueOut buffer;


  for(std::size_t i = 0; i < 68; ++i) {
    pl.shift_down(buffer);
  }

  Assert(pl.center_price() == 100 - 68);
  Assert(pl.min_price() == 100 - 3 - 68);
  Assert(pl.max_price() == 100 + 60 - 68);

  for(std::size_t i = 0; i < 68; ++i) {
    pl.shift_up(buffer);
  }

  Assert(pl.center_price() == 100);
  Assert(pl.min_price() == 100 - 3);
  Assert(pl.max_price() == 100 + 60);

  Assert(pl.price_to_index(100 - 3) == 0);
  Assert(pl.price_to_index(100 - 2) == 1);
  Assert(pl.price_to_index(100 - 1) == 2);
  Assert(pl.price_to_index(100) == 3);
  Assert(pl.price_to_index(101) == 4);
  Assert(pl.price_to_index(102) == 5);
  Assert(pl.price_to_index(160) == 63);
}

void test_price_levels_shift_2()
{
  PriceLevels<3, 60> pl(100);
  QueueOut buffer;

  for(std::size_t i = 0; i < 68; ++i) {
    pl.shift_up(buffer);
  }
  
  Assert(pl.center_price() == 100 + 68);
  Assert(pl.min_price() == 100 - 3 + 68);
  Assert(pl.max_price() == 100 + 60 + 68);

  for(std::size_t i = 0; i < 68; ++i) {
    pl.shift_down(buffer);
  }

  Assert(pl.center_price() == 100);
  Assert(pl.min_price() == 100 - 3);
  Assert(pl.max_price() == 100 + 60);

  Assert(pl.price_to_index(100 - 3) == 0);
  Assert(pl.price_to_index(100 - 2) == 1);
  Assert(pl.price_to_index(100 - 1) == 2);
  Assert(pl.price_to_index(100) == 3);
  Assert(pl.price_to_index(101) == 4);
  Assert(pl.price_to_index(160) == 63);
}

void test_price_levels_storage_access()
{
  PriceLevels<3, 4> pl(6);

  pl.at_price(3).insert(3, 33);
  pl.at_price(4).insert(4, 44);
  pl.at_price(5).insert(5, 55);
  pl.at_price(6).insert(6, 66);
  pl.at_price(7).insert(7, 77);
  pl.at_price(8).insert(8, 88);
  pl.at_price(9).insert(9, 99);

  Assert(pl.at_price(3).order() == Order(3, 33));
  Assert(pl.at_price(4).order() == Order(4, 44));
  Assert(pl.at_price(5).order() == Order(5, 55));
  Assert(pl.at_price(6).order() == Order(6, 66));
  Assert(pl.at_price(7).order() == Order(7, 77));
  Assert(pl.at_price(8).order() == Order(8, 88));
  Assert(pl.at_price(9).order() == Order(9, 99));
}

void test_price_levels_preserve_storage_for_in_range_prices_after_shifts()
{
  PriceLevels<3, 4> pl(10);
  QueueOut buffer;

  pl.at_price(9).insert(9, 90);
  pl.at_price(10).insert(10, 100);
  pl.at_price(11).insert(11, 110);

  pl.shift_up(buffer);
  pl.shift_up(buffer);

  Assert(pl.center_price() == 12);
  Assert(pl.at_price(9).order() == Order(9, 90));
  Assert(pl.at_price(10).order() == Order(10, 100));
  Assert(pl.at_price(11).order() == Order(11, 110));

  pl.shift_down(buffer);
  pl.shift_down(buffer);

  Assert(pl.center_price() == 10);
  Assert(pl.at_price(9).order() == Order(9, 90));
  Assert(pl.at_price(10).order() == Order(10, 100));
  Assert(pl.at_price(11).order() == Order(11, 110));
}

int main()
{
  test_price_levels_basic();

  test_price_levels_shift_1();
  test_price_levels_shift_2();

  test_price_levels_storage_access();
  test_price_levels_preserve_storage_for_in_range_prices_after_shifts();
}
