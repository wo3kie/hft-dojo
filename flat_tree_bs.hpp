#pragma once

/*
 * Project:
 *      CxxDojo (https://github.com/wo3kie/cpp-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <functional>

#include "common.hpp"

template<typename TKey, int32_t Capacity, typename TCompare = std::less<TKey>>
struct FlatTreeBS: noncopyable, nonmovable {
  static constexpr int32_t npos = -1;

  struct Node {
    TKey _key{};
    int32_t _leftId{npos};
    int32_t _rightId{npos};
    int32_t _parentId{npos};
  };

  struct iterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type = TKey;
    using difference_type = std::ptrdiff_t;
    using pointer = TKey*;
    using reference = TKey&;

    FlatTreeBS* _tree{nullptr};
    int32_t _nodeId{npos};

    iterator& operator++() noexcept {
      if(_nodeId == npos) {
        return *this;
      }

      Node& node = _tree->_buffer[_nodeId];

      if(node._rightId != npos) {
        _nodeId = node._rightId;

        while(_tree->_buffer[_nodeId]._leftId != npos) {
          _nodeId = _tree->_buffer[_nodeId]._leftId;
        }
      } else {
        int32_t parentId = node._parentId;

        while(parentId != npos && _tree->_buffer[parentId]._rightId == _nodeId) {
          _nodeId = parentId;
          parentId = _tree->_buffer[parentId]._parentId;
        }

        _nodeId = parentId;
      }

      return *this;
    }

    iterator& operator++(int) noexcept {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    TKey& operator*() noexcept {
      return _tree->_buffer[_nodeId]._key;
    }

    bool operator==(const iterator& other) const noexcept {
      return _tree == other._tree && _nodeId == other._nodeId;
    }

    bool operator!=(const iterator& other) const noexcept {
      return !(*this == other);
    }
  };

public:
  FlatTreeBS() noexcept {
    for(int32_t i = 0; i < Capacity; ++i) {
      _free[i] = i;
    }
  }

  explicit FlatTreeBS(TCompare cmp) noexcept
    : FlatTreeBS() 
  {
    _cmp = cmp;
  }

public:
  int32_t insert(const TKey& key) noexcept {
    return _insert(key);
  }

  TKey* find(const TKey& key) noexcept {
    return (_rootId == npos) ? nullptr : &_buffer[_find(key)]._key;
  }

  bool erase(const TKey& key) noexcept {
    return _erase(_rootId, key);
  }

  TKey* get_by_slot(int32_t slot_id) noexcept {
    return (slot_id == npos) ? nullptr : &_buffer[slot_id]._key;
  }

  iterator begin() noexcept {
    iterator it;
    it._tree = this;
    it._nodeId = npos;

    if(_rootId != npos) {
      it._nodeId = _rootId;

      while(_buffer[it._nodeId]._leftId != npos) {
        it._nodeId = _buffer[it._nodeId]._leftId;
      }
    }

    return it;
  }

  iterator end() noexcept {
    iterator it;
    it._tree = this;
    it._nodeId = npos;

    return it;
  }

protected:
  int32_t allocate(const TKey& key, int32_t left, int32_t right, int32_t parent) noexcept {
    if(_size >= Capacity) {
      return npos;
    }

    const int32_t id = _free[_size++];
    _buffer[id] = Node{key, left, right, parent};

    return id;
  }

  void deallocate(int32_t id) noexcept {
    _free[--_size] = id;
  }

protected:
  int32_t _insert(const TKey& key) noexcept {
    if(_rootId == npos) {
      return (_rootId = allocate(key, npos, npos, npos));
    }

    int32_t nodeId = _rootId;

    while(true) {
      Node& node = _buffer[nodeId];

      if(_cmp(key, node._key)) {
        if(node._leftId == npos) {
          return (node._leftId = allocate(key, npos, npos, nodeId));
        }

        nodeId = node._leftId;
      } else if(_cmp(node._key, key)) {
        if(node._rightId == npos) {
          return (node._rightId = allocate(key, npos, npos, nodeId));
        }

        nodeId = node._rightId;
      } else {
        return npos;
      }
    }
  }

  int32_t _find(const TKey& key) noexcept {
    int32_t nodeId = _rootId;

    while(nodeId != npos) {
      Node& node = _buffer[nodeId];

      if(_cmp(key, node._key)) {
        nodeId = node._leftId;
      } else if(_cmp(node._key, key)) {
        nodeId = node._rightId;
      } else {
        return nodeId;
      }
    }

    return npos;
  }

  bool _erase(int32_t nodeId, const TKey& key) noexcept {
    while(nodeId != npos) {
      Node& node = _buffer[nodeId];

      if(_cmp(key, node._key)) {
        nodeId = node._leftId;
      } else if(_cmp(node._key, key)) {
        nodeId = node._rightId;
      } else {
        break;
      }
    }

    if(nodeId == npos) {
      return false;
    }

    Node& node = _buffer[nodeId];

    if(node._leftId == npos && node._rightId == npos) {
      if(nodeId == _rootId) {
        _rootId = npos;
      } else {
        Node& parent = _buffer[node._parentId];

        if(parent._leftId == nodeId) {
          parent._leftId = npos;
        } else {
          parent._rightId = npos;
        }
      }

      deallocate(nodeId);
      return true;
    }

    if(node._leftId == npos || node._rightId == npos) {
      int32_t childId = (node._leftId != npos) ? node._leftId : node._rightId;
      Node& child = _buffer[childId];
      child._parentId = node._parentId;

      if(nodeId == _rootId) {
        _rootId = childId;
      } else {
        Node& parent = _buffer[node._parentId];

        if(parent._leftId == nodeId) {
          parent._leftId = childId;
        } else {
          parent._rightId = childId;
        }
      }

      deallocate(nodeId);
      return true;
    }

    Node& left = _buffer[node._leftId];
    Node& right = _buffer[node._rightId];

    if (right._leftId == npos) {
      right._leftId = node._leftId;
      left._parentId = node._rightId;

      right._parentId = node._parentId;

      if(nodeId == _rootId) {
        _rootId = node._rightId;
      } else {
        Node& parent = _buffer[node._parentId];

        if(parent._leftId == nodeId) {
          parent._leftId = node._rightId;
        } else {
          parent._rightId = node._rightId;
        }
      }

      deallocate(nodeId);
      return true;
    } else {
      int32_t minNodeId = node._rightId;
      
      while(_buffer[minNodeId]._leftId != npos) {
        minNodeId = _buffer[minNodeId]._leftId;
      }
      
      Node& minNode = _buffer[minNodeId];

      minNode._leftId = node._leftId;
      left._parentId = minNodeId;

      _buffer[minNode._parentId]._leftId = minNode._rightId;

      if (minNode._rightId != npos) {
        _buffer[minNode._rightId]._parentId = minNode._parentId;
      }

      minNode._rightId = node._rightId;
      right._parentId = minNodeId;

      minNode._parentId = node._parentId;

      if(nodeId == _rootId) {
        _rootId = minNodeId;
      } else {
        Node& parent = _buffer[node._parentId];

        if(parent._leftId == nodeId) {
          parent._leftId = minNodeId;
        } else {
          parent._rightId = minNodeId;
        }
      }

      deallocate(nodeId);
      return true;
    }
  }

protected:
  int32_t _free[Capacity];
  int32_t _size{0};
  int32_t _rootId{npos};

  TCompare _cmp{};
  Node _buffer[Capacity];
};
