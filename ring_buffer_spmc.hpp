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

#include "storage.hpp"

template<typename TValue, std::size_t Capacity>
class RingBufferSPMC {
public:
  using value_type = TValue;

public:
  RingBufferSPMC()
    : _pushed(0)
    , _popped(0)
    , _claim(0) {
  }

  RingBufferSPMC(RingBufferSPMC&&) = delete;
  RingBufferSPMC(const RingBufferSPMC&) = delete;

  ~RingBufferSPMC() = default;

  RingBufferSPMC& operator=(RingBufferSPMC&&) = delete;
  RingBufferSPMC& operator=(const RingBufferSPMC&) = delete;

public:
  template<typename TT>
  bool push(TT&& value) {
    const std::size_t popped = _popped.load(std::memory_order_acquire);
    const std::size_t pushed = _pushed.load(std::memory_order_relaxed);

    if(_full(pushed, popped)) {
      return false;
    }

    const std::size_t index = _index(pushed);
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

  static constexpr std::size_t capacity() {
    return Capacity;
  }

  /* approximate */ bool empty_approx() const {
    const std::size_t pushed = _pushed.load(std::memory_order_acquire);
    const std::size_t popped = _popped.load(std::memory_order_acquire);
    return _empty(pushed, popped);
  }

  /* approximate */ bool full_approx() const {
    const std::size_t pushed = _pushed.load(std::memory_order_acquire);
    const std::size_t popped = _popped.load(std::memory_order_acquire);
    return _full(pushed, popped);
  }

  /* extension */ TValue pop() {
    TValue out;

    if(pop(out) == false) {
      throw std::runtime_error("Buffer is empty");
    }

    return out;
  }

private:
  /* approximate */ bool _empty(std::size_t pushed, std::size_t popped) const {
    return popped >= pushed;
  }

  /* approximate */ bool _full(std::size_t pushed, std::size_t popped) const {
    return (pushed - popped) >= Capacity;
  }

  static constexpr std::size_t _index(std::size_t i) {
    constexpr bool isPowerOf2 = ((Capacity) & (Capacity - 1)) == 0;

    if constexpr(isPowerOf2) {
      return i & (Capacity - 1);
    } else {
      return i % Capacity;
    }
  }

private:
  alignas(64) std::atomic<std::size_t> _pushed;
  alignas(64) std::atomic<std::size_t> _popped;
  alignas(64) std::atomic<std::size_t> _claim;
  alignas(64) Storage<TValue, Capacity> _buffer;
};