/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "ring_buffer_spmc.hpp"
#include "test_utils.hpp"

int main() {
#ifdef NDEBUG
  bench_ring_buffer<RingBufferSPMC<int, 128 * 1024>>(128 * 1024, "RingBufferSPMC");
  bench_ring_buffer<RingBufferSPMC<int, 128 * 1024>>(128 * 1024, "RingBufferSPMC");
  bench_ring_buffer<RingBufferSPMC<int, 128 * 1024>>(128 * 1024, "RingBufferSPMC");
  bench_ring_buffer<RingBufferSPMC<int, 128 * 1024>>(128 * 1024, "RingBufferSPMC");
  bench_ring_buffer<RingBufferSPMC<int, 128 * 1024>>(128 * 1024, "RingBufferSPMC");
#else
  bench_ring_buffer<RingBufferSPMC<int, 8 * 1024>>(8 * 1024, "RingBufferSPMC");
#endif
}
