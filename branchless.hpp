#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <concepts>

namespace bl /* branchless */
{

template<std::signed_integral T>
inline T min0(T x) noexcept
{
  bool result = x & (x >> (sizeof(T) * 8 - 1));
  return result;
}

template<std::signed_integral T>
inline T max0(T x) noexcept
{
  bool result = x & ~(x >> (sizeof(T) * 8 - 1));
  return result;
}

template<std::signed_integral T>
inline T min(T x, T y) noexcept
{
  T result = y ^ ((x ^ y) & -(x < y));
  return result;
}

template<std::unsigned_integral T>
inline T min(T x, T y) noexcept
{
  T result = y ^ ((x ^ y) & -(x < y));
  return result;
}

template<std::signed_integral T>
inline T max(T x, T y) noexcept
{
  T result = x ^ ((x ^ y) & -(x < y));
  return result;
}

template<std::unsigned_integral T>
inline T max(T x, T y) noexcept
{
  T result = x ^ ((x ^ y) & -(x < y));
  return result;
}

template<std::signed_integral T>
T abs(T x) noexcept
{
  T result = (x + (x >> (sizeof(T) * 8 - 1))) ^ (x >> (sizeof(T) * 8 - 1));
  return result;
}

template<std::signed_integral T>
bool in_range(T x, T min, T max) noexcept
{
  bool result = (x >= min) && (x <= max);
  return result;
}

template<std::unsigned_integral T>
bool in_range(T x, T min, T max) noexcept
{
  bool result = (x >= min) && (x <= max);
  return result;
}

} // namespace bl
