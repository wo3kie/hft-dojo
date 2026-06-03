/*
* Project:
*      HFTDojo (https://github.com/wo3kie/hft-dojo)
*
* Author:
*      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
*/

#include "flat_list.hpp"

void test_flat_list() {
  FlatList<int, 4> list;
  assert(list.empty());

  int8_t slot;
  slot = list.push_front(1);
  slot = list.push_back(2);
  list.insert(slot, 3);

  assert(list.front() == 1);
  list.pop_front();

  assert(list.front() == 2);
  list.pop_front();

  assert(list.front() == 3);
  list.pop_front();

  assert(list.empty());
}

int main() {
  test_flat_list();
}
