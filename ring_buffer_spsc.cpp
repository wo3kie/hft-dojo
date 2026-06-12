/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "ring_buffer_spsc.hpp"
#include "test_utils.hpp"

int main() {
#ifdef NDEBUG
  bench_ring_buffer<RingBufferSPSC<int, 128 * 1024>>(128 * 1024, "RingBufferSPSC");
  bench_ring_buffer<RingBufferSPSC<int, 128 * 1024>>(128 * 1024, "RingBufferSPSC");
  bench_ring_buffer<RingBufferSPSC<int, 128 * 1024>>(128 * 1024, "RingBufferSPSC");
  bench_ring_buffer<RingBufferSPSC<int, 128 * 1024>>(128 * 1024, "RingBufferSPSC");
  bench_ring_buffer<RingBufferSPSC<int, 128 * 1024>>(128 * 1024, "RingBufferSPSC");
#else
  bench_ring_buffer<RingBufferSPSC<int, 8 * 1024>>(8 * 1024, "RingBufferSPSC");
#endif
}
