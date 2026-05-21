/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <iostream>

#include "task_worker.hpp"
#include "trade_engine.hpp"
#include "timer.hpp"

void micro_bench_insert()
{
  TradeEngine<3, 4> engine(100);
  
  struct _Insert {
    TradeEngine<3, 4>& engine;
    
    void setup() {
    }
    
    void run() {
      engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 1);
    }
    
    void teardown() noexcept {
      engine.cancel_sell_order(1, /* price */ 100, /* slot */ 0);
      engine.out().pop();
      engine.out().pop();
    }
  } insert{engine};
   
  std::cout << "Micro benchmark - Insert order PL: " << Cycles<32>(insert) << " cycles" << std::endl;
}

void micro_bench_update()
{
  TradeEngine<3, 4> engine(100);
  
  struct _Update {
    TradeEngine<3, 4>& engine;
    
    void setup() {
      engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 1);
    }
    
    void run() {
      engine.update_sell_order(1, /* price */ 100, /* slot */ 0, /* qty */ 2);
    }
    
    void teardown() noexcept {
      engine.cancel_sell_order(1, /* price */ 100, /* slot */ 0);
      engine.out().pop();
      engine.out().pop();
      engine.out().pop();
    }
  } update{engine};
   
  std::cout << "Micro benchmark - Update order PL: " << Cycles<32>(update) << " cycles" << std::endl;
}

void micro_bench_cancel()
{
  TradeEngine<3, 4> engine(100);
  
  struct _Cancel {
    TradeEngine<3, 4>& engine;
    
    void setup() {
      engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 1);
    }
    
    void run() {
      engine.cancel_sell_order(1, /* price */ 100, /* slot */ 0);
    }
    
    void teardown() noexcept {
      engine.out().pop();
      engine.out().pop();
    }
  } cancel{engine};
   
  std::cout << "Micro benchmark - Cancel order PL: " << Cycles<32>(cancel) << " cycles" << std::endl;
}

void micro_bench_trade_PL()
{
  TradeEngine<3, 4> engine(100);
  
  struct _Trade {
    TradeEngine<3, 4>& engine;
    
    void setup() {
      engine.insert_sell_order_PL(1, /* price */ 100, /* qty */ 1);
    }
    
    void run() {
      engine.insert_buy_order_PL(2, /* price */ 100, /* qty */ 1);
    }
    
    void teardown() noexcept {
      engine.out().pop();
      engine.out().pop();
    }
  } trade{engine};
   
  std::cout << "Micro benchmark - Trade order PL: " << Cycles<32>(trade) << " cycles" << std::endl;
}

template<uint32_t Levels, uint32_t Orders>
void micro_bench_trade_MKT()
{
  TradeEngine<127, 128> engine(1000);
  
  struct _Trade {
    TradeEngine<127, 128>& engine;
    
    void setup() {
      engine.reset(1000);

      for (Price p = 1; p <= Levels; ++p) {
        for(OrderId o = 1; o <= Orders; ++o) {
          engine.insert_sell_order_PL(1000 * p + o, /* price */ 1000 + p, /* qty */ 1);
          engine.out().pop();
        }
      }
    }
    
    void run() {
      engine.insert_buy_order_MKT(999999999, /* qty */ Levels * Orders);
    }
    
    void teardown() noexcept {
      for (Price p = 1; p <= Levels; ++p) {
        for(OrderId o = 1; o <= Orders; ++o) {
          engine.out().pop();
        }
      }
    }
  } trade{engine};
   
  std::cout << "Micro benchmark - Trade order MKT<" << Levels << ", " << Orders << ">: " << Cycles<32>(trade) << " cycles" << std::endl;
}

int main()
{
  micro_bench_insert();
  micro_bench_update();
  micro_bench_cancel();
  micro_bench_trade_PL();
  micro_bench_trade_MKT<1, 32>();
  micro_bench_trade_MKT<16, 32>();
  micro_bench_trade_MKT<24, 32>();

  return 0;
}

