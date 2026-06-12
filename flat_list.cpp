/*
* Project:
*      HFTDojo (https://github.com/wo3kie/hft-dojo)
*
* Author:
*      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
*/

#include "flat_list.hpp"
#include "test_utils.hpp"

int main() {
  for(int32_t i = 0; i < 1024; ++i) {
    test_flat_list<FlatList<int, 1>>();
    test_flat_list<FlatList<int, 2>>();
    test_flat_list<FlatList<int, 4>>();
    test_flat_list<FlatList<int, 8>>();
    test_flat_list<FlatList<int, 16>>();
    test_flat_list<FlatList<int, 32>>();
    test_flat_list<FlatList<int, 64>>();
  }

#ifdef NDEBUG
  bench_flat_list<FlatList<int, 100'000>>(100'000, "FlatList");
  bench_flat_list<FlatList<int, 100'000>>(100'000, "FlatList");
  bench_flat_list<FlatList<int, 100'000>>(100'000, "FlatList");
  bench_flat_list<FlatList<int, 100'000>>(100'000, "FlatList");
  bench_flat_list<FlatList<int, 100'000>>(100'000, "FlatList");
#else
  bench_flat_list<FlatList<int, 10'000>>(10'000, "FlatList");
  bench_flat_list<std::list<int>>(10'000, "std::list");
#endif
}
