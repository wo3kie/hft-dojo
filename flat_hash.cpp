
#include "flat_hash.hpp"
#include "test_utils.hpp"

int main(){
  for (int i = 0; i < 1024; ++i) {
    test_flat_hash<FlatHash<int, int, 8>>(8);
    test_flat_hash<FlatHash<int, int, 16>>(16);
    test_flat_hash<FlatHash<int, int, 32>>(32);
    test_flat_hash<FlatHash<int, int, 64>>(64);
    test_flat_hash<FlatHash<int, int, 128>>(128);
    test_flat_hash<FlatHash<int, int, 256>>(256);
    test_flat_hash<FlatHash<int, int, 512>>(512);
    test_flat_hash<FlatHash<int, int, 1024>>(1024);
  }

#ifdef NDEBUG
  bench_flat_hash<FlatHash<int, int, 128 * 1024>>(128 * 1024, "FlatHash");
  bench_flat_hash<FlatHash<int, int, 128 * 1024>>(128 * 1024, "FlatHash");
  bench_flat_hash<FlatHash<int, int, 128 * 1024>>(128 * 1024, "FlatHash");
  bench_flat_hash<FlatHash<int, int, 128 * 1024>>(128 * 1024, "FlatHash");
  bench_flat_hash<FlatHash<int, int, 128 * 1024>>(128 * 1024, "FlatHash");
#else
  bench_flat_hash<FlatHash<int, int, 8 * 1024>>(8 * 1024, "FlatHash");
#endif
}