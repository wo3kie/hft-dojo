#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cassert>
#include <cstdint>

template<uint32_t Bits>
struct PriceBits final {
  static_assert(Bits >= 64);
  static_assert(Bits % 64 == 0);
  static constexpr uint32_t Chunks = Bits / 64;

public:
  PriceBits() = default;

  void set(uint32_t bit) noexcept {
    const uint32_t idx = bit >> 6;
    const uint32_t off = bit & 63;
    data[idx] |= (uint64_t(1) << off);
  }

  void clear(uint32_t bit) noexcept {
    const uint32_t idx = bit >> 6;
    const uint32_t off = bit & 63;
    data[idx] &= ~(uint64_t(1) << off);
  }

  [[nodiscard]] bool empty() const noexcept {
    for(uint32_t i = 0; i < Chunks; ++i) {
      if(data[i] != 0) {
        return false;
      }
    }

    return true;
  }

  void reset() noexcept {
    for(uint32_t i = 0; i < Chunks; ++i) {
      data[i] = 0;
    }
  }

  uint32_t clz() const noexcept {
    for(uint32_t i = Chunks; i-- > 0;) {
      const uint64_t x = data[i];

      if(x != 0) {
        const int lz = __builtin_clzll(x);
        const int bit_in_chunk = 63 - lz;
        return uint32_t(i * 64 + bit_in_chunk);
      }
    }

    assert(false);
    return 0;
  }

  uint32_t ctz() const noexcept {
    for(uint32_t i = 0; i < Chunks; ++i) {
      const uint64_t x = data[i];

      if(x != 0) {
        const int tz = __builtin_ctzll(x);
        const int bit_in_chunk = tz;
        return uint32_t(i * 64 + bit_in_chunk);
      }
    }

    assert(false);
    return 0;
  }

  template<uint8_t N>
  void shl() noexcept {
    static_assert(N <= 64);

    if constexpr(N == 0) {
      return;
    } 
    
    if constexpr(N < 64) {
      for(uint32_t i = Chunks; i-- > 1;) {
        const uint64_t lo = data[i - 1];
        const uint64_t hi = data[i];
        data[i] = (hi << N) | (lo >> (64 - N));
      }

      data[0] <<= N;
    } else { 
      for(uint32_t i = Chunks; i-- > 1;) {
        data[i] = data[i - 1];
      }

      data[0] = 0;
    }
  }

  template<uint8_t N>
  void shr() noexcept {
    static_assert(N <= 64);

    if constexpr(N == 0) {
      return;
    } 
    
    if constexpr(N < 64) {
      for(uint32_t i = 0; i < Chunks - 1; ++i) {
        const uint64_t lo = data[i];
        const uint64_t hi = data[i + 1];
        data[i] = (lo >> N) | (hi << (64 - N));
      }

      data[Chunks - 1] >>= N;
    } else {
      for(uint32_t i = 0; i < Chunks - 1; ++i) {
        data[i] = data[i + 1];
      }

      data[Chunks - 1] = 0;
    }
  }

private:
  uint64_t data[Chunks]{};
};
