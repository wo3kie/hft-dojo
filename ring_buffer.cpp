/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 *
 */

#include "assert.hpp"
#include "common.hpp"

#include "ring_buffer.hpp"
#include "test_utils.hpp"

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
};

/*
 * main
 */

int main() {
  test_ring_buffer_gdb<RingBuffer>();

#ifdef NDEBUG
  bench_ring_buffer<RingBuffer<int, 128 * 1024>>(128 * 1024, "RingBuffer");
  bench_ring_buffer<RingBuffer<int, 128 * 1024>>(128 * 1024, "RingBuffer");
  bench_ring_buffer<RingBuffer<int, 128 * 1024>>(128 * 1024, "RingBuffer");
  bench_ring_buffer<RingBuffer<int, 128 * 1024>>(128 * 1024, "RingBuffer");
  bench_ring_buffer<RingBuffer<int, 128 * 1024>>(128 * 1024, "RingBuffer");
#else
  bench_ring_buffer<RingBuffer<int, 8 * 1024>>(8 * 1024, "RingBuffer");
#endif
}
