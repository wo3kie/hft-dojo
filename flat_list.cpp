/*
* Project:
*      HFTDojo (https://github.com/wo3kie/hft-dojo)
*
* Author:
*      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
*/

#include "flat_list.hpp"

#include <cassert>
#include <list>

namespace {

void test_empty_list() {
	FlatList<int, 4> list;

	assert(list.capacity() == 4);
	assert(list._ext_equal({}));

	assert(list.find(99) == -1);
}

void test_push_front_and_back() {
	FlatList<int, 5> list;

	const int32_t front_slot = list.push_front(20);
	assert(front_slot != -1);
	assert(list._ext_equal({20}));

	const int32_t back_slot = list.push_back(30);
	assert(back_slot != -1);
	assert(list._ext_equal({20, 30}));

	list.push_front(10);
	assert(list._ext_equal({10, 20, 30}));

	assert(list.find(10) != -1);
	assert(list.find(20) == front_slot);
	assert(list.find(30) == back_slot);
	assert(list[front_slot] == 20);
	assert(list[back_slot] == 30);
}

void test_insert_and_erase_middle() {
	FlatList<int, 6> list;

	const int32_t slot20 = list.push_back(20);
	const int32_t slot40 = list.push_back(40);
	assert(list._ext_equal({20, 40}));

	const int32_t slot30 = list.insert(slot40, 30);
	assert(slot30 != -1);
	assert(list._ext_equal({20, 30, 40}));

	const int32_t slot10 = list.insert(slot20, 10);
	assert(slot10 != -1);
	assert(list._ext_equal({10, 20, 30, 40}));

	const int32_t slot50 = list.insert(-1, 50);
	assert(slot50 != -1);
	assert(list._ext_equal({10, 20, 30, 40, 50}));

	list.erase(slot30);
	assert(list._ext_equal({10, 20, 40, 50}));
	assert(list.find(30) == -1);

	list.erase(slot10);
	assert(list._ext_equal({20, 40, 50}));

	list.erase(slot50);
	assert(list._ext_equal({20, 40}));

	const int32_t reused = list.push_back(60);
	assert(reused == slot50 || reused == slot30 || reused == slot10);
	assert(list._ext_equal({20, 40, 60}));
}

void test_pop_front_and_back() {
	FlatList<int, 4> list;

	list.push_back(1);
	list.push_back(2);
	list.push_back(3);
	assert(list._ext_equal({1, 2, 3}));

	list.pop_front();
	assert(list._ext_equal({2, 3}));

	list.pop_back();
	assert(list._ext_equal({2}));

	list.pop_back();
	assert(list._ext_equal({}));
}

void test_fill_list() {
	FlatList<int, 3> list;

	list.push_back(7);
	list.push_back(8);
	list.push_back(9);

	assert(list._ext_equal({7, 8, 9}));
	assert(list.full());
}

} // namespace

int main() {
	test_empty_list();
	test_push_front_and_back();
	test_insert_and_erase_middle();
	test_pop_front_and_back();
	test_fill_list();
}
