#pragma once

/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <utility>

#include "common.hpp"
#include "storage.hpp"

template<typename TValue, int32_t Capacity>
struct RingBufferSPMC : noncopyable, nonmovable {
  static_assert((Capacity > 0) && (Capacity <= 1024 * 1024 * 1024));

public:
  using value_type = TValue;

public:
  RingBufferSPMC() = default;
  ~RingBufferSPMC() = default;

public:
  static constexpr int32_t capacity() {
    return Capacity;
  }

  /* approximate */ int32_t _approx_size() const {
    const int32_t pushed = _pushed.load(std::memory_order_acquire);
    const int32_t popped = _popped.load(std::memory_order_acquire);

    return pushed - popped;
  }

  /* approximate */ [[nodiscard]] bool _approx_empty() const {
    const int32_t pushed = _pushed.load(std::memory_order_acquire);
    const int32_t popped = _popped.load(std::memory_order_acquire);
    return _empty(pushed, popped);
  }

  /* approximate */ bool _approx_full() const {
    const int32_t pushed = _pushed.load(std::memory_order_acquire);
    const int32_t popped = _popped.load(std::memory_order_acquire);
    return _full(pushed, popped);
  }

  template<typename TT>
  bool push(TT&& value) {
    const int32_t popped = _popped.load(std::memory_order_acquire);
    const int32_t pushed = _pushed.load(std::memory_order_relaxed);

    if(_full(pushed, popped)) {
      return false;
    }

    const int32_t index = _index(pushed);
    _buffer[index] = std::forward<TT>(value);

    _pushed.store(pushed + 1, std::memory_order_release);
    return true;
  }

  bool pop(TValue& out) {
    std::size_t claim;

    {
      claim = _claim.load(std::memory_order_acquire);

      while(true) {
        const std::size_t pushed = _pushed.load(std::memory_order_acquire);

        if(claim >= pushed) {
          return false;
        }

        if(_claim.compare_exchange_weak(/* ref */ claim, claim + 1, std::memory_order_acq_rel, std::memory_order_relaxed)) {
          break;
        }
      }
    }

    {
      const std::size_t index = _index(claim);
      out = std::move(_buffer[index]);
    }

    {
      while(true) {
        std::size_t claim2 = claim;

        if(_popped.compare_exchange_weak(/* ref */ claim2, claim + 1, std::memory_order_release, std::memory_order_relaxed)) {
          break;
        }
      }
    }

    return true;
  }

  /* approximate */ bool _approx_empty(std::size_t pushed, std::size_t popped) const {
    return popped >= pushed;
  }
  
  /* approximate */ bool _approx_full(std::size_t pushed, std::size_t popped) const {
    return (pushed - popped) >= Capacity;
  }

  /* extension */ TValue _ext_pop() {
    TValue out;

    if(pop(out) == false) {
      throw std::runtime_error("Buffer is empty");
    }

    return out;
  }

  /* extension */ bool _ext_equal(std::queue<TValue> expected) const {
    const std::size_t popped = _popped.load(std::memory_order_acquire);
    const std::size_t pushed = _pushed.load(std::memory_order_acquire);

    for(std::size_t i = popped; i < pushed; i += 1) {
      const std::size_t index = _index(i);
      
      if(_buffer[index] != expected.front()) {
        return false;
      }

      expected.pop();
    }

    return expected.empty();
  }

private:
  static constexpr std::size_t _index(std::size_t i) noexcept {
    constexpr bool isPowerOf2 = ((Capacity) & (Capacity - 1)) == 0;

    if constexpr(isPowerOf2) {
      return i & (Capacity - 1);
    } else {
      // return i % (Capacity + 1);
      return (i >= Capacity ? 0 : i);
    }
  }

private:
  alignas(CacheLineSize) std::atomic<std::size_t> _pushed{0};
  alignas(CacheLineSize) std::atomic<std::size_t> _popped{0};
  alignas(CacheLineSize) std::atomic<std::size_t> _claim{0};
  alignas(CacheLineSize) Storage<TValue, Capacity> _buffer;
};