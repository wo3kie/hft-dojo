/*
 * Author: Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "price_bits.hpp"

void test_set_clz_clear() {
  PriceBits<256> mask;

  mask.set(1);
  assert(mask.clz() == 1);

  mask.set(16);
  assert(mask.clz() == 16);

  mask.set(32);
  assert(mask.clz() == 32);

  mask.set(128);
  assert(mask.clz() == 128);

  mask.set(255);
  assert(mask.clz() == 255);

  mask.clear(255);
  assert(mask.clz() == 128);

  mask.clear(128);
  assert(mask.clz() == 32);

  mask.clear(32);
  assert(mask.clz() == 16);

  mask.clear(16);
  assert(mask.clz() == 1);
}

void test_set_shift_clz() {
  int32_t minIndex = 100;
  int32_t minPrice = 100;
  int32_t maxPrice = 200;

  {
    PriceBits<256> buyPricesMask;
    PriceBits<256> sellPricesMask;

    sellPricesMask.set(maxPrice - 10);
    buyPricesMask.set(120 - minPrice);

    minIndex += 4;
    minPrice += 4;
    maxPrice += 4;
    sellPricesMask.shl<4>();
    buyPricesMask.shr<4>();

    assert(sellPricesMask.clz() == maxPrice - 10);
    assert(buyPricesMask.clz() == 120 - minPrice);
  }

  {
    PriceBits<256> buyPricesMask;
    PriceBits<256> sellPricesMask;

    sellPricesMask.set(maxPrice - 10);
    buyPricesMask.set(120 - minPrice);

    minIndex -= 4;
    minPrice -= 4;
    maxPrice -= 4;
    sellPricesMask.shr<4>();
    buyPricesMask.shl<4>();

    assert(sellPricesMask.clz() == maxPrice - 10);
    assert(buyPricesMask.clz() == 120 - minPrice);
  }
}

int main() {
  test_set_clz_clear();
  test_set_shift_clz();
}
