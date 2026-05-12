#pragma once

/*
 * Project:
 *      CxxDojo (https://github.com/wo3kie/cpp-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cassert>
#include <cstddef>
#include <cstdint>

template<typename TType, std::size_t N>
class ObjectPool
{
public:
  using value_type = TType;

public:
  ObjectPool() noexcept
  {
    for(std::size_t i = 0; i < N; ++i) {
      _free[i] = &_storage[i];
    }

    _top = N;
  }

  ObjectPool(ObjectPool&&) = delete;
  ObjectPool(const ObjectPool&) = delete;

  ObjectPool& operator=(ObjectPool&&) = delete;
  ObjectPool& operator=(const ObjectPool&) = delete;

public:
  [[nodiscard]] TType* allocate() noexcept
  {
    assert(_top > 0);
    return _free[--_top];
  }

  void deallocate(TType* p) noexcept
  {
    _free[_top++] = p;
  }

  [[nodiscard]] static constexpr std::size_t capacity() noexcept
  {
    return N;
  }

private:
  alignas(TType) TType _storage[N];
  TType* _free[N];
  std::size_t _top;
};

class Arena
{
public:
    Arena(std::byte* mem, std::size_t size) noexcept
        : _size{size}
        , _raw{mem}
        , _memory{mem}
    {
    }

    Arena(Arena&&) = delete;
    Arena(const Arena&) = delete;

    Arena& operator=(const Arena&) = delete;
    Arena& operator=(Arena&&) = delete;

public:
    template<typename T, std::size_t N>
    [[nodiscard]] ObjectPool<T, N>& make_pool() noexcept
    {
        void* const aligned = _align_up(_raw, alignof(ObjectPool<T, N>));
        std::byte* const end = reinterpret_cast<std::byte*>(aligned) + sizeof(ObjectPool<T, N>);
        _raw = end;

        assert(end <= _memory + _size);

        return *new (aligned) ObjectPool<T, N>();
    }

private:
    static void* _align_up(void* ptr, std::size_t align) noexcept
    {
        std::uintptr_t uintptr = reinterpret_cast<std::uintptr_t>(ptr);
        std::uintptr_t aligned = (uintptr + align - 1) & ~(align - 1);
        return reinterpret_cast<void*>(aligned);
    }

    const std::size_t _size;
    std::byte* _raw;
    std::byte* const _memory;
};
