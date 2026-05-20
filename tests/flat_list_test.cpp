/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cassert>
#include <iostream>

#include "assert.hpp"
#include "flat_list.hpp"

void test_flat_list_1()
{
  FlatList<int, 8> list;

  Assert(list.empty());

  for(int i = 0; i < list.capacity(); ++i) {
    int32_t data = list.push_back(i);
    Assert(data != -1);
    Assert(list.at_slot(data) == i);
  }

  Assert(list.full());

  for(int i = 0; i < list.capacity(); ++i) {
    int v = list.front();
    list.pop_front();
    Assert(v == i);
  }

  Assert(list.empty());
}

void test_flat_list_2()
{
  FlatList<int, 8> list;

  Assert(list.empty());

  for(int i = 0; i < list.capacity(); ++i) {
    int32_t data = list.push_front(i);
    Assert(data != -1);
    Assert(list.at_slot(data) == i);
  }

  Assert(list.full());

  for(int i = 0; i < list.capacity(); ++i) {
    int v = list.back();
    list.pop_back();
    Assert(v == i);
  }

  Assert(list.empty());
}

void test_flat_list_remove()
{
  FlatList<int, 8> list;
  std::array<int32_t, 8> slots;

  Assert(list.empty());

  for(int i = 0; i < list.capacity(); ++i) {
    slots[i] = list.push_back(i);
  }

  list.remove(slots[1]);
  list.remove(slots[7]);
  list.remove(slots[5]);
  list.remove(slots[3]);
  Assert(list._debug_validate_links());

  Assert(list.at_slot(slots[0]) == 0);
  Assert(list.at_slot(slots[2]) == 2);
  Assert(list.at_slot(slots[4]) == 4);
  Assert(list.at_slot(slots[6]) == 6);

  list.remove(slots[0]);
  list.remove(slots[4]);
  list.remove(slots[6]);
  list.remove(slots[2]);
  Assert(list._debug_validate_links());

  Assert(list.empty());
}

void test_reuse_slots()
{
  FlatList<int, 4> list;

  int32_t a = list.push_back(1);
  int32_t b = list.push_back(2);
  int32_t c = list.push_back(3);
  int32_t d = list.push_back(4);
  Assert(list._debug_validate_links());

  Assert(list.full());

  list.remove(b);
  list.remove(d);
  Assert(list._debug_validate_links());

  int32_t x = list.push_back(10);
  int32_t y = list.push_back(20);
  Assert(list._debug_validate_links());

  Assert(x == d);
  Assert(y == b);

  Assert(list.at_slot(x) == 10);
  Assert(list.at_slot(y) == 20);
}

void test_remove_positions()
{
  FlatList<int, 8> list;

  int32_t a = list.push_back(1); // head
  int32_t b = list.push_back(2);
  int32_t c = list.push_back(3); // tail
  Assert(list._debug_validate_links());

  list.remove(a); // remove head
  Assert(list._debug_validate_links());
  Assert(list.front() == 2);
  Assert(list.back() == 3);

  list.remove(c); // remove tail
  Assert(list._debug_validate_links());
  Assert(list.front() == 2);
  Assert(list.back() == 2);

  list.remove(b); // remove last
  Assert(list._debug_validate_links());
  Assert(list.empty());
}

void test_mixed_operations()
{
  FlatList<int, 8> list;

  int32_t a = list.push_back(1);
  int32_t b = list.push_front(2);
  int32_t c = list.push_back(3);
  int32_t d = list.push_front(4);
  Assert(list._debug_validate_links());
  // 4, 2, 1, 3
  
  list.remove(b);
  // 4, 1, 3
  Assert(list._debug_validate_links());
  Assert(list.front() == 4);
  Assert(list.back() == 3);
  
  list.pop_front();
  // 1, 3
  Assert(list._debug_validate_links());
  Assert(list.front() == 1);
  Assert(list.back() == 3);
  
  
  list.pop_back(); // remove 3
  // 1
  Assert(list._debug_validate_links());
  Assert(list.front() == 1);
  Assert(list.back() == 1);
}

void test_clear()
{
  FlatList<int, 8> list;

  for(int i = 0; i < 8; ++i) {
    list.push_back(i);
  }

  list.clear();

  Assert(list.empty());
  Assert(!list.full());

  for(int i = 0; i < 8; ++i) {
    Assert(list._debug_is_free(i));
  }

  for(int i = 0; i < 8; ++i) {
    Assert(list.push_back(i) == i);
  }
}

void test_free_list_order()
{
  FlatList<int, 4> list;

  int32_t a = list.push_back(1);
  int32_t b = list.push_back(2);
  int32_t c = list.push_back(3);
  Assert(list._debug_validate_links());

  list.remove(b);
  list.remove(a);
  list.remove(c);
  Assert(list._debug_validate_links());

  int32_t x = list.push_back(10);
  int32_t y = list.push_back(20);
  int32_t z = list.push_back(30);
  Assert(list._debug_validate_links());

  Assert(x == c);
  Assert(y == a);
  Assert(z == b);
}

void test_gdb_function()
{
  FlatList<int, 32> list;

  for(int i = 0; i < 32; ++i) {
    list.push_back(i);
  }

  /*
   * (gdb) source ./gdb_utils.py 
   * (gdb) print_flat_list list
   * FlatList<int, 32> [ 0, 1, 2, 3, ..., 28, 29, 30, 31 ]
   */

  Assert(list.at_slot(0) == 0);
  Assert(list.at_slot(31) == 31);
}

int main()
{
  test_flat_list_1();
  test_flat_list_2();
  test_flat_list_remove();
  test_remove_positions();
  test_reuse_slots();
  test_mixed_operations();
  test_clear();
  test_free_list_order();
  
  test_gdb_function();
}
