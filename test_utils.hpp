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
#include <iostream>
#include <iomanip>
#include <list>
#include <set>
#include <string>
#include <sstream>
#include <unordered_map>
#include <random>
#include <ranges>
#include <utility>
#include <vector>

#include "common.hpp"
#include "object_pool.hpp"
#include "random.hpp"
#include "timer.hpp"

#ifdef NDEBUG
constexpr auto PROFILE = "Release";
#else
constexpr auto PROFILE = "Debug";
#endif

template <typename T>
inline void do_not_optimize(const T& value) {
    asm volatile("" : : "g"(value) : "memory");
}

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
    assert(tree.insert(values[i]) != -1);
    assert(set.insert(values[i]).second);
    assert(tree._ext_equal(set));
  }
  
  std::shuffle(values.begin(), values.end(), lcg);

  for(int i = 0; i < tree.capacity(); ++i) {
    assert(tree.find(values[i]) != -1);
  }
  
  std::shuffle(values.begin(), values.end(), lcg);

  for(int i = 0; i < tree.capacity(); ++i) {
    assert(tree.erase(tree.find(values[i])));
    assert(set.erase(values[i]) == 1);
    assert(tree._ext_equal(set));
  }
}



template<typename TContainer>
void bench_ring_buffer(int32_t iters, const std::string& label = ":") {
   
  struct Benchmark {
    Benchmark(int32_t iters)
      : _iters(iters)
    {
    }

    LCG _lcg;
    int32_t _iters;
    TContainer _buffer;
    std::vector<int> _values;
    
    void setup() {
      for(int i = 0; i < _iters; ++i) {
        _values.push_back(_lcg());
      }      
    }
    
    void run() {
      volatile int32_t no_opt = 0;

      for(const auto v : _values) {
        no_opt += _buffer.push(v);
        no_opt += _buffer._ext_pop();
      }

      do_not_optimize(no_opt);
    }

    void teardown() {
    }
  } bench(iters);

  Timer<1>(bench).log([iters, label](int ns, const std::string& msg) {
    std::cout << "Benchmark (" << PROFILE << "): "
              << label << ": "
              << "(iters=" << iters << "): " << ns/1000 << " μs :: " << (ns / iters)
              << " ns/iter :: " << (int)(1e9 * iters / ns) << " iter/s" << std::endl;
  });
}

template<typename TContainer>
void bench_flat_queue(const std::string& label = ":") {
   
  struct Benchmark {
    Benchmark()
    {
    }

    LCG _lcg;
    TContainer _queue;
    std::vector<int> _values;
    
    void setup() {
      for(int i = 0; i < _queue.capacity(); ++i) {
        _values.push_back(_lcg());
      }      
    }
    
    void run() {
      volatile int32_t no_opt = 0;

      for(const auto& [i, v] : std::ranges::views::enumerate(_values)) {
        _queue.push(i, v);
        no_opt += _queue.front();
        _queue.pop();
        no_opt += (_queue.empty()) ? (0) : (_queue.front());
      }

      do_not_optimize(no_opt);
    }

    void teardown() {
      while(! _queue.empty()) {
        _queue.pop();
      }
    }
  } bench;

  Timer<1>(bench).log([iters = TContainer::capacity(), label](int ns, const std::string& msg) {
    std::cout << "Benchmark (" << PROFILE << "): "
              << label << ": "
              << "(iters=" << iters << "): " << ns/1000 << " μs :: " << (ns / iters)
              << " ns/iter :: " << (int)(1e9 * iters / ns) << " iter/s" << std::endl;
  });
}

template<typename TContainer>
void bench_flat_tree(int32_t iters, const std::string& label = ":") {

  struct Benchmark {
    Benchmark(int32_t iters)
      : _iters(iters)
    {
    }

    int32_t _iters;
    TContainer _tree;
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
      volatile int32_t no_opt = 0;

      for(const auto& e : _requests) {
        _tree.insert(e.price);
        no_opt += _tree.size();
        _tree.find(e.price);
        no_opt += _tree.size();
      }

      do_not_optimize(no_opt);
    }

    void teardown() {
      for(const auto& e : _requests) {
        _tree.erase(_tree.find(e.price));
      }
    }

  } bench(iters);

  Timer<1>(bench).log([iters, label](int ns, const std::string& msg) {
    std::cout << "Benchmark (" << PROFILE << "): "
              << label << ": "
              << "(iters=" << iters << "): " << ns/1000 << " μs :: " << (ns / iters)
              << " ns/iter :: " << (int)(1e9 * iters / ns) << " iter/s" << std::endl;
  });
}