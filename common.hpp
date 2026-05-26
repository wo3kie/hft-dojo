#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstdint>
#include <limits>

#include <cstdlib>
#include <cxxabi.h>
#include <string>

inline std::string demangle(const char* mangled_name)
{
  int status = 0;
  char* demangled = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status);

  if(status != 0) {
    return mangled_name;
  }

  std::string result(demangled);
  std::free(demangled);

  return result;
}

#define LIKELY(x) (__builtin_expect(! ! (x), 1))
#define UNLIKELY(x) (__builtin_expect(! ! (x), 0))

typedef uint32_t OrderId;

typedef uint32_t Qty;

typedef uint32_t Price;
constexpr Price InvalidPrice = 0;

typedef uint32_t Index;
constexpr Index InvalidIndex = (Index)-1;

constexpr uint32_t UINT32_MIN = 0;

struct noncopyable {
    noncopyable() = default;
    ~noncopyable() = default;

    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};

struct nonmovable {
    nonmovable() = default;
    ~nonmovable() = default;

    nonmovable(nonmovable&&) = delete;
    nonmovable& operator=(nonmovable&&) = delete;
};
