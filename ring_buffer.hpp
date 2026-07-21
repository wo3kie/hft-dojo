#pragma once

/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstddef>
#include <stdexcept>
#include <queue>
#include <utility>

#include "common.hpp"
#include "storage.hpp"

/*
 * RingBuffer
 */

template<typename TValue, std::size_t Capacity>
struct RingBuffer : noncopyable, nonmovable {
  static_assert((Capacity > 0) && (Capacity <= 1024 * 1024 * 1024));

public:
  using value_type = TValue;

public:
  template<typename TT>
  bool push(TT&& t) noexcept {
    if(full()) {
      return false;
    }

    _buffer[_tail] = std::forward<TT>(t);
    _tail = _index(_tail + 1);

    return true;
  }

  bool pop(TValue& out) noexcept{
    if(empty()) {
      return false;
    }

    out = std::move(_buffer[_head]);
    _head = _index(_head + 1);

    return true;
  }

  static constexpr std::size_t capacity() noexcept{
    return Capacity;
  }

  [[nodiscard]] bool empty() const noexcept {
    return _head == _tail;
  }

  std::size_t size() const noexcept {
    if(_tail >= _head) {
      return _tail - _head;
    } else {
      return (Capacity + 1) - (_head - _tail);
    }
  }

  bool full() const noexcept {
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
    if (size() != expected.size()) {
      return false;
    }

    for(std::size_t i = _head; i != _tail; i = _index(i + 1)) {
      if(_buffer[i] != expected.front()) {
        return false;
      }

      expected.pop();
    }

    return expected.empty();
  }
  
private:
  static constexpr std::size_t _index(std::size_t i) noexcept {
    constexpr bool isPowerOf2 = ((Capacity + 1) & Capacity) == 0;

    if constexpr(isPowerOf2) {
      return i & Capacity;
    } else {
      return (i == (Capacity + 1) ? 0 : i); // return i % (Capacity + 1);
    }
  }

private:
  std::size_t _head{0};
  std::size_t _tail{0};
  Storage<TValue, Capacity + /* N+1 trick */ 1> _buffer;
};
