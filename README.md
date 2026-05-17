### Copyright (C) 2026 Łukasz Czerwiński

# HFTDojo
HFT‑Dojo is a compact training ground for low‑latency systems programming. Each module is a self‑contained kata focused on one idea: predictable execution, minimal abstractions, and data structures that behave well under pressure. The repository is not a framework and not a library — it is a collection of precise, inspectable building blocks that illustrate how high‑performance components are designed, reasoned about, and debugged.

The exercises cover the full spectrum of latency‑sensitive techniques: branchless logic, fixed‑capacity containers, atomic ring buffers, cache‑aware layouts, and the foundations of a matching engine. Every piece is intentionally small, deterministic, and easy to explore in GDB. The goal is clarity, not completeness; understanding, not abstraction.

HFT‑Dojo is a place to practice the craft: to read code that does exactly what it says, to experiment with micro‑optimizations, and to build intuition for how real‑time systems behave at the machine level. It’s a sandbox for learning how to write code that runs predictably fast, even under the most demanding conditions.
  
## Website
https://github.com/wo3kie/hft-dojo

## Requirements
C++20  
  
## How to build it?
mkdir build  
cd build
cmake ..  
cmake --build .  

## How to clean it?
rm -rf build

## Content
  
- **array** - Fixed‑size array with positive/negative/modulo indexing. For best performance, the size should be a power of two to enable bitwise indexing.
  
  ```{r, engine='cpp'}
  Array<int 8> arr = {0, 1, 2, 3, 4, 5, 6, 7};
  assert(arr[ 0] == 0);
  assert(arr[-1] == 7);
  ```

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
- **branchless** - A small collection of branchless functions like `min`, `max`, `abs`...

- **flat_list** - A doubly‑linked flat list with fixed maximum capacity. All operations (`push_front`, `push_back`, `pop_front`, `pop_back`, `remove`) run in O(1) time.

  ```{r, engine='cpp'}
  FlatList<int, 8> list;
  
  list.push_front(1);
  assert(list.front() == 1);

  list.push_back(2);
  assert(list.back() == 2);
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
  
- **likely** - `LIKELY` and `UNLIKELY` macros that can be used to provide branch prediction hints to the compiler.
  
  ```{r, engine='cpp'}
  if(UNLIKELY(qty != 0)) {
    _orderBook.insertSellOrder(orderId, price, qty);
  }
  ```
  
- **map_reduce** - An implementation of the parallel map and serial reduce paradigm in C++.
  
- **matching_engine** - Ultra‑fast matching engine example that generates events in nanoseconds. Supports limit orders, market orders, cancellation, and updates. The matching logic is designed to be branchless and cache‑friendly, making it suitable for high‑performance trading systems.
  
```{r, engine='cpp'}
    MatchingEngine</* LevelsBelow */ 3, /* LevelsAbove */ 4> engine(/* initialPrice */ 100);

    engine.insertBuyOrder_PL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == CreateAccepted(/* orderId */ 1, /* slot */ 0));

    engine.insertSellOrder_MKT(2, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(/* price */ 100, /* qty */ 200, /* maker */ 2, /* taker */ 1));
```
  
- **order_book** -  Simple limit order book with add/remove/update operations and a sliding‑window mechanism for tracking price trends. The order book is designed to be fast and efficient, with a focus on low latency and high throughput. It supports both buy and sell orders, and can be used as a building block for more complex trading systems.
  
  ```{r, engine='cpp'}

  OrderBook</* LevelsBelow */ 31, /* LevelsAbove */ 32> ob(buffer, /* initialPrice */ 100);

  ob.insertBuyOrder(/* orderId */ 1, /* price */ 100, /* qty */ 10);
  Assert(ob.bufferOut().pop() == CreateAccepted(/* orderId */ 1, /* slot */ 0));
  
  ob.updateBuyOrder(/* orderId */ 1, /* slot */ 0, /* qty */ 20);
  Assert(ob.bufferOut().pop() == UpdateAccepted(/* orderId */ 1));
  ```

- **price_levels** - An implementation of a price level data structure that can be used to store orders at a specific price level in a limit order book.
  
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