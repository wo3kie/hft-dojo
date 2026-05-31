### Copyright (C) 2026 Łukasz Czerwiński

# HFTDojo
HFT‑Dojo is a compact training ground for low‑latency systems programming. Each module is a self‑contained kata focused on one idea: predictable execution, minimal abstractions, and data structures that behave well under pressure. The repository is not a framework and not a library — it is a collection of precise, inspectable building blocks that illustrate how high‑performance components are designed, reasoned about, and debugged.

The exercises cover the full spectrum of latency‑sensitive techniques: branchless logic, fixed‑capacity containers, atomic ring buffers, cache‑aware layouts, and the foundations of a trade engine. Every piece is intentionally small, deterministic, and easy to explore in GDB. The goal is clarity, not completeness; understanding, not abstraction.

HFT‑Dojo is a place to practice the craft: to read code that does exactly what it says, to experiment with micro‑optimizations, and to build intuition for how real‑time systems behave at the machine level. It’s a sandbox for learning how to write code that runs predictably fast, even under the most demanding conditions.
  
## Website
https://github.com/wo3kie/hft-dojo

## Requirements
C++20  
  
## How to build it?
cd hft-dojo
cmake --preset debug
cmake --build --preset debug

This builds unit tests under `build/debug/bin/`.

For an optimized build:
cmake --preset release
cmake --build --preset release

This builds benchmarks under `build/release/bin/`.

## How to run tests?
ctest --preset debug

From the project root you can also run everything with one command:
./test

To run a single test binary through CTest:
ctest --preset debug -R trade_engine

You can pass extra CTest arguments through the wrapper, for example:
./test -R trade_engine

## How to run benchmarks?
cmake --preset release
cmake --build --preset release
./build/release/bin/trade_engine_bench

## How to clean it?
rm -rf build

Debug artifacts are generated under `build/debug/` and release artifacts under `build/release/`.

## Content
  
- **assert** - Improved assert that accepts simple comparison expressions (`==`, `!=`, `<`, `<=`, `>`, `>=`) and prints both actual and expected values on failure.
  
  ```{r, engine='bash'}
  $ cat assert.cpp
  #include "./assert.hpp"
  int main() {
    int a = 1;
    int b = 2;
    Assert(a == b);
  }
  ```
  
- **gdb_utils** - GDB helpers for inspecting the internal state of the data structures in this repository.

  ```{r, engine='bash'}
  (gdb) source ../gdb_utils.py

  (gdb) print_ring_buffer rBuffer
  RingBuffer<int, 128> [ 0, 1, 2, 3, ..., 124, 125, 126, 127 ]
  
  (gdb) print_ring_buffer_spsc rBuffer
  RingBufferSPSC<int, 128> [ 0, 1, 2, 3, ..., 124, 125, 126, 127 ]
  
  (gdb) print_ring_buffer_spmc rBuffer
  RingBufferSPMC<int, 128> [ 0, 1, 2, 3, ..., 124, 125, 126, 127 ]
  ```
    
- **map_reduce** - An implementation of the parallel map and serial reduce paradigm in C++.
  
- **ring_buffer** - Single‑threaded ring buffer. It is designed for scenarios where only one thread is producing and consuming data, so it does not include any synchronization mechanisms. This makes it very fast and efficient for single‑threaded use cases.
  
- **ring_buffer_mutex** - Multi‑producer/multi‑consumer ring buffer using mutexes. It is designed for scenarios where multiple threads are producing and consuming data, so it uses mutexes to synchronize access to the buffer. This allows it to be used in multi‑threaded environments, but it may have higher latency compared to the single‑threaded version due to the overhead of locking and unlocking the mutexes.
  
- **ring_buffer_spsc** - Single‑producer/single‑consumer ring buffer using atomics. It is designed for scenarios where one thread is producing data and another thread is consuming data, so it uses atomic operations to synchronize access to the buffer. This allows it to be used in multi‑threaded environments while still maintaining low latency, as it avoids the overhead of mutexes.
  
- **ring_buffer_spmc** - Single‑producer/multiple‑consumer ring buffer using atomics. It is designed for scenarios where one thread is producing data and multiple threads are consuming data, so it uses atomic operations to synchronize access to the buffer. This allows it to be used in multi‑threaded environments while still maintaining low latency, as it avoids the overhead of mutexes.
  
- **task_executor** - Executes a function in a separate thread (optional thread affinity). Result retrieved via `Channel::get` (optional thread affinity).
  
  ```{r, engine='cpp'}
  TaskExecutorSPSC<4> exec;

  Channel<int> channel1;
  Channel<int> channel2;
  Channel<int> channel3;
  Channel<int> channel4;

  Assert( exec.try_submit([]( Channel<int>& channel ) noexcept { channel.set(1); }, channel1 ) );
  Assert( exec.try_submit([]( Channel<int>& channel ) noexcept { channel.set(2); }, channel2 ) );
  Assert( exec.try_submit([]( Channel<int>& channel ) noexcept { channel.set(3); }, channel3 ) );
  Assert( exec.try_submit([]( Channel<int>& channel ) noexcept { channel.set(4); }, channel4 ) );

  Assert(channel1.get() == 1);
  Assert(channel2.get() == 2);
  Assert(channel3.get() == 3);
  Assert(channel4.get() == 4);

  ```

- **task_worker** -  Executes a void procedure in a separate thread (optional thread affinity).

  ```{r, engine='cpp'}
  TaskWorkerSPSC<4, std::function<void()>> worker;

  Assert(worker.push([]() noexcept { /* do something */ }));
  Assert(worker.push([]() noexcept { /* do something */ }));
  Assert(worker.push([]() noexcept { /* do something */ }));
  Assert(worker.push([]() noexcept { /* do something */ }));
  ```

- **trade_engine** - Ultra‑fast trade engine example that generates events in nanoseconds. Supports limit orders, market orders, cancellation, and updates. The trade logic is designed to be branchless and cache‑friendly, making it suitable for high‑performance trading systems.
  
  ```{r, engine='cpp'}
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<Sell>(/* id */ 1, /* price */ 100, /* qty */ 200);
  Assert(engine.out().pop() == CreateAccepted(1, 100, 200));

  engine.insert_order<Buy>(/* id */ 2, /* price */ 100, /* qty */ 200);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 200, /* maker */ 2, /* taker */ 1));
  ```

  ```{r, engine='bash'}
  $ ./trade_engine 
  Micro benchmark (Release): insert: 8ns
  Micro benchmark (Release): trade: 12ns
  ```
  