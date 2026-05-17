/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "./order_book.hpp"
#include "./assert.hpp"
#include "./ring_buffer_spsc.hpp"

void test_order_book_insert_buy_order()
{
  RingBufferSPSC<Event, 1024> buffer;
  OrderBook<31, 32> ob(buffer, 100);

  ob.insertBuyOrder(1, 100, 10);
  Assert(ob.topBuyPrice() == 100);
  Assert(ob.topBuyIndex() == 32);
}

void test_order_book_insert_sell_order()
{
  RingBufferSPSC<Event, 1024> buffer;
  OrderBook<31, 32> ob(buffer, 100);

  ob.insertSellOrder(1, 100, 10);
  Assert(ob.topSellPrice() == 100);
  Assert(ob.topSellIndex() == 31);
}

void test_order_book_shift_up()
{
  RingBufferSPSC<Event, 1024> buffer;
  OrderBook<31, 32> ob(buffer, 100);

  ob.shiftUp();
  Assert(ob.centerPrice() == 101);
}

void test_order_book_shift_down()
{
  RingBufferSPSC<Event, 1024> buffer;
  OrderBook<31, 32> ob(buffer, 100);

  ob.shiftDown();
  Assert(ob.centerPrice() == 99);
}

int main()
{
  test_order_book_insert_buy_order();
  test_order_book_insert_sell_order();
  test_order_book_shift_up();
  test_order_book_shift_down();
}
