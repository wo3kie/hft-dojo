/*
* Project:
*      HFTDojo (https://github.com/wo3kie/hft-dojo)
*
* Author:
*      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
*/

#include <cassert>

#include "branchless.hpp"

/*
 * test_min
 */

void test_min() {
	assert(bl::min(3, 7) == 3);
	assert(bl::min(7, 3) == 3);
	assert(bl::min(-3, 2) == -3);
	assert(bl::min(-8, -2) == -8);
	assert(bl::min(5, 5) == 5);
}

/*
 * test_max
 */

void test_max() {
	assert(bl::max(3, 7) == 7);
	assert(bl::max(7, 3) == 7);
	assert(bl::max(-3, 2) == 2);
	assert(bl::max(-8, -2) == -2);
	assert(bl::max(5, 5) == 5);
}

/*
 * test_in_range
 */

void test_in_range() {
	assert(bl::in_range(5, 0, 10));
	assert(bl::in_range(0, 0, 10));
	assert(bl::in_range(10, 0, 10));
	assert(! bl::in_range(-1, 0, 10));
	assert(! bl::in_range(11, 0, 10));
}

/*
 * test_zero_if_true
 */

void test_zero_if_true() {
	assert(bl::zero_if_true(42, true) == 0);
	assert(bl::zero_if_true(42, false) == 42);
	assert(bl::zero_if_true(-7, true) == 0);
	assert(bl::zero_if_true(-7, false) == -7);
}

/*
 * test_one_if_true
 */

void test_one_if_true() {
	assert(bl::one_if_true(42, true) == 1);
	assert(bl::one_if_true(42, false) == 42);
	assert(bl::one_if_true(0, true) == 1);
	assert(bl::one_if_true(-7, false) == -7);
}

/*
 * test_add_if_true
 */

void test_add_if_true() {
	assert(bl::add_if_true(10, 5, true) == 15);
	assert(bl::add_if_true(10, 5, false) == 10);
	assert(bl::add_if_true(-10, 3, true) == -7);
	assert(bl::add_if_true(-10, 3, false) == -10);
}

/*
 * test_sub_if_true
 */

void test_sub_if_true() {
	assert(bl::sub_if_true(10, 5, true) == 5);
	assert(bl::sub_if_true(10, 5, false) == 10);
	assert(bl::sub_if_true(-10, 3, true) == -13);
	assert(bl::sub_if_true(-10, 3, false) == -10);
}

/*
 * test_inc_if_true
 */

void test_inc_if_true() {
	assert(bl::inc_if_true(10, true) == 11);
	assert(bl::inc_if_true(10, false) == 10);
	assert(bl::inc_if_true(-1, true) == 0);
	assert(bl::inc_if_true(-1, false) == -1);
}

/*
 * test_dec_if_true
 */

void test_dec_if_true() {
	assert(bl::dec_if_true(10, true) == 9);
	assert(bl::dec_if_true(10, false) == 10);
	assert(bl::dec_if_true(-1, true) == -2);
	assert(bl::dec_if_true(-1, false) == -1);
}

int main() {
	test_min();
	test_max();
	test_in_range();
	test_zero_if_true();
	test_one_if_true();
	test_add_if_true();
	test_sub_if_true();
	test_inc_if_true();
	test_dec_if_true();
}
