/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "./assert.hpp"
#include "./matching_engine.hpp"

void test_simple_transaction()
{
  {
    MatchingEngine<3> engine(100);
    
    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));
    
    engine.insertSellOrder_PL(2, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(100, 200, 2, 1));
  }

  {
    MatchingEngine<3> engine(100);
    
    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));
    
    engine.insertBuyOrder_PL(2, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(100, 200, 2, 1));
  }
}

void test_partial_fill()
{
  {
    MatchingEngine<3> engine(100);
    
    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 180);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));
    
    engine.insertSellOrder_PL(2, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(100, 180, 2, 1));
  }

  {
    MatchingEngine<3> engine(100);
    
    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));
    
    engine.insertBuyOrder_PL(2, /* price */ 100, /* qty */ 180);
    Assert(engine.bufferOut().pop() == Trade(100, 180, 2, 1));
  }
}

void test_single_price_level()
{
  {
    MatchingEngine<3> engine(100);
    
    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 180);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));
    
    engine.insertBuyOrder_PL(2, /* price */ 100, /* qty */ 20);
    Assert(engine.bufferOut().pop() == CreateAccepted(2, 1));
    
    engine.insertSellOrder_PL(3, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(100, 180, 3, 1));
    Assert(engine.bufferOut().pop() == Trade(100, 20, 3, 2));
  }

  {
    MatchingEngine<3> engine(100);
    
    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));
    
    engine.insertSellOrder_PL(2, /* price */ 100, /* qty */ 20);
    Assert(engine.bufferOut().pop() == CreateAccepted(2, 1));
    
    engine.insertBuyOrder_PL(3, /* price */ 100, /* qty */ 220);
    Assert(engine.bufferOut().pop() == Trade(100, 200, 3, 1));
    Assert(engine.bufferOut().pop() == Trade(100, 20, 3, 2));
  }
}

void test_insert_level()
{
  MatchingEngine<3> engine(100);
  
  engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

  engine.insertBuyOrder_PL(2, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(2, 1));

  engine.insertBuyOrder_PL(3, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(3, 2));

  engine.insertBuyOrder_PL(4, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(4, 3));

  engine.insertBuyOrder_PL(5, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(5, 4));

  engine.insertBuyOrder_PL(6, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(6, 5));

  engine.insertBuyOrder_PL(7, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(7, 6));

  engine.insertBuyOrder_PL(8, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(8, 7));

  engine.insertBuyOrder_PL(9, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateRejected(9, 100));
}

void test_insert_levels()
{
  MatchingEngine<3> engine(100);
  
  engine.insertBuyOrder_PL(1, /* price */ 96, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateRejected(1, 100));

  engine.insertBuyOrder_PL(2, /* price */ 97, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(2, 0));

  engine.insertBuyOrder_PL(3, /* price */ 98, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(3, 0));

  engine.insertBuyOrder_PL(4, /* price */ 99, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(4, 0));

  engine.insertBuyOrder_PL(5, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(5, 0));

  engine.insertBuyOrder_PL(6, /* price */ 101, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(6, 0));

  engine.insertBuyOrder_PL(7, /* price */ 102, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(7, 0));

  engine.insertBuyOrder_PL(8, /* price */ 103, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(8, 0));

  engine.insertBuyOrder_PL(9, /* price */ 104, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateRejected(9, 100));
}

void test_insert_market_order()
{
  {
    MatchingEngine<3> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.insertBuyOrder_PL(2, /* price */ 99, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(2, 0));

    engine.insertBuyOrder_PL(3, /* price */ 98, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(3, 0));

    engine.insertSellOrder_MKT(4, /* qty */ 300);
    Assert(engine.bufferOut().pop() == Trade(100, 100, 4, 1));
    Assert(engine.bufferOut().pop() == Trade(99,  100, 4, 2));
    Assert(engine.bufferOut().pop() == Trade(98,  100, 4, 3));
  }

  {
    MatchingEngine<3> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.insertSellOrder_PL(2, /* price */ 101, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(2, 0));

    engine.insertSellOrder_PL(3, /* price */ 102, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(3, 0));

    engine.insertBuyOrder_MKT(4, /* qty */ 300);
    Assert(engine.bufferOut().pop() == Trade(100, 100, 4, 1));
    Assert(engine.bufferOut().pop() == Trade(101, 100, 4, 2));
    Assert(engine.bufferOut().pop() == Trade(102, 100, 4, 3));
  }
}

void test_update_buy_order()
{
  {
    MatchingEngine<3> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.updateBuy(1, /* slot */ 0, /* price */ 100, /* qty */ 50);
    Assert(engine.bufferOut().pop() == UpdateAccepted(1));
  }

  {
    MatchingEngine<3> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.updateBuy(99, /* slot */ 0, /* price */ 100, /* qty */ 50);
    Assert(engine.bufferOut().pop() == UpdateRejected(99, 50));
  }

  {
    MatchingEngine<3> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.updateBuy(1, /* slot */ 0, /* price */ 200, /* qty */ 50);
    Assert(engine.bufferOut().pop() == UpdateRejected(1, 50));
  }
}

void test_update_sell_order()
{
  {
    MatchingEngine<3> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.updateSell(1, /* slot */ 0, /* price */ 100, /* qty */ 50);
    Assert(engine.bufferOut().pop() == UpdateAccepted(1));
  }

  {
    MatchingEngine<3> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.updateSell(99, /* slot */ 0, /* price */ 100, /* qty */ 50);
    Assert(engine.bufferOut().pop() == UpdateRejected(99, 50));
  }

  {
    MatchingEngine<3> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.updateSell(1, /* slot */ 0, /* price */ 200, /* qty */ 50);
    Assert(engine.bufferOut().pop() == UpdateRejected(1, 50));
  }
}

int I = 0;

void bench() 
{
  MatchingEngine<3> engine(100);

  I = 0;

  int32_t orderId = 0;
  int32_t minPrice = 96;
  int32_t maxPrice = 104;

  for (int32_t i = 0; i < 1000; ++i) {
    for (int32_t p = minPrice; p <= maxPrice; ++p) {
      for(int32_t o = 0; o < 32 ; ++o) {
        engine.insertBuyOrder_PL(orderId++, p, 100);
      }

      engine.insertSellOrder_MKT(orderId++, 32 * 100);
    }
  }
}

#include "./timer.hpp"

int main()
{

#ifdef NDEBUG
  timer( [](){ bench(); }, 1000 ).log([&](long int ns, const std::string& formatted) {
    std::cout << "Bench took " << formatted << " (" << ns << "ns) for " << I << " events" << std::endl;
  });
#else
  test_simple_transaction();
  test_partial_fill();
  test_single_price_level();
  test_insert_levels();
  test_insert_market_order();
  test_update_buy_order();
  test_update_sell_order();
#endif

  return 0;
}
