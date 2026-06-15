/*
* Project:
*      HFTDojo (https://github.com/wo3kie/hft-dojo)
*
* Author:
*      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
*/

#include "flat_queue.hpp"
#include <cassert>
#include <list>

namespace {

void test_empty_queue() {
  FlatQueue<int, 4> queue;

  assert(queue.capacity() == 4);
  queue._ext_equal({});
  assert(queue.find(42) == -1);
}

void test_push_pop_fifo_order() {
  FlatQueue<int, 4> queue;

  queue.push(2, 20);
  queue._ext_equal({20});

  queue.push(0, 10);
  queue._ext_equal({20, 10});

  queue.push(3, 30);
  queue._ext_equal({20, 10, 30});

  queue.pop();
  queue._ext_equal({10, 30});

  queue.pop();
  queue._ext_equal({30});

  queue.pop();
  queue._ext_equal({});
}

void test_erase_middle_and_reuse_slot() {
  FlatQueue<int, 5> queue;

  queue.push(4, 40);
  queue.push(1, 10);
  queue.push(3, 30);
  queue.push(0, 0);
  queue._ext_equal({40, 10, 30, 0});

  queue.erase(queue.find(10));
  queue._ext_equal({40, 30, 0});

  queue.push(1, 10);
  queue._ext_equal({40, 30, 0, 10});

  queue.erase(queue.find(40));
  queue._ext_equal({30, 0, 10});

  queue.erase(queue.find(10));
  queue._ext_equal({30, 0});

  queue.erase(queue.find(0));
  queue._ext_equal({30});

  queue.erase(queue.find(30));
  queue._ext_equal({});
}

} // namespace

int main() {
  test_empty_queue();
  test_push_pop_fifo_order();
  test_erase_middle_and_reuse_slot();
}
