#pragma once

/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <condition_variable>
#include <queue>
#include <mutex>

#include "common.hpp"
#include "./ring_buffer.hpp"

/*
 * RingBufferMutex - Ring Buffer MuTex
 */

template<typename TValue, std::size_t Capacity>
struct RingBufferMutex : noncopyable, nonmovable {
  static_assert((Capacity > 0) && (Capacity <= 1024 * 1024 * 1024));

public:
  using value_type = TValue;

public:
  static constexpr std::size_t capacity() noexcept {
    return Capacity;
  }

  std::size_t size() const noexcept {
    std::lock_guard<std::mutex> lock(_mutex);
    return _buffer.size();
  }

  [[nodiscard]] bool empty() const noexcept {
    std::lock_guard<std::mutex> lock(_mutex);
    return _buffer.empty();
  }

  bool full() const noexcept {
    std::lock_guard<std::mutex> lock(_mutex);
    return _buffer.full();
  }

  template<typename TT>
  void push(TT&& t) noexcept {
    std::unique_lock<std::mutex> lock(_mutex);

    _notFull.wait(lock, [this]() {
      return this->_buffer.full() == false;
    });

    _buffer.push(std::forward<TT>(t));
    _notEmpty.notify_one();
  }

  void pop(TValue& out) noexcept {
    std::unique_lock<std::mutex> lock(_mutex);

    _notEmpty.wait(lock, [this]() {
      return this->_buffer.empty() == false;
    });

    _buffer.pop(out);
    _notFull.notify_one();
  }

  /* extension */ TValue _ext_pop() {
    TValue out;

    {
      std::unique_lock<std::mutex> lock(_mutex);
      
      if (empty()) {
        throw std::runtime_error("RingBuffer is empty");
      }
      
      _buffer.pop(out);
      _notFull.notify_one();
    }

    return out;
  }

  /* extension */ bool _ext_equal(std::queue<TValue> expected) const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _buffer._ext_equal(expected);
  }

private:
  mutable std::mutex _mutex;
  std::condition_variable _notFull;
  std::condition_variable _notEmpty;
  RingBuffer<TValue, Capacity> _buffer;
};
