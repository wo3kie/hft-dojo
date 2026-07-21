#pragma once

/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
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
  static_assert((Capacity > 0) && (Capacity <= 1024 * 1024 * 1024));

  using index_type = index_type_t<Capacity>;

  static constexpr int32_t npos = -1;

  ObjectPool() {
    _size = 0;

    for(index_type i = 0; i < (index_type)Capacity; ++i) {
      _free[i] = i;
    }
  }

  ~ObjectPool() {
  }

  static constexpr int32_t capacity() noexcept {
    return Capacity;
  }

  int32_t size() const noexcept {
    return (int32_t)_size;
  }

  [[nodiscard]] bool empty() const noexcept {
    return _size == 0;
  }  

  bool full() const noexcept {
    return _size == (index_type)Capacity;
  }  

  int32_t allocate() noexcept {
    if(full()) {
      return (int32_t)npos;
    }

    return (int32_t)_free[_size++];
  }

  template<typename... Args>
  int32_t allocate(Args&&... args) noexcept {
    const int32_t slot = allocate();
   
    if(slot != npos) {
      new(&_buffer[slot]) T(std::forward<Args>(args)...);
    }
   
    return slot;
  }

  void deallocate(int32_t slot) noexcept {
    assert(empty() == false);
    assert((slot >= 0) && (slot < Capacity));
    
    _free[--_size] = (index_type)slot;
  }
  
  T& operator[](int32_t slot) noexcept {
    assert(empty() == false);
    assert((slot >= 0) && (slot < Capacity));
    
    return _buffer[slot];
  }
  
  const T& operator[](int32_t slot) const noexcept {
    assert(empty() == false);
    assert((slot >= 0) && (slot < Capacity));

    return _buffer[slot];
  }

private:
  index_type _size{0};
  Storage<T, Capacity> _buffer;
  Storage<index_type, Capacity> _free;
};
