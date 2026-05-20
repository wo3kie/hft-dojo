#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <string>

template<unsigned Iters, typename F>
struct LoopUnroll {
    static void run(const F& f) {
        f();

        LoopUnroll<Iters - 1, F>::run(f);
    }
};

template<typename F>
struct LoopUnroll<1, F> {
    static void run(const F& f) {
        f();
    }
};

struct Duration
{
  using duration = std::chrono::duration<long int, std::nano>;

  Duration(const duration& d)
    : _d(d)
  {
  }

  operator long int() const
  {
    return ns();
  }

  long int ns() const
  {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(_d).count();
  }

  long int us() const
  {
    return std::chrono::duration_cast<std::chrono::microseconds>(_d).count();
  }

  long int ms() const
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(_d).count();
  }

  long int s() const
  {
    return std::chrono::duration_cast<std::chrono::seconds>(_d).count();
  }

  std::string format() const
  {
    if(ns() < 1000) {
      return std::to_string(ns()) + "ns";
    } else if(us() < 1000) {
      return std::to_string(us()) + "µs";
    } else if(ms() < 1000) {
      return std::to_string(ms()) + "ms";
    } else {
      return std::to_string(s()) + "s";
    }
  }

  void log(std::function<void(long int, const std::string&)> f) const
  {
    f(ns(), format());
  }

  duration _d;
};

template<unsigned Iters = 8>
struct _Timer
{
  template<typename F>
  Duration operator()(const F& f) {
    {
      /*
       * Warmup
       */
      LoopUnroll<Iters, F>::run(f);
    }
  
    std::chrono::duration<long int, std::nano> best = std::chrono::duration<long int, std::nano>::max();

    for(int i = 0; i < Iters; ++i) {
      const auto start = std::chrono::high_resolution_clock::now();
      LoopUnroll<Iters, F>::run(f);
      const auto end = std::chrono::high_resolution_clock::now();

      best = std::min(best, end - start);
    }

    return Duration(best / Iters);

  }
};

template<unsigned Iters>
inline static _Timer<Iters> Timer;

template <unsigned Iters = 8>
struct _Cycles
{
    template<typename F>
    std::uint64_t operator()(const F& f, unsigned aux = 0u) {
        {
            /*
             * Warmup
             */
            LoopUnroll<Iters, F>::run(f);
        }

        std::uint64_t best = std::numeric_limits<std::uint64_t>::max();

        for(int i = 0; i < Iters; ++i) {
          const std::uint64_t start = __rdtscp(&aux);
          LoopUnroll<Iters, F>::run(f);
          const std::uint64_t end = __rdtscp(&aux);

          best = std::min(best, end - start);
        }

        return best / Iters;
    }
};

template<unsigned Iters>
inline static _Cycles<Iters> Cycles;
