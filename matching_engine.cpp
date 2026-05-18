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

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.insertSellOrder_PL(2, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(100, 200, 2, 1));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.insertBuyOrder_PL(2, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(100, 200, 2, 1));
  }
}

void test_partial_fill()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 180);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.insertSellOrder_PL(2, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(100, 180, 2, 1));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.insertBuyOrder_PL(2, /* price */ 100, /* qty */ 180);
    Assert(engine.bufferOut().pop() == Trade(100, 180, 2, 1));
  }
}

void test_partial_fill_rests_remaining_qty()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.insertSellOrder_PL(2, /* price */ 100, /* qty */ 150);
    Assert(engine.bufferOut().pop() == Trade(100, 100, 2, 1));
    Assert(engine.bufferOut().pop() == CreateAccepted(2, 0));

    engine.insertBuyOrder_PL(3, /* price */ 100, /* qty */ 50);
    Assert(engine.bufferOut().pop() == Trade(100, 50, 3, 2));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.insertBuyOrder_PL(2, /* price */ 100, /* qty */ 150);
    Assert(engine.bufferOut().pop() == Trade(100, 100, 2, 1));
    Assert(engine.bufferOut().pop() == CreateAccepted(2, 0));

    engine.insertSellOrder_PL(3, /* price */ 100, /* qty */ 50);
    Assert(engine.bufferOut().pop() == Trade(100, 50, 3, 2));
  }
}

void test_single_price_level()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 180);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.insertBuyOrder_PL(2, /* price */ 100, /* qty */ 20);
    Assert(engine.bufferOut().pop() == CreateAccepted(2, 1));

    engine.insertSellOrder_PL(3, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(100, 180, 3, 1));
    Assert(engine.bufferOut().pop() == Trade(100, 20, 3, 2));
  }

  {
    MatchingEngine<3, 4> engine(100);

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
  MatchingEngine<3, 4> engine(100);

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
  MatchingEngine<3, 4> engine(100);

  engine.insertBuyOrder_PL(1, /* price */ 95, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateRejected(1, 100));

  engine.insertBuyOrder_PL(1, /* price */ 96, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

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
    MatchingEngine<3, 4> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.insertBuyOrder_PL(2, /* price */ 99, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(2, 0));

    engine.insertBuyOrder_PL(3, /* price */ 98, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(3, 0));

    engine.insertBuyOrder_PL(4, /* price */ 97, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(4, 0));

    engine.insertBuyOrder_PL(5, /* price */ 96, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(5, 0));

    engine.insertBuyOrder_PL(6, /* price */ 95, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateRejected(6, 100));

    engine.insertSellOrder_MKT(7, /* qty */ 600);
    Assert(engine.bufferOut().pop() == Trade(100, 100, 7, 1));
    Assert(engine.bufferOut().pop() == Trade(99, 100, 7, 2));
    Assert(engine.bufferOut().pop() == Trade(98, 100, 7, 3));
    Assert(engine.bufferOut().pop() == Trade(97, 100, 7, 4));
    Assert(engine.bufferOut().pop() == Trade(96, 100, 7, 5));
    Assert(engine.bufferOut().pop() == CreateRejected(7, 100));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.insertSellOrder_PL(2, /* price */ 101, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(2, 0));

    engine.insertSellOrder_PL(3, /* price */ 102, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(3, 0));

    engine.insertSellOrder_PL(4, /* price */ 103, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(4, 0));

    engine.insertSellOrder_PL(5, /* price */ 104, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(5, 0));

    engine.insertSellOrder_PL(6, /* price */ 105, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateRejected(6, 100));

    engine.insertBuyOrder_MKT(7, /* qty */ 600);
    Assert(engine.bufferOut().pop() == Trade(100, 100, 7, 1));
    Assert(engine.bufferOut().pop() == Trade(101, 100, 7, 2));
    Assert(engine.bufferOut().pop() == Trade(102, 100, 7, 3));
    Assert(engine.bufferOut().pop() == Trade(103, 100, 7, 4));
    Assert(engine.bufferOut().pop() == Trade(104, 100, 7, 5));
    Assert(engine.bufferOut().pop() == CreateRejected(7, 100));
  }
}

void test_update_buy_order()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.updateBuyOrder(1, /* slot */ 0, /* price */ 100, /* qty */ 50);
    Assert(engine.bufferOut().pop() == UpdateAccepted(1));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.updateBuyOrder(99, /* slot */ 0, /* price */ 100, /* qty */ 50);
    Assert(engine.bufferOut().pop() == UpdateRejected(99, 50));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.updateBuyOrder(1, /* slot */ 0, /* price */ 200, /* qty */ 50);
    Assert(engine.bufferOut().pop() == UpdateRejected(1, 50));
  }
}

void test_update_sell_order()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.updateSellOrder(1, /* slot */ 0, /* price */ 100, /* qty */ 50);
    Assert(engine.bufferOut().pop() == UpdateAccepted(1));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.updateSellOrder(99, /* slot */ 0, /* price */ 100, /* qty */ 50);
    Assert(engine.bufferOut().pop() == UpdateRejected(99, 50));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.updateSellOrder(1, /* slot */ 0, /* price */ 200, /* qty */ 50);
    Assert(engine.bufferOut().pop() == UpdateRejected(1, 50));
  }
}

void test_cancel_buy_order()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.cancelBuyOrder(1, /* slot */ 0, /* price */ 100);
    Assert(engine.bufferOut().pop() == CancelAccepted(1));

    engine.insertSellOrder_MKT(2, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateRejected(2, 100));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.cancelBuyOrder(99, /* slot */ 0, /* price */ 100);
    Assert(engine.bufferOut().pop() == CancelRejected(99));

    engine.cancelBuyOrder(1, /* slot */ 0, /* price */ 200);
    Assert(engine.bufferOut().pop() == CancelRejected(1));
  }
}

void test_cancel_sell_order()
{
  {
    MatchingEngine<3, 4> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.cancelSellOrder(1, /* slot */ 0, /* price */ 100);
    Assert(engine.bufferOut().pop() == CancelAccepted(1));

    engine.insertBuyOrder_MKT(2, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateRejected(2, 100));
  }

  {
    MatchingEngine<3, 4> engine(100);

    engine.insertSellOrder_PL(1, /* price */ 100, /* qty */ 100);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

    engine.cancelSellOrder(99, /* slot */ 0, /* price */ 100);
    Assert(engine.bufferOut().pop() == CancelRejected(99));

    engine.cancelSellOrder(1, /* slot */ 0, /* price */ 200);
    Assert(engine.bufferOut().pop() == CancelRejected(1));
  }
}

template<uint32_t InsideLevels, uint32_t OutsideLevels, uint32_t Orders>
void micro_bench_insert(std::size_t iters = 1000) 
{
  uint32_t events = 0;
  uint32_t orderId = 1;
  uint32_t minPrice = 100 - InsideLevels + 1;
  uint32_t maxPrice = 100 + OutsideLevels - 1;
  MatchingEngine<InsideLevels, OutsideLevels, Orders> engine(100);
  
  const auto task = [&]() {
    events = 0;
    orderId = 1;

    for(uint32_t p = minPrice; p <= maxPrice; ++p) {
      for(uint32_t o = 0; o < Orders; ++o) {
        engine.insertSellOrder_PL(orderId++, p, 100);

#ifdef HFT_DOJO_COUT
        std::cout << engine.bufferOut().pop() << std::endl;
#else
        engine.bufferOut().pop();
#endif

        events += 1;
      }
    }
  };
  
  timer([&]() { task(); }, iters).log([&](long int ns, const std::string& /* formatted */) {
    std::cout << "micro_bench_insert: " << ns << "ns / " << events << " events / " << (1.0 * ns / events) << "ns/event" << std::endl;
  });
}

template<uint32_t InsideLevels, uint32_t OutsideLevels, uint32_t Orders>
void micro_bench_trade(std::size_t iters = 1000) 
{ 
  uint32_t events = 0;
  uint32_t orderId = 1;
  uint32_t minPrice = 100 - InsideLevels + 1;
  uint32_t maxPrice = 100 + OutsideLevels - 1;
  MatchingEngine<InsideLevels, OutsideLevels, Orders> engine(100);
  
  const auto task = [&]() {
    events = 0;
    orderId = 1;

    for(uint32_t p = minPrice; p <= maxPrice; ++p) {
      for(uint32_t o = 0; o < Orders; ++o) {
        engine.insertSellOrder_PL(orderId++, p, 100);

#ifdef HFT_DOJO_COUT
        std::cout << engine.bufferOut().pop() << std::endl;
#else        
        engine.bufferOut().pop();
#endif

        events += 1;
      }
    }
    
    for(uint32_t p = minPrice; p <= maxPrice; ++p) {
      for(uint32_t o = 0; o < Orders; ++o) {
        engine.insertBuyOrder_PL(orderId++, p, 100);

#ifdef HFT_DOJO_COUT
        std::cout << engine.bufferOut().pop() << std::endl;
#else        
        engine.bufferOut().pop();
#endif

        events += 1;
      }
    }
  };
  
  timer([&]() { task(); }, iters).log([&](long int ns, const std::string& /* formatted */) {
    std::cout << "micro_bench_trade: " << ns << "ns / " << events << " events / " << (1.0 * ns / events) << "ns/event" << std::endl;
  });
}

#include "./timer.hpp"

int main()
{

#ifdef NDEBUG
  micro_bench_insert<27, 100, 32>();
  micro_bench_trade<27, 100, 32>();
#else

  micro_bench_insert<3, 4, 4>(1);
  micro_bench_trade<3, 4, 4>(2);

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
