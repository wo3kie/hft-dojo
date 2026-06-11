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

#include "storage.hpp"

/*
 * RingBuffer
 */

template<typename TValue, std::size_t Capacity>
class RingBuffer {
public:
  using value_type = TValue;

  RingBuffer() = default;
  RingBuffer(RingBuffer&&) = delete;
  RingBuffer(const RingBuffer&) = delete;

  ~RingBuffer() = default;

  RingBuffer& operator=(RingBuffer&&) = delete;
  RingBuffer& operator=(const RingBuffer&) = delete;

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

  static constexpr std::size_t capacity() {
    return Capacity;
  }

  [[nodiscard]] bool empty() const {
    return _head == _tail;
  }

  bool full() const {
    return _index(_tail + 1) == _head;
  }

  /* extension */ TValue pop() {
    TValue out;

    if(! pop(out)) {
      throw std::runtime_error("RingBuffer is empty");
    }

    return out;
  }

private:
  static constexpr std::size_t _index(std::size_t i) {
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
