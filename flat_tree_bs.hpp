#pragma once

/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <functional>
#include <set>
#include <vector>

#include "common.hpp"
#include "object_pool.hpp"

#include <iostream>

template<typename TKey, int32_t Capacity, typename TCompare = std::less<TKey>>
struct FlatTreeBS: noncopyable, nonmovable {
  using index_type = index_type_t<Capacity>;

  struct Node {
    TKey _key{};

    index_type _leftId{-1};
    index_type _rightId{-1};
    index_type _parentId{-1};

    /* for balanced tree */
    index_type _height{1};
  };

public:
  explicit FlatTreeBS(TCompare cmp = TCompare()) noexcept
    : _cmp(cmp) 
  {
  }

public:
  int32_t insert(const TKey& key) noexcept {
    return _insert(key);
  }

  int32_t find(const TKey& key) noexcept {
    if(UNLIKELY(_rootId == -1)) {
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

  int32_t size() const noexcept {
    return _pool.size();
  }

  constexpr int32_t capacity() const noexcept {
    return Capacity;
  }

  public:

  void dump(int32_t nodeId, int32_t depth) const noexcept {
    if(nodeId == -1) {
      return;
    }

    const Node& node = _pool[nodeId];

    dump(node._leftId, depth + 1);
    std::cout << std::string(depth * 2, ' ') << node._key << " (h=" << int(node._height) << ")" << std::endl;
    dump(node._rightId, depth + 1);
  }

  void dump() const noexcept {
    dump(_rootId, 0);
    std::cout << std::endl << std::endl;
  }


  int32_t _insert(const TKey& key) noexcept {
    if(UNLIKELY(_rootId == -1)) {
      return (_rootId = _pool.allocate(key, -1, -1, -1));
    }

    index_type nodeId = _rootId;

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
    index_type nodeId = _rootId;

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
      if(nodeId != _rootId) {
        Node& parent = _node(node._parentId);

        if(parent._leftId == nodeId) {
          parent._leftId = -1;
        } else {
          parent._rightId = -1;
        }
      } else {
        _rootId = -1;
      }

      const index_type result = node._parentId;
      _pool.deallocate(nodeId);
      return result;
    }

    /* case 2 */ if(node._leftId == -1 || node._rightId == -1) {
      index_type childId = (node._rightId == -1) ? node._leftId : node._rightId;
      Node& child = _node(childId);
      child._parentId = node._parentId;

      if(nodeId != _rootId) {
        Node& parent = _node(node._parentId);

        if(parent._leftId == nodeId) {
          parent._leftId = childId;
        } else {
          parent._rightId = childId;
        }
      } else {
        _rootId = childId;
      }

      const index_type result = node._parentId;
      _pool.deallocate(nodeId);
      return result;
    }

    Node& left = _node(node._leftId);
    Node& right = _node(node._rightId);

    /* case 3 */ if (right._leftId == -1) {
      right._leftId = node._leftId;
      left._parentId = node._rightId;
      right._parentId = node._parentId;

      if(nodeId != _rootId) {
        Node& parent = _node(node._parentId);

        if(parent._leftId == nodeId) {
          parent._leftId = node._rightId;
        } else {
          parent._rightId = node._rightId;
        }
      } else {
        _rootId = node._rightId;
      }

      const index_type result = node._rightId;
      _pool.deallocate(nodeId);
      return result;
    } 
    
    /* case 4 */ /* if (right._leftId != -1) */ {
      index_type minNodeId = node._rightId;
      
      while(_node(minNodeId)._leftId != -1) {
        minNodeId = _node(minNodeId)._leftId;
      }
      
      Node& minNode = _node(minNodeId);
      minNode._leftId = node._leftId;
      left._parentId = minNodeId;
      _node(minNode._parentId)._leftId = minNode._rightId;
      
      if (minNode._rightId != -1) {
        _node(minNode._rightId)._parentId = minNode._parentId;
      }
      
      minNode._rightId = node._rightId;
      right._parentId = minNodeId;
      
      const index_type minNodeParentId = minNode._parentId;
      minNode._parentId = node._parentId;

      if(nodeId != _rootId) {
        Node& parent = _node(node._parentId);

        if(parent._leftId == nodeId) {
          parent._leftId = minNodeId;
        } else {
          parent._rightId = minNodeId;
        }
      } else {
        _rootId = minNodeId;
      }

      _pool.deallocate(nodeId);
      return minNodeParentId;
    }
  }

  Node& _node(int32_t nodeId) noexcept {
    return _pool[nodeId];
  }

  /* extension */ bool _ext_equal(const std::set<TKey>& expected) const noexcept {
    std::vector<TKey> actual;

    const auto insert_keys_inorder = [this](auto&& self, int32_t nodeId, std::vector<TKey>& actual) noexcept {
      if(nodeId == -1) {
        return;
      }

      const Node& node = _pool[nodeId];

      assert((node._parentId == -1) || (_pool[node._parentId]._leftId == nodeId || _pool[node._parentId]._rightId == nodeId));
      assert((node._leftId == -1) || (_pool[node._leftId]._parentId == nodeId));
      assert((node._leftId == -1) || _cmp(_pool[node._leftId]._key, node._key));
      assert((node._rightId == -1) || (_pool[node._rightId]._parentId == nodeId));
      assert((node._rightId == -1) || _cmp(node._key, _pool[node._rightId]._key));

      self(self, node._leftId, actual);
      actual.push_back(node._key);
      self(self, node._rightId, actual);
    };

    insert_keys_inorder(insert_keys_inorder, _rootId, actual);  

    return std::equal(actual.begin(), actual.end(), expected.begin(), expected.end());
  }

  /* extension */ void _ext_dump() const noexcept {
    const auto dump = [this](auto&& self, int32_t nodeId, int32_t depth) noexcept {
      if(nodeId == -1) {
        return;
      }

      const Node& node = _pool[nodeId];

      self(self, node._rightId, depth + 1);
      std::cout << std::string(depth * 2, ' ') << node._key << std::endl;
      self(self, node._leftId, depth + 1);
    };

    dump(dump, _rootId, 0);
  }

public:
  TCompare _cmp{};
  int32_t _rootId{-1};
  ObjectPool<Node, Capacity> _pool;
};
