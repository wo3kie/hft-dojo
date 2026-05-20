/*
 * Project:
 *      HFTDojo (https://github.com/wo3kie/hft-dojo)
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "map_reduce.hpp"
#include "assert.hpp"

using Trivial = int;

struct Moveable
{
  Moveable(int v)
    : value(v)
  {
  }

  Moveable(Moveable&&) = default;
  Moveable& operator=(Moveable&&) = default;

  Moveable(const Moveable&) = delete;
  Moveable& operator=(const Moveable&) = delete;

  ~Moveable() = default;

  int value;
};

struct BigData
{
  BigData(std::initializer_list<int> data)
    : _data(data)
  {
  }

  BigData(const BigData& data)
  {
    _data = data._data;
  }

  BigData(BigData&& data)
  {
    _data = std::move(data._data);
  }

  BigData& operator=(BigData&& data)
  {
    _data = std::move(data._data);
    return *this;
  }

  BigData& operator=(const BigData& data)
  {
    _data = data._data;
    return *this;
  }

  ~BigData() = default;

  std::size_t size() const
  {
    return _data.size();
  }

  std::vector<int> _data;
};

void test_map_reduce_run_sums_trivial_lvalues()
{
  std::function<int(Trivial)> map = [](Trivial /* data */) -> int {
    return 12;
  };

  std::function<Trivial(Trivial, Trivial)> reduce = [](Trivial a, Trivial i) -> Trivial {
    return a + i;
  };

  Trivial value_trivial = 12;

  Assert(48 == MapReduce(map, reduce).run(value_trivial,
                                          value_trivial,
                                          value_trivial,
                                          value_trivial));
}

void test_map_reduce_run_sums_trivial_temporaries()
{
  std::function<int(Trivial)> map = [](Trivial /* data */) -> int {
    return 12;
  };

  std::function<Trivial(Trivial, Trivial)> reduce = [](Trivial a, Trivial i) -> Trivial {
    return a + i;
  };

  Assert(48 == MapReduce(map, reduce).run(Trivial(12),
                                          Trivial(12),
                                          Trivial(12),
                                          Trivial(12)));
}

void test_map_reduce_run_accepts_move_only_values()
{
  std::function<int(Moveable)> map = [](Moveable data) -> int {
    return data.value;
  };

  std::function<int(int, int)> reduce = [](int a, int i) -> int {
    return a + i;
  };

  Moveable value_moveable1(12);
  Moveable value_moveable2(12);
  Moveable value_moveable3(12);
  Moveable value_moveable4(12);

  Assert(48 == MapReduce(map, reduce).run(std::move(value_moveable1),
                                          std::move(value_moveable2),
                                          std::move(value_moveable3),
                                          std::move(value_moveable4)));
}

void test_map_reduce_run_with_init_accepts_move_only_values()
{
  std::function<int(Moveable)> map = [](Moveable data) -> int {
    return data.value;
  };

  std::function<int(int, int)> reduce = [](int a, int i) -> int {
    return a + i;
  };

  Assert(2 + 48 == MapReduce(map, reduce).run_with_init(2,
                                                        Moveable(12),
                                                        Moveable(12),
                                                        Moveable(12),
                                                        Moveable(12)));
}

void test_map_reduce_run_preserves_bigdata_lvalues()
{
  std::function<int(const BigData&)> map = [](const BigData& data) -> int {
    return data.size();
  };

  std::function<int(int, int)> reduce = [](int a, int i) -> int {
    return a + i;
  };

  BigData value_bigdata1({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  BigData value_bigdata2({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  BigData value_bigdata3({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  BigData value_bigdata4({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});

  Assert(48 == MapReduce(map, reduce).run(value_bigdata1, value_bigdata2, value_bigdata3, value_bigdata4));

  Assert(value_bigdata1.size() == 12);
  Assert(value_bigdata2.size() == 12);
  Assert(value_bigdata3.size() == 12);
  Assert(value_bigdata4.size() == 12);
}

void test_map_reduce_run_accepts_bigdata_temporaries()
{
  std::function<int(const BigData&)> map = [](const BigData& data) -> int {
    return data.size();
  };

  std::function<int(int, int)> reduce = [](int a, int i) -> int {
    return a + i;
  };

  Assert(48 == MapReduce(map, reduce).run(BigData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}),
                                          BigData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}),
                                          BigData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}),
                                          BigData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12})));
}

void test_map_reduce_run_accepts_std_array_input()
{
  std::function<int(const BigData&)> map = [](const BigData& data) -> int {
    return data.size();
  };

  std::function<int(int, int)> reduce = [](int a, int i) -> int {
    return a + i;
  };

  std::array<BigData, 4> bigdata_array = {BigData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}),
                                          BigData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}),
                                          BigData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}),
                                          BigData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12})};

  Assert(48 == MapReduce(map, reduce).run(bigdata_array));

  Assert(bigdata_array[0].size() == 12);
  Assert(bigdata_array[1].size() == 12);
  Assert(bigdata_array[2].size() == 12);
  Assert(bigdata_array[3].size() == 12);
}

void test_map_reduce_run_with_init_accepts_std_array_input()
{
  std::function<int(const BigData&)> map = [](const BigData& data) -> int {
    return data.size();
  };

  std::function<int(int, int)> reduce = [](int a, int i) -> int {
    return a + i;
  };

  std::array<BigData, 4> bigdata_array = {BigData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}),
                                          BigData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}),
                                          BigData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}),
                                          BigData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12})};

  Assert(2 + 48 == MapReduce(map, reduce).run_with_init(2, bigdata_array));

  Assert(bigdata_array[0].size() == 12);
  Assert(bigdata_array[1].size() == 12);
  Assert(bigdata_array[2].size() == 12);
  Assert(bigdata_array[3].size() == 12);
}

int main()
{
  test_map_reduce_run_sums_trivial_lvalues();
  test_map_reduce_run_sums_trivial_temporaries();
  test_map_reduce_run_accepts_move_only_values();
  test_map_reduce_run_with_init_accepts_move_only_values();
  test_map_reduce_run_preserves_bigdata_lvalues();
  test_map_reduce_run_accepts_bigdata_temporaries();
  test_map_reduce_run_accepts_std_array_input();
  test_map_reduce_run_with_init_accepts_std_array_input();
}
