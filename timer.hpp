#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <string>

#include <x86intrin.h>

struct Duration {
  using duration = std::chrono::duration<long int, std::nano>;

  Duration(const duration& d)
    : _d(d) {
  }

  operator long int() const {
    return ns();
  }

  long int ns() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(_d).count();
  }

  long int us() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(_d).count();
  }

  long int ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(_d).count();
  }

  long int s() const {
    return std::chrono::duration_cast<std::chrono::seconds>(_d).count();
  }

  std::string format() const {
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

  void log(std::function<void(long int, const std::string&)> f) const {
    f(ns(), format());
  }

  duration _d;
};

template<unsigned Iters = 8>
struct _Timer {
  template<typename F>
  Duration operator()(F& f) {
    {
      /*
          * Warm up
          */

      for(int i = 1; i < Iters; ++i) {
        f.setup();
        f.run();
        f.teardown();
      }
    }

    std::chrono::duration<long int, std::nano> best = std::chrono::duration<long int, std::nano>::max();

    for(int i = 0; i < Iters; ++i) {
      for(int j = 0; j < Iters; ++j) {
        f.setup();

        const auto start = std::chrono::high_resolution_clock::now();
        asm volatile("" ::: "memory");

        f.run();

        asm volatile("" ::: "memory");
        const auto end = std::chrono::high_resolution_clock::now();

        f.teardown();
        best = std::min(best, end - start);
      }
    }

    return Duration(best);
  }
};

template<unsigned Iters>
inline static _Timer<Iters> Timer;

template<unsigned Iters = 32>
struct _Cycles {
  template<typename F>
  std::uint64_t operator()(F& f, unsigned aux = 0u) {
    {
      /*
       * Warm up
       */

      for(int i = 1; i < Iters; ++i) {
        f.setup();
        f.run();
        f.teardown();
      }
    }

    std::uint64_t best = std::numeric_limits<std::uint64_t>::max();

    for(int i = 0; i < Iters; ++i) {
      f.setup();

      const std::uint64_t start = __rdtscp(&aux);
      asm volatile("" ::: "memory");

      f.run();

      asm volatile("" ::: "memory");
      const std::uint64_t end = __rdtscp(&aux);

      f.teardown();
      best = std::min(best, end - start);
    }

    return best;
  }
};

template<unsigned Iters>
inline static _Cycles<Iters> Cycles;
