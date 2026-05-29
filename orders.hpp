#include <cassert>
#include <cstdint>

#include "common.hpp"

/*
 * Side
 */

typedef int8_t Side;
constexpr Side Sell = -1;
constexpr Side Buy = 1;

/*
 * Order
 */

struct Order {
  static constexpr OrderId InvalidId = 0;

  static constexpr Price MinPrice = 1;
  static constexpr Price MaxPrice = 128 * 1024 * 1024;

  template<Side side>
  static constexpr Price AnyPrice = (side == Sell) ? MinPrice : MaxPrice;

  static constexpr Qty MinQty = 1;
  static constexpr Qty MaxQty = 32 * 1024 * 1024;

  OrderId id{Order::InvalidId};
  Qty qty{0};
};

/*
 * _Orders8
 */

class _Orders8 {
public:
  _Orders8() {
    for(int8_t i = 0; i < 8; i += 1) {
      _buffer[i]._value.id = 0;
      _buffer[i]._value.qty = 0;
      _buffer[i]._next = -1;
      _buffer[i]._prev = -1;
    }
  }

  bool empty() const noexcept {
    return _size == 0;
  }

  bool full() const noexcept {
    return _size == 8;
  }

  Order& front() noexcept {
    assert(_size > 0);
    return _buffer[_head]._value;
  }

  bool insert(int8_t slot, int32_t orderId, int32_t qty) noexcept {
    assert(slot >= 0 && slot < 8);

    if(_buffer[slot]._value.id != 0) {
      return false;
    }

    _buffer[slot]._value.id = orderId;
    _buffer[slot]._value.qty = qty;
    _buffer[slot]._next = -1;
    _buffer[slot]._prev = _tail;

    if(_tail == -1) {
      _head = slot;
      _tail = slot;
    } else {
      _buffer[_tail]._next = slot;
      _tail = slot;
    }

    _size += 1;
    return true;
  }

  bool update(int8_t slot, int32_t orderId, int32_t& oldQty, int32_t newQty) noexcept {
    assert(slot >= 0 && slot < 8);

    if(_buffer[slot]._value.id != orderId) {
      return false;
    }

    oldQty = _buffer[slot]._value.qty;
    _buffer[slot]._value.qty = newQty;

    return true;
  }

  bool cancel(int8_t slot, int32_t orderId, int32_t& oldQty) noexcept {
    assert(slot >= 0 && slot < 8);

    if(_buffer[slot]._value.id != orderId) {
      return false;
    }

    oldQty = _buffer[slot]._value.qty;

    const int8_t prev = _buffer[slot]._prev;
    const int8_t next = _buffer[slot]._next;

    if(prev == -1) {
      _head = next;
    } else {
      _buffer[prev]._next = next;
    }

    if(next == -1) {
      _tail = prev;
    } else {
      _buffer[next]._prev = prev;
    }

    _buffer[slot]._value.id = 0;
    _buffer[slot]._value.qty = 0;
    _buffer[slot]._next = -1;
    _buffer[slot]._prev = -1;
    _size -= 1;

    return true;
  }

  void pop() noexcept {
    assert(_size > 0);

    const int8_t slot = _head;
    const int8_t next = _buffer[slot]._next;

    _head = next;

    if(next == -1) {
      _tail = -1;
    } else {
      _buffer[next]._prev = -1;
    }

    _buffer[slot]._value.id = 0;
    _buffer[slot]._value.qty = 0;
    _buffer[slot]._next = -1;
    _buffer[slot]._prev = -1;

    _size -= 1;
  }

private:
  struct _Node {
    Order _value;
    int8_t _next;
    int8_t _prev;
  };

  int8_t _head{-1};
  int8_t _tail{-1};
  int8_t _size{0};

  _Node _buffer[8];
};

class Orders8 {
public:
  Orders8() {
  }

  bool empty() const noexcept {
    return _buffer.empty();
  }

  bool full() const noexcept {
    return _buffer.full();
  }

  Order& front() noexcept {
    return _buffer.front();
  }

  int8_t push(int8_t slot, int32_t orderId, int32_t qty) noexcept {
    if(full()) {
      return false;
    }

    while(_buffer.insert(slot, orderId, qty) == false) {
      slot = (slot + 3) & 7;
    }

    return slot;
  }

  bool update(int8_t slot, int32_t orderId, int32_t& oldQty, int32_t newQty) noexcept {
    for(int8_t iter = 0; iter < 8; iter += 1) {
      if(_buffer.update(slot, orderId, oldQty, newQty)) {
        return true;
      }

      slot = (slot + 3) & 7;
    }

    return false;
  }

  bool cancel(int8_t slot, int32_t orderId, int32_t& oldQty) noexcept {
    for(int8_t iter = 0; iter < 8; iter += 1) {
      if(_buffer.cancel(slot, orderId, oldQty)) {
        return true;
      }

      slot = (slot + 3) & 7;
    }

    return false;
  }

  void pop() noexcept {
    _buffer.pop();
  }

private:
  _Orders8 _buffer;
};
