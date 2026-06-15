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
#include <list>
#include <type_traits>

#include "assert.hpp"
#include "common.hpp"
#include "storage.hpp"

/*
 * FlatQueue Open-Addressing
 */

template<typename Value, int32_t Capacity>
struct FlatQueue {
  static_assert((Capacity > 0) && (Capacity <= 1024 * 1024 * 1024));

public:
  using index_type = index_type_t<Capacity>;

public:
  static constexpr index_type Sentinel = Capacity;
  static constexpr index_type Free = -1;
  
public:
  FlatQueue() {
    for(int32_t i = 0; i < Capacity + 1; i += 1) {
      _buffer[i]._prev = Free;
      _buffer[i]._next = Free;
    }

    _buffer[Sentinel]._prev = Sentinel;
    _buffer[Sentinel]._next = Sentinel;
    _buffer[Sentinel]._size = 0;
  }

  static constexpr int32_t capacity() noexcept {
    return Capacity;
  }

  int32_t size() const noexcept {
    return _buffer[Sentinel]._size;
  }

  [[nodiscard]] bool empty() const noexcept {
    return size() == 0;
  }

  bool full() const noexcept {
    return size() == Capacity;
  }

  Value& front() noexcept {
    return _buffer[_buffer[Sentinel]._next]._value;
  }

  void push(int32_t slot, const Value& value) noexcept {
    assert(slot >= 0 && slot < Sentinel);
    assert(_ext_is_free(slot));

    const index_type prev = _buffer[Sentinel]._prev;
    const index_type next = Sentinel;

    slot = _allocate(slot, value, prev, next);
    
    _buffer[prev]._next = slot;
    _buffer[Sentinel]._prev = slot;
  }

  void erase(int32_t slot) noexcept {
    assert(slot >= 0 && slot < Sentinel);
    assert(! _ext_is_free(slot));

    const index_type prev = _buffer[slot]._prev;
    const index_type next = _buffer[slot]._next;

    _buffer[prev]._next = next;
    _buffer[next]._prev = prev;

    _deallocate(slot);
  }

  void pop() noexcept {
    assert(! _ext_is_free(_buffer[Sentinel]._next));

    const index_type head = _buffer[Sentinel]._next;
    const index_type next = _buffer[head]._next;

    _buffer[next]._prev = Sentinel;
    _buffer[Sentinel]._next = next;

    _deallocate(head);
  }

  Value& operator[](int32_t slot) noexcept {
    return _buffer[slot]._value;
  }

  const Value& operator[](int32_t slot) const noexcept {
    return _buffer[slot]._value;
  }

  int32_t find(const Value& value) const noexcept {
    for(index_type slot = _buffer[Sentinel]._next; slot != Sentinel; slot = _buffer[slot]._next) {
      if(_buffer[slot]._value == value) {
        return slot;
      }
    }

    return -1;
  }

  /* extension */ bool _ext_is_free(int32_t slot) const noexcept {
    assert(slot >= 0 && slot < Sentinel);
    return (_buffer[slot]._prev == Free) && (_buffer[slot]._next == Free);
  }

  /* extension */ bool _ext_equal(std::list<Value> expected) const noexcept {
    if (this->size() != expected.size()) {
      return false;
    }

    for(index_type slot = _buffer[Sentinel]._next; slot != Sentinel; slot = _buffer[slot]._next) {
      assert(_buffer[_buffer[slot]._prev]._next == slot);
      assert(_buffer[_buffer[slot]._next]._prev == slot);

      if(_buffer[slot]._value != expected.front()) {
        return false;
      }

      expected.pop_front();
    }

    return expected.empty();
  }

private:
  struct _Node {
    Value _value;
    index_type _next;
    index_type _prev;
    index_type _size; // only for Sentinel
  };

private:
  int32_t _allocate(int32_t slot, const Value& value, int32_t prev, int32_t next) noexcept {
    assert(slot >= 0 && slot < Sentinel);
    assert(_buffer[slot]._prev == Free);
    assert(_buffer[slot]._next == Free);

    _buffer[slot]._value = value;
    _buffer[slot]._prev = prev;
    _buffer[slot]._next = next;
    _buffer[Sentinel]._size += 1;

    return slot;
  }

  void _deallocate(int32_t slot) noexcept {
    _buffer[slot]._prev = Free;
    _buffer[slot]._next = Free;
    _buffer[Sentinel]._size -= 1;
  }

private:
  Storage<_Node, Capacity + /* Sentinel */ 1> _buffer;
};
