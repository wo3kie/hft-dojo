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
struct FlatTreeSplay: FlatTreeBS<TKey, Capacity, TCompare> {
public:
  explicit FlatTreeSplay(TCompare cmp = TCompare()) noexcept
    : FlatTreeBS<TKey, Capacity, TCompare>(cmp) {
  }

  int32_t insert(const TKey& key) noexcept {
    const int32_t insertedId = FlatTreeBS<TKey, Capacity, TCompare>::insert(key);

    if(insertedId != -1) {
      _rebalance(insertedId);
    }

    return insertedId;
  }

  int32_t find(const TKey& key) noexcept {
    const int32_t foundId = FlatTreeBS<TKey, Capacity, TCompare>::find(key);

    if(foundId != -1) {
      _rebalance(foundId);
    }

    return foundId;
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
  void _rotate_left(int32_t nodeId) {
    const int32_t rightId = this->_node(nodeId)._rightId;
    const int32_t rightLeft = this->_node(rightId)._leftId;
    const int32_t parentId = this->_node(nodeId)._parentId;

    this->_node(rightId)._parentId = parentId;
    this->_node(rightId)._leftId = nodeId;
    this->_node(nodeId)._parentId = rightId;
    this->_node(nodeId)._rightId = rightLeft;

    if(rightLeft != -1) {
      this->_node(rightLeft)._parentId = nodeId;
    }

    if(parentId == -1) {
      this->_rootId = rightId;
    } else if(this->_node(parentId)._leftId == nodeId) {
      this->_node(parentId)._leftId = rightId;
    } else {
      this->_node(parentId)._rightId = rightId;
    }
  }

  void _rotate_right(int32_t nodeId) {
    const int32_t leftId = this->_node(nodeId)._leftId;
    const int32_t leftRight = this->_node(leftId)._rightId;
    const int32_t parentId = this->_node(nodeId)._parentId;

    this->_node(leftId)._parentId = parentId;
    this->_node(leftId)._rightId = nodeId;
    this->_node(nodeId)._parentId = leftId;
    this->_node(nodeId)._leftId = leftRight;

    if(leftRight != -1) {
      this->_node(leftRight)._parentId = nodeId;
    }

    if(parentId == -1) {
      this->_rootId = leftId;
    } else if(this->_node(parentId)._leftId == nodeId) {
      this->_node(parentId)._leftId = leftId;
    } else {
      this->_node(parentId)._rightId = leftId;
    }
  }

  void _rebalance(int32_t nodeId) {
    while(true) {
      const int32_t parentId = this->_node(nodeId)._parentId;
      
      if(parentId == -1) {
        break;
      }

      const int32_t grandId = this->_node(parentId)._parentId;

      if(grandId == -1) { // Zig
        if(this->_node(parentId)._leftId == nodeId) {
          _rotate_right(parentId);
        } else {
          _rotate_left(parentId);
        }
      } else {
        const bool parentIsLeftChild = (this->_node(grandId)._leftId == parentId);
        const bool nodeIsLeftChild = (this->_node(parentId)._leftId == nodeId);

        if(parentIsLeftChild && nodeIsLeftChild) { // Zig-Zig (LL)
          _rotate_right(grandId);
          _rotate_right(parentId);
        } else if(! parentIsLeftChild && ! nodeIsLeftChild) { // Zig-Zig (RR)
          _rotate_left(grandId);
          _rotate_left(parentId);
        } else if(parentIsLeftChild && ! nodeIsLeftChild) { // Zig-Zag (LR)
          _rotate_left(parentId);
          _rotate_right(grandId);
        } else { // Zig-Zag (RL)
          _rotate_right(parentId);
          _rotate_left(grandId);
        }
      }
    }
  }
};
