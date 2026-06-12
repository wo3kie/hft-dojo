#pragma once

/*
 * Project:
 *      CxxDojo (https://github.com/wo3kie/cpp-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstdint>
#include <type_traits>

#include "assert.hpp"
#include "common.hpp"
#include "storage.hpp"

template<typename TKey, typename TValue, int32_t Capacity>
  requires(std::is_integral_v<TKey>)
struct FlatHash: noncopyable, nonmovable {
  static_assert((Capacity & (Capacity - 1)) == 0);

  static constexpr int32_t Mask = Capacity - 1;
  static constexpr int32_t Step = (Capacity / 2) - 1;
  static constexpr int32_t Bits = Capacity / 8;

public:
  FlatHash() {
    for(int32_t i = 0; i < Bits; ++i) {
      _freeBits[i] = 0xFF;
      _tombBits[i] = 0x00;
    }
  }

  ~FlatHash() {
  }

  static inline uint8_t _get_bit(const Storage<uint8_t, Bits>& bits, int32_t idx) noexcept {
    return (bits[idx >> 3] >> (idx & 7)) & 1;
  }

  static inline void _set_bit(Storage<uint8_t, Bits>& bits, int32_t idx) noexcept {
    bits[idx >> 3] |= uint8_t(1u << (idx & 7));
  }

  static inline void _clear_bit(Storage<uint8_t, Bits>& bits, int32_t idx) noexcept {
    bits[idx >> 3] &= uint8_t(~(1u << (idx & 7)));
  }

  int32_t insert(const std::pair<TKey, TValue>& kv) noexcept {
    return insert(kv.first, kv.second);
  }

  int32_t insert(TKey key, const TValue& value) noexcept {
    int32_t idx = key & Mask;
    int32_t firstTomb = -1;

    for(int32_t iter = 0; iter < Capacity; ++iter) {
      const uint8_t f = _get_bit(_freeBits, idx);
      const uint8_t t = _get_bit(_tombBits, idx);

      const uint8_t isFree = f;
      const uint8_t isTomb = t;
      const uint8_t isUsed = (~f & ~t) & 1;

      if(isUsed && _keys[idx] == key) {
        _values[idx] = value;
        return idx;
      }

      if(isTomb && firstTomb == -1) {
        firstTomb = idx;
      }

      if(isFree) {
        const int32_t target = (firstTomb != -1) ? firstTomb : idx;

        _keys[target] = key;
        _values[target] = value;

        _clear_bit(_freeBits, target);
        _clear_bit(_tombBits, target);

        return target;
      }

      idx = (idx + Step) & Mask;
    }

    return -1;
  }

  int32_t find(TKey key) const noexcept {
    int32_t idx = key & Mask;

    for(int32_t iter = 0; iter < Capacity; ++iter) {
      const uint8_t f = _get_bit(_freeBits, idx);
      const uint8_t t = _get_bit(_tombBits, idx);

      const uint8_t isFree = f;
      const uint8_t isUsed = (~f & ~t) & 1;

      if(isUsed && _keys[idx] == key) {
        return idx;
      }

      if(isFree) {
        return -1;
      }

      idx = (idx + Step) & Mask;
    }

    return -1;
  }

  bool contains(TKey key) const noexcept {
    return find(key) != -1;
  }

  TValue* get(int32_t idx) noexcept {
    return (idx >= 0 && idx < Capacity) ? &_values[idx] : nullptr;
  }

  bool erase(TKey key) noexcept {
    const int32_t idx = find(key);

    if(idx == -1) {
      return false;
    }

    _set_bit(_freeBits, idx);
    _set_bit(_tombBits, idx);

    return true;
  }

private:
  Storage<TKey, Capacity> _keys;
  Storage<TValue, Capacity> _values;
  Storage<uint8_t, Bits> _freeBits;
  Storage<uint8_t, Bits> _tombBits;
};
