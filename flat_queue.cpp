/*
* Project:
*      HFTDojo (https://github.com/wo3kie/hft-dojo)
*
* Author:
*      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
*/

#include "flat_queue.hpp"

void test_flat_queue() {
  FlatQueue<int, 4> queue;
  assert(queue.empty());

  queue.insert(1);
  assert(queue.empty() == false);

  queue.insert(2);
  queue.insert(3);
  queue.insert(4);
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
  test_flat_queue();
}
