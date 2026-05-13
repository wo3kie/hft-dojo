#pragma once

/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include <cstdint>
#include <limits>

typedef uint32_t OrderId;
constexpr OrderId InvalidOrderId = 0;

typedef uint32_t Qty;
constexpr Qty InvalidQty = 0;

typedef uint32_t Price;
constexpr Price InvalidPrice = 0;
constexpr Price MinPrice = 1;
constexpr Price MaxPrice = std::numeric_limits<Price>::max();

typedef int32_t Index;
constexpr Index InvalidIndex = -1;