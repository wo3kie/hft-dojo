#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <tuple>

#include "./assert.hpp"

template<typename TValue, std::size_t Capacity>
class FlatList
{
  static_assert(Capacity > 0);
  static_assert(std::is_trivially_copyable_v<TValue>);

  struct Node
  {
    int32_t next;
    int32_t prev;
    TValue value;
  };

  constexpr static int32_t None = -1;
  constexpr static int32_t Free = -2;

public:
  FlatList()
  {
    clear();
  }

  FlatList(FlatList&&) = delete;
  FlatList(const FlatList&) = delete;

  FlatList& operator=(FlatList&&) = delete;
  FlatList& operator=(const FlatList&) = delete;

public:
  TValue& front()
  {
    return _value(_head);
  }

  TValue& back()
  {
    return _value(_tail);
  }

  int32_t push_front(const TValue& value)
  {
    if(full()) {
      return -1;
    }

    const int32_t slot = _push_front(_allocate_node());
    _value(slot) = value;
    return slot;
  }

  int32_t push_back(const TValue& value)
  {
    if(full()) {
      return -1;
    }

    const int32_t slot = _push_back(_allocate_node());
    _value(slot) = value;
    return slot;
  }

  void pop_front()
  {
    _deallocate_node(_pop_front());
  }

  void pop_back()
  {
    _deallocate_node(_pop_back());
  }

  void remove(int32_t slot)
  {
    _deallocate_node(_remove(slot));
  }

  std::size_t capacity() const
  {
    return Capacity;
  }

  bool empty() const
  {
    return _head == -1;
  }

  bool full() const
  {
    return _free == -1;
  }

  void clear()
  {
    _free = 0;
    _head = -1;
    _tail = -1;

    for(std::size_t i = 0; i < Capacity - 1; ++i) {
      _buffer[i].next = i + 1;
      _buffer[i].prev = Free;
    }

    _buffer[Capacity - 1].next = None;
    _buffer[Capacity - 1].prev = Free;
  }

  const TValue& value(int32_t slot) const
  {
    return _value(slot);
  }

  TValue& value(int32_t slot)
  {
    return _value(slot);
  }

public: /* debug/utests */
   bool _debug_is_free(int32_t slot) const
  {
    return _is_free(slot);
  }

  bool _debug_validate_links() const
  {
    int32_t prev = -1;
    int32_t cur = _head;

    while(cur != -1) {
      const auto& node = _buffer[cur];
      
      if(node.prev != prev) {
        return false;
      }

      prev = cur;
      cur = node.next;
    }

    return prev == _tail;
  }

private:
  bool _validate_slot(int32_t slot) const
  {
    return slot >= 0 && slot < capacity();
  }

  bool _is_free(int32_t slot) const
  {
    Assert(_validate_slot(slot));
    
    return _is_free(_buffer[slot]);
  }

  bool _is_free(const Node& node) const
  {
    return node.prev == Free;
  }

  TValue& _value(int32_t slot)
  {
    Assert(!_is_free(slot));

    return _buffer[slot].value;
  }

  const TValue& _value(int32_t slot) const
  {
    Assert(!_is_free(slot));

    return _buffer[slot].value;
  }

  int32_t _allocate_node()
  {
    Assert(! full());
    Assert(_is_free(_free));
    
    const int32_t slot = _free;
    _free = _buffer[slot].next;

    return slot;
  }

  void _deallocate_node(int32_t slot)
  {
    Assert(! _is_free(slot));

    Node& node = _buffer[slot];
    node.next = _free;
    node.prev = Free;
    _free = slot;
  }

  int32_t _push_front(int32_t slot)
  {
    Assert(_is_free(slot));

    Node& node = _buffer[slot];
    node.next = _head;
    node.prev = None;

    if(empty()) {
      _tail = slot;
    } else {
      _buffer[_head].prev = slot;
    }

    return (_head = slot);
  }

  int32_t _push_back(int32_t slot)
  {
    Assert(_is_free(slot));

    Node& node = _buffer[slot];
    node.next = None;
    node.prev = _tail;

    if(empty()) {
      _head = slot;
    } else {
      _buffer[_tail].next = slot;
    }

    return (_tail = slot);
  }

  int32_t _pop_front()
  {
    const int32_t slot = _head;
    Node& node = _buffer[slot];

    Assert(! _is_free(node));

    _head = node.next;

    if(_head != None) {
      _buffer[_head].prev = None;
    } else {
      _tail = None;
    }

    return slot;
  }

  int32_t _pop_back()
  {
    const int32_t slot = _tail;
    Node& node = _buffer[slot];
    
    Assert(! _is_free(node));

    _tail = node.prev;

    if(_tail != None) {
      _buffer[_tail].next = None;
    } else {
      _head = None;
    }

    return slot;
  }

  int32_t _remove(int32_t slot)
  {
    Assert(! _is_free(slot));

    Node& node = _buffer[slot];

    if(node.prev != None) {
      _buffer[node.prev].next = node.next;
    } else {
      _head = node.next;
    }

    if(node.next != None) {
      _buffer[node.next].prev = node.prev;
    } else {
      _tail = node.prev;
    }

    return slot;
  }

private:
  int32_t _free;
  int32_t _head;
  int32_t _tail;

  Node _buffer[Capacity];
};
