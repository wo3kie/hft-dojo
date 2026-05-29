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
#include <string>
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

#define LIKELY(x) (__builtin_expect(! ! (x), 1))
#define UNLIKELY(x) (__builtin_expect(! ! (x), 0))

typedef int32_t OrderId;
typedef int32_t Qty;
typedef int32_t Price;
typedef int32_t Index;

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

struct Request {
  uint32_t id;
  int32_t price; // 0 = market
  int32_t qty;   // >0 buy, <0 sell
};

class RequestGenerator {
public:
  RequestGenerator(int32_t centerPrice, int32_t levels, double laplaceScale, double marketProb, uint32_t seed = 12345)
    : _center(centerPrice)
    , _levels(levels)
    , _laplaceScale(laplaceScale)
    , _p_Markets(marketProb)
    , _rng(seed)
    , _uni(0.0, 1.0)
    , _uniSign(-0.5, 0.5) {
  }

std::vector<Request> generate(size_t count) {
  std::vector<Request> out(count);

  for(size_t i = 0; i < count; i++) {
    auto &e = out[i];
    
    e.id = uint32_t(i + 1);
    e.qty = 1 + int32_t(_uni(_rng) * 10);

    if(_uni(_rng) > 0.5) { 
      e.qty = -e.qty; 
    }

    if(_uni(_rng) < _p_Markets) {
      e.price = 0;
    } else {
      double u = _uniSign(_rng);
      double x = -_laplaceScale * ((u < 0.0) ? -1.0 : 1.0) * std::log(1.0 - 2.0 * std::abs(u));
      int32_t price = _center + int32_t(std::round(x));
      e.price = std::max(_center - _levels, std::min(price, _center + _levels));
    }
  }

  return out;
}

private:
  int32_t _center;
  int32_t _levels;
  double _laplaceScale;
  double _p_Markets;

  std::mt19937 _rng;
  std::uniform_real_distribution<double> _uni;
  std::uniform_real_distribution<double> _uniSign;
};
