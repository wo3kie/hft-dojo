#pragma once

/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>

#ifndef NDEBUG
#include <bitset>
#endif

#include "common.hpp"
#include "storage.hpp"

template<typename T, std::size_t Capacity>
  requires std::is_trivially_destructible_v<T>
struct ObjectPool: noncopyable, nonmovable {
public:
  static_assert((Capacity > 0) && (Capacity <= 1024 * 1024 * 1024));

private:
  using index_type = index_type_t<Capacity>;

public:
  static constexpr std::size_t npos = -1;

public:
  static constexpr std::size_t capacity() noexcept {
    return Capacity;
  }

public:
  ObjectPool() {
    _size = 0;

    for(std::size_t i = 0; i < Capacity; ++i) {
      _free[i] = i;
    }
  }

  std::size_t allocate() noexcept {
    if(full()) {
      return npos;
    }

    return (std::size_t)_free[_size++];
  }

  template<typename... Args>
  std::size_t allocate(Args&&... args) noexcept {
    const std::size_t index = allocate();

    if(index != npos) {
      new(&_buffer[index]) T(std::forward<Args>(args)...);

#ifndef NDEBUG
      assert(_debug_allocated.test(index) == false);
      _debug_allocated.set(index);
#endif

    }

    return index;
  }

  void deallocate(std::size_t index) noexcept {
    assert(empty() == false);
    assert(index < Capacity);

#ifndef NDEBUG
    assert(_debug_allocated.test(index));
    _debug_allocated.reset(index);
#endif

    _free[--_size] = index;
  }

  T& operator[](std::size_t index) noexcept {
    assert(empty() == false);
    assert(index < Capacity);

#ifndef NDEBUG
    assert(_debug_allocated.test(index));
#endif

    return _buffer[index];
  }

  const T& operator[](std::size_t index) const noexcept {
    assert(empty() == false);
    assert(index < Capacity);

#ifndef NDEBUG
    assert(_debug_allocated.test(index));
#endif

    return _buffer[index];
  }

  std::size_t size() const noexcept {
    return _size;
  }

  [[nodiscard]] bool empty() const noexcept {
    return size() == 0;
  }

  bool full() const noexcept {
    return size() == Capacity;
  }

private:
  index_type _size{0};
  Storage<T, Capacity> _buffer;
  Storage<index_type, Capacity> _free;

#ifndef NDEBUG
  std::bitset<Capacity> _debug_allocated;
#endif
};
