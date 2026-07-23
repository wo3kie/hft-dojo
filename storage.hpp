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
  using index_type = index_type_t<Capacity>;

public:
  static constexpr index_type capacity() noexcept {
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

  T& operator[](index_type index) noexcept {
    assert(index >= 0 && index < Capacity);
    return _buffer[index];
  }

  const T& operator[](index_type index) const noexcept {
    assert(index >= 0 && index < Capacity);
    return _buffer[index];
  }

private:
  std::conditional_t<Capacity <= 1024, T[Capacity], T*> _buffer;
};
