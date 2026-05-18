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

void test_order_book_tracks_best_prices_across_levels()
{
  RingBufferSPSC<Event, 1024> buffer;
  OrderBook<31, 32> ob(buffer, 100);

  ob.insertBuyOrder(1, 99, 10);
  Assert(buffer.pop() == CreateAccepted(1, 0));

  ob.insertBuyOrder(2, 101, 20);
  Assert(buffer.pop() == CreateAccepted(2, 0));

  ob.insertBuyOrder(3, 100, 30);
  Assert(buffer.pop() == CreateAccepted(3, 0));

  ob.insertSellOrder(4, 101, 10);
  Assert(buffer.pop() == CreateAccepted(4, 0));

  ob.insertSellOrder(5, 99, 20);
  Assert(buffer.pop() == CreateAccepted(5, 0));

  ob.insertSellOrder(6, 100, 30);
  Assert(buffer.pop() == CreateAccepted(6, 0));

  Assert(ob.topBuyPrice() == 101);
  Assert(ob.topBuyIndex() == 33);

  Assert(ob.topSellPrice() == 99);
  Assert(ob.topSellIndex() == 30);
}

void test_order_book_price_limits_and_range_checks()
{
  RingBufferSPSC<Event, 1024> buffer;
  OrderBook<31, 32> ob(buffer, 100);

  Assert(ob.buyPriceLimit() == 68);
  Assert(ob.buyPriceLimit(90) == 90);
  Assert(ob.buyPriceLimit(67) == 68);

  Assert(ob.sellPriceLimit() == 132);
  Assert(ob.sellPriceLimit(120) == 120);
  Assert(ob.sellPriceLimit(133) == 132);

  Assert(ob.checkBuyPrice(68) == true);
  Assert(ob.checkBuyPrice(131) == true);
  Assert(ob.checkBuyPrice(67) == false);
  Assert(ob.checkBuyPrice(132) == false);

  Assert(ob.checkSellPrice(69) == true);
  Assert(ob.checkSellPrice(132) == true);
  Assert(ob.checkSellPrice(68) == false);
  Assert(ob.checkSellPrice(133) == false);
}

void test_order_book_update_and_cancel_orders()
{
  RingBufferSPSC<Event, 1024> buffer;
  OrderBook<31, 32> ob(buffer, 100);

  ob.insertBuyOrder(1, 100, 10);
  Assert(buffer.pop() == CreateAccepted(1, 0));

  ob.updateBuyOrder(1, 0, 100, 15);
  Assert(buffer.pop() == UpdateAccepted(1));

  ob.updateBuyOrder(99, 0, 100, 15);
  Assert(buffer.pop() == UpdateRejected(99, 15));

  ob.cancelBuyOrder(99, 0, 100);
  Assert(buffer.pop() == CancelRejected(99));

  ob.cancelBuyOrder(1, 0, 100);
  Assert(buffer.pop() == CancelAccepted(1));

  ob.insertSellOrder(2, 100, 20);
  Assert(buffer.pop() == CreateAccepted(2, 0));

  ob.updateSellOrder(2, 0, 100, 25);
  Assert(buffer.pop() == UpdateAccepted(2));

  ob.updateSellOrder(98, 0, 100, 25);
  Assert(buffer.pop() == UpdateRejected(98, 25));

  ob.cancelSellOrder(98, 0, 100);
  Assert(buffer.pop() == CancelRejected(98));

  ob.cancelSellOrder(2, 0, 100);
  Assert(buffer.pop() == CancelAccepted(2));
}

void test_order_book_shift_expires_out_of_range_orders()
{
  {
    RingBufferSPSC<Event, 1024> buffer;
    OrderBook<31, 32> ob(buffer, 100);

    ob.insertSellOrder(1, 69, 10);
    Assert(buffer.pop() == CreateAccepted(1, 0));

    ob.shiftUp();
    Assert(buffer.pop() == OrderExpired(1));
    Assert(ob.centerPrice() == 101);
  }

  {
    RingBufferSPSC<Event, 1024> buffer;
    OrderBook<31, 32> ob(buffer, 100);

    ob.insertBuyOrder(2, 131, 10);
    Assert(buffer.pop() == CreateAccepted(2, 0));

    ob.shiftDown();
    Assert(buffer.pop() == OrderExpired(2));
    Assert(ob.centerPrice() == 99);
  }
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
  test_order_book_tracks_best_prices_across_levels();
  test_order_book_price_limits_and_range_checks();
  test_order_book_update_and_cancel_orders();
  test_order_book_shift_expires_out_of_range_orders();
  test_order_book_shift_up();
  test_order_book_shift_down();

  return 0;
}
