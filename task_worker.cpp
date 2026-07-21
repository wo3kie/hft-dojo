/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <functional>
#include <iostream>
#include <thread>

#include "task_worker.hpp"

void test_stop_drains_all_queued_tasks() {
  std::size_t counter = 0;

  {
    TaskWorkerSPSC<4, std::function<void()>> worker;

    for(std::size_t i = 0; i < 1024; ++i) {
      while(! worker.push([i, &counter]() {
        counter += 1;
      })) {
        _mm_pause();
      };
    }

    worker.stop();
  }

  assert(counter == 1024);
}

void test_hard_stop_skips_queued_tasks_after_current_task() {
  std::atomic<std::size_t> counter{0};
  std::atomic<bool> current_task_started{false};
  std::atomic<bool> release_current_task{false};

  {
    TaskWorkerSPSC<4, std::function<void()>, YieldIdlePolicy> worker;

    while(! worker.push([&]() {
      current_task_started.store(true, std::memory_order_release);

      while(! release_current_task.load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }

      counter.fetch_add(1, std::memory_order_relaxed);
    })) {
      std::this_thread::yield();
    }

    while(! worker.push([&]() {
      counter.fetch_add(100, std::memory_order_relaxed);
    })) {
      std::this_thread::yield();
    }

    while(! worker.push([&]() {
      counter.fetch_add(1000, std::memory_order_relaxed);
    })) {
      std::this_thread::yield();
    }

    while(! current_task_started.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }

    worker.hard_stop();
    release_current_task.store(true, std::memory_order_release);
  }

  assert(counter.load(std::memory_order_relaxed) == 1);
}

int main() {
  test_stop_drains_all_queued_tasks();
  test_hard_stop_skips_queued_tasks_after_current_task();
}
