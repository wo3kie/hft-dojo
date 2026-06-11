/*
* Project:
*      HFTDojo (https://github.com/wo3kie/hft-dojo)
*
* Author:
*      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
*/

#include "flat_queue.hpp"
#include "test_utils.hpp"

void test_flat_queue_oa() {
  FlatQueue<int, 4> queue;
  assert(queue.empty());

  queue.push(0, 1);
  assert(queue.empty() == false);

  queue.push(1, 2);
  queue.push(2, 3);
  queue.push(3, 4);
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

#ifdef NDEBUG
  bench_flat_queue<FlatQueue<int, 100'000>>("FlatQueue");
  bench_flat_queue<FlatQueue<int, 100'000>>("FlatQueue");
  bench_flat_queue<FlatQueue<int, 100'000>>("FlatQueue");
  bench_flat_queue<FlatQueue<int, 100'000>>("FlatQueue");
  bench_flat_queue<FlatQueue<int, 100'000>>("FlatQueue");
#else
  bench_flat_queue<FlatQueue<int, 10'000>>("FlatQueue");
#endif
}
