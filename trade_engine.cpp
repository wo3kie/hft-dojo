/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "assert.hpp"
#include "trade_engine.hpp"

int main() {
  QueueOut out;
  TradeEngine engine(out);
  
  { // insert/update
    engine.insert_order<Buy>(1, 10, 120);
    Assert(engine.out().pop() == CreateAccepted(1, 0));
    
    engine.update_order<Buy>(1, 10, 0, 100);
    Assert(engine.out().pop() == UpdateAccepted(1));
  }

  { // insert/cancel
    engine.insert_order<Buy>(2, 10, 100);
    Assert(engine.out().pop() == CreateAccepted(2, 1));

    engine.cancel_order<Buy>(2, 10, 1);
    Assert(engine.out().pop() == CancelAccepted(2));

    engine.cancel_order<Buy>(2, 10, 1);
    Assert(engine.out().pop() == CancelRejected(2));
  }

  { // insert too high price
    engine.insert_order<Buy>(3, engine.max_price() + 1, 100);
    Assert(engine.out().pop() == CreateRejected(3, 100));
  }

  { // insert too low price
    engine.insert_order<Buy>(3, engine.min_price() - 1, 100);
    Assert(engine.out().pop() == CreateRejected(3, 100));
  }

  { // insert many orders at the same price
    engine.insert_order<Buy>(3, 9, 200);
    Assert(engine.out().pop() == CreateAccepted(3, 0));

    engine.insert_order<Buy>(4, 9, 300);
    Assert(engine.out().pop() == CreateAccepted(4, 1));

    engine.insert_order<Buy>(5, 9, 400);
    Assert(engine.out().pop() == CreateAccepted(5, 2));
  }

  { // trade one price
    engine.insert_order<Sell>(6, 10, 50);
    Assert(engine.out().pop() == Trade(10, 50, 6, 1));
  }

  { // trade many prices
    engine.insert_order<Sell>(7, 9, 300);
    Assert(engine.out().pop() == Trade(10, 50, 7, 1));
    Assert(engine.out().pop() == Trade(9, 200, 7, 3));
    Assert(engine.out().pop() == Trade(9, 50, 7, 4));
  }

  { // trade market sell order
    engine.insert_order<Sell>(8, 1000);
    Assert(engine.out().pop() == Trade(9, 250, 8, 4));
    Assert(engine.out().pop() == Trade(9, 400, 8, 5));
    Assert(engine.out().pop() == CreateRejected(8, 350));
  }

  { // insert too many orders
    for (int32_t i = 0; i < engine.order_per_level(); i++) {
      engine.insert_order<Buy>(100 + i, 8, 10);
      Assert(engine.out().pop() == CreateAccepted(100 + i, i));
    }

    engine.insert_order<Buy>(200, 8, 10);
    Assert(engine.out().pop() == CreateRejected(200, 10));
  }

  { // clear level
    engine.insert_order<Sell>(300, 10 * engine.order_per_level() + 1);

    for (int32_t i = 0; i < engine.order_per_level(); i++) {
      Assert(engine.out().pop() == Trade(8, 10, 300, 100 + i));
    }

    Assert(engine.out().pop() == CreateRejected(300, 1));
  }

  { // trade all prices
    for(int32_t price = engine.min_price(); price <= engine.max_price(); price++) {
      for(int32_t i = 0; i < 4; i++) {
        engine.insert_order<Buy>(1000 + price * 100 + i, price, 10);
        engine.out().pop();
      }
    }

    engine.insert_order<Sell>(2000, 10 * (engine.max_price() - engine.min_price() + 1) * 4 + 1);

    for(int32_t price = engine.max_price(); price >= engine.min_price(); price--) {
      for(int32_t i = 0; i < 4; i++) {
        Assert(engine.out().pop() == Trade(price, 10, 2000, 1000 + price * 100 + i));
      }
    }

    Assert(engine.out().pop() == CreateRejected(2000, 1));
  }

  return 0;
}