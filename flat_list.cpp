/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "flat_list.hpp"

#include <cassert>
#include <list>

namespace {

void test_push_front_and_back() {
	FlatList<int, 5> list;

	list.push_front(20);
	assert(list._ext_equal({20}));

	list.push_back(30);
	assert(list._ext_equal({20, 30}));

	list.push_front(10);
	assert(list._ext_equal({10, 20, 30}));

	list.push_back(40);
	assert(list._ext_equal({10, 20, 30, 40}));

	list.push_front(0);
	assert(list._ext_equal({0, 10, 20, 30, 40}));

	assert(list.full());
}

void test_pop_front_and_back() {
	FlatList<int, 5> list;

	list.push_back(10);
	list.push_back(20);
	list.push_back(30);
	list.push_back(40);
	list.push_back(50);
	assert(list._ext_equal({10, 20, 30, 40, 50}));
	
	list.pop_front();
	assert(list._ext_equal({20, 30, 40, 50}));

	list.pop_back();
	assert(list._ext_equal({20, 30, 40}));

	list.pop_front();
	assert(list._ext_equal({30, 40}));

	list.pop_front();
	assert(list._ext_equal({40}));

	list.pop_front();
	assert(list._ext_equal({}));
}

void test_insert_middle() {
	FlatList<int, 5> list;

	list.insert(-1, 40);
	assert(list._ext_equal({40}));

	list.insert(list.find(40), 20);
	assert(list._ext_equal({20, 40}));

	list.insert(list.find(40), 30);
	assert(list._ext_equal({20, 30, 40}));

	list.insert(list.find(20), 10);
	assert(list._ext_equal({10, 20, 30, 40}));

	list.insert(-1, 50);
	assert(list._ext_equal({10, 20, 30, 40, 50}));
}

void test_erase_middle() {
	FlatList<int, 5> list;

	list.push_back(10);
	list.push_back(20);
	list.push_back(30);
	list.push_back(40);
	list.push_back(50);
	assert(list._ext_equal({10, 20, 30, 40, 50}));
	
	list.erase(list.find(10));
	assert(list._ext_equal({20, 30, 40, 50}));

	list.erase(list.find(50));
	assert(list._ext_equal({20, 30, 40}));

	list.erase(list.find(30));
	assert(list._ext_equal({20, 40}));

	list.erase(list.find(20));
	assert(list._ext_equal({40}));

	list.erase(list.find(40));
	assert(list._ext_equal({}));
}

void test_reuse_slots() {
	FlatList<int, 5> list;

	list.push_back(10);
	list.push_back(20);
	list.push_back(30);
	list.push_back(40);
	list.push_back(50);
	assert(list._ext_equal({10, 20, 30, 40, 50}));
	
	list.erase(list.find(30));
	assert(list._ext_equal({10, 20, 40, 50}));

	list.insert(list.find(40), 35);
	assert(list._ext_equal({10, 20, 35, 40, 50}));

	list.erase(list.find(10));
	assert(list._ext_equal({20, 35, 40, 50}));

	list.insert(list.find(20), 15);
	assert(list._ext_equal({15, 20, 35, 40, 50}));

	list.erase(list.find(50));
	assert(list._ext_equal({15, 20, 35, 40}));

	list.insert(-1, 55);
	assert(list._ext_equal({15, 20, 35, 40, 55}));

	list.erase(list.find(20));
	assert(list._ext_equal({15, 35, 40, 55}));

	list.insert(list.find(35), 25);
	assert(list._ext_equal({15, 25, 35, 40, 55}));

	list.erase(list.find(40));
	assert(list._ext_equal({15, 25, 35, 55}));

	list.insert(list.find(55), 45);
	assert(list._ext_equal({15, 25, 35, 45, 55}));
}

} // namespace

int main() {
	test_push_front_and_back();
	test_pop_front_and_back();

	test_insert_middle();
	test_erase_middle();
	test_reuse_slots();
}
