/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 *
 */

#include <stdexcept>
#include <utility>
#include <queue>

#include "common.hpp"

#include "ring_buffer.hpp"
#include "test_utils.hpp"

namespace {

template<int32_t Capacity>
void test_push_pop() {
  RingBuffer<int, Capacity> actual;
  std::queue<int> expected;

  for(int32_t i = 0; i < Capacity / 2; i += 1) {
    actual.push(i);
    expected.push(i);
    assert(actual._ext_equal(expected));
  }

  for(int32_t i = 0; i < Capacity; i += 1) {
    int out;

    actual.push(i);
    expected.push(i);
    assert(actual._ext_equal(expected));

    actual.pop(out);
    expected.pop();
    assert(actual._ext_equal(expected));
  }

  for(int32_t i = 0; i < Capacity / 2; i += 1) {
    int out;

    actual.pop(out);
    expected.pop();
    assert(actual._ext_equal(expected));
  }
}

} // namespace

/*
 * main
 */

int main() {
  test_push_pop<8>();
  test_push_pop<15>();
  test_push_pop<16>();
  test_push_pop<17>();
  test_push_pop<1023>();
  test_push_pop<1024>();
}
