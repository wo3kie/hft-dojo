/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <unordered_set>

#include "flat_tree_avl.hpp"
#include "test_utils.hpp"

int main() {
  for(unsigned i = 0; i < 1024; ++i) {
    test_flat_tree<FlatTreeAVL<int, 1>>();
    test_flat_tree<FlatTreeAVL<int, 2>>();
    test_flat_tree<FlatTreeAVL<int, 4>>();
    test_flat_tree<FlatTreeAVL<int, 8>>();
    test_flat_tree<FlatTreeAVL<int, 16>>();
    test_flat_tree<FlatTreeAVL<int, 32>>();
    test_flat_tree<FlatTreeAVL<int, 64>>();
  }

#ifdef NDEBUG
  bench_flat_tree<FlatTreeAVL<int, 100'000>>(100'000, "FlatTreeAVL");
  bench_flat_tree<FlatTreeAVL<int, 100'000>>(100'000, "FlatTreeAVL");
  bench_flat_tree<FlatTreeAVL<int, 100'000>>(100'000, "FlatTreeAVL");
  bench_flat_tree<FlatTreeAVL<int, 100'000>>(100'000, "FlatTreeAVL");
  bench_flat_tree<FlatTreeAVL<int, 100'000>>(100'000, "FlatTreeAVL");
  bench_flat_tree<std::set<int>>(100'000, "std::set");
  bench_flat_tree<std::set<int>>(100'000, "std::set");
  bench_flat_tree<std::set<int>>(100'000, "std::set");
  bench_flat_tree<std::set<int>>(100'000, "std::set");
  bench_flat_tree<std::set<int>>(100'000, "std::set");
#else
  bench_flat_tree<FlatTreeAVL<int, 10'000>>(10'000, "FlatTreeAVL");
  bench_flat_tree<std::set<int>>(10'000, "std::set");
#endif

  return 0;
}
