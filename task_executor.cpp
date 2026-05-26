/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstdint>
#include <vector>

#include "assert.hpp"
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

  Assert(exec.try_submit(
      [](Channel<int>& channel) noexcept {
        channel.set(1);
      },
      channel1));
  Assert(exec.try_submit(
      [](Channel<int>& channel) noexcept {
        channel.set(2);
      },
      channel2));
  Assert(exec.try_submit(
      [](Channel<int>& channel) noexcept {
        channel.set(3);
      },
      channel3));
  Assert(exec.try_submit(
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

  Assert(channel1.get() == 1);
  Assert(channel2.get() == 2);
  Assert(channel3.get() == 3);
  Assert(channel4.get() == 4);
  Assert(channel5.get() == 5);
  Assert(channel6.get() == 6);
  Assert(channel7.get() == 7);
  Assert(channel8.get() == 8);
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

  Assert(channel1.get() == 1);
  Assert(channel2.get() == 2);
  Assert(channel3.get() == 3);
  Assert(channel4.get() == 4);

  Assert(execution_order[0] == 1);
  Assert(execution_order[1] == 2);
  Assert(execution_order[2] == 3);
  Assert(execution_order[3] == 4);
}

int main() {
  test_submit_delivers_results_to_channels();
  test_submit_preserves_task_execution_order();
}