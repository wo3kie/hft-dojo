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
