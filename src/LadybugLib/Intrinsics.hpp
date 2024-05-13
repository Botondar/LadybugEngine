#pragma once

// TODO(boti): make this file cross-compiler compatible

#include <intrin.h>
#include <immintrin.h>

#include "Core.hpp"

#if DEVELOPER
#define LB_INLINE inline
#else
#define LB_INLINE __forceinline
#endif

LB_INLINE u64 ReadTSC();

//
// Bit
//
LB_INLINE u16 EndianFlip16(u16 Value);
LB_INLINE u32 EndianFlip32(u32 Value);
LB_INLINE u64 EndianFlip64(u64 Value);
LB_INLINE void EndianFlip(void* Dst, const void* Src, u32 ByteCount);

LB_INLINE u32 FindLeastSignificantSetBit(u32 Value);
LB_INLINE u32 TrailingZeroCount(u32 Value);
LB_INLINE u32 CountSetBits(u32 Value);

LB_INLINE u8 BitScanForward(u32* Result, u32 Value);
LB_INLINE u8 BitScanReverse(u32* Result, u32 Value);
LB_INLINE u8 BitScanForward(u32* Result, u64 Value);
LB_INLINE u8 BitScanReverse(u32* Result, u64 Value);

LB_INLINE u32 SetBitsBelowHighInclusive(u32 Value);

LB_INLINE u32 CeilPowerOf2(u32 Value);

//
// Atomic
//
#define SpinWait _mm_pause()
LB_INLINE u32 AtomicLoadAndIncrement(volatile u32* Value);

//
// Implementation
//

LB_INLINE u64 ReadTSC()
{
    return __rdtsc();
}

LB_INLINE u16 EndianFlip16(u16 Value)
{
    u16 Result = (Value << 8) | (Value >> 8);
    return(Result);
}

LB_INLINE u32 EndianFlip32(u32 Value)
{
    u32 Result = 
        (Value << 24) | 
        (Value >> 24) |
        ((Value << 8) & 0x00FF0000u) |
        ((Value >> 8) & 0x0000FF00u);
    return(Result);
}

LB_INLINE u64 EndianFlip64(u64 Value)
{
    u64 Result = 
        (Value << 56) |
        (Value >> 56) |
        ((Value << 40) & 0x00FF000000000000llu) |
        ((Value >> 40) & 0x000000000000FF00llu) |
        ((Value << 24) & 0x0000FF0000000000llu) |
        ((Value >> 24) & 0x0000000000FF0000llu) |
        ((Value <<  8) & 0x000000FF00000000llu) |
        ((Value >>  8) & 0x00000000FF000000llu);
    return(Result);
}

LB_INLINE void EndianFlip(void* Dst, const void* Src, u32 ByteCount)
{
    switch (ByteCount)
    {
        case 1:
        {
            *(u8*)Dst = *(const u8*)Src;
        } break;
        case 2:
        {
            *(u16*)Dst = EndianFlip16(*(const u16*)Src);
        } break;
        case 4:
        {
            *(u32*)Dst = EndianFlip32(*(const u32*)Src);
        } break;
        case 8:
        {
            *(u64*)Dst = EndianFlip64(*(const u64*)Src);
        } break;
        InvalidDefaultCase;
    }
}

LB_INLINE u32 TrailingZeroCount(u32 Value)
{
    u32 Result = (u32)_mm_tzcnt_32(Value);
    return(Result);
}

LB_INLINE u32 CountSetBits(u32 Value)
{
    u32 Result = (u32)_mm_popcnt_u32(Value);
    return(Result);
}

LB_INLINE u32 FindLeastSignificantSetBit(u32 Value)
{
    u32 Result = TrailingZeroCount(Value);
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

LB_INLINE u32 SetBitsBelowHighInclusive(u32 Value)
{
    u32 Result = 0;

    u32 HighBit;
    if (BitScanReverse(&HighBit, Value))
    {
        // NOTE(boti): Shifting by (HighBit + 1) can result in UB
        Result = ((1u << HighBit) << 1) - 1;
    }

    return(Result);
}

LB_INLINE u32 CeilPowerOf2(u32 Value)
{
    u32 Result = Value;

    if (CountSetBits(Value) > 1)
    {
        u32 HighBit;
        BitScanReverse(&HighBit, Value);
        // NOTE(boti): Shifting by (HighBit + 1) can result in UB
        Result = ((1u << HighBit) << 1);
    }

    return(Result);
}

LB_INLINE u32 AtomicLoadAndIncrement(volatile u32* Value)
{
    u32 Result = (u32)(_InterlockedIncrement((long*)Value) - 1);
    return(Result);
}