
#include "flat_hash.hpp"

#include <cassert>
#include <unordered_map>

namespace {

template<typename THash>
void test_empty_hash() {
  THash hash;

  assert(hash.capacity() >= 8);
  assert(hash._ext_equal({}));
  assert(hash.find(123) == -1);
  assert(hash.contains(123) == false);
  assert(hash.get(-1) == nullptr);
  assert(hash.get(THash::capacity()) == nullptr);
  assert(hash.erase(-1) == false);
}

template<typename THash>
void test_insert_find_and_update() {
  THash hash;
  std::unordered_map<int, int> expected;

  const int32_t idx10 = hash.insert(10, 100);
  assert(idx10 != -1);
  expected[10] = 100;
  assert(hash._ext_equal(expected));

  const int32_t idx18 = hash.insert(18, 180);
  assert(idx18 != -1);
  expected[18] = 180;
  assert(hash._ext_equal(expected));

  const int32_t idx26 = hash.insert({26, 260});
  assert(idx26 != -1);
  expected[26] = 260;
  assert(hash._ext_equal(expected));

  const int32_t idx10_updated = hash.insert(10, 101);
  assert(idx10_updated == idx10);
  expected[10] = 101;
  assert(hash._ext_equal(expected));
}

template<typename THash>
void test_collisions_erase_and_tomb_reuse() {
  THash hash;
  std::unordered_map<int, int> expected;

  const int key1 = 1;
  const int key2 = key1 + THash::capacity();
  const int key3 = key2 + THash::capacity();
  const int key4 = key3 + THash::capacity();

  const int32_t idx1 = hash.insert(key1, 11);
  const int32_t idx2 = hash.insert(key2, 99);
  const int32_t idx3 = hash.insert(key3, 171);

  assert(idx1 != -1);
  assert(idx2 != -1);
  assert(idx3 != -1);

  expected[key1] = 11;
  expected[key2] = 99;
  expected[key3] = 171;
  assert(hash._ext_equal(expected));

  assert(hash.erase(idx2));
  expected.erase(key2);
  assert(hash._ext_equal(expected));
  assert(hash.find(key2) == -1);
  assert(hash.contains(key2) == false);

  const int32_t idx4 = hash.insert(key4, 2525);
  assert(idx4 == idx2);
  expected[key4] = 2525;
  assert(hash._ext_equal(expected));
}

template<typename THash>
void test_fill_to_capacity() {
  THash hash;
  std::unordered_map<int, int> expected;

  for(int32_t key = 0; key < THash::capacity(); ++key) {
    const int32_t idx = hash.insert(key, key * 10);
    assert(idx != -1);
    expected[key] = key * 10;
  }

  assert(hash._ext_equal(expected));
  assert(hash.full());
  assert(hash.insert(1000, 10000) == -1);
}

} // namespace

int main(){
  test_empty_hash<FlatHash<int, int, 8>>();
  test_insert_find_and_update<FlatHash<int, int, 8>>();
  test_collisions_erase_and_tomb_reuse<FlatHash<int, int, 8>>();
  test_fill_to_capacity<FlatHash<int, int, 8>>();
}