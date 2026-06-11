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

template<typename T, std::size_t Capacity>
  requires std::is_trivially_destructible_v<T>
struct ObjectPool: noncopyable, nonmovable {
public:
  static constexpr std::size_t npos = static_cast<std::size_t>(-1);

  ObjectPool() {
    _size = 0;

    for(std::size_t i = 0; i < Capacity; ++i) {
      _free[i] = i;
    }
  }

  ~ObjectPool() {
  }

  std::size_t allocate() noexcept {
    if(full()) {
      return npos;
    }

    return _free[_size++];
  }

  template<typename... Args>
  std::size_t allocate(Args&&... args) noexcept {
    std::size_t slot = allocate();
   
    if(slot != npos) {
      new(&_buffer[slot]) T(std::forward<Args>(args)...);
    }
   
    return slot;
  }

  void deallocate(std::size_t slot) noexcept {
    assert(empty() == false);
    assert(slot < Capacity);
    
    _free[--_size] = slot;
  }

  T& operator[](std::size_t slot) noexcept {
    assert(slot < Capacity);
    return _buffer[slot];
  }

  const T& operator[](std::size_t slot) const noexcept {
    assert(slot < Capacity);
    return _buffer[slot];
  }

  bool empty() const noexcept {
    return _size == 0;
  }

  bool full() const noexcept {
    return _size == Capacity;
  }

  std::size_t capacity() const noexcept {
    return Capacity;
  }

  std::size_t size() const noexcept {
    return _size;
  }

private:
  std::size_t _size{0};

  Storage<T, Capacity> _buffer;
  Storage<std::size_t, Capacity> _free;
};
