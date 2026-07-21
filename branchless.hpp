#pragma once

/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <concepts>

namespace bl /* branchless */
{

template<std::signed_integral T>
inline T min(T x, T y) noexcept {
  const T diff = x - y;
  const T mask = diff >> (sizeof(T) * 8 - 1);
  return y + (diff & mask);
}

template<std::signed_integral T>
inline T max(T x, T y) noexcept {
  const T diff = x - y;
  const T mask = diff >> (sizeof(T) * 8 - 1);
  return x - (diff & mask);
}

template<std::signed_integral T>
inline bool in_range(T x, T lo, T hi) noexcept {
  return (x >= lo) && (x <= hi);
}

template<std::signed_integral T>
inline T zero_if_true(T x, bool cond) noexcept {
    return x & (T(cond) - 1);
}

template<std::signed_integral T>
inline T one_if_true(T x, bool cond) noexcept {
    const T mask = -T(cond);
    return (x & ~mask) | (mask & 1);
}

template<std::signed_integral T>
inline T add_if_true(T x, T v, bool cond) noexcept {
    return x + (v & -T(cond));
}

template<std::signed_integral T>
inline T sub_if_true(T x, T v, bool cond) noexcept {
    return x - (v & -T(cond));
}

template<std::signed_integral T>
inline T inc_if_true(T x, bool cond) noexcept {
    return add_if_true(x, T(1), cond);
}

template<std::signed_integral T>
inline T dec_if_true(T x, bool cond) noexcept {
    return sub_if_true(x, T(1), cond);
}

} // namespace bl