#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <future>
#include <memory>
#include <type_traits>
#include <utility>

#include "./task_worker.hpp"

/*
 * Channel
 */

template<class TType, typename IdlePolicy = PauseIdlePolicy>
struct Channel {
public:
  Channel() = default;

  Channel(Channel&&) = delete;
  Channel(const Channel&) = delete;

  Channel& operator=(Channel&&) = delete;
  Channel& operator=(const Channel&) = delete;

public:
  void set(TType value) {
    _value = std::move(value);
    _ready.store(true, std::memory_order_release);
  }

  TType& get() {
    while(! _ready.load(std::memory_order_acquire)) {
      IdlePolicy::doIt();
    }

    return _value;
  }

private:
  TType _value;
  std::atomic<bool> _ready{false};
};

/*
 * TaskExecutorSPSC
 */

template<std::size_t QueueSize, typename IdlePolicy = PauseIdlePolicy>
class TaskExecutorSPSC {
public:
  explicit TaskExecutorSPSC(std::size_t cpuAffinity = NoAffinity)
    : _worker(cpuAffinity) {
  }

  TaskExecutorSPSC(TaskExecutorSPSC&&) = delete;
  TaskExecutorSPSC(const TaskExecutorSPSC&) = delete;

  ~TaskExecutorSPSC() = default;

  TaskExecutorSPSC& operator=(TaskExecutorSPSC&&) = delete;
  TaskExecutorSPSC& operator=(const TaskExecutorSPSC&) = delete;

public:
  template<class F, typename T>
  bool try_submit(F&& f, Channel<T>& channel) {
    assert(_worker.running_approx());

    if(_worker.full_approx()) {
      return false;
    }

    std::function<void()> task = [f = std::forward<F>(f), &channel]() mutable {
      static_assert(noexcept(f(channel)));
      f(channel);
    };

    assert(_worker.push(std::move(task)));
    return true;
  }

  template<class F, typename T>
  void submit(F&& f, Channel<T>& channel) {
    while(! try_submit(std::forward<F>(f), channel)) {
      IdlePolicy::doIt();
    }
  }

  void stop() {
    _worker.stop();
  }

  void hard_stop() {
    _worker.hard_stop();
  }

private:
  TaskWorkerSPSC<QueueSize, std::function<void()>, IdlePolicy> _worker;
};
