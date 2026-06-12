/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "ring_buffer_mutex.hpp"
#include "test_utils.hpp"

int main() {
#ifdef NDEBUG
  bench_ring_buffer<RingBufferMutex<int, 128 * 1024>>(128 * 1024, "RingBufferMutex");
  bench_ring_buffer<RingBufferMutex<int, 128 * 1024>>(128 * 1024, "RingBufferMutex");
  bench_ring_buffer<RingBufferMutex<int, 128 * 1024>>(128 * 1024, "RingBufferMutex");
  bench_ring_buffer<RingBufferMutex<int, 128 * 1024>>(128 * 1024, "RingBufferMutex");
  bench_ring_buffer<RingBufferMutex<int, 128 * 1024>>(128 * 1024, "RingBufferMutex");
#else
  bench_ring_buffer<RingBufferMutex<int, 8 * 1024>>(8 * 1024, "RingBufferMutex");
#endif
}
