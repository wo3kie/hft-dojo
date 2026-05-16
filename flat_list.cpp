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

void test_push_front()
{
  FlatList<int, 8> list;

  int32_t s1 = list.push_front(10);
  Assert(s1 != -1);

  int32_t s2 = list.push_front(20);
  Assert(s2 != -1);

  int32_t s3 = list.push_front(30);
  Assert(s3 != -1);

  int v1 = list.back();
  list.pop_back();
  Assert(v1 == 10);

  int v2 = list.back();
  list.pop_back();
  Assert(v2 == 20);

  int v3 = list.back();
  list.pop_back();
  Assert(v3 == 30);

  Assert(list.empty());
}

void test_push_back()
{
  FlatList<int, 8> list;

  int32_t s1 = list.push_back(10);
  int32_t s2 = list.push_back(20);
  int32_t s3 = list.push_back(30);
  Assert(s1 != -1);
  Assert(s2 != -1);
  Assert(s3 != -1);

  int v1 = list.front();
  list.pop_front();
  int v2 = list.front();
  list.pop_front();
  int v3 = list.front();
  list.pop_front();
  Assert(v1 == 10);
  Assert(v2 == 20);
  Assert(v3 == 30);
  Assert(list.empty());
}

void test_remove_middle()
{
  FlatList<int, 8> list;

  int32_t s1 = list.push_back(1);
  int32_t s2 = list.push_back(2);
  int32_t s3 = list.push_back(3);
  list.remove(s2);

  int v1 = list.front();
  list.pop_front();
  int v2 = list.front();
  list.pop_front();
  Assert(v1 == 1);
  Assert(v2 == 3);
  Assert(list.empty());
}

void test_remove_head()
{
  FlatList<int, 8> list;

  int32_t s1 = list.push_back(5);
  int32_t s2 = list.push_back(6);
  int32_t s3 = list.push_back(7);
  list.remove(s1);

  int v1 = list.front();
  list.pop_front();
  int v2 = list.front();
  list.pop_front();
  Assert(v1 == 6);
  Assert(v2 == 7);
  Assert(list.empty());
}

void test_remove_tail()
{
  FlatList<int, 8> list;

  int32_t s1 = list.push_back(11);
  int32_t s2 = list.push_back(22);
  int32_t s3 = list.push_back(33);
  list.remove(s3);

  int v1 = list.front();
  list.pop_front();
  int v2 = list.front();
  list.pop_front();
  Assert(v1 == 11);
  Assert(v2 == 22);
  Assert(list.empty());
}

void test_full()
{
  FlatList<int, 3> list;

  int32_t s1 = list.push_back(1);
  int32_t s2 = list.push_back(2);
  int32_t s3 = list.push_back(3);
  Assert(list.full() == true);
}

void test_reuse_slots()
{
  FlatList<int, 4> list;

  int32_t s1 = list.push_back(10);
  int32_t s2 = list.push_back(20);
  list.remove(s1);

  int32_t s3 = list.push_back(30);
  Assert(s3 == s1);

  int v1 = list.front();
  list.pop_front();
  int v2 = list.front();
  list.pop_front();
  Assert(v1 == 20);
  Assert(v2 == 30);
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

  Assert(list.at(0) == 0);
  Assert(list.at(31) == 31);
}

int main()
{
  test_push_front();
  test_push_back();
  test_remove_middle();
  test_remove_head();
  test_remove_tail();
  test_full();
  test_reuse_slots();
  test_gdb_function();
}
