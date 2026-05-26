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

#include "./ring_buffer_spsc.hpp"

enum EventType : int32_t {
  ETrade = -1,

  ECreateAccepted = -2,
  ECreateRejected = -3,

  EUpdateAccepted = -4,
  EUpdateRejected = -5,

  ECancelAccepted = -6,
  ECancelRejected = -7,

  EOrderExpired = -8,
};

struct Event {
  int32_t m1{0};
  int32_t m2{0};
  int32_t m3{0};
  int32_t m4{0};
};

bool operator==(const Event& lhs, const Event& rhs) {
  bool result = true;

  result = result && (lhs.m1 == 0 || rhs.m1 == 0 || lhs.m1 == rhs.m1);
  result = result && (lhs.m2 == 0 || rhs.m2 == 0 || lhs.m2 == rhs.m2);
  result = result && (lhs.m3 == 0 || rhs.m3 == 0 || lhs.m3 == rhs.m3);
  result = result && (lhs.m4 == 0 || rhs.m4 == 0 || lhs.m4 == rhs.m4);
  return result;
}

Event Trade(int32_t price, int32_t qty, int32_t maker, int32_t taker) {
  return {.m1 = price, .m2 = qty, .m3 = maker, .m4 = taker};
}

Event CreateAccepted(int32_t id, int32_t slot, int32_t qty) {
  return {.m1 = EventType::ECreateAccepted, .m2 = id, .m3 = slot, .m4 = qty};
}

Event CreateRejected(int32_t id, int32_t size) {
  return {.m1 = EventType::ECreateRejected, .m2 = id, .m3 = size};
}

Event CancelAccepted(int32_t id) {
  return {.m1 = EventType::ECancelAccepted, .m2 = id};
}

Event CancelRejected(int32_t id) {
  return {.m1 = EventType::ECancelRejected, .m2 = id};
}

Event UpdateAccepted(int32_t id) {
  return {.m1 = EventType::EUpdateAccepted, .m2 = id};
}

Event UpdateRejected(int32_t id, int32_t size) {
  return {.m1 = EventType::EUpdateRejected, .m2 = id, .m3 = size};
}

Event OrderExpired(int32_t id) {
  return {.m1 = EventType::EOrderExpired, .m2 = id};
}

std::ostream& operator<<(std::ostream& os, const Event& event) {
  if(event.m1 > 0) {
    return os << "Trade: price=" << event.m1 << ", qty=" << event.m2 << ", maker=" << event.m3 << ", taker=" << event.m4;
  }

  if(event.m1 == EventType::ECreateAccepted) {
    return os << "CreateAccepted: id=" << event.m2 << ", slot=" << event.m3 << ", qty=" << event.m4;
  }

  if(event.m1 == EventType::ECreateRejected) {
    return os << "CreateRejected: id=" << event.m2 << ", size=" << event.m3;
  }

  if(event.m1 == EventType::EUpdateAccepted) {
    return os << "UpdateAccepted: id=" << event.m2;
  }

  if(event.m1 == EventType::EUpdateRejected) {
    return os << "UpdateRejected: id=" << event.m2 << ", size=" << event.m3;
  }

  if(event.m1 == EventType::ECancelAccepted) {
    return os << "CancelAccepted: id=" << event.m2;
  }

  if(event.m1 == EventType::ECancelRejected) {
    return os << "CancelRejected: id=" << event.m2;
  }

  if(event.m1 == EventType::EOrderExpired) {
    return os << "OrderExpired: id=" << event.m2;
  }

  return os << "UnknownEvent: m1=" << event.m1 << ", m2=" << event.m2 << ", m3=" << event.m3 << ", m4=" << event.m4;
}

/*
 * QueueOut
 */

struct QueueOut {
  void push(const Event& event) noexcept {
    for(int i = 0; i < 8; i++) {
      if(_queue.push(event)) {
        return;
      } else {
        _mm_pause();
      }
    }

    for(;;) {
      if(_queue.push(event)) {
        return;
      } else {
        std::this_thread::yield();
      }
    }
  }

  Event pop() noexcept {
    return _queue.pop();
  }

  bool empty() const noexcept {
    return _queue.empty_approx();
  }

  void clear() noexcept {
    while(_queue.empty_approx() == false) {
      _queue.pop();
    }
  }

  void log(const std::string& prefix = "") {
    while(_queue.empty_approx() == false) {
      const Event event = _queue.pop();
      std::cout << prefix << event << std::endl;
    }
  }

  RingBufferSPSC<Event, 1024> _queue;
};