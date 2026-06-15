#pragma once

/*
 * Project:
 *      HFTDojo (www.github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (www.lukaszczerwinski.pl)
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

  /* approximate */ int32_t size_approx() const {
    const int32_t pushed = _pushed.load(std::memory_order_acquire);
    const int32_t popped = _popped.load(std::memory_order_acquire);

    return pushed - popped;
  }

  /* approximate */ [[nodiscard]] bool empty_approx() const {
    const int32_t pushed = _pushed.load(std::memory_order_acquire);
    const int32_t popped = _popped.load(std::memory_order_acquire);
    return _empty(pushed, popped);
  }

  /* approximate */ bool full_approx() const {
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
    int32_t claim;

    {
      claim = _claim.load(std::memory_order_acquire);

      while(true) {
        const int32_t pushed = _pushed.load(std::memory_order_acquire);

        if(claim >= pushed) {
          return false;
        }

        if(_claim.compare_exchange_weak(/* ref */ claim, claim + 1, std::memory_order_acq_rel, std::memory_order_relaxed)) {
          break;
        }
      }
    }

    {
      const int32_t index = _index(claim);
      out = std::move(_buffer[index]);
    }

    {
      while(true) {
        int32_t claim2 = claim;

        if(_popped.compare_exchange_weak(/* ref */ claim2, claim + 1, std::memory_order_release, std::memory_order_relaxed)) {
          break;
        }
      }
    }

    return true;
  }

  /* extension */ TValue _ext_pop() {
    TValue out;

    if(pop(out) == false) {
      throw std::runtime_error("Buffer is empty");
    }

    return out;
  }

private:
  /* approximate */ bool _empty(int32_t pushed, int32_t popped) const {
    return popped >= pushed;
  }

  /* approximate */ bool _full(int32_t pushed, int32_t popped) const {
    return (pushed - popped) >= Capacity;
  }

  static constexpr int32_t _index(int32_t i) {
    constexpr bool isPowerOf2 = ((Capacity) & (Capacity - 1)) == 0;

    if constexpr(isPowerOf2) {
      return i & (Capacity - 1);
    } else {
      return i % Capacity;
    }
  }

private:
  alignas(64) std::atomic<int32_t> _pushed{0};
  alignas(64) std::atomic<int32_t> _popped{0};
  alignas(64) std::atomic<int32_t> _claim{0};
  alignas(64) Storage<TValue, Capacity> _buffer;
};