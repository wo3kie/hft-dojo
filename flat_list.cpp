/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cassert>
#include <iostream>

#include "./flat_list.hpp"

void test_push_front()
{
  FlatList<8, int> list;

  int32_t s1 = list.push_front(10);
  assert(s1 != -1);

  int32_t s2 = list.push_front(20);
  assert(s2 != -1);

  int32_t s3 = list.push_front(30);
  assert(s3 != -1);

  int v1 = list.back();
  list.pop_back();
  assert(v1 == 10);

  int v2 = list.back();
  list.pop_back();
  assert(v2 == 20);

  int v3 = list.back();
  list.pop_back();
  assert(v3 == 30);

  assert(list.empty());
}

void test_push_back()
{
  FlatList<8, int> list;

  int32_t s1 = list.push_back(10);
  int32_t s2 = list.push_back(20);
  int32_t s3 = list.push_back(30);
  assert(s1 != -1);
  assert(s2 != -1);
  assert(s3 != -1);

  int v1 = list.front();
  list.pop_front();
  int v2 = list.front();
  list.pop_front();
  int v3 = list.front();
  list.pop_front();
  assert(v1 == 10);
  assert(v2 == 20);
  assert(v3 == 30);
  assert(list.empty());
}

void test_remove_middle()
{
  FlatList<8, int> list;

  int32_t s1 = list.push_back(1);
  int32_t s2 = list.push_back(2);
  int32_t s3 = list.push_back(3);
  list.remove(s2);

  int v1 = list.front();
  list.pop_front();
  int v2 = list.front();
  list.pop_front();
  assert(v1 == 1);
  assert(v2 == 3);
  assert(list.empty());
}

void test_remove_head()
{
  FlatList<8, int> list;

  int32_t s1 = list.push_back(5);
  int32_t s2 = list.push_back(6);
  int32_t s3 = list.push_back(7);
  list.remove(s1);

  int v1 = list.front();
  list.pop_front();
  int v2 = list.front();
  list.pop_front();
  assert(v1 == 6);
  assert(v2 == 7);
  assert(list.empty());
}

void test_remove_tail()
{
  FlatList<8, int> list;

  int32_t s1 = list.push_back(11);
  int32_t s2 = list.push_back(22);
  int32_t s3 = list.push_back(33);
  list.remove(s3);

  int v1 = list.front();
  list.pop_front();
  int v2 = list.front();
  list.pop_front();
  assert(v1 == 11);
  assert(v2 == 22);
  assert(list.empty());
}

void test_full()
{
  FlatList<3, int> list;

  int32_t s1 = list.push_back(1);
  int32_t s2 = list.push_back(2);
  int32_t s3 = list.push_back(3);
  assert(list.full() == true);
}

void test_reuse_slots()
{
  FlatList<4, int> list;

  int32_t s1 = list.push_back(10);
  int32_t s2 = list.push_back(20);
  list.remove(s1);

  int32_t s3 = list.push_back(30);
  assert(s3 == s1);

  int v1 = list.front();
  list.pop_front();
  int v2 = list.front();
  list.pop_front();
  assert(v1 == 20);
  assert(v2 == 30);
}

void test_for_each()
{
  FlatList<4, int> list;

  list.push_back(1);
  list.push_back(2);
  list.push_back(3);

  int sum = 0;
  list.for_each([&sum](int value) {
    sum += value;
  });

  assert(sum == 6);
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
  test_for_each();
}
