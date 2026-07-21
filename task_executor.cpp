/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstdint>
#include <vector>

#include "task_executor.hpp"

#include <cassert>
#include <chrono>
#include <iostream>

void test_submit_delivers_results_to_channels() {
  TaskExecutorSPSC<4> exec;

  Channel<int> channel1;
  Channel<int> channel2;
  Channel<int> channel3;
  Channel<int> channel4;
  Channel<int> channel5;
  Channel<int> channel6;
  Channel<int> channel7;
  Channel<int> channel8;

  assert(exec.try_submit(
      [](Channel<int>& channel) noexcept {
        channel.set(1);
      },
      channel1));
  assert(exec.try_submit(
      [](Channel<int>& channel) noexcept {
        channel.set(2);
      },
      channel2));
  assert(exec.try_submit(
      [](Channel<int>& channel) noexcept {
        channel.set(3);
      },
      channel3));
  assert(exec.try_submit(
      [](Channel<int>& channel) noexcept {
        channel.set(4);
      },
      channel4));
  exec.submit(
      [](Channel<int>& channel) noexcept {
        channel.set(5);
      },
      channel5);
  exec.submit(
      [](Channel<int>& channel) noexcept {
        channel.set(6);
      },
      channel6);
  exec.submit(
      [](Channel<int>& channel) noexcept {
        channel.set(7);
      },
      channel7);
  exec.submit(
      [](Channel<int>& channel) noexcept {
        channel.set(8);
      },
      channel8);

  assert(channel1.get() == 1);
  assert(channel2.get() == 2);
  assert(channel3.get() == 3);
  assert(channel4.get() == 4);
  assert(channel5.get() == 5);
  assert(channel6.get() == 6);
  assert(channel7.get() == 7);
  assert(channel8.get() == 8);
}

void test_submit_preserves_task_execution_order() {
  TaskExecutorSPSC<4> exec;

  Channel<int> channel1;
  Channel<int> channel2;
  Channel<int> channel3;
  Channel<int> channel4;

  std::array<int, 4> execution_order = {0, 0, 0, 0};
  std::size_t next = 0;

  exec.submit(
      [&](Channel<int>& channel) noexcept {
        execution_order[next++] = 1;
        channel.set(1);
      },
      channel1);

  exec.submit(
      [&](Channel<int>& channel) noexcept {
        execution_order[next++] = 2;
        channel.set(2);
      },
      channel2);

  exec.submit(
      [&](Channel<int>& channel) noexcept {
        execution_order[next++] = 3;
        channel.set(3);
      },
      channel3);

  exec.submit(
      [&](Channel<int>& channel) noexcept {
        execution_order[next++] = 4;
        channel.set(4);
      },
      channel4);

  exec.stop();

  assert(channel1.get() == 1);
  assert(channel2.get() == 2);
  assert(channel3.get() == 3);
  assert(channel4.get() == 4);

  assert(execution_order[0] == 1);
  assert(execution_order[1] == 2);
  assert(execution_order[2] == 3);
  assert(execution_order[3] == 4);
}

int main() {
  test_submit_delivers_results_to_channels();
  test_submit_preserves_task_execution_order();
}