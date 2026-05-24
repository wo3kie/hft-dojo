/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "array.hpp"
#include "assert.hpp"

void test_array()
{
  Array<int, 32> a;

  for(int i = -16; i < 16; ++i) {
    a[i] = i;
  }

  for(int i = -16; i < 16; ++i) {
    Assert(a[i] == i);
  }
}

int main()
{
  test_array();
}
