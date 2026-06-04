#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstdint>
#include <immintrin.h>
#include <iostream>
#include <sstream>
#include <thread>

#include "./ring_buffer_spsc.hpp"

struct Reason {
  static constexpr int32_t Request = 0;
  static constexpr int32_t Price = 1;
  static constexpr int32_t Buffer = 2;
  static constexpr int32_t Level = 3;
  static constexpr int32_t Slot = 4;
  static constexpr int32_t IOC = 5;
};

std::string to_string(int32_t reason) {
  switch(reason) {
    case Reason::Request: return "Request";
    case Reason::Price: return "Price";
    case Reason::Buffer: return "Buffer";
    case Reason::Level: return "Level";
    case Reason::Slot: return "Slot";
    case Reason::IOC: return "IOC";
    default: return "UnknownReason(" + std::to_string(reason) + ")";
  }
}

enum EventType : int32_t {
  ETrade = -1,

  ECreateAccepted = -2,
  ECreateRejected = -3,

  EUpdateAccepted = -4,
  EUpdateRejected = -5,

  ECancelAccepted = -6,
  ECancelRejected = -7,

  ELevelExpired = -8,
  ELevelCreated = -9,
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

Event CreateRejected(int32_t id, int32_t size, int32_t reason = Reason::Request) {
  return {.m1 = EventType::ECreateRejected, .m2 = id, .m3 = size, .m4 = reason};
}

Event CancelAccepted(int32_t id) {
  return {.m1 = EventType::ECancelAccepted, .m2 = id};
}

Event CancelRejected(int32_t id, int32_t reason = Reason::Request) {
  return {.m1 = EventType::ECancelRejected, .m2 = id, .m3 = reason};
}

Event UpdateAccepted(int32_t id) {
  return {.m1 = EventType::EUpdateAccepted, .m2 = id};
}

Event UpdateRejected(int32_t id, int32_t size, int32_t reason = Reason::Request) {
  return {.m1 = EventType::EUpdateRejected, .m2 = id, .m3 = size, .m4 = reason};
}

Event LevelExpired(int32_t price, int32_t id) {
  return {.m1 = EventType::ELevelExpired, .m2 = price, .m3 = id};
}

Event LevelsCreated(int32_t fromPrice, int32_t toPrice) {
  return {.m1 = EventType::ELevelCreated, .m2 = fromPrice, .m3 = toPrice};
}

std::string to_string(const Event& event) {
  std::ostringstream os;
  
  if(event.m1 > 0) {
    os << "Trade: price=" << event.m1 << " qty=" << event.m2 << " maker=" << event.m3 << " taker=" << event.m4;
  } else if(event.m1 == EventType::ECreateAccepted) {
    os << "CreateAccepted: id=" << event.m2 << " slot=" << event.m3 << " qty=" << event.m4;
  } else if(event.m1 == EventType::ECreateRejected) {
    os << "CreateRejected: id=" << event.m2 << " size=" << event.m3 << " reason=" << to_string(event.m4);
  } else if(event.m1 == EventType::EUpdateAccepted) {
    os << "UpdateAccepted: id=" << event.m2;
  } else if(event.m1 == EventType::EUpdateRejected) {
    os << "UpdateRejected: id=" << event.m2 << " size=" << event.m3 << " reason=" << to_string(event.m4);
  } else if(event.m1 == EventType::ECancelAccepted) {
    os << "CancelAccepted: id=" << event.m2;
  } else if(event.m1 == EventType::ECancelRejected) {
    os << "CancelRejected: id=" << event.m2 << " reason=" << to_string(event.m3);
  } else if(event.m1 == EventType::ELevelExpired) {
    os << "LevelExpired: price=" << event.m2 << " id=" << event.m3;
  } else if(event.m1 == EventType::ELevelCreated) {
    os << "LevelsCreated: fromPrice=" << event.m2 << " toPrice=" << event.m3;
  } else {
    os << "UnknownEvent: m1=" << event.m1 << " m2=" << event.m2 << " m3=" << event.m3 << " m4=" << event.m4;
  }
  
  return os.str();
}

std::ostream& operator<<(std::ostream& os, const Event& event) {
  return os << to_string(event);
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

  std::size_t clear() noexcept {
    std::size_t count = 0;

    while(_queue.empty_approx() == false) {
      count += 1;
      _queue.pop();
    }

    return count;
  }

  std::size_t log(const std::string& prefix = "") {
    std::size_t count = 0;

    while(_queue.empty_approx() == false) {
      count += 1;
      const Event event = _queue.pop();
      std::cout << prefix << event << std::endl;
    }

    return count;
  }

  RingBufferSPSC<Event, 1024> _queue;
};