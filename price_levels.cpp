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
  PriceLevels<3> pl(7);

  Assert(pl.centerPrice() == 7);
  Assert(pl.minPrice() == 4);
  Assert(pl.maxPrice() == 10);

  Assert(pl.priceToIndex(4) == 0);
  Assert(pl.priceToIndex(5) == 1);
  Assert(pl.priceToIndex(6) == 2);
  Assert(pl.priceToIndex(7) == 3);
  Assert(pl.priceToIndex(8) == 4);
  Assert(pl.priceToIndex(9) == 5);
  Assert(pl.priceToIndex(10) == 6);
}

void test_price_levels_basic_2()
{
  PriceLevels<31> pl(32);

  Assert(pl.centerPrice() == 32);
  Assert(pl.minPrice() == 1);
  Assert(pl.maxPrice() == 63);

  Assert(pl.priceToIndex(1) == 0);
  Assert(pl.priceToIndex(31) == 30);
  Assert(pl.priceToIndex(32) == 31);
  Assert(pl.priceToIndex(33) == 32);
  Assert(pl.priceToIndex(63) == 62);
}

void test_price_levels_shift_1()
{
  PriceLevels<31> pl(100);
  RingBufferSPSC<Event, 1024> buffer;


  for(std::size_t i = 0; i < 68; ++i) {
    pl.shiftDown(buffer);
  }

  Assert(pl.centerPrice() == 100 - 68);
  Assert(pl.minPrice() == 100 - 31 - 68);
  Assert(pl.maxPrice() == 100 + 31 - 68);

  for(std::size_t i = 0; i < 68; ++i) {
    pl.shiftUp(buffer);
  }

  Assert(pl.centerPrice() == 100);
  Assert(pl.minPrice() == 100 - 31);
  Assert(pl.maxPrice() == 100 + 31);

  Assert(pl.priceToIndex(69) == 0);
  Assert(pl.priceToIndex(99) == 30);
  Assert(pl.priceToIndex(100) == 31);
  Assert(pl.priceToIndex(101) == 32);
  Assert(pl.priceToIndex(131) == 62);
}

void test_price_levels_shift_2()
{
  PriceLevels<31> pl(100);
  RingBufferSPSC<Event, 1024> buffer;

  for(std::size_t i = 0; i < 68; ++i) {
    pl.shiftUp(buffer);
  }
  
  Assert(pl.centerPrice() == 100 + 68);
  Assert(pl.minPrice() == 100 - 31 + 68);
  Assert(pl.maxPrice() == 100 + 31 + 68);

  for(std::size_t i = 0; i < 68; ++i) {
    pl.shiftDown(buffer);
  }

  Assert(pl.centerPrice() == 100);
  Assert(pl.minPrice() == 69);
  Assert(pl.maxPrice() == 131);

  Assert(pl.priceToIndex(69) == 0);
  Assert(pl.priceToIndex(99) == 30);
  Assert(pl.priceToIndex(100) == 31);
  Assert(pl.priceToIndex(101) == 32);
  Assert(pl.priceToIndex(131) == 62);
}

void test_price_levels_storage_access()
{
  PriceLevels<3> pl(6);

  int32_t node3 = pl.price(3).orders.push_back(Order(3, 33));
  int32_t node4 = pl.price(4).orders.push_back(Order(4, 44));
  int32_t node5 = pl.price(5).orders.push_back(Order(5, 55));
  int32_t node6 = pl.price(6).orders.push_back(Order(6, 66));
  int32_t node7 = pl.price(7).orders.push_back(Order(7, 77));
  int32_t node8 = pl.price(8).orders.push_back(Order(8, 88));
  int32_t node9 = pl.price(9).orders.push_back(Order(9, 99));

  Assert( pl.price(3).orders.front() == Order(3, 33) );
  Assert(pl.price(4).orders.front() == Order(4, 44));
  Assert(pl.price(5).orders.front() == Order(5, 55));
  Assert(pl.price(6).orders.front() == Order(6, 66));
  Assert(pl.price(7).orders.front() == Order(7, 77));
  Assert(pl.price(8).orders.front() == Order(8, 88));
  Assert(pl.price(9).orders.front() == Order(9, 99));
}

int main()
{
  test_price_levels_basic_1();
  test_price_levels_basic_2();

  test_price_levels_shift_1();
  test_price_levels_shift_2();

  test_price_levels_storage_access();
}
