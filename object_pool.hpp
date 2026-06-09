#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>

#include "common.hpp"

template<typename T, std::size_t MaxSize>
  requires std::is_trivially_destructible_v<T>
struct ObjectPool: noncopyable, nonmovable {
public:
  using index_t = std::size_t;

  static constexpr index_t npos = static_cast<index_t>(-1);

  ObjectPool() {
    _buffer = static_cast<T*>(::operator new(sizeof(T) * MaxSize));
    _free = new index_t[MaxSize];

    for(index_t i = 0; i < MaxSize; ++i) {
      _free[i] = i;
    }

    _size = 0;
  }

  ~ObjectPool() {
    ::operator delete(_buffer);
    delete[] _free;
  }

  template<typename... Args>
  index_t allocate(Args&&... args) noexcept {
    if(full()) {
      return npos;
    }

    const index_t slot = _free[_size++];
    new(_buffer + slot) T(std::forward<Args>(args)...);
    return slot;
  }

  void deallocate(index_t slot) noexcept {
    assert(empty() == false);
    
    _buffer[slot].~T();
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
    return _size == MaxSize;
  }

  index_t capacity() const noexcept {
    return MaxSize;
  }

  index_t size() const noexcept {
    return _size;
  }

private:
  T* _buffer{nullptr};
  index_t* _free{nullptr};
  index_t _size{0};
};
