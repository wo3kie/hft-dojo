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

typedef uint32_t Qty;

typedef uint32_t Price;
constexpr Price InvalidPrice = 0;

typedef uint32_t Index;
constexpr Index InvalidIndex = (Index)-1;

constexpr uint32_t UINT32_MIN = 0;
