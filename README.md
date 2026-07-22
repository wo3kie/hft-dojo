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
  
- **branchless** - A collection of branchless functions like `min`, `max`... These functions are implemented using bitwise operations and arithmetic to avoid branching, which can improve performance in certain scenarios by reducing pipeline stalls.  
    
- **flat_hash** - A fixed‑capacity, array‑based open‑addressing hash map. It is designed for scenarios where the maximum number of elements is known in advance, and it provides fast insertions, deletions, and lookups without dynamic memory allocation. The map maintains a free list of available slots, which enables it to reuse memory efficiently without fragmentation.

  ```bash
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Micro Benchmark: insert: (Release): FlatHash<int,int,8>: (iters=10000): 15 μs :: 1 ns/iter :: 638977635 iter/s
  ```
    
- **flat_list** - A fixed‑capacity, array‑based linked list. It is designed for scenarios where the maximum number of elements is known in advance, and it provides fast insertions and deletions without dynamic memory allocation. The list maintains a free list of available slots, which enables it to reuse memory efficiently without fragmentation.

- **flat_queue** - It uses an open‑addressing approach to manage free slots, which can be more efficient in certain scenarios, especially when slot `id`s can't be cached.

  ```bash
  Benchmark (Release): FlatQueue: (iters=100000): 581 μs :: 5 ns/iter :: 171927357 iter/s
  Benchmark (Release): FlatQueue: (iters=100000): 579 μs :: 5 ns/iter :: 172554216 iter/s
  Benchmark (Release): FlatQueue: (iters=100000): 579 μs :: 5 ns/iter :: 172557789 iter/s
  Benchmark (Release): FlatQueue: (iters=100000): 599 μs :: 5 ns/iter :: 166870248 iter/s
  Benchmark (Release): FlatQueue: (iters=100000): 579 μs :: 5 ns/iter :: 172422116 iter/s
  ```

- **flat_tree_bs** - The simplest binary-search variant of the flat tree without nodes balancing. It is designed as a base for balanced versions, but it can be useful on its own in scenarios where the input data is mostly random.

  ```bash
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release): FlatTreeBS: (iters=100000): 69806 μs :: 698 ns/iter :: 1432525 iter/s
  Benchmark (Release): FlatTreeBS: (iters=100000): 69219 μs :: 692 ns/iter :: 1444681 iter/s
  Benchmark (Release): FlatTreeBS: (iters=100000): 71617 μs :: 716 ns/iter :: 1396311 iter/s
  Benchmark (Release): FlatTreeBS: (iters=100000): 70392 μs :: 703 ns/iter :: 1420597 iter/s
  Benchmark (Release): FlatTreeBS: (iters=100000): 68570 μs :: 685 ns/iter :: 1458362 iter/s
  ```

- **flat_tree_avl** - An AVL‑balanced variant of the flat tree. It maintains the AVL balance property, which ensures that the tree remains balanced after insertions and deletions, providing O(log n) time complexity for insertions, lookups and deletions. It is implemented as a subclass of the binary‑search variant, reusing its core logic and adding the necessary rotations and balance factor updates to maintain the AVL property.

  ```bash
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release): FlatTreeAVL: (iters=100000): 3455 μs :: 34 ns/iter :: 28943224 iter/s
  Benchmark (Release): FlatTreeAVL: (iters=100000): 3433 μs :: 34 ns/iter :: 29121602 iter/s
  Benchmark (Release): FlatTreeAVL: (iters=100000): 3555 μs :: 35 ns/iter :: 28124521 iter/s
  Benchmark (Release): FlatTreeAVL: (iters=100000): 3604 μs :: 36 ns/iter :: 27745469 iter/s
  Benchmark (Release): FlatTreeAVL: (iters=100000): 3430 μs :: 34 ns/iter :: 29152623 iter/s
  ```

- **flat_tree_splay** - A splay‑tree variant of the flat tree. It is a self‑adjusting binary search tree that performs splaying operations to keep recently accessed elements near the root, which can improve access times for certain access patterns. It is designed for scenarios where there is a high degree of locality in the access patterns, as it can provide faster access to recently accessed elements compared to other balanced tree structures.

  ```bash
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release): FlatTreeSplay:: (iters=100000): 7348 μs :: 73 ns/iter :: 13608936 iter/s
  Benchmark (Release): FlatTreeSplay:: (iters=100000): 7703 μs :: 77 ns/iter :: 12980603 iter/s
  Benchmark (Release): FlatTreeSplay:: (iters=100000): 7379 μs :: 73 ns/iter :: 13551644 iter/s
  Benchmark (Release): FlatTreeSplay:: (iters=100000): 7354 μs :: 73 ns/iter :: 13597701 iter/s
  Benchmark (Release): FlatTreeSplay:: (iters=100000): 7908 μs :: 79 ns/iter :: 12644042 iter/s
  ```

- **gdb_utils** - GDB helpers for inspecting the internal state of the data structures in this repository.

  ```bash
  (gdb) source ../gdb_utils.py

  (gdb) print_ring_buffer rBuffer
  RingBuffer<int, 128> [ 0, 1, 2, 3, ..., 124, 125, 126, 127 ]
  
  (gdb) print_ring_buffer_spsc rBuffer
  RingBufferSPSC<int, 128> [ 0, 1, 2, 3, ..., 124, 125, 126, 127 ]
  ```
    
- **map_reduce** - An implementation of the parallel map and serial reduce paradigm in C++.
  
- **object_pool** - A fixed‑capacity pool of pre‑allocated objects. It is designed for scenarios where the maximum number of objects is known in advance, and it provides fast allocation and deallocation with one dynamic memory allocation. The pool maintains a free list of available slots, which enables it to reuse memory efficiently without fragmentation.

- **price_bits** — a compact bitmask structure that tracks the occupancy of price levels in the order book. It provides efficient methods for setting, clearing, and querying bits, as well as counting leading zeros to quickly identify the best bid and ask levels.
  
- **random** - A simple linear congruential generator (LCG) for producing pseudo‑random numbers. It is designed for scenarios where a fast and deterministic random number generator is needed, such as in testing and benchmarking.

- **ring_buffer** - Single‑threaded ring buffer. It is designed for scenarios where only one thread is producing and consuming data, so it does not include any synchronization mechanisms.

  ```bash
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release): RingBuffer: (iters=131072): 314 μs :: 2 ns/iter :: 416557817 iter/s
  Benchmark (Release): RingBuffer: (iters=131072): 288 μs :: 2 ns/iter :: 454133462 iter/s
  Benchmark (Release): RingBuffer: (iters=131072): 239 μs :: 1 ns/iter :: 546684573 iter/s
  Benchmark (Release): RingBuffer: (iters=131072): 193 μs :: 1 ns/iter :: 675904105 iter/s
  Benchmark (Release): RingBuffer: (iters=131072): 190 μs :: 1 ns/iter :: 686273175 iter/s
  ```

- **ring_buffer_mutex** - Multi‑producer/multi‑consumer ring buffer using mutexes. It is designed for scenarios where multiple threads are producing and consuming data, so it uses mutexes to synchronize access to the buffer. This allows it to be used in multi‑threaded environments, but it may have higher latency compared to the single‑threaded version due to the overhead of locking and unlocking the mutexes.
  
  ```bash
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release): RingBufferMutex: (iters=131072): 1978 μs :: 15 ns/iter :: 66234442 iter/s
  Benchmark (Release): RingBufferMutex: (iters=131072): 1943 μs :: 14 ns/iter :: 67445552 iter/s
  Benchmark (Release): RingBufferMutex: (iters=131072): 1693 μs :: 12 ns/iter :: 77407162 iter/s
  Benchmark (Release): RingBufferMutex: (iters=131072): 1744 μs :: 13 ns/iter :: 75134853 iter/s
  Benchmark (Release): RingBufferMutex: (iters=131072): 1752 μs :: 13 ns/iter :: 74781285 iter/s
  ```

- **ring_buffer_spsc** - Single‑producer/single‑consumer ring buffer using atomics. It is designed for scenarios where one thread is producing data and another thread is consuming data, so it uses atomic operations to synchronize access to the buffer. This allows it to be used in multi‑threaded environments while still maintaining low latency, as it avoids the overhead of mutexes.
  
  ```bash
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release): RingBufferSPSC: (iters=131072): 528 μs :: 4 ns/iter :: 247889369 iter/s
  Benchmark (Release): RingBufferSPSC: (iters=131072): 499 μs :: 3 ns/iter :: 262249948 iter/s
  Benchmark (Release): RingBufferSPSC: (iters=131072): 408 μs :: 3 ns/iter :: 320999204 iter/s
  Benchmark (Release): RingBufferSPSC: (iters=131072): 415 μs :: 3 ns/iter :: 315483411 iter/s
  Benchmark (Release): RingBufferSPSC: (iters=131072): 415 μs :: 3 ns/iter :: 315760057 iter/s
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

  ```bash
  # Intel(R) Core(TM) Ultra 7 165H GenuineIntel
  Benchmark (Release)(events=12561862): 249 ms :: 19 ns/event :: 50279376 events/s
  Benchmark (Release)(events=12561862): 239 ms :: 19 ns/event :: 52518164 events/s
  Benchmark (Release)(events=12561862): 237 ms :: 18 ns/event :: 52796424 events/s
  Benchmark (Release)(events=12561862): 230 ms :: 18 ns/event :: 54516315 events/s
  Benchmark (Release)(events=12561862): 243 ms :: 19 ns/event :: 51540000 events/s
  ```
