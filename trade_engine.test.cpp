/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "./trade_engine.hpp"
#include "./assert.hpp"

using TestTradeEngine = TradeEngine<3, 4>;

void test_matches_across_multiple_price_levels()
{
  TestTradeEngine engine(100);

  for(unsigned i = 100 - 4; i <= 100 + 3; ++i) {
    engine.insert_buy_order_PL(/* id */ 1000 * (i - 95), /* price */ i, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1000 * (i - 95), 0));
  }

  engine.insert_sell_order_PL(/* id */ 10000, /* price */ 103, /* qty */ 100);
  Assert(engine.out().pop() == Trade(/* price */ 103, /* qty */ 100, /* maker */ 10000, /* taker */ 8000));

  engine.insert_sell_order_PL(/* id */ 20000, /* price */ 101, /* qty */ 200);
  Assert(engine.out().pop() == Trade(/* price */ 102, /* qty */ 100, /* maker */ 20000, /* taker */ 7000));
  Assert(engine.out().pop() == Trade(/* price */ 101, /* qty */ 100, /* maker */ 20000, /* taker */ 6000));

  engine.insert_sell_order_PL(/* id */ 30000, /* price */ 98, /* qty */ 300);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 100, /* maker */ 30000, /* taker */ 5000));
  Assert(engine.out().pop() == Trade(/* price */ 99, /* qty */ 100, /* maker */ 30000, /* taker */ 4000));
  Assert(engine.out().pop() == Trade(/* price */ 98, /* qty */ 100, /* maker */ 30000, /* taker */ 3000));

  engine.insert_sell_order_MKT(/* id */ 40000, /* qty */ 300);
  Assert(engine.out().pop() == Trade(/* price */ 97, /* qty */ 100, /* maker */ 40000, /* taker */ 2000));
  Assert(engine.out().pop() == Trade(/* price */ 96, /* qty */ 100, /* maker */ 40000, /* taker */ 1000));
  Assert(engine.out().pop() == CreateRejected(/* id */ 40000, /* qty */ 100));
}

void test_buy_side_flow_with_update_cancel_and_resting_sell_remainder()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 150);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.update_buy_order(1, /* price */ 100, /* slot */ 0, /* qty */ 200);
  Assert(engine.out().pop() == UpdateAccepted(1));

  engine.insert_buy_order_PL(2, /* price */ 100, /* qty */ 200);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 2, /* slot */ 1));

  engine.update_buy_order(2, /* price */ 100, /* slot */ 1, /* newQty */ 150);
  Assert(engine.out().pop() == UpdateAccepted(2));

  engine.cancel_buy_order(1, /* price */ 100, /* slot */ 0);
  Assert(engine.out().pop() == CancelAccepted(1));

  engine.insert_sell_order_PL(3, /* price */ 100, /* qty */ 200);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 150, /* maker */ 3, /* taker */ 2));
  Assert(engine.out().pop() == CreateAccepted(/* id */ 3, /* slot */ 0));

  engine.insert_buy_order_PL(4, /* price */ 100, /* qty */ 50);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 50, /* maker */ 4, /* taker */ 3));
}

void test_sell_side_resting_order_matches_incoming_buy()
{
  TestTradeEngine engine(100);

  engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 200);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.insert_buy_order_PL(2, /* price */ 100, /* qty */ 200);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 200, /* maker */ 2, /* taker */ 1));
}

void test_partial_fill_incoming_sell_larger_than_resting_buy()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 180);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.insert_sell_order_PL(2, /* price */ 100, /* qty */ 200);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 180, /* maker */ 2, /* taker */ 1));
}

void test_partial_fill_incoming_buy_larger_than_resting_sell()
{
  TestTradeEngine engine(100);

  engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 200);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.insert_buy_order_PL(2, /* price */ 100, /* qty */ 180);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 180, /* maker */ 2, /* taker */ 1));
}

void test_partial_fill_rests_remaining_sell_qty()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.insert_sell_order_PL(2, /* price */ 100, /* qty */ 150);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 100, /* maker */ 2, /* taker */ 1));
  Assert(engine.out().pop() == CreateAccepted(/* id */ 2, /* slot */ 0));

  engine.insert_buy_order_PL(3, /* price */ 100, /* qty */ 50);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 50, /* maker */ 3, /* taker */ 2));
}

void test_partial_fill_rests_remaining_buy_qty()
{
  TestTradeEngine engine(100);

  engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.insert_buy_order_PL(2, /* price */ 100, /* qty */ 150);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 100, /* maker */ 2, /* taker */ 1));
  Assert(engine.out().pop() == CreateAccepted(/* id */ 2, /* slot */ 0));

  engine.insert_sell_order_PL(3, /* price */ 100, /* qty */ 50);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 50, /* maker */ 3, /* taker */ 2));
}

void test_multiple_buy_orders_match_fifo_within_single_price_level()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 180);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.insert_buy_order_PL(2, /* price */ 100, /* qty */ 20);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 2, /* slot */ 1));

  engine.insert_sell_order_PL(3, /* price */ 100, /* qty */ 200);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 180, /* maker */ 3, /* taker */ 1));
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 20, /* maker */ 3, /* taker */ 2));
}

void test_multiple_sell_orders_match_fifo_within_single_price_level()
{
  TestTradeEngine engine(100);

  engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 200);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.insert_sell_order_PL(2, /* price */ 100, /* qty */ 20);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 2, /* slot */ 1));

  engine.insert_buy_order_PL(3, /* price */ 100, /* qty */ 220);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 200, /* maker */ 3, /* taker */ 1));
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 20, /* maker */ 3, /* taker */ 2));
}

void test_rejects_buy_order_outside_supported_price_window()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 95, /* qty */ 100);
  Assert(engine.out().pop() == CreateRejected(/* id */ 1, /* qty */ 100));

  engine.insert_buy_order_PL(1, /* price */ 96, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.insert_buy_order_PL(2, /* price */ 97, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 2, /* slot */ 0));

  engine.insert_buy_order_PL(3, /* price */ 98, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 3, /* slot */ 0));

  engine.insert_buy_order_PL(4, /* price */ 99, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 4, /* slot */ 0));

  engine.insert_buy_order_PL(5, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 5, /* slot */ 0));

  engine.insert_buy_order_PL(6, /* price */ 101, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 6, /* slot */ 0));

  engine.insert_buy_order_PL(7, /* price */ 102, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 7, /* slot */ 0));

  engine.insert_buy_order_PL(8, /* price */ 103, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 8, /* slot */ 0));

  engine.insert_buy_order_PL(9, /* price */ 104, /* qty */ 100);
  Assert(engine.out().pop() == CreateRejected(/* id */ 9, /* qty */ 100));
}

void test_sell_market_order_consumes_buy_book_and_rejects_remainder()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.insert_buy_order_PL(2, /* price */ 99, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 2, /* slot */ 0));

  engine.insert_buy_order_PL(3, /* price */ 98, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 3, /* slot */ 0));

  engine.insert_buy_order_PL(4, /* price */ 97, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 4, /* slot */ 0));

  engine.insert_buy_order_PL(5, /* price */ 96, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 5, /* slot */ 0));

  engine.insert_buy_order_PL(6, /* price */ 95, /* qty */ 100);
  Assert(engine.out().pop() == CreateRejected(/* id */ 6, /* qty */ 100));

  engine.insert_sell_order_MKT(7, /* qty */ 600);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 100, /* maker */ 7, /* taker */ 1));
  Assert(engine.out().pop() == Trade(/* price */ 99, /* qty */ 100, /* maker */ 7, /* taker */ 2));
  Assert(engine.out().pop() == Trade(/* price */ 98, /* qty */ 100, /* maker */ 7, /* taker */ 3));
  Assert(engine.out().pop() == Trade(/* price */ 97, /* qty */ 100, /* maker */ 7, /* taker */ 4));
  Assert(engine.out().pop() == Trade(/* price */ 96, /* qty */ 100, /* maker */ 7, /* taker */ 5));
  Assert(engine.out().pop() == CreateRejected(/* id */ 7, /* qty */ 100));
}

void test_buy_market_order_consumes_sell_book_and_rejects_remainder()
{
  TestTradeEngine engine(100);

  engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.insert_sell_order_PL(2, /* price */ 101, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 2, /* slot */ 0));

  engine.insert_sell_order_PL(3, /* price */ 102, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 3, /* slot */ 0));

  engine.insert_sell_order_PL(4, /* price */ 103, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 4, /* slot */ 0));

  engine.insert_sell_order_PL(5, /* price */ 104, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 5, /* slot */ 0));

  engine.insert_sell_order_PL(6, /* price */ 105, /* qty */ 100);
  Assert(engine.out().pop() == CreateRejected(/* id */ 6, /* qty */ 100));

  engine.insert_buy_order_MKT(7, /* qty */ 600);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 100, /* maker */ 7, /* taker */ 1));
  Assert(engine.out().pop() == Trade(/* price */ 101, /* qty */ 100, /* maker */ 7, /* taker */ 2));
  Assert(engine.out().pop() == Trade(/* price */ 102, /* qty */ 100, /* maker */ 7, /* taker */ 3));
  Assert(engine.out().pop() == Trade(/* price */ 103, /* qty */ 100, /* maker */ 7, /* taker */ 4));
  Assert(engine.out().pop() == Trade(/* price */ 104, /* qty */ 100, /* maker */ 7, /* taker */ 5));
  Assert(engine.out().pop() == CreateRejected(/* id */ 7, /* qty */ 100));
}

void test_update_buy_order_accepts_existing_order()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.update_buy_order(1, /* price */ 100, /* slot */ 0, /* qty */ 50);
  Assert(engine.out().pop() == UpdateAccepted(/* id */ 1));
}

void test_update_buy_order_rejects_unknown_order_id()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.update_buy_order(99, /* price */ 100, /* slot */ 0, /* qty */ 50);
  Assert(engine.out().pop() == UpdateRejected(/* id */ 99, /* qty */ 50));
}

void test_update_buy_order_rejects_wrong_price()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.update_buy_order(1, /* price */ 200, /* slot */ 0, /* qty */ 50);
  Assert(engine.out().pop() == UpdateRejected(/* id */ 1, /* qty */ 50));
}

void test_update_sell_order_accepts_existing_order()
{
  TestTradeEngine engine(100);

  engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.update_sell_order(1, /* price */ 100, /* slot */ 0, /* qty */ 50);
  Assert(engine.out().pop() == UpdateAccepted(1));
}

void test_update_sell_order_rejects_unknown_order_id()
{
  TestTradeEngine engine(100);

  engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.update_sell_order(99, /* price */ 100, /* slot */ 0, /* qty */ 50);
  Assert(engine.out().pop() == UpdateRejected(/* id */ 99, /* qty */ 50));
}

void test_update_sell_order_rejects_wrong_price()
{
  TestTradeEngine engine(100);

  engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.update_sell_order(1, /* price */ 200, /* slot */ 0, /* qty */ 50);
  Assert(engine.out().pop() == UpdateRejected(/* id */ 1, /* qty */ 50));
}

void test_cancel_buy_order_removes_resting_liquidity()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.cancel_buy_order(1, /* price */ 100, /* slot */ 0);
  Assert(engine.out().pop() == CancelAccepted(1));

  engine.insert_sell_order_MKT(2, /* qty */ 100);
  Assert(engine.out().pop() == CreateRejected(/* id */ 2, /* qty */ 100));
}

void test_cancel_buy_order_rejects_unknown_order_id()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.cancel_buy_order(99, /* price */ 100, /* slot */ 0);
  Assert(engine.out().pop() == CancelRejected(99));
}

void test_cancel_buy_order_rejects_wrong_price()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.cancel_buy_order(1, /* price */ 200, /* slot */ 0);
  Assert(engine.out().pop() == CancelRejected(1));
}

void test_cancel_sell_order_removes_resting_liquidity()
{
  TestTradeEngine engine(100);

  engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.cancel_sell_order(1, /* price */ 100, /* slot */ 0);
  Assert(engine.out().pop() == CancelAccepted(1));

  engine.insert_buy_order_MKT(2, /* qty */ 100);
  Assert(engine.out().pop() == CreateRejected(/* id */ 2, /* qty */ 100));
}

void test_cancel_sell_order_rejects_unknown_order_id()
{
  TestTradeEngine engine(100);

  engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.cancel_sell_order(99, /* price */ 100, /* slot */ 0);
  Assert(engine.out().pop() == CancelRejected(99));
}

void test_cancel_sell_order_rejects_wrong_price()
{
  TestTradeEngine engine(100);

  engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.cancel_sell_order(1, /* price */ 200, /* slot */ 0);
  Assert(engine.out().pop() == CancelRejected(1));
}

void test_trade_IOC_order_matches_and_rejects_remainder()
{  
  {    
    TestTradeEngine engine(100);

    engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));
    
    engine.insert_sell_order_IOC(2, /* price */ 100, /* qty */ 150);
    Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 100, /* maker */ 2, /* taker */ 1));
    Assert(engine.out().pop() == CreateRejected(/* id */ 2, /* qty */ 50));
  }

  {    
    TestTradeEngine engine(100);
    
    engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));
    
    engine.insert_buy_order_IOC(2, /* price */ 100, /* qty */ 150);
    Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 100, /* maker */ 2, /* taker */ 1));
    Assert(engine.out().pop() == CreateRejected(/* id */ 2, /* qty */ 50));
  }
}

void test_trade_FOK_order_does_not_match_partial()
{
  TestTradeEngine engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 1, /* slot */ 0));

  engine.insert_buy_order_PL(2, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 2, /* slot */ 1));

  engine.insert_sell_order_FOK(3, /* price */ 100, /* qty */ 300);
  Assert(engine.out().pop() == CreateRejected(/* id */ 3, /* qty */ 300));

  engine.insert_buy_order_PL(4, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(/* id */ 4, /* slot */ 2));

  engine.insert_sell_order_FOK(5, /* price */ 100, /* qty */ 300);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 100, /* maker */ 5, /* taker */ 1));
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 100, /* maker */ 5, /* taker */ 2));
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 100, /* maker */ 5, /* taker */ 4));
}

int event = 0;

void bench(uint32_t iters = 1000)
{
  event = 0;
  
  {
    TradeEngine<3, 60> engine(100);
    
    uint32_t orderId = 1;
    uint32_t minPrice = 97 + 1;
    uint32_t maxPrice = 160 - 1;
    
    for(uint32_t i = 0; i < iters; ++i) {
      for(uint32_t p = minPrice; p <= maxPrice; ++p) {
        for(uint32_t o = 0; o < 32; ++o) {
          engine.insert_buy_order_PL(orderId++, p, 100);
          engine.out().pop();
          event += 1;
        }
        
        engine.insert_sell_order_MKT(orderId++, 32 * 100);
        engine.out().pop();
        event += 1;
      }
    }
  }
}

#include "./timer.hpp"

int main()
{

#ifdef NDEBUG
  timer([]() { bench(); }, 1000).log([&](long int ns, const std::string& formatted) {
        std::cout << "Bench took " << formatted << " (" << ns << "ns) for " << event << " events" << std::endl;
      });
#else

  test_matches_across_multiple_price_levels();
  test_buy_side_flow_with_update_cancel_and_resting_sell_remainder();
  test_sell_side_resting_order_matches_incoming_buy();
  test_partial_fill_incoming_sell_larger_than_resting_buy();
  test_partial_fill_incoming_buy_larger_than_resting_sell();
  test_partial_fill_rests_remaining_sell_qty();
  test_partial_fill_rests_remaining_buy_qty();
  test_multiple_buy_orders_match_fifo_within_single_price_level();
  test_multiple_sell_orders_match_fifo_within_single_price_level();
  test_rejects_buy_order_outside_supported_price_window();
  test_sell_market_order_consumes_buy_book_and_rejects_remainder();
  test_buy_market_order_consumes_sell_book_and_rejects_remainder();
  test_update_buy_order_accepts_existing_order();
  test_update_buy_order_rejects_unknown_order_id();
  test_update_buy_order_rejects_wrong_price();
  test_update_sell_order_accepts_existing_order();
  test_update_sell_order_rejects_unknown_order_id();
  test_update_sell_order_rejects_wrong_price();
  test_cancel_buy_order_removes_resting_liquidity();
  test_cancel_buy_order_rejects_unknown_order_id();
  test_cancel_buy_order_rejects_wrong_price();
  test_cancel_sell_order_removes_resting_liquidity();
  test_cancel_sell_order_rejects_unknown_order_id();
  test_cancel_sell_order_rejects_wrong_price();
  test_trade_IOC_order_matches_and_rejects_remainder();
  test_trade_FOK_order_does_not_match_partial();

  #endif

  return 0;
}
