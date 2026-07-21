#pragma once

/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <functional>
#include <set>

#include "common.hpp"
#include "flat_tree_bs.hpp"

template<typename TKey, int32_t Capacity, typename TCompare = std::less<TKey>>
struct FlatTreeAVL: FlatTreeBS<TKey, Capacity, TCompare> {
  static_assert((Capacity > 0) && (Capacity <= 1024 * 1024 * 1024));

public:
  explicit FlatTreeAVL(TCompare cmp = TCompare()) noexcept
    : FlatTreeBS<TKey, Capacity, TCompare>(cmp) 
  {
  }

  static constexpr int32_t capacity() noexcept {
    return Capacity;
  }

  int32_t size() const noexcept {
    return this->_pool.size();
  }

  [[nodiscard]] bool empty() const noexcept {
    return this->_pool.empty();
  }

  bool full() const noexcept {
    return this->_pool.full();
  }

  int32_t insert(const TKey& key) noexcept {
    const int32_t insertedId = FlatTreeBS<TKey, Capacity, TCompare>::insert(key);

    if (UNLIKELY(insertedId == -1)) {
      return insertedId;
    }

    if (UNLIKELY(this->_pool[insertedId]._parentId == -1)) {
      return insertedId;
    }

    _rebalance(this->_pool[insertedId]._parentId);

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
  int32_t _get_height(int32_t nodeId) noexcept {
    return (nodeId == -1) ? (0) : (this->_node(nodeId)._height);
  }

  void _update_height(int32_t nodeId) noexcept {
    const int32_t hl = _get_height(this->_node(nodeId)._leftId);
    const int32_t hr = _get_height(this->_node(nodeId)._rightId);
    this->_node(nodeId)._height = (hl > hr ? hl : hr) + 1;
  }

  int32_t _balance_factor(int32_t nodeId) noexcept {
    return _get_height(this->_node(nodeId)._leftId) - _get_height(this->_node(nodeId)._rightId);
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
    do {
      _update_height(nodeId);

      const int32_t parentId = this->_node(nodeId)._parentId;
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
    } while(nodeId != -1);
  }

  /* extension */ bool _ext_equal(const std::set<TKey>& expected) const noexcept {
    if(FlatTreeBS<TKey, Capacity, TCompare>::_ext_equal(expected) == false) {
      return false;
    }

    const auto validate = [this](auto&& self, int32_t nodeId) noexcept -> int32_t {
      if(nodeId == -1) {
        return 0;
      }

      const auto& node = this->_pool[nodeId];
      const int32_t leftHeight = self(self, node._leftId);
      const int32_t rightHeight = self(self, node._rightId);
      const int32_t expectedHeight = (leftHeight > rightHeight ? leftHeight : rightHeight) + 1;
      const int32_t balanceFactor = leftHeight - rightHeight;

      assert(node._height == expectedHeight);
      assert(balanceFactor >= -1 && balanceFactor <= 1);

      return expectedHeight;
    };

    validate(validate, this->_rootId);
    return true;
  }

  void dump() const noexcept {
    FlatTreeBS<TKey, Capacity, TCompare>::_ext_dump();
  }
};
