#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cassert>
#include <cstdint>

template<typename T, int8_t Capacity>
class FlatQueue {
public:
  static_assert(Capacity > 0 && Capacity < 127);
  static_assert(std::is_nothrow_copy_assignable_v<T>);

private:
  struct Node {
    int8_t next{-1};
    int8_t prev{-1};
    T value{};
  };

public:
  FlatQueue() {
    for(int8_t i = 0; i < Capacity - 1; i++) {
      nodes[i].next = i + 1;
    }

    nodes[Capacity - 1].next = -1;
  }

  FlatQueue(FlatQueue&&) = delete;
  FlatQueue(const FlatQueue&) = delete;

  FlatQueue& operator=(FlatQueue&&) = delete;
  FlatQueue& operator=(const FlatQueue&) = delete;

public:
  int8_t push(const T& v) noexcept {
    assert(! full());

    const int8_t slot = _allocate();
    Node& n = nodes[slot];
    n.value = v;
    n.next = -1;
    n.prev = _tail;

    if(_tail == -1) {
      _head = slot;
      _tail = slot;
    } else {
      nodes[_tail].next = slot;
      _tail = slot;
    }

    return slot;
  }

  void pop() noexcept {
    assert(! empty());

    const int8_t old = _head;
    const int8_t new_head = nodes[old].next;
    _head = new_head;

    if(new_head == -1) {
      _tail = -1;
    } else {
      nodes[new_head].prev = -1;
    }

    _deallocate(old);
  }

  void remove(int8_t slot) noexcept {
    assert(slot >= 0 && slot < Capacity);

    const int8_t prev = nodes[slot].prev;
    const int8_t next = nodes[slot].next;

    if(prev == -1) {
      _head = next;
    } else {
      nodes[prev].next = next;
    }

    if(next == -1) {
      _tail = prev;
    } else {
      nodes[next].prev = prev;
    }

    _deallocate(slot);
  }

  T& front() noexcept {
    assert(! empty());
    return nodes[_head].value;
  }

  T& at(int8_t slot) noexcept {
    assert(slot >= 0 && slot < Capacity);
    return nodes[slot].value;
  }

  bool empty() const noexcept {
    return _head == -1;
  }

  bool full() const noexcept {
    return _free == -1;
  }

  Node& operator[](int8_t slot) noexcept {
    return nodes[slot];
  }

private:
  int8_t _allocate() {
    const int8_t slot = _free;
    _free = nodes[slot].next;
    return slot;
  }

  void _deallocate(int8_t slot) {
    nodes[slot].next = _free;
    nodes[slot].prev = -1;
    _free = slot;
  }

private:
  int8_t _head{-1};
  int8_t _tail{-1};
  int8_t _free{0};

  Node nodes[Capacity];
};
