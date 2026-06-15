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
#include <new>
#include <type_traits>

#include "common.hpp"
#include "storage.hpp"

template<typename T, int32_t Capacity>
  requires std::is_trivially_destructible_v<T>
struct ObjectPool: noncopyable, nonmovable {
public:
  using index_type = index_type_t<Capacity>;

  static constexpr int32_t npos = -1;

  ObjectPool() {
    _size = 0;

    for(int32_t i = 0; i < Capacity; ++i) {
      _free[i] = i;
    }
  }

  ~ObjectPool() {
  }

  static constexpr int32_t capacity() noexcept {
    return Capacity;
  }

  int32_t size() const noexcept {
    return _size;
  }

  bool empty() const noexcept {
    return _size == 0;
  }  

  bool full() const noexcept {
    return _size == Capacity;
  }  

  int32_t allocate() noexcept {
    if(full()) {
      return npos;
    }

    return _free[_size++];
  }

  template<typename... Args>
  int32_t allocate(Args&&... args) noexcept {
    int32_t slot = allocate();
   
    if(slot != npos) {
      new(&_buffer[slot]) T(std::forward<Args>(args)...);
    }
   
    return slot;
  }

  void deallocate(int32_t slot) noexcept {
    assert(empty() == false);
    assert(slot >= 0);
    assert(slot < Capacity);
    
    _free[--_size] = (index_type)slot;
  }

  T& operator[](int32_t slot) noexcept {
    assert(slot >= 0);
    assert(slot < Capacity);

    return _buffer[slot];
  }

  const T& operator[](int32_t slot) const noexcept {
    assert(slot >= 0);
    assert(slot < Capacity);

    return _buffer[slot];
  }

private:
  index_type _size{0};
  Storage<T, Capacity> _buffer;
  Storage<index_type, Capacity> _free;
};
