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

- **branchless** - A collection of branchless functions like `min`, `max`... These functions are implemented using bitwise operations and arithmetic to avoid branching, which can improve performance in certain scenarios by reducing pipeline stalls.  
    
- **flat_hash** - A fixed‑capacity, array‑based open‑addressing hash map. It is designed for scenarios where the maximum number of elements is known in advance, and it provides fast insertions, deletions, and lookups without dynamic memory allocation. The map maintains a free list of available slots, which enables it to reuse memory efficiently without fragmentation.

  ```{r, engine='bash'}
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release)            FlatHash: (iters=131072): 683 μs :: 5 ns/iter :: 191882977 iter/s
  Benchmark (Release)            FlatHash: (iters=131072): 584 μs :: 4 ns/iter :: 224124428 iter/s
  Benchmark (Release)            FlatHash: (iters=131072): 411 μs :: 3 ns/iter :: 318787424 iter/s
  Benchmark (Release)            FlatHash: (iters=131072): 419 μs :: 3 ns/iter :: 312812790 iter/s
  Benchmark (Release)            FlatHash: (iters=131072): 388 μs :: 2 ns/iter :: 337572563 iter/s
  ```

- **flat_list** - A fixed‑capacity, array‑based linked list. It is designed for scenarios where the maximum number of elements is known in advance.

  ```{r, engine='bash'}
  Benchmark (Release)       FlatList: (iters=100000): 240 μs :: 2 ns/iter :: 416375204 iter/s
  Benchmark (Release)       FlatList: (iters=100000): 241 μs :: 2 ns/iter :: 413703515 iter/s
  Benchmark (Release)       FlatList: (iters=100000): 246 μs :: 2 ns/iter :: 405924879 iter/s
  Benchmark (Release)       FlatList: (iters=100000): 304 μs :: 3 ns/iter :: 328945204 iter/s
  Benchmark (Release)       FlatList: (iters=100000): 241 μs :: 2 ns/iter :: 414801786 iter/s
  ```
    
- **flat_queue** - It uses an open‑addressing approach to manage free slots, which can be more efficient in certain scenarios, especially when slot `id`s can't be cached.

  ```{r, engine='bash'}
  Benchmark (Release)      FlatQueue: (iters=100000): 769 μs :: 7 ns/iter :: 129882443 iter/s
  Benchmark (Release)      FlatQueue: (iters=100000): 578 μs :: 5 ns/iter :: 172757819 iter/s
  Benchmark (Release)      FlatQueue: (iters=100000): 578 μs :: 5 ns/iter :: 172796328 iter/s
  Benchmark (Release)      FlatQueue: (iters=100000): 590 μs :: 5 ns/iter :: 169407395 iter/s
  Benchmark (Release)      FlatQueue: (iters=100000): 629 μs :: 6 ns/iter :: 158804771 iter/s
  ```

- **flat_tree_bs** - The simplest binary-search variant of the flat tree without nodes balancing. It is designed as a base for balanced versions, but it can be useful on its own in scenarios where the input data is mostly random.

  ```{r, engine='cpp'}
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark    FlatTreeBS: (Release)(iters=100000): 77 ms :: 773 ns/iter :: 1292312 iter/s
  Benchmark    FlatTreeBS: (Release)(iters=100000): 76 ms :: 763 ns/iter :: 1310497 iter/s
  Benchmark    FlatTreeBS: (Release)(iters=100000): 77 ms :: 776 ns/iter :: 1287738 iter/s
  Benchmark    FlatTreeBS: (Release)(iters=100000): 76 ms :: 767 ns/iter :: 1303581 iter/s
  Benchmark    FlatTreeBS: (Release)(iters=100000): 77 ms :: 778 ns/iter :: 1284413 iter/s
  ```

- **flat_tree_avl** - An AVL‑balanced variant of the flat tree. It maintains the AVL balance property, which ensures that the tree remains balanced after insertions and deletions, providing O(log n) time complexity for insertions, lookups and deletions. It is implemented as a subclass of the binary‑search variant, reusing its core logic and adding the necessary rotations and balance factor updates to maintain the AVL property.

  ```{r, engine='cpp'}
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark   FlatTreeAVL: (Release)(iters=100000): 5 ms :: 50 ns/iter :: 19888497 iter/s
  Benchmark   FlatTreeAVL: (Release)(iters=100000): 4 ms :: 49 ns/iter :: 20275704 iter/s
  Benchmark   FlatTreeAVL: (Release)(iters=100000): 4 ms :: 49 ns/iter :: 20064157 iter/s
  Benchmark   FlatTreeAVL: (Release)(iters=100000): 5 ms :: 51 ns/iter :: 19413363 iter/s
  Benchmark   FlatTreeAVL: (Release)(iters=100000): 5 ms :: 50 ns/iter :: 19876815 iter/s
  ```

- **flat_tree_splay** - A splay‑tree variant of the flat tree. It is a self‑adjusting binary search tree that performs splaying operations to keep recently accessed elements near the root, which can improve access times for certain access patterns. It is designed for scenarios where there is a high degree of locality in the access patterns, as it can provide faster access to recently accessed elements compared to other balanced tree structures.

  ```{r, engine='cpp'}
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark FlatTreeSplay: (Release)(iters=100000): 8 ms :: 80 ns/iter :: 12440871 iter/s
  Benchmark FlatTreeSplay: (Release)(iters=100000): 8 ms :: 82 ns/iter :: 12100216 iter/s
  Benchmark FlatTreeSplay: (Release)(iters=100000): 8 ms :: 81 ns/iter :: 12288132 iter/s
  Benchmark FlatTreeSplay: (Release)(iters=100000): 8 ms :: 81 ns/iter :: 12270715 iter/s
  Benchmark FlatTreeSplay: (Release)(iters=100000): 8 ms :: 82 ns/iter :: 12124596 iter/s
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
  
- **object_pool** - A fixed‑capacity pool of pre‑allocated objects. It is designed for scenarios where the maximum number of objects is known in advance, and it provides fast allocation and deallocation with one dynamic memory allocation. The pool maintains a free list of available slots, which enables it to reuse memory efficiently without fragmentation.

- **price_bits** — a compact bitmask structure that tracks the occupancy of price levels in the order book. It provides efficient methods for setting, clearing, and querying bits, as well as counting leading zeros to quickly identify the best bid and ask levels.
  
- **random** - A simple linear congruential generator (LCG) for producing pseudo‑random numbers. It is designed for scenarios where a fast and deterministic random number generator is needed, such as in testing and benchmarking.

- **ring_buffer** - Single‑threaded ring buffer. It is designed for scenarios where only one thread is producing and consuming data, so it does not include any synchronization mechanisms.

  ```{r, engine='bash'}
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release)          RingBuffer: (iters=131072): 334 μs :: 2 ns/iter :: 392091872 iter/s
  Benchmark (Release)          RingBuffer: (iters=131072): 304 μs :: 2 ns/iter :: 430194203 iter/s
  Benchmark (Release)          RingBuffer: (iters=131072): 201 μs :: 1 ns/iter :: 651895177 iter/s
  Benchmark (Release)          RingBuffer: (iters=131072): 200 μs :: 1 ns/iter :: 653905061 iter/s
  Benchmark (Release)          RingBuffer: (iters=131072): 198 μs :: 1 ns/iter :: 659554768 iter/s
  ```

- **ring_buffer_mutex** - Multi‑producer/multi‑consumer ring buffer using mutexes. It is designed for scenarios where multiple threads are producing and consuming data, so it uses mutexes to synchronize access to the buffer. This allows it to be used in multi‑threaded environments, but it may have higher latency compared to the single‑threaded version due to the overhead of locking and unlocking the mutexes.
  
  ```{r, engine='bash'}
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release)     RingBufferMutex: (iters=131072): 2145 μs :: 16 ns/iter :: 61090846 iter/s
  Benchmark (Release)     RingBufferMutex: (iters=131072): 1843 μs :: 14 ns/iter :: 71106713 iter/s
  Benchmark (Release)     RingBufferMutex: (iters=131072): 1726 μs :: 13 ns/iter :: 75898190 iter/s
  Benchmark (Release)     RingBufferMutex: (iters=131072): 1725 μs :: 13 ns/iter :: 75981125 iter/s
  Benchmark (Release)     RingBufferMutex: (iters=131072): 1813 μs :: 13 ns/iter :: 72271206 iter/s
  ```

- **ring_buffer_spsc** - Single‑producer/single‑consumer ring buffer using atomics. It is designed for scenarios where one thread is producing data and another thread is consuming data, so it uses atomic operations to synchronize access to the buffer. This allows it to be used in multi‑threaded environments while still maintaining low latency, as it avoids the overhead of mutexes.
  
  ```{r, engine='bash'}
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release)      RingBufferSPSC: (iters=131072): 545 μs :: 4 ns/iter :: 240424087 iter/s
  Benchmark (Release)      RingBufferSPSC: (iters=131072): 497 μs :: 3 ns/iter :: 263594825 iter/s
  Benchmark (Release)      RingBufferSPSC: (iters=131072): 388 μs :: 2 ns/iter :: 337537791 iter/s
  Benchmark (Release)      RingBufferSPSC: (iters=131072): 388 μs :: 2 ns/iter :: 337647349 iter/s
  Benchmark (Release)      RingBufferSPSC: (iters=131072): 384 μs :: 2 ns/iter :: 340754760 iter/s
  ```

- **ring_buffer_spmc** - Single‑producer/multiple‑consumer ring buffer using atomics. It is designed for scenarios where one thread is producing data and multiple threads are consuming data, so it uses atomic operations to synchronize access to the buffer. This allows it to be used in multi‑threaded environments while still maintaining low latency, as it avoids the overhead of mutexes.

  ```{r, engine='bash'}
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release)      RingBufferSPMC: (iters=131072): 1555 μs :: 11 ns/iter :: 84279022 iter/s
  Benchmark (Release)      RingBufferSPMC: (iters=131072): 1540 μs :: 11 ns/iter :: 85104283 iter/s
  Benchmark (Release)      RingBufferSPMC: (iters=131072): 1464 μs :: 11 ns/iter :: 89524123 iter/s
  Benchmark (Release)      RingBufferSPMC: (iters=131072): 1470 μs :: 11 ns/iter :: 89161957 iter/s
  Benchmark (Release)      RingBufferSPMC: (iters=131072): 1854 μs :: 14 ns/iter :: 70686310 iter/s
  ```

- **task_executor** - Executes a function in a separate thread (optional thread affinity). Result retrieved via `Channel::get` (optional thread affinity).
    
- **storage** - a utility providing a contiguous block of memory for fixed‑capacity containers. It uses an internal array for small capacities (up to 1024 elements) to avoid dynamic memory allocation and heap fragmentation, and it falls back to dynamic allocation for larger capacities.
  
  ```{r, engine='cpp'}
  Storage<int, 1024> on_stack;
  Storage<int, 1025> on_heap;
  ```

- **task_worker** -  Executes a void procedure in a separate thread (optional thread affinity).

- **trade_engine** - a minimalistic, single‑header matching engine built with zero external dependencies (no STL, no Boost, no runtime allocations). It implements limit orders, market orders, FOK, IOC, insert, quantity update and cancellation, producing events in just a few nanoseconds. The entire design is intentionally branchless, cache‑friendly, and deterministic, making it suitable as a foundation for ultra‑low‑latency trading systems and research on high‑performance market microstructure.
  
  - **Order** containing only an `id` and `quantity` is far simpler than what an OMS typically maintains, but it is fully sufficient for a matching engine to execute trades.

  - **Orders** — a compact container holding up to eight orders. Internally it is a doubly linked list built directly on top of a flat array, providing fast insert, update, and delete operations without dynamic allocation. A dedicated sentinel node simplifies the logic and enables branchless manipulation of list links.

  - **Level** — represents a single price level and holds all orders queued at that price. When the OMS provides the correct `slot_id`, updates and cancels are resolved in constant time. If not, the engine falls back to an efficient open‑addressing hash probe to locate the order by `order_id`. Each level also tracks its aggregate quantity, which is essential for fast FOK/IOC validation.

  - **OrderBook** — the structure responsible for holding all orders across price levels. It delegates *create*, *update*, and *delete* operations to its underlying components. The book can shift its price window up or down in place, allowing efficient trend‑following adjustments without reallocating data. Orders that fall outside the shifted window are treated as expired. It uses bitmasks to track which price levels are active, enabling efficient retrieval of the best bid and ask levels in constant time.
    
  - **TradeEngine** — the high‑level orchestrator of the matching system. All internal parameters, such as the number of price levels and the capacity of each level, are chosen so the complete data structure resides entirely in a 32 KB L1 cache for maximum performance. Requests are submitted via API calls, and resulting events are delivered through a FIFO SPSC ring buffer. The engine handles limit orders, market orders, and advanced execution types like FOK and IOC.

  ```{r, engine='bash'}
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release)(events=12561862): 249 ms :: 19 ns/event :: 50279376 events/s
  Benchmark (Release)(events=12561862): 239 ms :: 19 ns/event :: 52518164 events/s
  Benchmark (Release)(events=12561862): 237 ms :: 18 ns/event :: 52796424 events/s
  Benchmark (Release)(events=12561862): 230 ms :: 18 ns/event :: 54516315 events/s
  Benchmark (Release)(events=12561862): 243 ms :: 19 ns/event :: 51540000 events/s
  ```
