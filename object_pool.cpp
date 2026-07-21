/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "object_pool.hpp"

namespace {

void test_allocate_deallocate() {
  ObjectPool<int, 4> pool;

  assert(pool.empty());
  assert(pool.full() == false);
  assert(pool.capacity() == 4);

  int s0 = pool.allocate();
  int s1 = pool.allocate();
  int s2 = pool.allocate();
  int s3 = pool.allocate();

  assert(s0 == 0);
  assert(s1 == 1);
  assert(s2 == 2);
  assert(s3 == 3);

  assert(pool.empty() == false);
  assert(pool.full());
  
  pool.deallocate(s1);
  pool.deallocate(s3);

  assert(pool.empty() == false);
  assert(pool.full() == false);

  int s4 = pool.allocate();
  int s5 = pool.allocate();

  assert(s4 == s3);
  assert(s5 == s1);

  assert(pool.empty() == false);
  assert(pool.full());
  pool.deallocate(s0);
  pool.deallocate(s2);
  pool.deallocate(s4);
  pool.deallocate(s5);
  assert(pool.empty());
  assert(pool.full() == false);
}

}

int main() {
  test_allocate_deallocate();
}
