#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include "storage.hpp"

#include "common.hpp"

/*
 * Circular Buffer, Single Producer Single Consumer, Lock Free
 */

template<typename TValue, int32_t Capacity>
struct RingBufferSPSC : noncopyable, nonmovable {
  static_assert((Capacity > 0) && (Capacity <= 1024 * 1024 * 1024));

public:
  using value_type = TValue;

public:
  RingBufferSPSC() = default;
  ~RingBufferSPSC() = default;

public:  
  static constexpr int32_t capacity() {
    return Capacity;
  }

  /* approximate */ int32_t size_approx() const {
    const int32_t head = _head.load(std::memory_order_acquire);
    const int32_t tail = _tail.load(std::memory_order_acquire);

    if(tail >= head) {
      return tail - head;
    } else {
      return (Capacity + 1) - (head - tail);
    }
  }

  /* approximate */ [[nodiscard]] bool empty_approx() const {
    const int32_t head = _head.load(std::memory_order_acquire);
    const int32_t tail = _tail.load(std::memory_order_acquire);

    return head == tail;
  }

  /* approximate */ bool full_approx() const {
    const int32_t head = _head.load(std::memory_order_acquire);
    const int32_t tail = _tail.load(std::memory_order_acquire);

    return head == _index(tail + 1);
  }

  template<typename T>
  bool push(T&& value) {
    const int32_t head = _head.load(std::memory_order_acquire);
    const int32_t tail = _tail.load(std::memory_order_relaxed);

    if(head == _index(tail + 1)) {
      return false;
    }

    _buffer[tail] = std::forward<T>(value);
    _tail.store(_index(tail + 1), std::memory_order_release);

    return true;
  }

  bool pop(TValue& out) {
    const int32_t head = _head.load(std::memory_order_relaxed);
    const int32_t tail = _tail.load(std::memory_order_acquire);

    if(head == tail) {
      return false;
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

  /* extension */ bool _ext_equal(std::queue<TValue> expected) const {
    const int32_t head = _head.load(std::memory_order_acquire);
    const int32_t tail = _tail.load(std::memory_order_acquire);

    for(int32_t i = head; i != tail; i = _index(i + 1)) {
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
  alignas(64) std::atomic<int32_t> _head{0};
  alignas(64) std::atomic<int32_t> _tail{0};
  alignas(64) Storage<TValue, Capacity + /* N+1 trick */ 1> _buffer;
};