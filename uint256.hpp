#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstdint>

typedef unsigned __int128 uint128_t;

struct uint256_t {
  uint128_t data[2]; // data[0] = low, data[1] = high

  uint256_t() noexcept {
    data[0] = 0;
    data[1] = 0;
  }

  uint256_t(uint128_t lo128) noexcept {
    data[0] = lo128;
    data[1] = 0;
  }

  uint256_t(uint128_t lo128, uint128_t hi128) noexcept {
    data[0] = lo128;
    data[1] = hi128;
  }

  uint256_t(const uint256_t& other) noexcept {
    data[0] = other.data[0];
    data[1] = other.data[1];
  }

  uint256_t& operator=(const uint256_t& other) noexcept {
    data[0] = other.data[0];
    data[1] = other.data[1];
    return *this;
  }

  friend bool operator==(uint256_t a, uint256_t b) noexcept {
    return (a.data[0] == b.data[0]) && (a.data[1] == b.data[1]);
  }

  friend bool operator!=(uint256_t a, uint256_t b) noexcept {
    return ! (a == b);
  }

  friend bool operator==(uint256_t a, uint64_t b) noexcept {
    return (a.data[1] == 0) && (a.data[0] == (uint128_t)b);
  }

  friend bool operator!=(uint256_t a, uint64_t b) noexcept {
    return ! (a == b);
  }

  friend bool operator==(uint256_t a, uint128_t b) noexcept {
    return (a.data[1] == 0) && (a.data[0] == b);
  }

  friend bool operator!=(uint256_t a, uint128_t b) noexcept {
    return ! (a == b);
  }

  template<uint8_t N>
  constexpr void shl() noexcept {
    static_assert(N != 0);
        
    if constexpr(N < 128) {
      const uint128_t lo = data[0];
      const uint128_t hi = data[1];

      data[1] = (hi << N) | (lo >> (128 - N));
      data[0] = lo << N;
    } else {
      data[1] = data[0] << (N - 128);
      data[0] = 0;
    }
  }

  template<uint8_t N>
  constexpr void shr() noexcept {
    static_assert(N != 0);
    
    if constexpr(N < 128) {
      const uint128_t lo = data[0];
      const uint128_t hi = data[1];

      data[0] = (lo >> N) | (hi << (128 - N));
      data[1] = hi >> N;
    } else {
      data[0] = data[1] >> (N - 128);
      data[1] = 0;
    }
  }

  uint256_t& operator<<=(uint8_t n) noexcept {
    if(n == 0) {
      return *this;
    }

    if(n < 128) {
      const uint128_t lo = data[0];
      const uint128_t hi = data[1];

      data[1] = (hi << n) | (lo >> (128 - n));
      data[0] = lo << n;
    } else {
      data[1] = data[0] << (n - 128);
      data[0] = 0;
    }

    return *this;
  }

  uint256_t& operator>>=(uint8_t n) noexcept {
    if(n == 0) {
      return *this;
    }

    if(n < 128) {
      const uint128_t lo = data[0];
      const uint128_t hi = data[1];

      data[0] = (lo >> n) | (hi << (128 - n));
      data[1] = hi >> n;
    } else {
      data[0] = data[1] >> (n - 128);
      data[1] = 0;
    }

    return *this;
  }

  friend uint256_t operator<<(uint256_t a, uint8_t n) noexcept {
    return (a <<= n);
  }

  friend uint256_t operator>>(uint256_t a, uint8_t n) noexcept {
    return (a >>= n);
  }
};

inline int ctz256(const uint256_t& x) noexcept {
  const uint64_t lo_lo = (uint64_t)x.data[0];

  if(lo_lo != 0ULL) {
    return __builtin_ctzll(lo_lo);
  }

  const uint64_t lo_hi = (uint64_t)(x.data[0] >> 64);

  if(lo_hi != 0ULL) {
    return 64 + __builtin_ctzll(lo_hi);
  }

  const uint64_t hi_lo = (uint64_t)x.data[1];

  if(hi_lo != 0ULL) {
    return 128 + __builtin_ctzll(hi_lo);
  }

  const uint64_t hi_hi = (uint64_t)(x.data[1] >> 64);

  if(hi_hi != 0ULL) {
    return 192 + __builtin_ctzll(hi_hi);
  }

  return 256;
}

inline int clz256(const uint256_t& x) noexcept {
  const uint64_t hi_hi = (uint64_t)(x.data[1] >> 64);

  if(hi_hi != 0ULL) {
    return __builtin_clzll(hi_hi);
  }

  const uint64_t hi_lo = (uint64_t)(x.data[1]);

  if(hi_lo != 0ULL) {
    return 64 + __builtin_clzll(hi_lo);
  }

  const uint64_t lo_hi = (uint64_t)(x.data[0] >> 64);

  if(lo_hi != 0ULL) {
    return 128 + __builtin_clzll(lo_hi);
  }

  const uint64_t lo_lo = (uint64_t)(x.data[0]);

  if(lo_lo != 0ULL) {
    return 192 + __builtin_clzll(lo_lo);
  }

  return 256;
}
