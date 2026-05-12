/*
 * Project:
 *      CxxDojo (https://github.com/wo3kie/cpp-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "./object_pool.hpp"
#include "./assert.hpp"

int main()
{
  ObjectPool<int, 10> pool;

  int* a = pool.allocate();
  int* b = pool.allocate();

  *a = 42;
  *b = 24;

  Assert(*a == 42);
  Assert(*b == 24);

  pool.deallocate(a);
  pool.deallocate(b);

  return 0;
}
