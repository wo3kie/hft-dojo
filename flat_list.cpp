/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cassert>
#include <iostream>

#include "./assert.hpp"
#include "./flat_list.hpp"

void test_flat_list_1()
{
  FlatList<int, 8> list;

  Assert(list.empty());

  for(int i = 0; i < list.capacity(); ++i) {
    int32_t data = list.push_back(i);
    Assert(data != -1);
    Assert(list.data(data) == i);
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
    Assert(list.data(data) == i);
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

  Assert(list.empty());

  for(int i = 0; i < list.capacity(); ++i) {
    int32_t data = list.push_back(i);
  }

  list.remove(1);
  list.remove(7);
  list.remove(5);
  list.remove(3);

  Assert(list.data(0) == 0);
  Assert(list.data(2) == 2);
  Assert(list.data(4) == 4);
  Assert(list.data(6) == 6);

  list.remove(0);
  list.remove(4);
  list.remove(6);
  list.remove(2);

  Assert(list.empty());
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

  Assert(list.data(0) == 0);
  Assert(list.data(31) == 31);
}

int main()
{
  test_flat_list_1();
  test_flat_list_2();
  test_flat_list_remove();
  test_gdb_function();
}
