/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "order_book.hpp"
#include "assert.hpp"
#include "ring_buffer_spsc.hpp"

using TestOrderBook = OrderBook<31, 32>;

void test_order_book_insert_buy_order_updates_best_buy_price()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  ob.insert_buy_order(1, 100, 10);
  Assert(ob.buy_price_from() == 100);
  Assert(ob.buy_index_from() == 32);
}

void test_order_book_insert_sell_order_updates_best_sell_price()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  ob.insert_sell_order(1, 100, 10);
  Assert(ob.sell_price_from() == 100);
  Assert(ob.sell_index_from() == 31);
}

void test_order_book_tracks_best_buy_price_across_levels()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  ob.insert_buy_order(1, 99, 10);
  Assert(buffer.pop() == CreateAccepted(1, 0));

  ob.insert_buy_order(2, 101, 20);
  Assert(buffer.pop() == CreateAccepted(2, 0));

  ob.insert_buy_order(3, 100, 30);
  Assert(buffer.pop() == CreateAccepted(3, 0));

  ob.insert_sell_order(4, 101, 10);
  Assert(buffer.pop() == CreateAccepted(4, 0));

  ob.insert_sell_order(5, 99, 20);
  Assert(buffer.pop() == CreateAccepted(5, 0));

  ob.insert_sell_order(6, 100, 30);
  Assert(buffer.pop() == CreateAccepted(6, 0));

  Assert(ob.buy_price_from() == 101);
  Assert(ob.buy_index_from() == 33);
}

void test_order_book_tracks_best_sell_price_across_levels()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  ob.insert_buy_order(1, 99, 10);
  Assert(buffer.pop() == CreateAccepted(1, 0));

  ob.insert_buy_order(2, 101, 20);
  Assert(buffer.pop() == CreateAccepted(2, 0));

  ob.insert_buy_order(3, 100, 30);
  Assert(buffer.pop() == CreateAccepted(3, 0));

  ob.insert_sell_order(4, 101, 10);
  Assert(buffer.pop() == CreateAccepted(4, 0));

  ob.insert_sell_order(5, 99, 20);
  Assert(buffer.pop() == CreateAccepted(5, 0));

  ob.insert_sell_order(6, 100, 30);
  Assert(buffer.pop() == CreateAccepted(6, 0));

  Assert(ob.sell_price_from() == 99);
  Assert(ob.sell_index_from() == 30);
}

void test_order_book_buy_price_to_uses_book_floor()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  Assert(ob.buy_price_to() == 68);
  Assert(ob.buy_price_to(90) == 90);
  Assert(ob.buy_price_to(67) == 68);
}

void test_order_book_sell_price_to_uses_book_ceiling()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  Assert(ob.sell_price_to() == 132);
  Assert(ob.sell_price_to(120) == 120);
  Assert(ob.sell_price_to(133) == 132);
}

void test_order_book_check_buy_price_accepts_values_within_range()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  Assert(ob.check_buy_price(68) == true);
  Assert(ob.check_buy_price(131) == true);
}

void test_order_book_check_buy_price_rejects_values_out_of_range()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  Assert(ob.check_buy_price(67) == false);
  Assert(ob.check_buy_price(132) == false);
}

void test_order_book_check_sell_price_accepts_values_within_range()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  Assert(ob.check_sell_price(69) == true);
  Assert(ob.check_sell_price(132) == true);
}

void test_order_book_check_sell_price_rejects_values_out_of_range()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  Assert(ob.check_sell_price(68) == false);
  Assert(ob.check_sell_price(133) == false);
}

void test_order_book_update_buy_order_accepts_matching_order()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  ob.insert_buy_order(1, 100, 10);
  Assert(buffer.pop() == CreateAccepted(1, 0));

  ob.update_buy_order(1, 0, 100, 15);
  Assert(buffer.pop() == UpdateAccepted(1));
}

void test_order_book_update_buy_order_rejects_unknown_order()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  ob.insert_buy_order(1, 100, 10);
  Assert(buffer.pop() == CreateAccepted(1, 0));

  ob.update_buy_order(99, 0, 100, 15);
  Assert(buffer.pop() == UpdateRejected(99, 15));
}

void test_order_book_cancel_buy_order_rejects_unknown_order()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  ob.insert_buy_order(1, 100, 10);
  Assert(buffer.pop() == CreateAccepted(1, 0));

  ob.cancel_buy_order(99, 0, 100);
  Assert(buffer.pop() == CancelRejected(99));
}

void test_order_book_cancel_buy_order_accepts_matching_order()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  ob.insert_buy_order(1, 100, 10);
  Assert(buffer.pop() == CreateAccepted(1, 0));

  ob.cancel_buy_order(1, 0, 100);
  Assert(buffer.pop() == CancelAccepted(1));
}

void test_order_book_update_sell_order_accepts_matching_order()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  ob.insert_sell_order(2, 100, 20);
  Assert(buffer.pop() == CreateAccepted(2, 0));

  ob.update_sell_order(2, 0, 100, 25);
  Assert(buffer.pop() == UpdateAccepted(2));
}

void test_order_book_update_sell_order_rejects_unknown_order()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  ob.insert_sell_order(2, 100, 20);
  Assert(buffer.pop() == CreateAccepted(2, 0));

  ob.update_sell_order(98, 0, 100, 25);
  Assert(buffer.pop() == UpdateRejected(98, 25));
}

void test_order_book_cancel_sell_order_rejects_unknown_order()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  ob.insert_sell_order(2, 100, 20);
  Assert(buffer.pop() == CreateAccepted(2, 0));

  ob.cancel_sell_order(98, 0, 100);
  Assert(buffer.pop() == CancelRejected(98));
}

void test_order_book_cancel_sell_order_accepts_matching_order()
{
  QueueOut buffer;
  TestOrderBook ob(buffer, 100);

  ob.insert_sell_order(2, 100, 20);
  Assert(buffer.pop() == CreateAccepted(2, 0));

  ob.cancel_sell_order(2, 0, 100);
  Assert(buffer.pop() == CancelAccepted(2));
}


int main()
{
  test_order_book_insert_buy_order_updates_best_buy_price();
  test_order_book_insert_sell_order_updates_best_sell_price();
  test_order_book_tracks_best_buy_price_across_levels();
  test_order_book_tracks_best_sell_price_across_levels();
  test_order_book_buy_price_to_uses_book_floor();
  test_order_book_sell_price_to_uses_book_ceiling();
  test_order_book_check_buy_price_accepts_values_within_range();
  test_order_book_check_buy_price_rejects_values_out_of_range();
  test_order_book_check_sell_price_accepts_values_within_range();
  test_order_book_check_sell_price_rejects_values_out_of_range();
  test_order_book_update_buy_order_accepts_matching_order();
  test_order_book_update_buy_order_rejects_unknown_order();
  test_order_book_cancel_buy_order_rejects_unknown_order();
  test_order_book_cancel_buy_order_accepts_matching_order();
  test_order_book_update_sell_order_accepts_matching_order();
  test_order_book_update_sell_order_rejects_unknown_order();
  test_order_book_cancel_sell_order_rejects_unknown_order();
  test_order_book_cancel_sell_order_accepts_matching_order();

  return 0;
}
