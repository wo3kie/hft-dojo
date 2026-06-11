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

#include "assert.hpp"
#include "common.hpp"
#include "storage.hpp"

/*
 * FlatQueue Open-Addressing
 */

template<typename Value, std::size_t Capacity>
class FlatQueue {
  using index_type = index_type_t<Capacity>;

  static constexpr index_type Sentinel = Capacity;
  
public:
  FlatQueue() {
    for(std::size_t i = 0; i < Capacity + 1; i += 1) {
      _buffer[i]._next = Sentinel;
      _buffer[i]._prev = Sentinel;
    }

    _buffer[Sentinel]._size = 0;
  }

  static constexpr std::size_t capacity() noexcept {
    return Capacity;
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

  Value& operator[](std::size_t slot) noexcept {
    return _buffer[slot]._value;
  }

  const Value& operator[](std::size_t slot) const noexcept {
    return _buffer[slot]._value;
  }

  void push(std::size_t slot, const Value& value) noexcept {
    assert(slot >= 0 && slot < Sentinel);

    slot = _allocate(slot);
    _buffer[Sentinel]._size += 1;

    _Node& node = _buffer[slot];
    node._value = value;

    const index_type tail = _buffer[Sentinel]._prev;

    node._prev = tail;
    node._next = Sentinel;

    _buffer[tail]._next = slot;
    _buffer[Sentinel]._prev = slot;
  }

  void remove(std::size_t slot) noexcept {
    const index_type prev = _buffer[slot]._prev;
    const index_type next = _buffer[slot]._next;

    _buffer[prev]._next = next;
    _buffer[next]._prev = prev;

    _deallocate(slot);
    _buffer[Sentinel]._size -= 1;
  }

  void pop() noexcept {
    const index_type head = _buffer[Sentinel]._next;
    const index_type next = _buffer[head]._next;

    _buffer[next]._prev = Sentinel;
    _buffer[Sentinel]._next = next;

    _deallocate(head);
    _buffer[Sentinel]._size -= 1;
  }

private:
  struct _Node {
    Value _value;
    index_type _next;
    index_type _prev;
    index_type _size; // only for Sentinel
  };

private:
  std::size_t _allocate(std::size_t slot) noexcept {
    assert(! full());
    assert(slot >= 0 && slot < Sentinel);
    assert(_buffer[slot]._prev == Sentinel);
    assert(_buffer[slot]._next == Sentinel);

    return slot;
  }

  void _deallocate(std::size_t slot) noexcept {
    _buffer[slot]._next = Sentinel;
    _buffer[slot]._prev = Sentinel;
  }

private:
  Storage<_Node, Capacity + /* Sentinel */ 1> _buffer;
};
