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
inline T min(T x, T y)
{
  T result = y ^ ((x ^ y) & -(x < y));
  return result;
}

template<std::unsigned_integral T>
inline T min(T x, T y)
{
  T result = y ^ ((x ^ y) & -(x < y));
  return result;
}

template<std::signed_integral T>
inline T max(T x, T y)
{
  T result = x ^ ((x ^ y) & -(x < y));
  return result;
}

template<std::unsigned_integral T>
inline T max(T x, T y)
{
  T result = x ^ ((x ^ y) & -(x < y));
  return result;
}

template<std::signed_integral T>
bool in_range(T x, T min, T max)
{
  bool result = (x >= min) && (x <= max);
  return result;
}

template<std::unsigned_integral T>
bool in_range(T x, T min, T max)
{
  bool result = (x >= min) && (x <= max);
  return result;
}

} // namespace bl
