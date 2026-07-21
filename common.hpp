#pragma once

/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cxxabi.h>
#include <limits>
#include <string>

/*
 * index_type_t
 */

template<std::size_t Capacity>
struct index_type {
    using type =
        std::conditional_t<(Capacity < (1ULL << 7)),  int8_t,
          std::conditional_t<(Capacity < (1ULL << 15)), int16_t,
            std::conditional_t<(Capacity < (1ULL << 31)), int32_t,
                                                            int64_t>>>;
};


template<std::size_t Capacity>
using index_type_t = typename index_type<Capacity>::type;

/*
 * demangle
 */

inline std::string demangle(const char* mangled_name) {
  int status = 0;
  char* demangled = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status);

  if(status != 0) {
    return mangled_name;
  }

  std::string result(demangled);
  std::free(demangled);

  return result;
}

/*
 * Branch prediction hints
 */

#define LIKELY(x) (__builtin_expect(! ! (x), 1))
#define UNLIKELY(x) (__builtin_expect(! ! (x), 0))

/*
 * types
 */

typedef int32_t OrderId;
typedef int32_t Qty;
typedef int32_t Price;
typedef int32_t Index;

/*
 * Side
 */

typedef int8_t Side;
constexpr Side Sell = -1;
constexpr Side Buy = 1;

/*
 * Slot
 */

typedef bool Slot;
constexpr Slot NoSlot = false;
constexpr Slot HasSlot = true;

/*
 * Constants
 */

static constexpr Price MinPrice = 1;
static constexpr Price MaxPrice = 128 * 1024 * 1024;

static constexpr Qty MinQty = 1;
static constexpr Qty MaxQty = 64 * 1024 * 1024;

static constexpr Index InvalidOrderId = 0;

/*
 * noncopyable
 */

struct noncopyable {
  noncopyable() = default;
  ~noncopyable() = default;

  noncopyable(const noncopyable&) = delete;
  noncopyable& operator=(const noncopyable&) = delete;
};

/*
 * nonmovable
 */

struct nonmovable {
  nonmovable() = default;
  ~nonmovable() = default;

  nonmovable(nonmovable&&) = delete;
  nonmovable& operator=(nonmovable&&) = delete;
};

// std::hardware_destructive_interference_size
inline constexpr std::size_t CacheLineSize = 64;
