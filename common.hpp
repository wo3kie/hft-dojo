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

#define LIKELY(x) (__builtin_expect(! ! (x), 1))
#define UNLIKELY(x) (__builtin_expect(! ! (x), 0))

typedef int32_t OrderId;
typedef int32_t Qty;
typedef int32_t Price;
typedef int32_t Index;

typedef unsigned __int128 uint128_t;

inline int ctz128(uint128_t x) noexcept {
  const uint64_t lo = (uint64_t)x;

  if(lo != 0ULL) {
    return __builtin_ctzll(lo);
  }

  const uint64_t hi = (uint64_t)(x >> 64);

  if(hi != 0ULL) {
    return 64 + __builtin_ctzll(hi);
  }

  return 128;
}

inline int clz128(uint128_t x) noexcept {
  const uint64_t hi = (uint64_t)(x >> 64);

  if(hi != 0ULL) {
    return __builtin_clzll(hi);
  }

  const uint64_t lo = (uint64_t)x;

  if(lo != 0ULL) {
    return 64 + __builtin_clzll(lo);
  }

  return 128;
}

typedef unsigned __int128 uint128_t;

struct uint256_t {
  uint128_t data[2];
};

inline void clear(uint256_t& x) noexcept {
  x.data[0] = 0;
  x.data[1] = 0;
}

inline bool is_zero(const uint256_t& x) noexcept {
  return (x.data[0] == 0 && x.data[1] == 0);
}

inline void shift_left(uint256_t& x, uint32_t n) noexcept {
  if(n == 0) {
    return;
  }

  if(n < 128) {
    x.data[1] = (x.data[1] << n) | (x.data[0] >> (128 - n));
    x.data[0] = x.data[0] << n;
  } else if(n < 256) {
    x.data[1] = x.data[0] << (n - 128);
    x.data[0] = 0;
  } else {
    x.data[1] = 0;
    x.data[0] = 0;
  }
}

inline void shift_right(uint256_t& x, uint32_t n) noexcept {
  if(n == 0) {
    return;
  }

  if(n < 128) {
    x.data[0] = (x.data[0] >> n) | (x.data[1] << (128 - n));
    x.data[1] = x.data[1] >> n;
  } else if(n < 256) {
    x.data[0] = x.data[1] >> (n - 128);
    x.data[1] = 0;
  } else {
    x.data[1] = 0;
    x.data[0] = 0;
  }
}

inline int32_t ctz256(const uint256_t& x) noexcept {
  const int32_t lo_ctz = ctz128(x.data[0]);

  if(lo_ctz != 128) {
    return lo_ctz;
  }

  const int32_t hi_ctz = ctz128(x.data[1]);

  if(hi_ctz != 128) {
    return 128 + hi_ctz;
  }

  return 256;
}

inline int32_t clz256(const uint256_t& x) noexcept {
  const int32_t hi_clz = clz128(x.data[1]);

  if(hi_clz != 128) {
    return hi_clz;
  }

  const int32_t lo_clz = clz128(x.data[0]);

  if(lo_clz != 128) {
    return 128 + lo_clz;
  }

  return 256;
}

void set_price_bit(uint256_t& mask, uint8_t price) noexcept {
  mask.data[price >> 7] |= (uint128_t)1 << (price & 127);
}

void clear_price_bit(uint256_t& mask, uint8_t price) noexcept {
  mask.data[price >> 7] &= ~((uint128_t)1 << (price & 127));
}

uint8_t get_price_bit(uint256_t mask) noexcept {
  assert(is_zero(mask) == false);
  return 256 - 1 - clz256(mask);
}

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

std::string to_string(const Request& request) {
  std::ostringstream os;

  std::string type;

  if (request.price == 0 && request.qty > 0) {
    type = "Market side:tBuy";
  } else if (request.price == 0 && request.qty < 0) {
    type = "Market side:Sell";
  } else if (request.price > 0 && request.qty > 0) {
    type = "Limit side:Buy";
  } else if (request.price > 0 && request.qty < 0) {
    type = "Limit side:Sell";
  } else if (request.price < 0 && request.qty == 0) {
    type = "Cancel side:Sell";
  } else if (request.price > 0 && request.qty == 0) {
    type = "Cancel side:Buy";
  } else {
    type = "Unknown";
  }

  os << "Request: type=" << type << " id=" << request.id << " price=" << std::abs(request.price) << " qty=" << std::abs(request.qty);
  return os.str();
}

std::ostream& operator<<(std::ostream& os, const Request& request) {
  return os << to_string(request);
}

class RequestGenerator {
public:
  RequestGenerator(int32_t centerPrice, int32_t levels, double laplaceScale, double marketProb, double cancelProb, uint32_t seed = 12345)
    : _center(centerPrice)
    , _levels(levels)
    , _laplaceScale(laplaceScale)
    , _p_Markets(marketProb)
    , _p_Cancels(cancelProb)
    , _rng(seed)
    , _uni(0.0, 1.0)
    , _uniSign(-0.5, 0.5)
  {
  }

std::vector<Request> generate(int32_t count) {
  std::vector<Request> out(count);
  std::unordered_map<int32_t, int32_t> idPrice;
  std::unordered_map<int32_t, int32_t> idSide;

  for(int32_t i = 0; i < count; i++) {
    auto &e = out[i];
    e.id = int32_t(i + 1);
    double u = _uni(_rng);

    if(u < _p_Cancels) {
      e.id = std::max(1, i - int32_t(_uni(_rng) * 20));
      e.price = idPrice[e.id] * idSide[e.id];
      e.qty = 0;
      continue;
    }

    if(u < _p_Cancels + _p_Markets) {
      e.price = 0;
      e.qty = 1 + int32_t(_uni(_rng) * 10);

      if(_uni(_rng) > 0.5) {
        e.qty = -e.qty;
      }
      
      continue;
    }

    e.qty = 1 + int32_t(_uni(_rng) * 10);

    if(_uni(_rng) > 0.5) {
      e.qty = -e.qty;
    }

    double s = _uniSign(_rng);
    double x = -_laplaceScale * ((s < 0.0) ? -1.0 : 1.0) * std::log(1.0 - 2.0 * std::abs(s));
    int32_t price = _center + int32_t(std::round(x));

    e.price = std::max(_center - _levels, std::min(price, _center + _levels));

    idPrice[e.id] = e.price;
    idSide[e.id] = (e.qty > 0) ? 1 : -1;
  }

  return out;
}


private:
  int32_t _center;
  int32_t _levels;
  double _laplaceScale;
  double _p_Markets;
  double _p_Cancels;

  std::mt19937 _rng;
  std::uniform_real_distribution<double> _uni;
  std::uniform_real_distribution<double> _uniSign;
};
