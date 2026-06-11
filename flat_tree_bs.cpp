/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <unordered_set>

#include "flat_tree_bs.hpp"
#include "test_utils.hpp"

int main() {
  for(unsigned i = 0; i < 1024; ++i) {
    test_flat_tree<FlatTreeBS<int, 1>>();
    test_flat_tree<FlatTreeBS<int, 2>>();
    test_flat_tree<FlatTreeBS<int, 4>>();
    test_flat_tree<FlatTreeBS<int, 8>>();
    test_flat_tree<FlatTreeBS<int, 16>>();
    test_flat_tree<FlatTreeBS<int, 32>>();
    test_flat_tree<FlatTreeBS<int, 64>>();
  }

#ifdef NDEBUG
  bench_flat_tree<FlatTreeBS<int, 100'000>>(100'000, "FlatTreeBS");
  bench_flat_tree<FlatTreeBS<int, 100'000>>(100'000, "FlatTreeBS");
  bench_flat_tree<FlatTreeBS<int, 100'000>>(100'000, "FlatTreeBS");
  bench_flat_tree<FlatTreeBS<int, 100'000>>(100'000, "FlatTreeBS");
  bench_flat_tree<FlatTreeBS<int, 100'000>>(100'000, "FlatTreeBS");
  bench_flat_tree<std::set<int>>(100'000, "std::set");
  bench_flat_tree<std::set<int>>(100'000, "std::set");
  bench_flat_tree<std::set<int>>(100'000, "std::set");
  bench_flat_tree<std::set<int>>(100'000, "std::set");
  bench_flat_tree<std::set<int>>(100'000, "std::set");
#else
  bench_flat_tree<FlatTreeBS<int, 10'000>>(10'000, "FlatTreeBS");
  bench_flat_tree<std::set<int>>(10'000, "std::set");
#endif

  return 0;
}
