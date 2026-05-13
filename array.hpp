#pragma once

/*
 * Project:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <array>
#include <cstdint>

template<typename Type, int32_t Size>
struct Array
{
  static_assert((Size & (Size - 1)) == 0);

public:
  int32_t size() const {
    return Size;
  }

  Type& operator[](int32_t index)
  {
    index = index & (Size - 1);
    return _data[index];
  }

  const Type& operator[](int32_t index) const
  {
    index = index & (Size - 1);
    return _data[index];
  }

  std::array<Type, Size>& data()
  {
    return _data;
  }

  const std::array<Type, Size>& data() const
  {
    return _data;
  }

private:
  std::array<Type, Size> _data;
};
