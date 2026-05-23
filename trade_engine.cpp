#include "trade_engine.hpp"


/*
 * main
 */

int main() {
  std::cout << sizeof(OrderBook) << std::endl;   // 25616 bytes
  std::cout << sizeof(TradeEngine) << std::endl; // 26880 bytes

  std::cout << "-----------------" << std::endl;

  QueueOut out;
  TradeEngine engine(out);
  
  {
    engine.insert_sell_order(1, 10, 100);
    engine.insert_buy_order(2, 10, 50);
    engine.insert_buy_order(3, 10, 75);
    engine.insert_sell_order(3, 10, 25);
    engine.out().log();
  }

  std::cout << "-----------------" << std::endl;

  {
    engine.insert_sell_order(1, 10, 100);
    engine.insert_sell_order(2, 11, 100);
    engine.insert_sell_order(3, 12, 100);
    engine.insert_sell_order(4, 13, 100);
    engine.insert_buy_order(5, 12, 500);
    engine.insert_buy_order(6, 13, 100);
    engine.insert_sell_order(7, 12, 200);
    engine.out().log();
  }

  std::cout << "-----------------" << std::endl;

  {
    engine.insert_sell_order(1, 10, 100);
    engine.insert_sell_order(2, 11, 100);
    engine.insert_sell_order(3, 12, 100);
    engine.insert_sell_order(4, 13, 100);
    engine.insert_buy_order(5, 500);
    engine.out().log();
  }

  std::cout << "-----------------" << std::endl;

  {
    engine.insert_buy_order(1, 10, 100);
    engine.insert_buy_order(2, 11, 100);
    engine.insert_buy_order(3, 12, 100);
    engine.insert_buy_order(4, 13, 100);
    engine.insert_sell_order(5, 500);
    engine.out().log();
  }

  return 0;
}
