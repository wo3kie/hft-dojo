#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <iostream>

#include "./common.hpp"

struct Order
{
  static constexpr OrderId InvalidId = 0;
  static constexpr Qty MinQty = 1;
  static constexpr Qty MaxQty = 128 * 1024 * 1024;

  Order()
  {
    clear();
  }

  Order(OrderId id, Qty qty)
    : _id(id)
    , _qty(qty)
  {
    Assert(id != InvalidId);
    Assert(qty >= MinQty);
    Assert(qty <= MaxQty);
  }

  OrderId _id;
  Qty _qty;

  OrderId id() const
  {
    return _id;
  }

  Qty qty() const
  {
    return _qty;
  }

  void update(Qty newQty)
  {
    Assert(newQty >= MinQty);
    Assert(newQty <= MaxQty);

    _qty = newQty;
  }

  void trade(Qty qty)
  {
    Assert(qty >= MinQty);
    Assert(qty <= MaxQty);
    Assert(qty <= _qty);

    _qty -= qty;
  }

  void clear()
  {
    _id = InvalidId;
    _qty = 0;
  }
};

inline bool operator==(const Order& lhs, const Order& rhs)
{
  return lhs.id() == rhs.id() && lhs.qty() == rhs.qty();
}

inline bool operator!=(const Order& lhs, const Order& rhs)
{
  return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& os, const Order& order)
{
  return os << "Order{id=" << order.id() << ", qty=" << order.qty() << "}";
}
