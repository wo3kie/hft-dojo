/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "flat_tree_avl.hpp"
#include "random.hpp"

#include <cassert>
#include <set>

namespace {

template<int32_t Capacity>
void test_insert() {
  LCG lcg;
  std::set<int> expected;
  FlatTreeAVL<int, Capacity> actual;

  for(int32_t i = 0; i < Capacity; i += 1) {
    const int32_t value = lcg();
    
    actual.insert(value);
    expected.insert(value);
    assert(actual._ext_equal(expected));
  }
}

template<int32_t Capacity>
void test_erase() {
  LCG lcg;
  std::set<int> expected;
  FlatTreeAVL<int, Capacity> actual;

  for(int32_t i = 0; i < Capacity; i += 1) {   
    actual.insert(i);
    expected.insert(i);
  }

  for(int32_t i = 0; i < Capacity; i += 1) {
    const int32_t value = lcg() % Capacity;
    
    const int32_t slotId = actual.find(value);

    if (slotId == -1) {
      continue;
    }

    actual.erase(slotId);
    expected.erase(value);
    assert(actual._ext_equal(expected));
  }
}

} // namespace

int main() {
  test_insert<1>();
  test_insert<2>();
  test_insert<4>();
  test_insert<8>();
  test_insert<16>();
  test_insert<32>();
  test_insert<64>();
  test_insert<128>();
  test_insert<256>();
  test_insert<512>();
  test_insert<1024>();

  test_erase<1>();
  test_erase<2>();
  test_erase<4>();
  test_erase<8>();
  test_erase<16>();
  test_erase<32>();
  test_erase<64>();
  test_erase<128>();
  test_erase<256>();
  test_erase<512>();
  test_erase<1024>();
}
