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

template<typename Value, int32_t Capacity>
struct FlatList {
public:
  using index_type = index_type_t<Capacity>;

public:
  FlatList() noexcept 
    : _head(-1)
    , _tail(-1) 
  {
  }

  static constexpr int32_t capacity() noexcept {
    return Capacity;
  }

  int32_t size() const noexcept {
    return _pool.size();
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

  Value& operator[](int32_t slot) noexcept {
    return _pool[slot]._value;
  }

  const Value& operator[](int32_t slot) const noexcept {
    return _pool[slot]._value;
  }

  int32_t insert(int32_t next, const Value& value) noexcept {
    assert(! full());

    if (UNLIKELY(next == _head)) {
      return push_front(value);
    }

    if (UNLIKELY(next == -1)) {
      return push_back(value);
    }

    const index_type prev = _pool[next]._prev;
    const index_type slot = _pool.allocate(value, prev, next);

    _pool[next]._prev = slot;
    _pool[prev]._next = slot;

    return slot;
  }

  void erase(int32_t pos) noexcept {
    assert(! empty());

    if (UNLIKELY(pos == _head)) {
      return pop_front();
    }

    if (UNLIKELY(pos == _tail)) {
      return pop_back();
    }

    const index_type prev = _pool[pos]._prev;
    const index_type next = _pool[pos]._next;

    _pool[prev]._next = next;
    _pool[next]._prev = prev;

    _pool.deallocate(pos);
  }

  index_type push_front(const Value& value) noexcept {
    assert(! full());

    const index_type next = _head;
    const index_type slot = (index_type)_pool.allocate(value, -1, next);

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
    const index_type slot = (index_type)_pool.allocate(value, prev, -1);

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

  int32_t find(const Value& value) const noexcept {
    index_type head = _head;

    while(head != -1) {
      if(_pool[head]._value == value) {
        return head;
      }

      head = _pool[head]._next;
    }

    return -1;
  }

  /* extension */ bool _ext_equal(std::list<Value> expected) const noexcept {
    if (this->size() != expected.size()) {
      return false;
    }

    for(index_type slot = _head; slot != -1; slot = _pool[slot]._next) {
      assert(_pool[slot]._prev == -1 || _pool[_pool[slot]._prev]._next == slot);
      assert(_pool[slot]._next == -1 || _pool[_pool[slot]._next]._prev == slot);

      if(_pool[slot]._value != expected.front()) {
        return false;
      }

      expected.pop_front();
    }

    return true;
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
