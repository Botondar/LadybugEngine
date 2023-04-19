#pragma once

// TODO(boti): make this file cross-compiler compatible

#include <intrin.h>
#include <immintrin.h>

#include "Core.hpp"

#define LB_INLINE __forceinline

LB_INLINE u8 BitScanForward(u32* Result, u32 Value);
LB_INLINE u8 BitScanReverse(u32* Result, u32 Value);
LB_INLINE u8 BitScanForward(u32* Result, u64 Value);
LB_INLINE u8 BitScanReverse(u32* Result, u64 Value);

LB_INLINE u8 BitScanForward(u32* Result, u32 Value)
{
    return _BitScanForward((unsigned long*)Result, Value);
}

LB_INLINE u8 BitScanReverse(u32* Result, u32 Value)
{
    return _BitScanReverse((unsigned long*)Result, Value);
}

LB_INLINE u8 BitScanForward(u32* Result, u64 Value)
{
    return _BitScanForward64((unsigned long*)Result, Value);
}

LB_INLINE u8 BitScanReverse(u32* Result, u64 Value)
{
    return _BitScanReverse64((unsigned long*)Result, Value);
}