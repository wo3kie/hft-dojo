#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <tuple>

#include <functional>

template<typename TValue, std::size_t Capacity>
class FlatList
{
  struct Node
  {
    int32_t next{-1};
    int32_t prev{-1};
    TValue value;
  };

public:
  FlatList()
    : _free(0)
    , _head(-1)
    , _tail(-1)
  {
    for(std::size_t i = 0; i < Capacity - 1; ++i) {
      _buffer[i].next = static_cast<int32_t>(i + 1);
    }
  }

  TValue& front()
  {
    assert(! empty());
    return _buffer[_head].value;
  }

  TValue& back()
  {
    assert(! empty());
    return _buffer[_tail].value;
  }

  int32_t push_front(const TValue& value)
  {
    if(full()) {
      return -1;
    }

    const int32_t node_id = _push_front(_allocate_node());
    _buffer[node_id].value = value;
    return node_id;
  }

  int32_t push_back(const TValue& value)
  {
    if(full()) {
      return -1;
    }

    const int32_t node_id = _push_back(_allocate_node());
    _buffer[node_id].value = value;
    return node_id;
  }

  void pop_front()
  {
    assert(! empty());

    _deallocate_node(_pop_front());
  }

  void pop_back()
  {
    assert(! empty());

    _deallocate_node(_pop_back());
  }

  void remove(int32_t index)
  {
    assert(! empty());
    assert(index >= 0 && index < static_cast<int32_t>(Capacity));

    _deallocate_node(_remove(index));
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
      _buffer[i].next = static_cast<int32_t>(i + 1);
      _buffer[i].prev = -1;
    }

    _buffer[Capacity - 1].next = -1;
    _buffer[Capacity - 1].prev = -1;
  }

  const TValue& at(int32_t index) const
  {
    assert(! empty());
    assert(index >= 0 && index < static_cast<int32_t>(Capacity));

    return _buffer[index].value;
  }

  TValue& at(int32_t index)
  {
    assert(! empty());
    assert(index >= 0 && index < static_cast<int32_t>(Capacity));

    return _buffer[index].value;
  }

  template<typename F>
  void for_each(F&& f) const
  {
    const int32_t head = _head;
    const int32_t tail = _tail;

    for(int32_t i = head; i != -1; i = _buffer[i].next) {
      f(_buffer[i].value);
    }
  }

private:
  int32_t _allocate_node()
  {
    assert(! full());

    const int32_t node_id = _free;
    _free = _buffer[node_id].next;

    return node_id;
  }

  void _deallocate_node(int32_t node_id)
  {
    Node& node = _buffer[node_id];
    node.next = _free;
    node.prev = -1;
    _free = node_id;
  }

  int32_t _push_front(int32_t node_id)
  {
    Node& node = _buffer[node_id];
    node.next = _head;
    node.prev = -1;

    if(empty()) {
      _tail = node_id;
    } else {
      _buffer[_head].prev = node_id;
    }

    return (_head = node_id);
  }

  int32_t _push_back(int32_t node_id)
  {
    Node& node = _buffer[node_id];
    node.next = -1;
    node.prev = _tail;

    if(empty()) {
      _head = node_id;
    } else {
      _buffer[_tail].next = node_id;
    }

    return (_tail = node_id);
  }

  int32_t _pop_front()
  {
    assert(! empty());

    const int32_t index = _head;
    Node& node = _buffer[index];

    _head = node.next;

    if(_head != -1) {
      _buffer[_head].prev = -1;
    } else {
      _tail = -1;
    }

    return index;
  }

  int32_t _pop_back()
  {
    assert(! empty());

    const int32_t index = _tail;
    Node& node = _buffer[index];

    _tail = node.prev;

    if(_tail != -1) {
      _buffer[_tail].next = -1;
    } else {
      _head = -1;
    }

    return index;
  }

  int32_t _remove(int32_t index)
  {
    assert(! empty());

    Node& node = _buffer[index];

    if(node.prev != -1) {
      _buffer[node.prev].next = node.next;
    } else {
      _head = node.next;
    }

    if(node.next != -1) {
      _buffer[node.next].prev = node.prev;
    } else {
      _tail = node.prev;
    }

    node.next = -1;
    node.prev = -1;

    return index;
  }

private:
  int32_t _free;
  int32_t _head;
  int32_t _tail;

  Node _buffer[Capacity];
};
