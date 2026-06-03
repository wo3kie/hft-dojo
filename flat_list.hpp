#include <cassert>
#include <cstdint>
#include <iterator>

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

template<typename Value, int8_t Capacity>
class FlatList {
public:
  static_assert(Capacity > 1 && Capacity < 127);
  static_assert(std::is_nothrow_copy_assignable_v<Value>);
  static constexpr int8_t Sentinel = Capacity;

public:
  FlatList() noexcept {
    for(int8_t i = 0; i < Capacity; i++) {
      _buffer[i]._next = i + 1;
    }

    _buffer[Capacity - 1]._next = -1;
    _buffer[Sentinel]._next = Sentinel;
    _buffer[Sentinel]._prev = Sentinel;
    _buffer[Sentinel]._size = 0;
  }

  bool empty() const noexcept {
    return _buffer[Sentinel]._size == 0;
  }

  bool full() const noexcept {
    return _buffer[Sentinel]._size == Capacity;
  }

  Value& front() noexcept {
    assert(! empty());
    return _buffer[_buffer[Sentinel]._next]._value;
  }

  Value& back() noexcept {
    assert(! empty());
    return _buffer[_buffer[Sentinel]._prev]._value;
  }

  Value& operator[](int8_t slot) noexcept {
    assert(slot >= 0 && slot < Sentinel);
    return _buffer[slot]._value;
  }

  const Value& operator[](int8_t slot) const noexcept {
    assert(slot >= 0 && slot < Sentinel);
    return _buffer[slot]._value;
  }

  int8_t insert(int8_t prev, const Value& value) noexcept {
    assert(! full());
    assert(prev >= -1 && prev <= Sentinel);

    const int8_t slot = _allocate();
    _Node& node = _buffer[slot];
    node._value = value;
    int8_t next;

    if(prev == -1) {
      next = _buffer[Sentinel]._next;
      _buffer[Sentinel]._next = slot;
      node._prev = Sentinel;
    } else {
      next = _buffer[prev]._next;
      _buffer[prev]._next = slot;
      node._prev = prev;
    }

    node._next = next;
    _buffer[next]._prev = slot;

    _buffer[Sentinel]._size += 1;
    return slot;
  }

  void remove(int8_t slot) noexcept {
    assert(slot >= 0 && slot < Sentinel);

    const int8_t prev = _buffer[slot]._prev;
    const int8_t next = _buffer[slot]._next;

    _buffer[prev]._next = next;
    _buffer[next]._prev = prev;

    _deallocate(slot);
    _buffer[Sentinel]._size -= 1;
  }

  int8_t push_front(const Value& v) noexcept {
    return insert(-1, v);
  }

  int8_t push_back(const Value& v) noexcept {
    return insert(_buffer[Sentinel]._prev, v);
  }

  void pop_front() noexcept {
    assert(! empty());
    remove(_buffer[Sentinel]._next);
  }

  void pop_back() noexcept {
    assert(! empty());
    remove(_buffer[Sentinel]._prev);
  }

private:
  struct _Node {
    Value _value;
    int8_t _next;
    int8_t _prev;
    int8_t _size; // only sentinel uses this
  };

  int8_t _allocate() noexcept {
    const int8_t slot = _free;
    _free = _buffer[slot]._next;
    return slot;
  }

  void _deallocate(int8_t slot) noexcept {
    _buffer[slot]._next = _free;
    _buffer[slot]._prev = -1;
    _free = slot;
  }

private:
  int8_t _free{0};
  _Node _buffer[Capacity + /* Sentinel */ 1];
};
