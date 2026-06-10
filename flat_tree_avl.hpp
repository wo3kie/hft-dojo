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
#include "flat_tree_bs.hpp"

template<typename TKey, int32_t Capacity, typename TCompare = std::less<TKey>>
struct FlatTreeAVL: FlatTreeBS<TKey, Capacity, TCompare> {
public:
  FlatTreeAVL() noexcept {
  }

  explicit FlatTreeAVL(TCompare cmp = TCompare()) noexcept
    : FlatTreeBS<TKey, Capacity, TCompare>(cmp) 
  {
  }

  int32_t insert(const TKey& key) noexcept {
    const int32_t insertedId = FlatTreeBS<TKey, Capacity, TCompare>::insert(key);

    if(insertedId != -1) {
      _rebalance(insertedId);
    }

    return insertedId;
  }

  int32_t find(const TKey& key) noexcept {
    return FlatTreeBS<TKey, Capacity, TCompare>::find(key);
  }

  TKey* get(int32_t slotId) noexcept {
    return FlatTreeBS<TKey, Capacity, TCompare>::get(slotId);
  }

  bool erase(int32_t slotId) noexcept {
    const int32_t oldSize = this->_pool.size();
    const int32_t parentId = FlatTreeBS<TKey, Capacity, TCompare>::_erase(slotId);
    const int32_t newSize = this->_pool.size();

    if(parentId != -1) {
      _rebalance(parentId);
    }
    
    return newSize != oldSize;
  }

public:
  int32_t _height(int32_t nodeId) noexcept {
    return (nodeId == -1) ? (0) : (this->_node(nodeId)._height);
  }

  void _update_height(int32_t nodeId) noexcept {
    const int32_t hl = _height(this->_node(nodeId)._leftId);
    const int32_t hr = _height(this->_node(nodeId)._rightId);
    this->_node(nodeId)._height = (hl > hr ? hl : hr) + 1;
  }

  int32_t _balance_factor(int32_t nodeId) noexcept {
    return _height(this->_node(nodeId)._leftId) - _height(this->_node(nodeId)._rightId);
  }

  void _rotate_left(int32_t nodeId) {
    const int32_t rightId = this->_node(nodeId)._rightId;
    const int32_t rightLeft = this->_node(rightId)._leftId;
    
    const int32_t parentId = this->_node(nodeId)._parentId;
    this->_node(rightId)._parentId = parentId;

    this->_node(rightId)._leftId = nodeId;
    this->_node(nodeId)._parentId = rightId;

    this->_node(nodeId)._rightId = rightLeft;
    
    if (rightLeft != -1) {
      this->_node(rightLeft)._parentId = nodeId;
    } 

    if(parentId == -1) {
      this->_rootId = rightId;
    } else if(this->_node(parentId)._leftId == nodeId) {
      this->_node(parentId)._leftId = rightId;
    } else {
      this->_node(parentId)._rightId = rightId;
    }

    _update_height(nodeId);
    _update_height(rightId);
  }

  void _rotate_right(int32_t nodeId) {
    const int32_t leftId = this->_node(nodeId)._leftId;
    const int32_t leftRight = this->_node(leftId)._rightId;
    
    const int32_t parentId = this->_node(nodeId)._parentId;
    this->_node(leftId)._parentId = parentId;

    this->_node(leftId)._rightId = nodeId;
    this->_node(nodeId)._parentId = leftId;

    this->_node(nodeId)._leftId = leftRight;
    
    if (leftRight != -1) {
      this->_node(leftRight)._parentId = nodeId;
    } 

    if(parentId == -1) {
      this->_rootId = leftId;
    } else if(this->_node(parentId)._leftId == nodeId) {
      this->_node(parentId)._leftId = leftId;
    } else {
      this->_node(parentId)._rightId = leftId;
    }

    _update_height(nodeId);
    _update_height(leftId);
  }

  void _rebalance(int32_t nodeId) noexcept {
    while(nodeId != -1) {
      const int32_t parentId = this->_node(nodeId)._parentId;

      _update_height(nodeId);
      const int32_t balance_factor = _balance_factor(nodeId);

      if(balance_factor > 1) {
        if(_balance_factor(this->_node(nodeId)._leftId) < 0) {
          _rotate_left(this->_node(nodeId)._leftId);
        }

        _rotate_right(nodeId);
      }

      if(balance_factor < -1) {
        if(_balance_factor(this->_node(nodeId)._rightId) > 0) {
          _rotate_right(this->_node(nodeId)._rightId);
        }

        _rotate_left(nodeId);
      }

      nodeId = parentId;
    }
  }
};

/*
 * Non-intrusive forward iterator for testing purposes. Not intended for production use.
 */

template<typename TKey, int32_t Capacity, typename TCompare>
struct FlatTreeAVLIterator {
  using Node = _Node<TKey>;

  using iterator_category = std::forward_iterator_tag;
  using value_type = TKey;
  using difference_type = std::ptrdiff_t;
  using pointer = TKey*;
  using reference = TKey&;

  FlatTreeAVL<TKey, Capacity, TCompare>* _tree{nullptr};
  int32_t _nodeId{-1};

  FlatTreeAVLIterator& operator++() noexcept {
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

  FlatTreeAVLIterator operator++(int) noexcept {
    FlatTreeAVLIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  TKey& operator*() noexcept {
    return _tree->_node(_nodeId)._key;
  }

  bool operator==(const FlatTreeAVLIterator& other) const noexcept {
    return _tree == other._tree && _nodeId == other._nodeId;
  }

  bool operator!=(const FlatTreeAVLIterator& other) const noexcept {
    return !(*this == other);
  }

  Node& _node(int32_t nodeId) noexcept {
    assert(nodeId != -1);
    return _tree->_node(nodeId);
  }
};

namespace std {

template<typename TKey, int32_t Capacity, typename TCompare>
::FlatTreeAVLIterator<TKey, Capacity, TCompare> begin(FlatTreeAVL<TKey, Capacity, TCompare>& tree) noexcept {
    ::FlatTreeAVLIterator<TKey, Capacity, TCompare> it;
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
::FlatTreeAVLIterator<TKey, Capacity, TCompare> end(FlatTreeAVL<TKey, Capacity, TCompare>& tree) noexcept {
    ::FlatTreeAVLIterator<TKey, Capacity, TCompare> it;
    it._tree = &tree;
    it._nodeId = -1;

    return it;
}

} // namespace std
