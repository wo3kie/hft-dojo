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

#include "object_pool.hpp"
#include "storage.hpp"

template<typename Value, std::size_t Capacity>
class FlatList {
public:
  using index_type = index_type_t<Capacity>;

public:
  FlatList() noexcept 
    : _head(-1)
    , _tail(-1) 
  {
  }

  bool empty() const noexcept {
    return _pool.empty();
  }

  bool full() const noexcept {
    return _pool.full();
  }

  Value& front() noexcept {
    return _pool[_head]._value;
  }

  Value& back() noexcept {
    return _pool[_tail]._value;
  }

  Value& operator[](std::size_t slot) noexcept {
    return _pool[slot]._value;
  }

  const Value& operator[](std::size_t slot) const noexcept {
    return _pool[slot]._value;
  }

int32_t insert(int32_t prev, const Value& value) noexcept {
    assert(! full());

    if (UNLIKELY(prev == -1)) {
      return push_front(value);
    }

    if (UNLIKELY(prev == _tail)) {
      return push_back(value);
    }

    const index_type next = _pool[prev]._next;
    const index_type slot = _pool.allocate(value, prev, next);

    _pool[prev]._next = slot;
    _pool[next]._prev = slot;

    return slot;
  }

  void remove(int32_t slot) noexcept {
    assert(! empty());

    if (UNLIKELY(slot == -1)) {
      return pop_front();
    }

    if (UNLIKELY(slot == _tail)) {
      return pop_back();
    }

    const index_type prev = _pool[slot]._prev;
    const index_type next = _pool[slot]._next;

    _pool[prev]._next = next;
    _pool[next]._prev = prev;

    _pool.deallocate(slot);
  }

  index_type push_front(const Value& value) noexcept {
    assert(! full());

    const index_type next = _head;
    const index_type slot = _pool.allocate(value, -1, next);

    _head = slot;

    if(next != -1) {
      _pool[next]._prev = slot;
    } else {
      _tail = slot;
    }

    return slot;
  }

  int32_t push_back(const Value& value) noexcept {
    assert(! full());

    const index_type prev = _tail;
    const index_type slot = _pool.allocate(value, prev, -1);

    if(prev != -1) {
      _pool[prev]._next = slot;
    } else {
      _head = slot;
    }

    _tail = slot;
    return slot;
  }

  void pop_front() noexcept {
    assert(! empty());

    const index_type slot = _head;
    const index_type next = _pool[slot]._next;

    _head = next;

    if(slot != _tail) {
      _pool[next]._prev = -1;
    } else {
      _tail = -1;
    }

    _pool.deallocate(slot);
  }

  void pop_back() noexcept {
    assert(! empty());

    const index_type slot = _tail;
    const index_type prev = _pool[slot]._prev;

    if(slot != _head) {
      _pool[prev]._next = -1;
    } else {
      _head = -1;
    }

    _tail = prev;
    _pool.deallocate(slot);
  }

  constexpr int32_t capacity() const noexcept {
    return _pool.capacity();
  }

  /* extension */ bool _debug_equal(std::list<Value>& expected) const noexcept {
    index_type head = _head;
    std::list<Value> actual;

    while(head != -1) {
      actual.push_back(_pool[head]._value);
      head = _pool[head]._next;
    }

    return actual == expected;
  }

private:
  struct _Node {
    Value _value;
    index_type _prev;
    index_type _next;
  };

private:
  index_type _head;
  index_type _tail;

  ObjectPool<_Node, Capacity> _pool;
};
