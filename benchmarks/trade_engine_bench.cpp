/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <iostream>

#include "trade_engine.hpp"
#include "timer.hpp"

void micro_bench_insert()
{
  TradeEngine<3, 4> engine1(100);
  
  auto insert_sell_order = [&engine1]() {
    engine1.insert_sell_order_PL(1, /* price */ 100, /* qty */ 1);
    engine1.out().pop();
  };

  TradeEngine<3, 4> engine2(100);

  auto insert_buy_order = [&engine2]() {
    engine2.insert_buy_order_PL(2, /* price */ 100, /* qty */ 1);
    engine2.out().pop();
  };
  
  std::cout << "Micro benchmark - Insert sell order PL: " << Cycles<32>(insert_sell_order) << " cycles" << std::endl;
  std::cout << "Micro benchmark - Insert buy order PL: " << Cycles<32>(insert_buy_order) << " cycles" << std::endl << std::endl;
}

void micro_bench_trade()
{
  TradeEngine<3, 4> engine1(100);

  auto trade_sell_order = [&engine1]() {
    engine1.insert_sell_order_PL(1, /* price */ 100, /* qty */ 1);
    engine1.out().pop();
    engine1.insert_buy_order_PL(2, /* price */ 100, /* qty */ 1);
    engine1.out().pop();
  };

  TradeEngine<3, 4> engine2(100);

  auto trade_buy_order = [&engine2]() {
    engine2.insert_sell_order_PL(1, /* price */ 100, /* qty */ 1);
    engine2.out().pop();
    engine2.insert_buy_order_PL(2, /* price */ 100, /* qty */ 1);
    engine2.out().pop();
  };

  std::cout << "Micro benchmark - Trade sell order PL: " << Cycles<32>(trade_sell_order) << " cycles" << std::endl;
  std::cout << "Micro benchmark - Trade buy order PL: " << Cycles<32>(trade_buy_order) << " cycles" << std::endl << std::endl;
}

int main()
{
  micro_bench_insert();
  micro_bench_trade();

  return 0;
}

