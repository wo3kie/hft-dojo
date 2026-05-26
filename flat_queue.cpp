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

  queue.push(1);
  assert(queue.empty() == false);

  queue.push(2);
  queue.push(3);
  queue.push(4);
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
