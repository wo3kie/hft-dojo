/*
* Project:
*      HFTDojo (https://github.com/wo3kie/hft-dojo)
*
* Author:
*      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
*/

#include "assert.hpp"
#include "branchless.hpp"

/*
 * test_min
 */

void test_min() {
	Assert(bl::min(3, 7) == 3);
	Assert(bl::min(7, 3) == 3);
	Assert(bl::min(-3, 2) == -3);
	Assert(bl::min(-8, -2) == -8);
	Assert(bl::min(5, 5) == 5);
}

/*
 * test_max
 */

void test_max() {
	Assert(bl::max(3, 7) == 7);
	Assert(bl::max(7, 3) == 7);
	Assert(bl::max(-3, 2) == 2);
	Assert(bl::max(-8, -2) == -2);
	Assert(bl::max(5, 5) == 5);
}

/*
 * test_in_range
 */

void test_in_range() {
	Assert(bl::in_range(5, 0, 10));
	Assert(bl::in_range(0, 0, 10));
	Assert(bl::in_range(10, 0, 10));
	Assert(! bl::in_range(-1, 0, 10));
	Assert(! bl::in_range(11, 0, 10));
}

/*
 * test_zero_if_true
 */

void test_zero_if_true() {
	Assert(bl::zero_if_true(42, true) == 0);
	Assert(bl::zero_if_true(42, false) == 42);
	Assert(bl::zero_if_true(-7, true) == 0);
	Assert(bl::zero_if_true(-7, false) == -7);
}

/*
 * test_one_if_true
 */

void test_one_if_true() {
	Assert(bl::one_if_true(42, true) == 1);
	Assert(bl::one_if_true(42, false) == 42);
	Assert(bl::one_if_true(0, true) == 1);
	Assert(bl::one_if_true(-7, false) == -7);
}

/*
 * test_add_if_true
 */

void test_add_if_true() {
	Assert(bl::add_if_true(10, 5, true) == 15);
	Assert(bl::add_if_true(10, 5, false) == 10);
	Assert(bl::add_if_true(-10, 3, true) == -7);
	Assert(bl::add_if_true(-10, 3, false) == -10);
}

/*
 * test_sub_if_true
 */

void test_sub_if_true() {
	Assert(bl::sub_if_true(10, 5, true) == 5);
	Assert(bl::sub_if_true(10, 5, false) == 10);
	Assert(bl::sub_if_true(-10, 3, true) == -13);
	Assert(bl::sub_if_true(-10, 3, false) == -10);
}

/*
 * test_inc_if_true
 */

void test_inc_if_true() {
	Assert(bl::inc_if_true(10, true) == 11);
	Assert(bl::inc_if_true(10, false) == 10);
	Assert(bl::inc_if_true(-1, true) == 0);
	Assert(bl::inc_if_true(-1, false) == -1);
}

/*
 * test_dec_if_true
 */

void test_dec_if_true() {
	Assert(bl::dec_if_true(10, true) == 9);
	Assert(bl::dec_if_true(10, false) == 10);
	Assert(bl::dec_if_true(-1, true) == -2);
	Assert(bl::dec_if_true(-1, false) == -1);
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
