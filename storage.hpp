#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstddef>
#include <cassert>
#include <cstdint>
#include <type_traits>

#include "common.hpp"

template<typename T, std::size_t Capacity>
struct Storage: noncopyable, nonmovable {
public:
  Storage() {
    if constexpr (Capacity <= 1024) {
      _buffer;
    } else {
      _buffer = new T[Capacity];
    }
  }

  ~Storage() {
    if constexpr (Capacity > 1024) {
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

  std::size_t capacity() const noexcept {
    return Capacity;
  }

private:
  std::conditional_t<Capacity <= 1024, T[Capacity], T*> _buffer;
};
