/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "assert.hpp"
#include "price_bits.hpp"

void test_price_bits_1() {
  PriceBits<256> mask;

  mask.set(0);
  Assert(mask.clz() == 0);

  mask.reset();
  mask.set(32);
  Assert(mask.clz() == 32);

  mask.reset();
  mask.set(128);
  Assert(mask.clz() == 128);

  mask.reset();
  mask.set(255);
  Assert(mask.clz() == 255);
}

void test_price_bits_2() {
  PriceBits<256> mask;

  mask.set(1);
  Assert(mask.clz() == 1);

  mask.set(16);
  Assert(mask.clz() == 16);

  mask.set(32);
  Assert(mask.clz() == 32);

  mask.set(128);
  Assert(mask.clz() == 128);

  mask.set(255);
  Assert(mask.clz() == 255);

  mask.clear(255);
  Assert(mask.clz() == 128);

  mask.clear(128);
  Assert(mask.clz() == 32);

  mask.clear(32);
  Assert(mask.clz() == 16);

  mask.clear(16);
  Assert(mask.clz() == 1);
}

void test_price_bits_3() {
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

    Assert(sellPricesMask.clz() == maxPrice - 10);
    Assert(buyPricesMask.clz() == 120 - minPrice);
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

    Assert(sellPricesMask.clz() == maxPrice - 10);
    Assert(buyPricesMask.clz() == 120 - minPrice);
  }
}

int main() {
  test_price_bits_1();
  test_price_bits_2();
  test_price_bits_3();
}
