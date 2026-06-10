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

template<typename T, std::size_t Capacity>
  requires std::is_trivially_destructible_v<T>
struct ObjectPool: noncopyable, nonmovable {
public:
  using index_t = std::size_t;

  static constexpr index_t npos = static_cast<index_t>(-1);

  ObjectPool() {
    _size = 0;

    if constexpr (Capacity <= 1024) {
      _buffer = {};
      _free = {};
    } else {
      _buffer = new T[Capacity];
      _free = new index_t[Capacity];
    }

    for(index_t i = 0; i < Capacity; ++i) {
      _free[i] = i;
    }
  }

  ~ObjectPool() {
    if constexpr (Capacity > 1024) {
      delete[] _buffer;
      delete[] _free;
    }
  }

  index_t allocate() noexcept {
    if(full()) {
      return npos;
    }

    return _free[_size++];
  }

  template<typename... Args>
  index_t allocate(Args&&... args) noexcept {
    index_t slot = allocate();
   
    if(slot != npos) {
      new(&_buffer[slot]) T(std::forward<Args>(args)...);
    }
   
    return slot;
  }

  void deallocate(index_t slot) noexcept {
    assert(empty() == false);
    
    _free[--_size] = slot;
  }

  T& operator[](index_t slot) noexcept {
    return _buffer[slot];
  }

  const T& operator[](index_t slot) const noexcept {
    return _buffer[slot];
  }

  bool empty() const noexcept {
    return _size == 0;
  }

  bool full() const noexcept {
    return _size == Capacity;
  }

  index_t capacity() const noexcept {
    return Capacity;
  }

  index_t size() const noexcept {
    return _size;
  }

private:
  index_t _size{0};
  std::conditional_t<Capacity <= 1024, std::array<T, Capacity>, T*> _buffer;
  std::conditional_t<Capacity <= 1024, std::array<index_t, Capacity>, index_t*> _free;
};
