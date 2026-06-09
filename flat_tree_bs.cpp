/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <set>
#include <unordered_set>

#include "assert.hpp"
#include "flat_tree_bs.hpp"
#include "random.hpp"
#include "test_utils.hpp"
#include "timer.hpp"

template<unsigned N>
void test_bs_tree() {
  LCG lcg;
  FlatTreeBS<int, N> tree;

  std::vector<int> values;

  for(int i = 0; i < N; ++i) {
    values.push_back(i);
  }

  std::shuffle(values.begin(), values.end(), lcg);
  
  for(int i = 0; i < N; ++i) {
    Assert(tree.insert(values[i]));
  }
  
  for(int i = 0; i < N; ++i) {
    Assert(tree.find(values[i]));
  }
  
  for(int i = 0; i < N; ++i) {
    Assert(tree.erase(values[i]));
  }
}

int main() {
  test_bs_tree<1>();
  test_bs_tree<2>();
  test_bs_tree<4>();
  test_bs_tree<8>();
  test_bs_tree<16>();
  test_bs_tree<32>();
  test_bs_tree<64>();
  test_bs_tree<128>();
  test_bs_tree<256>();
  test_bs_tree<512>();
  test_bs_tree<1024>();

  return 0;
}
