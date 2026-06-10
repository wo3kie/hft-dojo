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
#include "object_pool.hpp"

template<typename TKey>
struct _Node {
  TKey _key{};

  int32_t _leftId{-1};
  int32_t _rightId{-1};
  int32_t _parentId{-1};
  
  /* for balanced tree */
  int32_t _height{1};
};

template<typename TKey, int32_t Capacity, typename TCompare = std::less<TKey>>
struct FlatTreeBS: noncopyable, nonmovable {
  using Node = _Node<TKey>;

public:
  FlatTreeBS() noexcept {
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

  int32_t find(const TKey& key) noexcept {
    if(_rootId == -1) {
      return -1;
    }

    return _find(key);
  }

  TKey* get(int32_t slotId) noexcept {
    return (slotId == -1) ? nullptr : &_pool[slotId]._key;
  }

  bool erase(int32_t slotId) noexcept {
    const int32_t oldSize = _pool.size();
    /* parentId */ (void) _erase(slotId);
    const int32_t newSize = _pool.size();

    return newSize != oldSize;
  }  

  public:
  int32_t _insert(const TKey& key) noexcept {
    if(_rootId == -1) {
      return (_rootId = _pool.allocate(key, -1, -1, -1));
    }

    int32_t nodeId = _rootId;

    while(true) {
      Node& node = _pool[nodeId];

      if(_cmp(key, node._key)) {
        if(node._leftId == -1) {
          return (node._leftId = _pool.allocate(key, -1, -1, nodeId));
        }

        nodeId = node._leftId;
      } else if(_cmp(node._key, key)) {
        if(node._rightId == -1) {
          return (node._rightId = _pool.allocate(key, -1, -1, nodeId));
        }

        nodeId = node._rightId;
      } else {
        return -1;
      }
    }
  }

  int32_t _find(const TKey& key) noexcept {
    int32_t nodeId = _rootId;

    while(nodeId != -1) {
      Node& node = _pool[nodeId];

      if(_cmp(key, node._key)) {
        nodeId = node._leftId;
      } else if(_cmp(node._key, key)) {
        nodeId = node._rightId;
      } else {
        return nodeId;
      }
    }

    return -1;
  }
    
  int32_t _erase(int32_t nodeId) noexcept {
    if(nodeId == -1) {
      return -1;
    }

    Node& node = _pool[nodeId];

    /* case 1 */ if(node._leftId == -1 && node._rightId == -1) {
      if(nodeId == _rootId) {
        _rootId = -1;
      } else {
        Node& parent = _node(node._parentId);

        if(parent._leftId == nodeId) {
          parent._leftId = -1;
        } else {
          parent._rightId = -1;
        }
      }

      const int32_t parentId = node._parentId;
      _pool.deallocate(nodeId);
      return parentId;
    }

    /* case 2 */ if(node._leftId == -1 || node._rightId == -1) {
      int32_t childId = (node._leftId != -1) ? node._leftId : node._rightId;
      Node& child = _node(childId);
      child._parentId = node._parentId;

      if(nodeId == _rootId) {
        _rootId = childId;
      } else {
        Node& parent = _node(node._parentId);

        if(parent._leftId == nodeId) {
          parent._leftId = childId;
        } else {
          parent._rightId = childId;
        }
      }

      const int32_t parentId = node._parentId;
      _pool.deallocate(nodeId);
      return parentId;
    }

    Node& left = _node(node._leftId);
    Node& right = _node(node._rightId);

    /* case 3 */ if (right._leftId == -1) {
      right._leftId = node._leftId;
      left._parentId = node._rightId;

      right._parentId = node._parentId;

      if(nodeId == _rootId) {
        _rootId = node._rightId;
      } else {
        Node& parent = _node(node._parentId);

        if(parent._leftId == nodeId) {
          parent._leftId = node._rightId;
        } else {
          parent._rightId = node._rightId;
        }
      }

      const int32_t parentId = node._parentId;
      _pool.deallocate(nodeId);
      return parentId;
    } 
    
    /* case 4 */ /* if (right._leftId != -1) */ {
      int32_t minNodeId = node._rightId;
      
      while(_node(minNodeId)._leftId != -1) {
        minNodeId = _node(minNodeId)._leftId;
      }
      
      Node& minNode = _node(minNodeId);
      const int32_t minNodeParentId = minNode._parentId;

      minNode._leftId = node._leftId;
      left._parentId = minNodeId;

      _node(minNode._parentId)._leftId = minNode._rightId;

      if (minNode._rightId != -1) {
        _node(minNode._rightId)._parentId = minNode._parentId;
      }

      minNode._rightId = node._rightId;
      right._parentId = minNodeId;

      minNode._parentId = node._parentId;

      if(nodeId == _rootId) {
        _rootId = minNodeId;
      } else {
        Node& parent = _node(node._parentId);

        if(parent._leftId == nodeId) {
          parent._leftId = minNodeId;
        } else {
          parent._rightId = minNodeId;
        }
      }

      _pool.deallocate(nodeId);
      return minNodeParentId;
    }
  }

  Node& _node(int32_t nodeId) noexcept {
    assert(nodeId >= 0 && nodeId < Capacity);
    return _pool[nodeId];
  }

public:
  TCompare _cmp{};
  int32_t _rootId{-1};
  ObjectPool<Node, Capacity> _pool;
};

/*
 * Non-intrusive forward iterator for testing purposes. Not intended for production use.
 */

template<typename TKey, int32_t Capacity, typename TCompare>
struct FlatTreeBSIterator {
  using Node = _Node<TKey>;

  using iterator_category = std::forward_iterator_tag;
  using value_type = TKey;
  using difference_type = std::ptrdiff_t;
  using pointer = TKey*;
  using reference = TKey&;

  FlatTreeBS<TKey, Capacity, TCompare>* _tree{nullptr};
  int32_t _nodeId{-1};

  FlatTreeBSIterator& operator++() noexcept {
    if(_nodeId == -1) {
      return *this;
    }

    assert(_node(_nodeId)._leftId == -1 || _node(_node(_nodeId)._leftId)._parentId == _nodeId);
    assert(_node(_nodeId)._rightId == -1 || _node(_node(_nodeId)._rightId)._parentId == _nodeId);

    Node& node = _node(_nodeId);

    if(node._rightId != -1) {    
      _nodeId = node._rightId;

      while(_node(_nodeId)._leftId != -1) {
        _nodeId = _node(_nodeId)._leftId;
      }
    } else {
      int32_t parentId = node._parentId;

      while(parentId != -1 && _node(parentId)._rightId == _nodeId) {
        _nodeId = parentId;
        parentId = _node(parentId)._parentId;
      }

      _nodeId = parentId;
    }

    return *this;
  }

  FlatTreeBSIterator operator++(int) noexcept {
    FlatTreeBSIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  TKey& operator*() noexcept {
    return _tree->_node(_nodeId)._key;
  }

  bool operator==(const FlatTreeBSIterator& other) const noexcept {
    return _tree == other._tree && _nodeId == other._nodeId;
  }

  bool operator!=(const FlatTreeBSIterator& other) const noexcept {
    return !(*this == other);
  }

  Node& _node(int32_t nodeId) noexcept {
    assert(nodeId != -1);
    return _tree->_node(nodeId);
  }
};

namespace std {

template<typename TKey, int32_t Capacity, typename TCompare>
::FlatTreeBSIterator<TKey, Capacity, TCompare> begin(FlatTreeBS<TKey, Capacity, TCompare>& tree) noexcept {
    ::FlatTreeBSIterator<TKey, Capacity, TCompare> it;
    it._tree = &tree;
    it._nodeId = -1;

    if(tree._rootId != -1) {
      it._nodeId = tree._rootId;

      while(tree._pool[it._nodeId]._leftId != -1) {
        it._nodeId = tree._pool[it._nodeId]._leftId;
      }
    }

    return it;
}

template<typename TKey, int32_t Capacity, typename TCompare>
::FlatTreeBSIterator<TKey, Capacity, TCompare> end(FlatTreeBS<TKey, Capacity, TCompare>& tree) noexcept {
    ::FlatTreeBSIterator<TKey, Capacity, TCompare> it;
    it._tree = &tree;
    it._nodeId = -1;

    return it;
}

} // namespace std
