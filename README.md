### Copyright (C) 2015 Łukasz Czerwiński

# HFTDojo
Solutions to selected programming puzzles.  
  
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

- **likely** - `LIKELY` and `UNLIKELY` macros that can be used to provide branch prediction hints to the compiler.

