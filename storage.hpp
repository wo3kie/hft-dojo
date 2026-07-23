#pragma once

/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "common.hpp"

template<typename T, std::size_t Capacity>
struct Storage: noncopyable, nonmovable {
public:
  static_assert((Capacity > 0) && (Capacity <= 1024 * 1024 * 1024));

public:
  static constexpr std::size_t capacity() noexcept {
    return Capacity;
  }

public:
  Storage() {
    if constexpr(Capacity <= 1024) {
      /* empty */
    } else {
      _buffer = new T[Capacity];
    }
  }

  ~Storage() {
    if constexpr(Capacity <= 1024) {
      /* empty */
    } else {
      delete[] _buffer;
    }
  }

  T& operator[](std::size_t index) noexcept {
    assert(index < Capacity);
    return _buffer[index];
  }

  const T& operator[](std::size_t index) const noexcept {
    assert(index < Capacity);
    return _buffer[index];
  }

private:
  std::conditional_t<Capacity <= 1024, T[Capacity], T*> _buffer;
};
