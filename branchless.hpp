#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <concepts>

namespace bl /* branchless */ {

template<std::signed_integral T>
inline T min0(T x) noexcept {
    return x & (x >> (sizeof(T)*8 - 1));
}

template<std::signed_integral T>
inline T max0(T x) noexcept {
    return x & ~(x >> (sizeof(T)*8 - 1));
}

template<std::signed_integral T>
inline T min(T x, T y) noexcept {
    return y ^ ((x ^ y) & -(x < y));
}

template<std::signed_integral T>
inline T max(T x, T y) noexcept {
    return x ^ ((x ^ y) & -(x < y));
}

template<std::signed_integral T>
T abs(T x) noexcept {
  return (x + (x >> (sizeof(T) * 8 - 1))) ^ (x >> (sizeof(T) * 8 - 1));
}

template<std::signed_integral T>
bool in_range(T x, T min, T max) noexcept {
    return (x >= min) && (x <= max);
}

} // namespace bl
