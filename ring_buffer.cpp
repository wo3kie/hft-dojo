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
#include "ring_buffer_mutex.hpp"
#include "ring_buffer_spmc.hpp"
#include "ring_buffer_spsc.hpp"

#include <iostream>
#include <numeric>
#include <random>

/*
 * test_ring_buffer
 */

template<std::size_t P /* Producers */, std::size_t C /* Consumers */, typename TRBuffer>
void test_ring_buffer_correctness()
{
  static_assert(P > 0 && C > 0);

  constexpr std::size_t K = 100 * TRBuffer::capacity();
  constexpr std::size_t TOTAL = P * K;

  TRBuffer rBuffer;

  std::atomic<std::size_t> produced_total{0};
  std::atomic<std::size_t> consumed_total{0};
  std::atomic<std::size_t> duplicate_total{0};
  std::atomic<std::size_t> out_of_range_total{0};
  std::vector<std::atomic<std::size_t>> seen(TOTAL);

  for(auto& count : seen) {
    count.store(0, std::memory_order_relaxed);
  }

  std::vector<std::thread> producers;
  producers.reserve(P);

  for(std::size_t pid = 0; pid < P; ++pid) {
    producers.emplace_back([&, pid] {
      for(std::size_t i = 0; i < K; ++i) {
        typename TRBuffer::value_type v = static_cast<typename TRBuffer::value_type>(i + pid * K);

        while(! rBuffer.push(v)) {
          std::this_thread::yield();
        }

        produced_total.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  std::vector<std::thread> consumers;
  consumers.reserve(C);

  for(std::size_t cid = 0; cid < C; ++cid) {
    consumers.emplace_back([&] {
      typename TRBuffer::value_type v;

      while(true) {
        if(consumed_total.load(std::memory_order_acquire) == TOTAL) {
          break;
        }

        if(rBuffer.pop(v)) {
          const std::size_t index = static_cast<std::size_t>(v);

          if(index >= TOTAL) {
            out_of_range_total.fetch_add(1, std::memory_order_relaxed);
          } else if(seen[index].fetch_add(1, std::memory_order_relaxed) != 0) {
            duplicate_total.fetch_add(1, std::memory_order_relaxed);
          }

          consumed_total.fetch_add(1, std::memory_order_relaxed);
          continue;
        }

        std::this_thread::yield();
      }
    });
  }

  for(auto& t : producers) {
    t.join();
  }

  for(auto& t : consumers) {
    t.join();
  }

  Assert(produced_total.load(std::memory_order_relaxed) == TOTAL)
      .on_error([](const char* file, int line, const char* op, const auto& actual, const auto& expected) {
        std::cerr << "Assertion failed: actual: " << actual << ", "
                  << "expected: " << expected << " "
                  << "on " << demangle(typeid(TRBuffer).name()) << std::endl;
      });

  Assert(consumed_total.load(std::memory_order_relaxed) == TOTAL)
      .on_error([](const char* file, int line, const char* op, const auto& actual, const auto& expected) {
        std::cerr << "Assertion failed: actual: " << actual << ", "
                  << "expected: " << expected  << " "
                  << "on " << demangle(typeid(TRBuffer).name()) << std::endl;
      });

  Assert(out_of_range_total.load(std::memory_order_relaxed) == 0)
      .on_error([](const char* file, int line, const char* op, const auto& actual, const auto& expected) {
        std::cerr << "Assertion failed: actual: " << actual << ", "
                  << "expected: " << expected << " "
                  << "on " << demangle(typeid(TRBuffer).name()) << std::endl;
      });

  Assert(duplicate_total.load(std::memory_order_relaxed) == 0)
      .on_error([](const char* file, int line, const char* op, const auto& actual, const auto& expected) {
        std::cerr << "Assertion failed: actual: " << actual << ", "
                  << "expected: " << expected << " "
                  << "on " << demangle(typeid(TRBuffer).name()) << std::endl;
      });

  for(const auto& count : seen) {
    Assert(count.load(std::memory_order_relaxed) == 1)
        .on_error([](const char* file, int line, const char* op, const auto& actual, const auto& expected) {
          std::cerr << "Assertion failed: actual: " << actual << ", "
                    << "expected: " << expected << " "
                    << "on " << demangle(typeid(TRBuffer).name()) << std::endl;
        });
  }
}

template< template<typename, std::size_t> class TBuffer>
void test_ring_buffer_gdb()
{
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

int main()
{
  test_ring_buffer_gdb<RingBuffer>();
  test_ring_buffer_gdb<RingBufferSPSC>();
  test_ring_buffer_gdb<RingBufferSPMC>();

  test_ring_buffer_correctness<1, 1, RingBuffer<int, 2>>();
  test_ring_buffer_correctness<1, 1, RingBufferSPSC<int, 2>>();
  test_ring_buffer_correctness<1, 4, RingBufferSPMC<int, 2>>();
  test_ring_buffer_correctness<4, 1, RingBufferMT<int, 2>>();

  test_ring_buffer_correctness<1, 1, RingBuffer<int, 1023>>();
  test_ring_buffer_correctness<1, 1, RingBufferSPSC<int, 1023>>();
  test_ring_buffer_correctness<1, 4, RingBufferSPMC<int, 1023>>();
  test_ring_buffer_correctness<4, 1, RingBufferMT<int, 1023>>();
}
