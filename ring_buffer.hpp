#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "common.hpp"
#include "storage.hpp"

/*
 * RingBuffer
 */

template<typename TValue, int32_t Capacity>
struct RingBuffer : noncopyable, nonmovable {
  static_assert((Capacity > 0) && (Capacity <= 1024 * 1024 * 1024));

public:
  using value_type = TValue;

  RingBuffer() = default;
  ~RingBuffer() = default;

public:
  template<typename TT>
  bool push(TT&& t) {
    if(full()) {
      return false;
    }

    _buffer[_tail] = std::forward<TT>(t);
    _tail = _index(_tail + 1);

    return true;
  }

  bool pop(TValue& out) {
    if(empty()) {
      return false;
    }

    out = std::move(_buffer[_head]);
    _head = _index(_head + 1);

    return true;
  }

  static constexpr int32_t capacity() {
    return Capacity;
  }

  [[nodiscard]] bool empty() const {
    return _head == _tail;
  }

  bool full() const {
    return _index(_tail + 1) == _head;
  }

  /* extension */ TValue _ext_pop() {
    TValue out;

    if(! pop(out)) {
      throw std::runtime_error("RingBuffer is empty");
    }

    return out;
  }

  /* extension */ bool _ext_equal(std::queue<TValue> expected) const noexcept {
    for(int32_t i = _head; i != _tail; i = _index(i + 1)) {
      if(_buffer[i] != expected.front()) {
        return false;
      }

      expected.pop();
    }

    return expected.empty();
  }
  
private:
  static constexpr int32_t _index(int32_t i) {
    constexpr bool isPowerOf2 = ((Capacity + 1) & Capacity) == 0;

    if constexpr(isPowerOf2) {
      return i & Capacity;
    } else {
      return i % (Capacity + 1);
    }
  }

private:
  std::size_t _head{0};
  std::size_t _tail{0};
  Storage<TValue, /* N+1 trick */ Capacity + 1> _buffer;
};
