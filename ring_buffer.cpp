/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 *
 */

#include <stdexcept>

#include "assert.hpp"
#include "common.hpp"

#include "ring_buffer.hpp"
#include "test_utils.hpp"

namespace {

struct MoveOnly {
  int value = 0;

  MoveOnly() = default;
  explicit MoveOnly(int value): value(value) {
  }

  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;

  MoveOnly(MoveOnly&& other) noexcept: value(other.value) {
    other.value = -1;
  }

  MoveOnly& operator=(MoveOnly&& other) noexcept {
    if(this != &other) {
      value = other.value;
      other.value = -1;
    }

    return *this;
  }
};

template<template<typename, std::size_t> class TBuffer>
void test_ring_buffer_gdb() {
  TBuffer<int, 128> rBuffer;

  for(int i = 0; i < 128; ++i) {
    rBuffer.push(i);
  }

  /*
   * (gdb) source ../gdb_utils.py
   *
   * (gdb) print_ring_buffer rBuffer
   * RingBuffer<int, 128> [ 0, 1, 2, 3, ..., 124, 125, 126, 127 ]
   * 
   * (gdb) print_ring_buffer_spsc rBuffer
   * RingBufferSPSC<int, 128> [ 0, 1, 2, 3, ..., 124, 125, 126, 127 ]
   * 
   * (gdb) print_ring_buffer_spmc rBuffer
   * RingBufferSPMC<int, 128> [ 0, 1, 2, 3, ..., 124, 125, 126, 127 ]
   */

  Assert(rBuffer.capacity() == 128);
}

template<std::size_t Capacity>
void test_empty_full_and_fifo() {
  RingBuffer<int, Capacity> buffer;

  Assert(buffer.capacity() == Capacity);
  Assert(buffer.empty() == true);
  Assert(buffer.full() == false);

  int out = -1;
  Assert(buffer.pop(out) == false);

  for(std::size_t i = 0; i < Capacity; ++i) {
    Assert(buffer.push(static_cast<int>(i + 10)) == true);
  }

  Assert(buffer.empty() == false);
  Assert(buffer.full() == true);
  Assert(buffer.push(999) == false);

  for(std::size_t i = 0; i < Capacity; ++i) {
    Assert(buffer.pop(out) == true);
    Assert(out == static_cast<int>(i + 10));
  }

  Assert(buffer.empty() == true);
  Assert(buffer.full() == false);
  Assert(buffer.pop(out) == false);
}

template<std::size_t Capacity>
void test_wrap_around() {
  RingBuffer<int, Capacity> buffer;
  int out = -1;

  for(std::size_t i = 0; i < Capacity; ++i) {
    Assert(buffer.push(static_cast<int>(i)) == true);
  }

  for(std::size_t i = 0; i < Capacity / 2; ++i) {
    Assert(buffer.pop(out) == true);
    Assert(out == static_cast<int>(i));
  }

  for(std::size_t i = 0; i < Capacity / 2; ++i) {
    Assert(buffer.push(static_cast<int>(100 + i)) == true);
  }

  for(std::size_t i = Capacity / 2; i < Capacity; ++i) {
    Assert(buffer.pop(out) == true);
    Assert(out == static_cast<int>(i));
  }

  for(std::size_t i = 0; i < Capacity / 2; ++i) {
    Assert(buffer.pop(out) == true);
    Assert(out == static_cast<int>(100 + i));
  }

  Assert(buffer.empty() == true);
}

void test_ext_pop_throws_on_empty() {
  RingBuffer<int, 4> buffer;

  bool thrown = false;

  try {
    (void)buffer._ext_pop();
  } catch(const std::runtime_error& error) {
    thrown = true;
    Assert(std::string_view(error.what()) == "RingBuffer is empty");
  }

  Assert(thrown == true);
}

void test_move_only_values() {
  RingBuffer<MoveOnly, 4> buffer;

  MoveOnly pushed(42);
  Assert(buffer.push(std::move(pushed)) == true);
  Assert(pushed.value == -1);

  MoveOnly out;
  Assert(buffer.pop(out) == true);
  Assert(out.value == 42);
  Assert(buffer.empty() == true);
}

} // namespace

/*
 * main
 */

int main() {
  test_ring_buffer_gdb<RingBuffer>();

  test_empty_full_and_fifo<3>();
  test_empty_full_and_fifo<4>();
  test_wrap_around<4>();
  test_wrap_around<5>();
  test_ext_pop_throws_on_empty();
  test_move_only_values();

}
