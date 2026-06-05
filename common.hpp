#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cxxabi.h>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

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
