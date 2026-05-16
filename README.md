### Copyright (C) 2015 Łukasz Czerwiński

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
  
- **array** - Implementation of a fixed-size array with positive/negative/modulo indexing. For the best performance, the size must be a power of two to use bitwise operations.
  
  ```{r, engine='cpp'}
  Array<int 8> arr = {0, 1, 2, 3, 4, 5, 6, 7};
  assert(arr[ 0] == 0);
  assert(arr[-1] == 7);
  ```

- **assert** - Implement an improved `assert` utility that accepts a simple conditional expression (`==`, `!=`, `<=`, `<`, `>=`, `>`) and, upon failure, prints both the _actual_ and _expected_ values.
  
  ```{r, engine='bash'}
  $ cat assert.cpp
  #include "./assert.hpp"
  int main() {
    int a = 1;
    int b = 2;
    Assert(a == b);
  }
  ```
- **branchless** - A tiny collection of branchless functions like `min`, `max`, `abs`...

- **flat_list** - A implementation of a double linked flat list with a fixed maximum capacity. All supported operations like `push_front`, `push_back`, `pop_front`, `pop_back`, `remove` run in O(1) time.

- **gdb_utils** - A collection of GDB utilities that can be used to inspect the internal state of the data structures implemented in this repository.

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
  
- **map_reduce** - A simple implementation of the MapReduce paradigm in C++.
  
- **matching_engine** - A simple implementation of a matching engine for a limit order book. It supports basic order types like limit and market orders, as well as order cancellation and modification.
  
- **order_book** - A simple implementation of a limit order book that supports basic operations like adding, removing, and modifying orders. It also provides a feature of sliding window to follow price trends.
  
- **price_levels** - A simple implementation of a price level data structure that can be used to store orders at a specific price level in a limit order book.
  
- **ring_buffer** - A simple implementation of a ring buffer with no synchronization for single-threaded scenarios.
  
- **ring_buffer_mutex** - A simple implementation of a ring buffer that uses mutexes for synchronization. It supports multiple producers and multiple consumers.
  
- **ring_buffer_spsc** - A simple implementation of a ring buffer that is designed for single producer and single consumer scenarios. It uses atomic operations for synchronization.
  
- **ring_buffer_spmc** - A simple implementation of a ring buffer that is designed for single producer and multiple consumer scenarios. It uses atomic operations for synchronization.
  
- **task_executor** - A simple implementation of a function execution in a separate thread (with optional thread affinity). The value can be obtained by `Channel::get` method.
  
- **task_worker** - A simple implementation of a void procedure execution in a separate thread (with optional thread affinity).
