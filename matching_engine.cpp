/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "./matching_engine.hpp"
#include "./assert.hpp"

void test_simple_transaction()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.insert_sell_order_PL(2, /* price */ 100, /* qty */ 200);
    Assert(engine.out().pop() == Trade(100, 200, 2, 1));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.insert_buy_order_PL(2, /* price */ 100, /* qty */ 200);
    Assert(engine.out().pop() == Trade(100, 200, 2, 1));
  }
}

void test_partial_fill()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 180);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.insert_sell_order_PL(2, /* price */ 100, /* qty */ 200);
    Assert(engine.out().pop() == Trade(100, 180, 2, 1));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.insert_buy_order_PL(2, /* price */ 100, /* qty */ 180);
    Assert(engine.out().pop() == Trade(100, 180, 2, 1));
  }
}

void test_partial_fill_rests_remaining_qty()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.insert_sell_order_PL(2, /* price */ 100, /* qty */ 150);
    Assert(engine.out().pop() == Trade(100, 100, 2, 1));
    Assert(engine.out().pop() == CreateAccepted(2, 0));

    engine.insert_buy_order_PL(3, /* price */ 100, /* qty */ 50);
    Assert(engine.out().pop() == Trade(100, 50, 3, 2));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.insert_buy_order_PL(2, /* price */ 100, /* qty */ 150);
    Assert(engine.out().pop() == Trade(100, 100, 2, 1));
    Assert(engine.out().pop() == CreateAccepted(2, 0));

    engine.insert_sell_order_PL(3, /* price */ 100, /* qty */ 50);
    Assert(engine.out().pop() == Trade(100, 50, 3, 2));
  }
}

void test_single_price_level()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 180);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.insert_buy_order_PL(2, /* price */ 100, /* qty */ 20);
    Assert(engine.out().pop() == CreateAccepted(2, 1));

    engine.insert_sell_order_PL(3, /* price */ 100, /* qty */ 200);
    Assert(engine.out().pop() == Trade(100, 180, 3, 1));
    Assert(engine.out().pop() == Trade(100, 20, 3, 2));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.insert_sell_order_PL(2, /* price */ 100, /* qty */ 20);
    Assert(engine.out().pop() == CreateAccepted(2, 1));

    engine.insert_buy_order_PL(3, /* price */ 100, /* qty */ 220);
    Assert(engine.out().pop() == Trade(100, 200, 3, 1));
    Assert(engine.out().pop() == Trade(100, 20, 3, 2));
  }
}

void test_insert_level()
{
  MatchingEngine<3, 4> engine(100);

  engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.insert_buy_order_PL(2, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(2, 1));

  engine.insert_buy_order_PL(3, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(3, 2));

  engine.insert_buy_order_PL(4, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(4, 3));

  engine.insert_buy_order_PL(5, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(5, 4));

  engine.insert_buy_order_PL(6, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(6, 5));

  engine.insert_buy_order_PL(7, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(7, 6));

  engine.insert_buy_order_PL(8, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(8, 7));

  engine.insert_buy_order_PL(9, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateRejected(9, 100));
}

void test_insert_levels()
{
  MatchingEngine<3, 4> engine(100);

  engine.insert_buy_order_PL(1, /* price */ 95, /* qty */ 100);
  Assert(engine.out().pop() == CreateRejected(1, 100));

  engine.insert_buy_order_PL(1, /* price */ 96, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(1, 0));

  engine.insert_buy_order_PL(2, /* price */ 97, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(2, 0));

  engine.insert_buy_order_PL(3, /* price */ 98, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(3, 0));

  engine.insert_buy_order_PL(4, /* price */ 99, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(4, 0));

  engine.insert_buy_order_PL(5, /* price */ 100, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(5, 0));

  engine.insert_buy_order_PL(6, /* price */ 101, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(6, 0));

  engine.insert_buy_order_PL(7, /* price */ 102, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(7, 0));

  engine.insert_buy_order_PL(8, /* price */ 103, /* qty */ 100);
  Assert(engine.out().pop() == CreateAccepted(8, 0));

  engine.insert_buy_order_PL(9, /* price */ 104, /* qty */ 100);
  Assert(engine.out().pop() == CreateRejected(9, 100));
}

void test_insert_market_order()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.insert_buy_order_PL(2, /* price */ 99, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(2, 0));

    engine.insert_buy_order_PL(3, /* price */ 98, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(3, 0));

    engine.insert_buy_order_PL(4, /* price */ 97, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(4, 0));

    engine.insert_buy_order_PL(5, /* price */ 96, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(5, 0));

    engine.insert_buy_order_PL(6, /* price */ 95, /* qty */ 100);
    Assert(engine.out().pop() == CreateRejected(6, 100));

    engine.insert_sell_order_MKT(7, /* qty */ 600);
    Assert(engine.out().pop() == Trade(100, 100, 7, 1));
    Assert(engine.out().pop() == Trade(99, 100, 7, 2));
    Assert(engine.out().pop() == Trade(98, 100, 7, 3));
    Assert(engine.out().pop() == Trade(97, 100, 7, 4));
    Assert(engine.out().pop() == Trade(96, 100, 7, 5));
    Assert(engine.out().pop() == CreateRejected(7, 100));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.insert_sell_order_PL(2, /* price */ 101, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(2, 0));

    engine.insert_sell_order_PL(3, /* price */ 102, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(3, 0));

    engine.insert_sell_order_PL(4, /* price */ 103, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(4, 0));

    engine.insert_sell_order_PL(5, /* price */ 104, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(5, 0));

    engine.insert_sell_order_PL(6, /* price */ 105, /* qty */ 100);
    Assert(engine.out().pop() == CreateRejected(6, 100));

    engine.insert_buy_order_MKT(7, /* qty */ 600);
    Assert(engine.out().pop() == Trade(100, 100, 7, 1));
    Assert(engine.out().pop() == Trade(101, 100, 7, 2));
    Assert(engine.out().pop() == Trade(102, 100, 7, 3));
    Assert(engine.out().pop() == Trade(103, 100, 7, 4));
    Assert(engine.out().pop() == Trade(104, 100, 7, 5));
    Assert(engine.out().pop() == CreateRejected(7, 100));
  }
}

void test_update_buy_order()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.update_buy_order(1, /* slot */ 0, /* price */ 100, /* qty */ 50);
    Assert(engine.out().pop() == UpdateAccepted(1));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.update_buy_order(99, /* slot */ 0, /* price */ 100, /* qty */ 50);
    Assert(engine.out().pop() == UpdateRejected(99, 50));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.update_buy_order(1, /* slot */ 0, /* price */ 200, /* qty */ 50);
    Assert(engine.out().pop() == UpdateRejected(1, 50));
  }
}

void test_update_sell_order()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.update_sell_order(1, /* slot */ 0, /* price */ 100, /* qty */ 50);
    Assert(engine.out().pop() == UpdateAccepted(1));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.update_sell_order(99, /* slot */ 0, /* price */ 100, /* qty */ 50);
    Assert(engine.out().pop() == UpdateRejected(99, 50));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.update_sell_order(1, /* slot */ 0, /* price */ 200, /* qty */ 50);
    Assert(engine.out().pop() == UpdateRejected(1, 50));
  }
}

void test_cancel_buy_order()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.cancel_buy_order(1, /* slot */ 0, /* price */ 100);
    Assert(engine.out().pop() == CancelAccepted(1));

    engine.insert_sell_order_MKT(2, /* qty */ 100);
    Assert(engine.out().pop() == CreateRejected(2, 100));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_buy_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.cancel_buy_order(99, /* slot */ 0, /* price */ 100);
    Assert(engine.out().pop() == CancelRejected(99));

    engine.cancel_buy_order(1, /* slot */ 0, /* price */ 200);
    Assert(engine.out().pop() == CancelRejected(1));
  }
}

void test_cancel_sell_order()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.cancel_sell_order(1, /* slot */ 0, /* price */ 100);
    Assert(engine.out().pop() == CancelAccepted(1));

    engine.insert_buy_order_MKT(2, /* qty */ 100);
    Assert(engine.out().pop() == CreateRejected(2, 100));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.out().pop() == CreateAccepted(1, 0));

    engine.cancel_sell_order(99, /* slot */ 0, /* price */ 100);
    Assert(engine.out().pop() == CancelRejected(99));

    engine.cancel_sell_order(1, /* slot */ 0, /* price */ 200);
    Assert(engine.out().pop() == CancelRejected(1));
  }
}

int event = 0;

void bench(uint32_t iters = 1000)
{
  event = 0;
  
  {
    MatchingEngine<3, 60> engine(100);
    
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

  test_simple_transaction();
  test_partial_fill();
  test_partial_fill_rests_remaining_qty();
  test_single_price_level();
  test_insert_levels();
  test_insert_market_order();
  test_update_buy_order();
  test_update_sell_order();
  test_cancel_buy_order();
  test_cancel_sell_order();

  #endif

  return 0;
}
