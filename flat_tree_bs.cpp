/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "flat_tree_bs.hpp"

#include <cassert>
#include <set>

namespace {

template<typename TTree>
void test_empty_tree() {
  TTree tree;

  assert(tree.size() == 0);
  assert(tree.capacity() > 0);
  assert(tree._ext_equal({}));
  assert(tree.find(123) == -1);
  assert(tree.get(-1) == nullptr);
  assert(tree.erase(-1) == false);
}

template<typename TTree>
void test_insert_find_and_duplicates() {
  TTree tree;
  std::set<int> expected;

  assert(tree.insert(20) != -1);
  expected.insert(20);
  assert(tree._ext_equal(expected));

  assert(tree.insert(10) != -1);
  expected.insert(10);
  assert(tree._ext_equal(expected));

  assert(tree.insert(30) != -1);
  expected.insert(30);
  assert(tree._ext_equal(expected));

  assert(tree.insert(25) != -1);
  expected.insert(25);
  assert(tree._ext_equal(expected));

  assert(tree.insert(20) == -1);
  assert(tree._ext_equal(expected));
  assert(tree.find(999) == -1);
}

template<typename TTree>
void test_erase_leaf_and_single_child() {
  TTree tree;
  std::set<int> expected;

  for(int value : {20, 10, 30, 25}) {
    assert(tree.insert(value) != -1);
    expected.insert(value);
  }

  assert(tree._ext_equal(expected));

  const int32_t slot25 = tree.find(25);
  assert(slot25 != -1);
  assert(tree.erase(slot25));
  expected.erase(25);
  assert(tree._ext_equal(expected));
  assert(tree.find(25) == -1);

  const int32_t slot30 = tree.find(30);
  assert(slot30 != -1);
  assert(tree.erase(slot30));
  expected.erase(30);
  assert(tree._ext_equal(expected));
  assert(tree.find(30) == -1);
}

template<typename TTree>
void test_erase_root_with_one_child() {
  TTree tree;
  std::set<int> expected;

  assert(tree.insert(20) != -1);
  assert(tree.insert(10) != -1);
  expected.insert(20);
  expected.insert(10);
  assert(tree._ext_equal(expected));

  const int32_t root = tree.find(20);
  assert(root != -1);
  assert(tree.erase(root));
  expected.erase(20);
  assert(tree._ext_equal(expected));
  assert(tree.find(20) == -1);
  assert(tree.find(10) != -1);
}

template<typename TTree>
void test_erase_node_with_two_children() {
  TTree tree;
  std::set<int> expected;

  for(int value : {20, 10, 30, 5, 15, 25, 35, 27}) {
    assert(tree.insert(value) != -1);
    expected.insert(value);
  }

  assert(tree._ext_equal(expected));

  const int32_t slot20 = tree.find(20);
  assert(slot20 != -1);
  assert(tree.erase(slot20));
  expected.erase(20);
  assert(tree._ext_equal(expected));
  assert(tree.find(20) == -1);

  const int32_t slot30 = tree.find(30);
  assert(slot30 != -1);
  assert(tree.erase(slot30));
  expected.erase(30);
  assert(tree._ext_equal(expected));
  assert(tree.find(30) == -1);
}

} // namespace

int main() {
  test_empty_tree<FlatTreeBS<int, 8>>();
  test_empty_tree<FlatTreeBS<int, 16>>();

  test_insert_find_and_duplicates<FlatTreeBS<int, 8>>();
  test_insert_find_and_duplicates<FlatTreeBS<int, 16>>();

  test_erase_leaf_and_single_child<FlatTreeBS<int, 8>>();
  test_erase_leaf_and_single_child<FlatTreeBS<int, 16>>();

  test_erase_root_with_one_child<FlatTreeBS<int, 8>>();
  test_erase_root_with_one_child<FlatTreeBS<int, 16>>();

  test_erase_node_with_two_children<FlatTreeBS<int, 8>>();
  test_erase_node_with_two_children<FlatTreeBS<int, 16>>();


  return 0;
}
