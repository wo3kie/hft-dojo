#pragma once

/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstddef>
#include <stdexcept>

/*
 * RingBuffer
 */

template<typename TValue, std::size_t Capacity>
class RingBuffer
{
public:
  using value_type = TValue;

  RingBuffer() = default;
  RingBuffer(RingBuffer&&) = delete;
  RingBuffer(const RingBuffer&) = delete;

  ~RingBuffer() = default;

  RingBuffer& operator=(RingBuffer&&) = delete;
  RingBuffer& operator=(const RingBuffer&) = delete;

public:
  template<typename TT>
  bool push(TT&& t) noexcept
  {
    if(full()) {
      return false;
    }

    _buffer[_tail] = std::forward<TT>(t);
    _tail = _index(_tail + 1);

    return true;
  }

  bool pop(TValue& out) noexcept
  {
    if(empty()) {
      return false;
    }

    out = std::move(_buffer[_head]);
    _head = _index(_head + 1);

    return true;
  }

  [[nodiscard]] static constexpr std::size_t capacity() noexcept
  {
    return Capacity;
  }

  [[nodiscard]] bool empty() const noexcept
  {
    return _head == _tail;
  }

  [[nodiscard]] bool full() const noexcept
  {
    return _index(_tail + 1) == _head;
  }

  /* extension */ [[nodiscard]] TValue pop()
  {
    TValue out;

    if(! pop(out)) {
      throw std::runtime_error("RingBuffer is empty");
    }

    return out;
  }

private:
  [[nodiscard]] static constexpr std::size_t _index(std::size_t i) noexcept 
  {
    constexpr bool isPowerOf2 = ((Capacity + 1) & Capacity) == 0;

    if constexpr(isPowerOf2) {
      return i & Capacity;
    } else {
      return i % (Capacity + 1);
    }
  }

private:
  std::size_t _head{0};
  std::size_t _tail{0};

  TValue _buffer[/* N+1 trick */ Capacity + 1];
};
