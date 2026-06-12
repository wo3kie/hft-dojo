/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

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
#else
  bench_flat_tree<FlatTreeAVL<int, 10'000>>(10'000, "FlatTreeAVL");
#endif

  return 0;
}
