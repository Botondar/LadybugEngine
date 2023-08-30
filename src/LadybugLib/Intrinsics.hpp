#pragma once

// TODO(boti): make this file cross-compiler compatible

#include <intrin.h>
#include <immintrin.h>

#include "Core.hpp"

#define LB_INLINE __forceinline


LB_INLINE u32 FindLeastSignificantSetBit(u32 Value);

LB_INLINE u8 BitScanForward(u32* Result, u32 Value);
LB_INLINE u8 BitScanReverse(u32* Result, u32 Value);
LB_INLINE u8 BitScanForward(u32* Result, u64 Value);
LB_INLINE u8 BitScanReverse(u32* Result, u64 Value);

#define SpinWait _mm_pause()
LB_INLINE u32 AtomicLoadAndIncrement(volatile u32* Value);

LB_INLINE u32 FindLeastSignificantSetBit(u32 Value)
{
    u32 Result = (u32)_mm_tzcnt_32(Value);
    Result = (Result == 0) ? 32 : Result - 1;
    return(Result);
}

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

LB_INLINE u32 AtomicLoadAndIncrement(volatile u32* Value)
{
    u32 Result = (u32)(_InterlockedIncrement((long*)Value) - 1);
    return(Result);
}