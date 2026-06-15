#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <condition_variable>
#include <mutex>

#include "./ring_buffer.hpp"

/*
 * RingBufferMutex - Ring Buffer MuTex
 */

template<typename TValue, std::size_t Capacity>
class RingBufferMutex {
public:
  using value_type = TValue;

  RingBufferMutex() = default;
  RingBufferMutex(RingBufferMutex&&) = delete;
  RingBufferMutex(const RingBufferMutex&) = delete;

  ~RingBufferMutex() = default;

  RingBufferMutex& operator=(RingBufferMutex&&) = delete;
  RingBufferMutex& operator=(const RingBufferMutex&) = delete;

public:
  template<typename TT>
  bool push(TT&& t) {
    std::unique_lock<std::mutex> lock(_mutex);

    _notFull.wait(lock, [this]() {
      return ! this->_buffer.full();
    });

    _buffer.push(std::forward<TT>(t));
    _notEmpty.notify_one();

    return true;
  }

  bool pop(TValue& out) {
    std::unique_lock<std::mutex> lock(_mutex);

    _notEmpty.wait(lock, [this]() {
      return ! this->_buffer.empty();
    });

    _buffer.pop(out);
    _notFull.notify_one();

    return true;
  }

  static constexpr std::size_t capacity() {
    return Capacity;
  }

  [[nodiscard]] bool empty() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _buffer.empty();
  }

  bool full() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _buffer.full();
  }

  /* extension */ TValue _ext_pop() {
    TValue out;

    if(! pop(out)) {
      throw std::runtime_error("RingBufferMutex is empty");
    }

    return out;
  }

private:
  mutable std::mutex _mutex;

  std::condition_variable _notFull;
  std::condition_variable _notEmpty;

  RingBuffer<TValue, Capacity> _buffer;
};
