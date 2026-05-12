#pragma once

/*
 * Website:
 *      https://github.com/wo3kie/cpp-dojo
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

template<std::size_t Size, typename TType>
class FlatList
{
  struct Node
  {
    int32_t next{-1};
    int32_t prev{-1};
    TType value;
  };

public:
  FlatList()
    : _free(0)
    , _head(-1)
    , _tail(-1)
  {
    for(std::size_t i = 0; i < Size - 1; ++i) {
      _storage[i].next = static_cast<int32_t>(i + 1);
    }
  }

  TType& front()
  {
    assert(! empty());
    return _storage[_head].value;
  }

 TType& back()
  {
    assert(! empty());
    return _storage[_tail].value;
  }

  int32_t push_front(const TType& value)
  {
    if (full()) {
      return -1;
    }

    const int32_t node_id = _push_front(_allocate_node());
    _storage[node_id].value = value;
    return node_id;
  }

  int32_t push_back(const TType& value)
  {
    if (full()) {
      return -1;
    }

    const int32_t node_id = _push_back(_allocate_node());
    _storage[node_id].value = value;
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
    assert(index >= 0 && index < static_cast<int32_t>(Size));

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

    for(std::size_t i = 0; i < Size - 1; ++i) {
      _storage[i].next = static_cast<int32_t>(i + 1);
      _storage[i].prev = -1;
    }

    _storage[Size - 1].next = -1;
    _storage[Size - 1].prev = -1;
  }

  const TType& at(int32_t index) const 
  {
    assert(! empty());
    assert(index >= 0 && index < static_cast<int32_t>(Size));
    
    return _storage[index].value;
  }

  TType& at(int32_t index) 
  {
    assert(! empty());
    assert(index >= 0 && index < static_cast<int32_t>(Size));
    
    return _storage[index].value;
  }

  void for_each(const std::function<void(const TType&)>& func) const
  {
    int32_t index = _head;

    while(index != -1) {
      func(_storage[index].value);
      index = _storage[index].next;
    }
  }
  
private:
  int32_t _allocate_node()
  {
    assert(! full());

    const int32_t node_id = _free;
    _free = _storage[node_id].next;

    return node_id;
  }

  void _deallocate_node(int32_t node_id)
  {
    Node& node = _storage[node_id];
    node.next = _free;
    node.prev = -1;
    _free = node_id;
  }

  int32_t _push_front(int32_t node_id)
  {
    Node& node = _storage[node_id];
    node.next = _head;
    node.prev = -1;

    if(empty()) {
      _tail = node_id;
    } else {
      _storage[_head].prev = node_id;
    }

    return (_head = node_id);
  }

  int32_t _push_back(int32_t node_id)
  {
    Node& node = _storage[node_id];
    node.next = -1;
    node.prev = _tail;

    if(empty()) {
      _head = node_id;
    } else {
      _storage[_tail].next = node_id;
    }

    return (_tail = node_id);
  }

  int32_t _pop_front()
  {
    assert(! empty());

    const int32_t index = _head;
    Node& node = _storage[index];

    _head = node.next;

    if(_head != -1) {
      _storage[_head].prev = -1;
    } else {
      _tail = -1;
    }

    return index;
  }

  int32_t _pop_back()
  {
    assert(! empty());

    const int32_t index = _tail;
    Node& node = _storage[index];

    _tail = node.prev;

    if(_tail != -1) {
      _storage[_tail].next = -1;
    } else {
      _head = -1;
    }

    return index;
  }

  int32_t _remove(int32_t index)
  {
    assert(! empty());

    Node& node = _storage[index];

    if(node.prev != -1) {
      _storage[node.prev].next = node.next;
    } else {
      _head = node.next;
    }

    if(node.next != -1) {
      _storage[node.next].prev = node.prev;
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

  std::array<Node, Size> _storage;
};
