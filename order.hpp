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
  OrderId id;
  Qty qty;
};

inline bool operator==(const Order& lhs, const Order& rhs)
{
  return lhs.id == rhs.id && lhs.qty == rhs.qty;
}

inline bool operator!=(const Order& lhs, const Order& rhs)
{
  return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& os, const Order& order)
{
  return os << "Order{id=" << order.id << ", qty=" << order.qty << "}";
}
