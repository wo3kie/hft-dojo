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

#include "common.hpp"
#include "./ring_buffer.hpp"

/*
 * RingBufferMutex - Ring Buffer MuTex
 */

template<typename TValue, int32_t Capacity>
struct RingBufferMutex : noncopyable, nonmovable {
  static_assert((Capacity > 0) && (Capacity <= 1024 * 1024 * 1024));

public:
  using value_type = TValue;

public:
  RingBufferMutex() = default;
  ~RingBufferMutex() = default;

public:
  static constexpr std::size_t capacity() {
    return Capacity;
  }

  int32_t size() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _buffer.size();
  }

  [[nodiscard]] bool empty() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _buffer.empty();
  }

  bool full() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _buffer.full();
  }

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
