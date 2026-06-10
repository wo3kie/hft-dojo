/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "flat_tree_splay.hpp"
#include "test_utils.hpp"

int main() {
  for(unsigned i = 0; i < 1024; ++i) {
    test_flat_tree<FlatTreeSplay<int, 1>>();
    test_flat_tree<FlatTreeSplay<int, 2>>();
    test_flat_tree<FlatTreeSplay<int, 4>>();
    test_flat_tree<FlatTreeSplay<int, 8>>();
    test_flat_tree<FlatTreeSplay<int, 16>>();
    test_flat_tree<FlatTreeSplay<int, 32>>();
    test_flat_tree<FlatTreeSplay<int, 64>>();
  }

#ifdef NDEBUG
  bench_flat_tree<FlatTreeSplay<int, 1024>>(1'000'000);
  bench_flat_tree<std::set<int>>(1'000'000);
#else
  /* stack storage */
  bench_flat_tree<FlatTreeSplay<int, 1024>>(1'000);
  bench_flat_tree<std::set<int>>(1'000);

  /* heap storage */
  bench_flat_tree<FlatTreeSplay<int, 1024>>(10'000);
  bench_flat_tree<std::set<int>>(10'000);
#endif

  return 0;
}
