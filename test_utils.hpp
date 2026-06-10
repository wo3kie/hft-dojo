#pragma once

/*
 * Project:
 *      CxxDojo (https://github.com/wo3kie/cpp-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <set>
#include <string>
#include <sstream>
#include <unordered_map>
#include <random>
#include <vector>

#include "assert.hpp"
#include "common.hpp"
#include "object_pool.hpp"
#include "random.hpp"
#include "timer.hpp"

#ifdef NDEBUG
constexpr auto PROFILE = "Release";
#else
constexpr auto PROFILE = "Debug";
#endif

struct Request {
  uint32_t id;
  int32_t price; // {price!=0 : limit}, {price=0 : market}
  int32_t qty;   // {qty>0 : buy}, {qty<0 : sell}, {qty=0, price>0 : cancel buy}, {qty=0, price<0 : cancel sell}
};

std::string to_string(const Request& request) {
  std::ostringstream os;

  std::string type;

  if(request.price == 0 && request.qty > 0) {
    type = "Market side:tBuy";
  } else if(request.price == 0 && request.qty < 0) {
    type = "Market side:Sell";
  } else if(request.price > 0 && request.qty > 0) {
    type = "Limit side:Buy";
  } else if(request.price > 0 && request.qty < 0) {
    type = "Limit side:Sell";
  } else if(request.price < 0 && request.qty == 0) {
    type = "Cancel side:Sell";
  } else if(request.price > 0 && request.qty == 0) {
    type = "Cancel side:Buy";
  } else {
    type = "Unknown";
  }

  os << "Request: type=" << type << " id=" << request.id 
      << " price=" << std::abs(request.price) 
      << " qty=" << std::abs(request.qty);

  return os.str();
}

std::ostream& operator<<(std::ostream& os, const Request& request) {
  return os << to_string(request);
}

class RequestGenerator {
public:
  RequestGenerator(
      int32_t centerPrice, //
      int32_t levels,
      double laplaceScale,
      double marketProb,
      double cancelProb,
      double trend = 0.1,
      uint32_t seed = 12345)
    : _center(centerPrice)
    , _levels(levels)
    , _laplaceScale(laplaceScale)
    , _p_Markets(marketProb)
    , _p_Cancels(cancelProb)
    , _rng(seed)
    , _uni(0.0, 1.0)
    , _trend(trend)
    , _uniSign(-0.5, 0.5) 
  {
  }

  std::vector<Request> generate(int32_t count) {
    std::vector<Request> out(count);
    std::unordered_map<int32_t, int32_t> idPrice;
    std::unordered_map<int32_t, int32_t> idSide;

    for(int32_t i = 0; i < count; i++) {
      if (i == count / 2) {
        _trend = -_trend;
      }

      auto& e = out[i];
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
      double price = _center + int32_t(std::round(x));

      e.price = std::max(_center - _levels, std::min(price, _center + _levels));

      idPrice[e.id] = e.price;
      idSide[e.id] = (e.qty > 0) ? 1 : -1;

      _center += _trend;
    }

    return out;
  }

private:
  double _center;
  double _levels;
  double _laplaceScale;
  double _p_Markets;
  double _p_Cancels;
  double _trend;

  std::mt19937 _rng;
  std::uniform_real_distribution<double> _uni;
  std::uniform_real_distribution<double> _uniSign;
};

template<typename TContainer>
void test_flat_tree() {
  LCG lcg;

  TContainer tree;
  std::set<int>set;

  std::vector<int> values;

  for(int i = 0; i < tree.capacity(); ++i) {
    values.push_back(i);
  }

  std::shuffle(values.begin(), values.end(), lcg);
  
  for(int i = 0; i < tree.capacity(); ++i) {
    Assert(tree.insert(values[i]) != -1);
    Assert(set.insert(values[i]).second);
    Assert(tree._debug_equal(set));
  }
  
  std::shuffle(values.begin(), values.end(), lcg);

  for(int i = 0; i < tree.capacity(); ++i) {
    Assert(tree.find(values[i]) != -1);
  }
  
  std::shuffle(values.begin(), values.end(), lcg);

  for(int i = 0; i < tree.capacity(); ++i) {
    Assert(tree.erase(tree.find(values[i])));
    Assert(set.erase(values[i]) == 1);
    Assert(tree._debug_equal(set));
  }
}

template<typename TContainer>
void bench_flat_tree(int32_t iters, const std::string& label = ":") {
  int32_t events = 0;

  struct Benchmark {
    Benchmark(int32_t iters, int32_t& events)
      : _iters(iters)
      , _events(events)
    {
    }

    int32_t _iters;
    int32_t& _events;
    std::vector<Request> _requests;

    void setup() {
      RequestGenerator gen(
          /* centerPrice     */ 100,
          /* windowHalfSize  */ 124,
          /* laplaceScale b  */ 5.0,
          /* marketProb      */ 0.05,
          /* cancelProb      */ 0.25,
          /* trend           */ 0.10,
          /* seed            */ 123);

      _requests = gen.generate(_iters);
    }

    void run() {
      TContainer tree;

      for(const auto& e : _requests) {
        tree.insert(e.price);
        tree.find(e.price);
        _events += tree.size();
      }
    }

    void teardown() {
    }

  } bench(iters, events);

  Timer<1>(bench).log([iters, events, label](int ns, const std::string& msg) {
    std::cout << "Benchmark " << label << " (" << PROFILE << ")(iters=" << iters << "): " << ns / 1000000 << " ms :: " << (ns / iters)
              << " ns/iter :: " << (int)(1e9 * iters / ns) << " iter/s" << std::endl;
  });
}