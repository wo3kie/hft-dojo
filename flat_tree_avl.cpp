/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "flat_tree_avl.hpp"

#include <cassert>
#include <set>

namespace {

template<typename TTree>
void test_empty_tree() {
  TTree tree;

  assert(tree.capacity() > 0);
  assert(tree._ext_equal({}));
  assert(tree.find(123) == -1);
  assert(tree.get(-1) == nullptr);
  assert(tree.erase(-1) == false);
}

template<typename TTree>
void test_insert_and_duplicate_rejection() {
  TTree tree;
  std::set<int> expected;

  for(int value : {20, 10, 30, 25}) {
    assert(tree.insert(value) != -1);
    expected.insert(value);
    assert(tree._ext_equal(expected));
  }

  assert(tree.insert(20) == -1);
  assert(tree._ext_equal(expected));
  assert(tree.find(999) == -1);
}

template<typename TTree>
void test_ll_and_rr_rotations() {
  {
    TTree tree;
    std::set<int> expected;

    for(int value : {30, 20, 10}) {
      assert(tree.insert(value) != -1);
      expected.insert(value);
    }

    assert(tree._ext_equal(expected));
  }

  {
    TTree tree;
    std::set<int> expected;

    for(int value : {10, 20, 30}) {
      assert(tree.insert(value) != -1);
      expected.insert(value);
    }

    assert(tree._ext_equal(expected));
  }
}

template<typename TTree>
void test_lr_and_rl_rotations() {
  {
    TTree tree;
    std::set<int> expected;

    for(int value : {30, 10, 20}) {
      assert(tree.insert(value) != -1);
      expected.insert(value);
    }

    assert(tree._ext_equal(expected));
  }

  {
    TTree tree;
    std::set<int> expected;

    for(int value : {10, 30, 20}) {
      assert(tree.insert(value) != -1);
      expected.insert(value);
    }

    assert(tree._ext_equal(expected));
  }
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

  const int32_t slot30 = tree.find(30);
  assert(slot30 != -1);
  assert(tree.erase(slot30));
  expected.erase(30);
  assert(tree._ext_equal(expected));
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
  test_empty_tree<FlatTreeAVL<int, 8>>();
  test_empty_tree<FlatTreeAVL<int, 16>>();

  test_insert_and_duplicate_rejection<FlatTreeAVL<int, 8>>();
  test_insert_and_duplicate_rejection<FlatTreeAVL<int, 16>>();

  test_ll_and_rr_rotations<FlatTreeAVL<int, 8>>();
  test_ll_and_rr_rotations<FlatTreeAVL<int, 16>>();

  test_lr_and_rl_rotations<FlatTreeAVL<int, 8>>();
  test_lr_and_rl_rotations<FlatTreeAVL<int, 16>>();

  test_erase_leaf_and_single_child<FlatTreeAVL<int, 8>>();
  test_erase_leaf_and_single_child<FlatTreeAVL<int, 16>>();

  test_erase_node_with_two_children<FlatTreeAVL<int, 8>>();
  test_erase_node_with_two_children<FlatTreeAVL<int, 16>>();

  return 0;
}
