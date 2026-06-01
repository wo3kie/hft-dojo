/*
* Project:
*      HFTDojo (https://github.com/wo3kie/hft-dojo)
*
* Author:
*      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
*/

#include "flat_queue_oa.hpp"

void test_flat_queue_oa() {
  FlatQueue_OA<int, 4> queue;
  assert(queue.empty());

  queue.insert(0, 1);
  assert(queue.empty() == false);

  queue.insert(1, 2);
  queue.insert(2, 3);
  queue.insert(3, 4);
  assert(queue.full());

  assert(queue.front() == 1);
  queue.pop();

  assert(queue.front() == 2);
  queue.pop();

  assert(queue.front() == 3);
  queue.pop();

  assert(queue.front() == 4);
  queue.pop();

  assert(queue.empty());
}

int main() {
  test_flat_queue_oa();
}
