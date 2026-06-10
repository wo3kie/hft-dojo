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
#include "flat_tree_avl.hpp"
#include "random.hpp"
#include "test_utils.hpp"
#include "timer.hpp"

template<unsigned N>
void test_tree_avl() {
  LCG lcg;

  std::set<int>set;
  FlatTreeAVL<int, N> tree;

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
    test_tree_avl<1>();
    test_tree_avl<2>();
    test_tree_avl<4>();
    test_tree_avl<8>();
    test_tree_avl<16>();
    test_tree_avl<32>();
    test_tree_avl<64>();
  }

  return 0;
}
