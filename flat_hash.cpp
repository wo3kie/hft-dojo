
#include "flat_hash.hpp"
#include "test_utils.hpp"

int main(){
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