#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cassert>
#include <cstdint>
#include <type_traits>

#include "storage.hpp"

/*
 * FlatQueue Open-Addressing
 */

template<typename Value, uint8_t Capacity>
class FlatQueue_OA {
  static_assert(Capacity > 1 && Capacity < 127);
  static_assert(std::is_nothrow_copy_assignable_v<Value>);
  static constexpr int8_t Sentinel = Capacity;
  
public:
  FlatQueue_OA() {
    for(int8_t i = 0; i < Capacity + 1; i += 1) {
      _buffer[i]._next = Sentinel;
      _buffer[i]._prev = Sentinel;
    }

    _buffer[Sentinel]._size = 0;
  }

  bool empty() const noexcept {
    return _buffer[Sentinel]._size == 0;
  }

  bool full() const noexcept {
    return _buffer[Sentinel]._size == Capacity;
  }

  Value& front() noexcept {
    return _buffer[_buffer[Sentinel]._next]._value;
  }

  Value& operator[](int8_t slot) noexcept {
    assert(slot >= 0 && slot < Sentinel);
    return _buffer[slot]._value;
  }

  const Value& operator[](int8_t slot) const noexcept {
    assert(slot >= 0 && slot < Sentinel);
    return _buffer[slot]._value;
  }

  void insert(int8_t slot, const Value& value) noexcept {
    assert(slot >= 0 && slot < Sentinel);

    slot = _allocate(slot);
    _Node& node = _buffer[slot];
    node._value = value;

    const int8_t tail = _buffer[Sentinel]._prev;

    node._prev = tail;
    node._next = Sentinel;

    _buffer[tail]._next = slot;
    _buffer[Sentinel]._prev = slot;

    _buffer[Sentinel]._size += 1;
  }

  void remove(int8_t slot) noexcept {
    const int8_t prev = _buffer[slot]._prev;
    const int8_t next = _buffer[slot]._next;

    _buffer[prev]._next = next;
    _buffer[next]._prev = prev;

    _deallocate(slot);

    _buffer[Sentinel]._size -= 1;
  }

  void pop() noexcept {
    const int8_t head = _buffer[Sentinel]._next;
    const int8_t next = _buffer[head]._next;

    _buffer[next]._prev = Sentinel;
    _buffer[Sentinel]._next = next;

    _deallocate(head);

    _buffer[Sentinel]._size -= 1;
  }

private:
  struct _Node {
    Value _value;
    int8_t _next;
    int8_t _prev;
    int8_t _size; // only for Sentinel
  };

private:
  int8_t _allocate(int8_t slot) noexcept {
    return slot;
  }

  void _deallocate(int8_t slot) noexcept {
    _buffer[slot]._next = Sentinel;
    _buffer[slot]._prev = Sentinel;
  }

private:
  Storage<_Node, Capacity + /* Sentinel */ 1> _buffer;
};
