/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "./branchless.hpp"
#include "./assert.hpp"

void test_min()
{
  Assert(bl::min(3, 7) == 3);
  Assert(bl::min(7, 3) == 3);
  Assert(bl::min(-5, -3) == -5);
  Assert(bl::min(0, 10) == 0);
  Assert(bl::min(-10, 0) == -10);
}

void test_max()
{
  Assert(bl::max(3, 7) == 7);
  Assert(bl::max(7, 3) == 7);
  Assert(bl::max(-5, -3) == -3);
  Assert(bl::max(0, 10) == 10);
  Assert(bl::max(-10, 0) == 0);
}

void test_in_range()
{
  Assert(bl::in_range(9, 10, 20) == false);
  Assert(bl::in_range(10, 10, 20) == true);
  Assert(bl::in_range(11, 10, 20) == true);
  Assert(bl::in_range(19, 10, 20) == true);
  Assert(bl::in_range(20, 10, 20) == true);
  Assert(bl::in_range(21, 10, 20) == false);

  Assert(bl::in_range(-9, -20, -10) == false);
  Assert(bl::in_range(-10, -20, -10) == true);
  Assert(bl::in_range(-11, -20, -10) == true);
  Assert(bl::in_range(-19, -20, -10) == true);
  Assert(bl::in_range(-20, -20, -10) == true);
  Assert(bl::in_range(-21, -20, -10) == false);
}

int main()
{
  test_min();
  test_max();
  test_in_range();
}
