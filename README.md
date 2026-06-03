### Copyright (C) 2026 Łukasz Czerwiński

# HFTDojo
HFT‑Dojo is a compact training ground for low‑latency systems programming. The repository is not a framework and not a library — it is a collection of precise, inspectable building blocks that illustrate how high‑performance components are designed, reasoned about, and debugged.

The exercises cover the full spectrum of latency‑sensitive techniques: branchless logic, fixed‑capacity containers, atomic ring buffers, cache‑aware layouts, and the foundations of a trade engine. Every piece is intentionally small, deterministic, and easy to explore in GDB. The goal is clarity, not completeness; understanding, not abstraction.
  
## Website
https://github.com/wo3kie/hft-dojo

## Requirements
C++20  
  
## How to build it?
cd hft-dojo
cmake --preset debug
cmake --preset release

cmake --build --preset debug
cmake --build --preset release

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
  
- **flat_queue** - A fixed‑capacity, array‑based queue with a linked‑list structure. It is designed for scenarios where the maximum number of elements is known in advance, and it provides fast enqueue and dequeue operations without dynamic memory allocation. 

  - **flat_queue** - uses a sentinel node to manage the queue, for branchless insertions and deletions. It maintains a free list of available slots, which enables it to reuse memory efficiently without fragmentation.

  - **flat_queue_oa** - An open‑addressing variant of the flat queue. It uses a different approach to manage free slots, which can be more efficient in certain scenarios, especially when slot ids can't be cached.

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
  
- **ring_buffer** - A collection of ring buffer implementations for different concurrency scenarios. Each implementation is optimized for a specific use case, providing efficient and low‑latency communication between threads.

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

- **trade_engine** - a minimalistic, single‑header matching engine built with zero external dependencies (no STL, no Boost, no runtime allocations). It implements limit orders, market orders, cancellations, and quantity updates, producing events in just a few nanoseconds. The entire design is intentionally branchless, cache‑friendly, and deterministic, making it suitable as a foundation for ultra‑low‑latency trading systems and research on high‑performance market microstructure.
  
  - **Order** containing only an `id` and `quantity` is far simpler than what an OMS typically maintains, but it is fully sufficient for a matching engine to execute trades.

  - **Orders** — a compact container holding up to eight orders. Internally it is a doubly linked list built directly on top of a flat array, providing fast insert, update, and delete operations without dynamic allocation. A dedicated sentinel node simplifies the logic and enables branchless manipulation of list links.

  - **Level** — represents a single price level and holds all orders queued at that price. When the OMS provides the correct `slot_id`, updates and cancels are resolved in constant time. If not, the engine falls back to an efficient open‑addressing hash probe to locate the order by `order_id`. Each level also tracks its aggregate quantity, which is essential for fast FOK/IOC validation.

  - **OrderBook** — the structure responsible for holding all orders across price levels. It delegates *create*, *update*, and *delete* operations to its underlying components. The book can shift its price window up or down in place, allowing efficient trend‑following adjustments without reallocating data. Orders that fall outside the shifted window are treated as expired. It uses bitmasks to track which price levels are active, enabling efficient retrieval of the best bid and ask levels in constant time.

  - **TradeEngine** — the high‑level orchestrator of the matching system. All internal parameters, such as the number of price levels and the capacity of each level, are chosen so the complete data structure resides entirely in a 32 KB L1 cache for maximum performance. Requests are submitted via API calls, and resulting events are delivered through a FIFO SPSC ring buffer. The engine handles limit orders, market orders, and advanced execution types like FOK and IOC.
  
  ```{r, engine='cpp'}
  QueueOut out;
  TradeEngine engine(out, centerPrice);

  engine.insert_order<Sell>(/* id */ 1, /* price */ 100, /* qty */ 200);
  Assert(engine.out().pop() == CreateAccepted(1, 100, 200));

  engine.insert_order<Buy>(/* id */ 2, /* price */ 100, /* qty */ 200);
  Assert(engine.out().pop() == Trade(/* price */ 100, /* qty */ 200, /* maker */ 2, /* taker */ 1));
  ```

  ```{r, engine='bash'}
  $ # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  $ ./trade_engine 
  Micro benchmark (Release): insert: 8 ns
  Micro benchmark (Release): trade: 9 ns
  Benchmark (Release)(events=11877798): 265 ms :: 22 ns/event :: 44722752 events/s
  ```
    
  - **uint256_t** — a custom 256‑bit unsigned integer type implemented in `uint256.hpp`. It is designed to be a simple wrapper around two 128‑bit integers, providing basic arithmetic and bitwise operations. The implementation is minimalistic, focusing on the specific needs of the trade engine, such as setting and getting price bits for the order book.