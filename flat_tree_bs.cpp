/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <algorithm>
#include <set>
#include <unordered_set>

#include "assert.hpp"
#include "flat_tree_bs.hpp"
#include "random.hpp"
#include "test_utils.hpp"
#include "timer.hpp"

template<unsigned N>
void test_tree_bs() {
  LCG lcg;

  std::set<int>set;
  FlatTreeBS<int, N> tree;

  std::vector<int> values;

  for(int i = 0; i < N; ++i) {
    values.push_back(i);
  }

  std::shuffle(values.begin(), values.end(), lcg);
  
  for(int i = 0; i < N; ++i) {
    Assert(tree.insert(values[i]) != -1);
    Assert(set.insert(values[i]).second);

    Assert(std::equal(set.begin(), set.end(), std::begin(tree), std::end(tree)));
  }
  
  std::shuffle(values.begin(), values.end(), lcg);

  for(int i = 0; i < N; ++i) {
    Assert(tree.find(values[i]) != -1);
  }
  
  std::shuffle(values.begin(), values.end(), lcg);

  for(int i = 0; i < N; ++i) {
    Assert(tree.erase(tree.find(values[i])));
    Assert(set.erase(values[i]) == 1);
    Assert(std::equal(set.begin(), set.end(), std::begin(tree), std::end(tree)));
  }
}

int main() {

  for(unsigned i = 0; i < 1024; ++i) {
    test_tree_bs<1>();
    test_tree_bs<2>();
    test_tree_bs<4>();
    test_tree_bs<8>();
    test_tree_bs<16>();
    test_tree_bs<32>();
    test_tree_bs<64>();
  }

  return 0;
}
