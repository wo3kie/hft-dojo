#pragma once

/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <queue>

#include "common.hpp"
#include "storage.hpp"

/*
 * Circular Buffer, Single Producer Single Consumer, Lock Free
 */

template<typename TValue, std::size_t Capacity>
struct RingBufferSPSC : noncopyable, nonmovable {
  static_assert((Capacity > 0) && (Capacity <= 1024 * 1024 * 1024));

public:
  using value_type = TValue;

public:  
  static constexpr std::size_t capacity() {
    return Capacity;
  }

  template<typename T>
  bool push(T&& value) {
    const std::size_t tail = _tail.load(std::memory_order_relaxed);
    const std::size_t next = _index(tail + 1);

    if (UNLIKELY(next == _headCached)) {
      _headCached = _head.load(std::memory_order_acquire);

      if (UNLIKELY(next == _headCached)) {
        return false;
      }
    }

    _buffer[tail] = std::forward<T>(value);
    _tail.store(next, std::memory_order_release);

    return true;
  }

  bool pop(TValue& out) {
    const std::size_t head = _head.load(std::memory_order_relaxed);

    if (UNLIKELY(head == _tailCached)) {
      _tailCached = _tail.load(std::memory_order_acquire);

      if (UNLIKELY(head == _tailCached)) {
        return false;
      }
    }

    out = std::move(_buffer[head]);
    _head.store(_index(head + 1), std::memory_order_release);

    return true;
  }

  /* extension */ TValue _ext_pop() {
    TValue out;

    if(pop(out) == false) {
      throw std::runtime_error("Buffer is empty");
    }

    return out;
  }

  /* approximate */ std::size_t _approx_size() const {
    const std::size_t head = _head.load(std::memory_order_acquire);
    const std::size_t tail = _tail.load(std::memory_order_acquire);

    if(tail >= head) {
      return tail - head;
    } else {
      return (Capacity + 1) - (head - tail);
    }
  }

  /* approximate */ [[nodiscard]] bool _approx_empty() const {
    const std::size_t head = _head.load(std::memory_order_acquire);
    const std::size_t tail = _tail.load(std::memory_order_acquire);

    return head == tail;
  }

  /* approximate */ bool _approx_full() const {
    const std::size_t head = _head.load(std::memory_order_acquire);
    const std::size_t tail = _tail.load(std::memory_order_acquire);

    return head == _index(tail + 1);
  }

  /* extension */ bool _ext_equal(std::queue<TValue> expected) const {
    const std::size_t head = _head.load(std::memory_order_acquire);
    const std::size_t tail = _tail.load(std::memory_order_acquire);

    for(std::size_t i = head; i != tail; i = _index(i + 1)) {
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
  // producer line, writes _tail, uses _headCached
  alignas(CacheLineSize) std::atomic<std::size_t> _tail{0};
                         std::size_t _headCached{0};
  
  // consumer line, writes _head, uses _tailCached
  alignas(CacheLineSize) std::atomic<std::size_t> _head{0};
                         std::size_t _tailCached{0};

  alignas(CacheLineSize) Storage<TValue, Capacity + /* N+1 trick */ 1> _buffer;
};