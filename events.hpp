#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstdint>
#include <ostream>

enum EventType : uint32_t
{
  ETrade = 1,

  ECreateAccepted,
  ECreateRejected,
  
  EUpdateAccepted,
  EUpdateRejected,

  ECancelAccepted,
  ECancelRejected,

  EOrderExpired,
};

struct Event
{
  uint32_t m1{0};
  uint32_t m2{0};
  uint32_t m3{0};
  uint32_t m4{0};
};

bool operator==(const Event& lhs, const Event& rhs)
{
  return lhs.m1 == rhs.m1 && lhs.m2 == rhs.m2 && lhs.m3 == rhs.m3 && lhs.m4 == rhs.m4;
}

Event Trade(uint32_t price, uint32_t qty, uint32_t maker, uint32_t taker)
{
  return {.m1 = price, .m2 = qty, .m3 = maker, .m4 = taker};
}

Event CreateAccepted(uint32_t id, uint32_t slot)
{
  return {.m1 = EventType::ECreateAccepted, .m2 = id, .m3 = slot};
}

Event CreateRejected(uint32_t id, uint32_t size)
{
  return {.m1 = EventType::ECreateRejected, .m2 = id, .m3 = size};
}

Event CancelAccepted(uint32_t id)
{
  return {.m1 = EventType::ECancelAccepted, .m2 = id};
}

Event CancelRejected(uint32_t id)
{
  return {.m1 = EventType::ECancelRejected, .m2 = id};
}

Event UpdateAccepted(uint32_t id)
{
  return {.m1 = EventType::EUpdateAccepted, .m2 = id};
}

Event UpdateRejected(uint32_t id, uint32_t size)
{
  return {.m1 = EventType::EUpdateRejected, .m2 = id, .m3 = size};
}

Event OrderExpired(uint32_t id)
{
  return {.m1 = EventType::EOrderExpired, .m2 = id};
}

std::ostream& operator<<(std::ostream& os, const Event& event)
{
  if (event.m4 != 0) {
    return os << "Trade: price=" << event.m1 << ", qty=" << event.m2 << ", maker=" << event.m3 << ", taker=" << event.m4;
  }
  
  if (event.m1 == EventType::ECreateAccepted) {
    return os << "CreateAccepted: id=" << event.m2 << ", slot=" << event.m3;
  }
  
  if (event.m1 == EventType::ECreateRejected) {
    return os << "CreateRejected: id=" << event.m2 << ", size=" << event.m3;
  }
  
  if (event.m1 == EventType::EUpdateAccepted) {
    return os << "UpdateAccepted: id=" << event.m2;
  }
  
  if (event.m1 == EventType::EUpdateRejected) {
    return os << "UpdateRejected: id=" << event.m2 << ", size=" << event.m3;
  }
  
  if (event.m1 == EventType::ECancelAccepted) {
    return os << "CancelAccepted: id=" << event.m2;
  }
  
  if (event.m1 == EventType::ECancelRejected) {
    return os << "CancelRejected: id=" << event.m2;
  }
  
  if (event.m1 == EventType::EOrderExpired) {
    return os << "OrderExpired: id=" << event.m2;
  }

  return os << "UnknownEvent: m1=" << event.m1 << ", m2=" << event.m2 << ", m3=" << event.m3 << ", m4=" << event.m4;
}
