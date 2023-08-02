#pragma once

#if defined(_MSC_VER) && !defined(__clang__)
#define LB_COMPILER_MSVC 1
#define LB_COMPILER_CLANGCL 0
#define LB_COMPILER_CLANG 0
#define LB_COMPILER_GCC 0
#elif defined(_MSC_VER) && defined(__clang__)
#define LB_COMPILER_MSVC 0
#define LB_COMPILER_CLANGCL 1
#define LB_COMPILER_CLANG 0
#define LB_COMPILER_GCC 0
#elif defined(__clang__)
#define LB_COMPILER_MSVC 0
#define LB_COMPILER_CLANGCL 0
#define LB_COMPILER_CLANG 1
#define LB_COMPILER_GCC 0
#elif defined(__GNUC__)
#define LB_COMPILER_MSVC 0
#define LB_COMPILER_CLANGCL 0
#define LB_COMPILER_CLANG 0
#define LB_COMPILER_GCC 1
#else
#error Unknown compiler
#endif

#if !LB_COMPILER_CLANGCL
#error Unsupported compiler
#endif

#if defined(__X86_64__) || defined(__x86_64__) || defined(_M_X64)
#define LB_ARCH_X64 1
#else 
#error Unknown target architecture
#endif

#if !LB_ARCH_X64
#error Unsupported target architecture
#endif


#include <cstdint>
#include <cstddef>
#include <cfloat>
#include <cassert>

//extern "C" void* memcpy(void*, const void*, size_t);

typedef uintptr_t umm;
typedef intptr_t smm;

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

typedef u32 b32;
typedef u32 flags32;

typedef float f32;
typedef double f64;

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);
static_assert(sizeof(size_t) == 8);

#define internal static
#define lbfn static

#define HasFlag(b, f) (((b) & (f)) != 0)
#define HasAllFlags(b, f) (((b) & (f)) == (f))

// NOTE(boti): The default is 32-bit array count, 
//             Use CountOf<t>(Array) if you need something else
template<typename ret = u32, typename type, umm N>
constexpr ret CountOf(const type (&)[N])
{
    static_assert(N == (ret)N);
    return((ret)N);
}

#if 1
#define OffsetOf(s, m) offsetof(s, m)
#else
#define OffsetOf(s, m) ((umm)(&((s*)0)->m))
#endif

#define KiB(x) 1024*(x)
#define MiB(x) 1024*KiB(x)
#define GiB(x) 1024llu*MiB(x)
// ^NOTE(boti): we cast GiBs to 64 bits because that's probably what you want

#define Assert(...) assert(__VA_ARGS__)
#define InvalidCodePath Assert(!"Invalid code path")
#define InvalidDefaultCase default: InvalidCodePath; break

#if DEVELOPER
#define Verify(...) assert(__VA_ARGS__)
#define UnhandledError(msg) Assert(!msg)
#define UnimplementedCodePath Assert(!"Unimplemented code path")
#else
#define Verify(...) #error "Verify in non-developer build"
#define UnhandledError(...) #error "Unhandled error"
#define UnimplementedCodePath #error "Unimplemented code path"
#endif

#define UNREFERENCED_VARIABLE(x) (void)x

constexpr u8  U8_MAX  = 0xFFu;
constexpr u16 U16_MAX = 0xFFFFu;
constexpr u32 U32_MAX = 0xFFFFFFFFu;
constexpr u64 U64_MAX = 0xFFFFFFFFFFFFFFFFllu;

// TODO(boti): Do something about this warning
#pragma warning(push)
#pragma warning(disable:4309)
constexpr s8  S8_MIN  = 0x80;
constexpr s16 S16_MIN = 0x8000;
constexpr s32 S32_MIN = 0x80000000;
constexpr s64 S64_MIN = 0x8000000000000000ll;
#pragma warning(pop)

constexpr s8  S8_MAX  = 0x7F;
constexpr s16 S16_MAX = 0x7FFF;
constexpr s32 S32_MAX = 0x7FFFFFFF;
constexpr s64 S64_MAX = 0x7FFFFFFFFFFFFFFFll;

constexpr u32 INVALID_INDEX_U32 = U32_MAX;

inline size_t Align(size_t Value, size_t Alignment)
{
    size_t Result = (Value + Alignment - 1) & ~(Alignment - 1);
    return Result;
}

inline u32 Align(u32 Value, u32 Alignment)
{
    u32 Result = (Value + Alignment - 1) & ~(Alignment - 1);
    return Result;
}

inline void* AlignPtr(void* Base, size_t Alignment)
{
    void* Result = (void*)Align((size_t)Base, Alignment);
    return Result;
}

inline void* OffsetPtr(void* Base, size_t Size)
{
    void* Result = (void*)((u8*)Base + Size);
    return Result;
}

inline u32 TruncateU64ToU32(u64 Value);

struct buffer
{
    u64 Size;
    void* Data;
};

//
// Memory
//

enum memory_push_flags : flags32
{
    MemPush_None = 0,
    MemPush_Clear = 1 << 0,
};

struct memory_arena
{
    size_t Size;
    size_t Used;
    void* Base;
};

inline memory_arena InitializeArena(size_t Size, void* Base);

struct memory_arena_checkpoint
{
    size_t Used;
    memory_arena* Arena;
};

inline memory_arena_checkpoint ArenaCheckpoint(memory_arena* Arena);
inline void RestoreArena(memory_arena* Arena, memory_arena_checkpoint Checkpoint);

template<typename T>
inline T* PushStruct(memory_arena* Arena, flags32 Flags = 0, const T* InitialData = nullptr);

template<typename T>
inline T* PushArray(memory_arena* Arena, size_t Count, flags32 Flags = 0, const T* InitialData = nullptr);

inline void ResetArena(memory_arena* Arena);
inline void* PushSize(memory_arena* Arena, size_t Size, size_t Alignment = 0, flags32 Flags = 0, const void* Data = nullptr);

template<typename T>
inline T* FreeListInitialize(T* Head, u32 Count);

template<typename T>
inline T* FreeListAllocate(T*& List);

template<typename T>
inline void FreeListFree(T*& List, T* Elem);

template<typename T>
inline T* DListInitialize(T* Sentinel);

template<typename T>
inline bool DListIsEmpty(const T* Sentinel);

template<typename T>
inline T* DListInsert(T* Sentinel, T* Elem);

template<typename T>
inline T* DListRemove(T* Elem);


//
// Float
//

constexpr u32 F32_SIGN_MASK = 0x80000000u;
constexpr u32 F32_SIGN_SHIFT = 31u;
constexpr u32 F32_EXPONENT_MASK = 0x7F800000u;
constexpr u32 F32_EXPONENT_SHIFT = 23u;
constexpr u32 F32_MANTISSA_MASK = 0x007FFFFFu;
constexpr u32 F32_MANTISSA_SHIFT = 0u;
constexpr u32 F32_EXPONENT_ZERO = 0x3F800000u;

constexpr s32 F32_EXPONENT_BIAS = 0x7F;
constexpr s32 F32_EXPONENT_MIN = -126;
constexpr s32 F32_EXPONENT_MAX = 127;
constexpr u32 F32_EXPONENT_SPECIAL = 0xFF;
constexpr u32 F32_EXPONENT_SUBNORMAL = 0x00;

constexpr f32 F32_MAX_NORMAL = 3.40282347e+38f;
constexpr f32 F32_MIN_NORMAL = 1.17549435e-38f;
constexpr f32 F32_MIN_SUBNORMAL = 1.40129846e-45f;
constexpr f32 F32_EPSILON = 1.19209290e-7f;

//
// Math
//

#include <cmath>

union v2;
union v3;
union v4;
union m4;
struct mmrect2; // 2D min-max rect
struct mmbox;

constexpr f32 Pi = 3.14159265359f;
constexpr f32 EulerNumber = 2.71828182846f;
constexpr f32 Log2_10 = 3.32192809488736234787f;
constexpr f32 NOZ_Threshold = 1e-6f;

inline constexpr u32 CeilDiv(u32 x, u32 y);

inline constexpr f32 Abs(f32 x) { return x < 0.0f ? -x : x; }

inline f32 Floor(f32 x) { return floorf(x); }
inline f32 Ceil(f32 x) { return ceilf(x); }
inline f32 Round(f32 x) { return roundf(x); }

inline f32 Sqrt(f32 x) { return sqrtf(x); }

inline f32 Sin(f32 x) { return sinf(x); }
inline f32 Cos(f32 x) { return cosf(x); }
inline f32 Tan(f32 x) { return tanf(x); }

inline f32 Ln(f32 x) { return logf(x); }
inline f32 Pow(f32 a, f32 b) { return powf(a, b); }
inline f32 Ratio0(f32 Numerator, f32 Denominator);
inline f32 Modulo(f32 x, f32 d) { return fmodf(x, d); }


template<typename T> inline constexpr 
T Min(T a, T b);
template<typename T> inline constexpr 
T Max(T a, T b);
template<>inline constexpr 
v2 Min<v2>(v2 a, v2 b);
template<> inline constexpr 
v2 Max<v2>(v2 a, v2 b);
template<typename T> inline constexpr 
T Clamp(T x, T e0, T e1);

template<typename T> inline constexpr
T Lerp(T a, T b, f32 t);

inline constexpr f32 ToRadians(f32 Degrees);
inline constexpr f32 ToDegrees(f32 Radians);

struct entropy32
{
    u32 Value;
};

inline u32 RandU32(entropy32* Entropy);
inline f32 RandUnilateral(entropy32* Entropy);
inline f32 RandBilateral(entropy32* Entropy);

inline bool PointRectOverlap(v2 P, mmrect2 Rect);

union v2
{
    f32 E[2];
    struct { f32 x, y; };
};

union v3
{
    f32 E[3];
    struct { f32 x, y, z; };
};

union v4
{
    f32 E[4];
    struct { f32 x, y, z, w; };
    struct 
    {
        v3 xyz;
        f32 Ignored0_;
    };
};

// NOTE(boti): column major matrix order:
//   [ 0, 4,  8, 12 ]
//   [ 1, 5,  9, 13 ]
//   [ 2, 6, 10, 14 ]
//   { 3, 7, 11, 15 ]
union m4
{
    f32 E[4][4];
    f32 EE[16];
    v4 C[4];
    struct
    {
        v4 X, Y, Z, P;
    };
};

inline v2 operator-(v2 v);
inline v2 operator*(v2 v, f32 s);
inline v2 operator*(f32 s, v2 v);
inline v2 operator+(v2 a, v2 b);
inline v2 operator-(v2 a, v2 b);
inline v2& operator*=(v2& a, f32 s);
inline v2& operator+=(v2& a, v2 b);
inline v2& operator-=(v2& a, v2 b);

inline f32 Dot(v2 a, v2 b);
inline f32 VectorLength(v2 v);
inline v2 Normalize(v2 v);
inline v2 NOZ(v2 v);

inline v3 operator-(v3 v);
inline v3 operator*(v3 v, f32 s);
inline v3 operator*(f32 s, v3 v);
inline v3 operator+(v3 a, v3 b);
inline v3 operator-(v3 a, v3 b);
inline v3& operator+=(v3& a, v3 b);
inline v3& operator-=(v3& a, v3 b);

inline bool IsZeroVector(v3 v);

inline f32 Dot(v3 a, v3 b);
inline f32 VectorLength(v3 v);
inline v3 Normalize(v3 v);
inline v3 NOZ(v3 v);
inline v3 Cross(v3 a, v3 b);

inline v4 operator-(v4 v);
inline v4 operator*(v4 v, f32 s);
inline v4 operator*(f32 s, v4 v);
inline v4 operator+(v4 a, v4 b);
inline v4 operator-(v4 a, v4 b);

inline f32 Dot(v4 a, v4 b);
inline v4 Normalize(v4 V);

inline m4 M4(
    f32 m00, f32 m01, f32 m02, f32 m03,
    f32 m10, f32 m11, f32 m12, f32 m13,
    f32 m20, f32 m21, f32 m22, f32 m23,
    f32 m30, f32 m31, f32 m32, f32 m33);

inline m4 operator*(const m4& A, const m4& B);
inline v4 operator*(const m4& M, const v4& v);
inline v4 operator*(const v4& v, const m4& M);

inline v3 TransformPoint(const m4& M, v3 v);
inline v3 TransformDirection(const m4& M, v3 v);

inline m4 AffineOrthonormalInverse(const m4& M);
inline m4 AffineInverse(const m4& M);

inline m4 PerspectiveFov(f32 Fov, f32 AspectRatio, f32 NearZ, f32 FarZ);

inline m4 Identity4();

struct mmrect2
{
    v2 Min;
    v2 Max;
};

struct mmbox 
{
    v3 Min;
    v3 Max;
};

inline m4 QuaternionToM4(v4 Q);

//
// Implementations
//

//
// Common
//
inline u32 TruncateU64ToU32(u64 Value)
{
    Assert(Value <= 0xFFFFFFFFllu);
    u32 Result = (u32)Value;
    return(Result);
}

//
// Memory
//

inline memory_arena InitializeArena(size_t Size, void* Base)
{
    memory_arena Arena = 
    {
        .Size = Size,
        .Used = 0,
        .Base = Base,
    };
    return Arena;
}

inline void ResetArena(memory_arena* Arena)
{
    Arena->Used = 0;
}

inline void* PushSize(memory_arena* Arena, 
                      size_t Size, 
                      size_t Alignment /*= 0*/, 
                      flags32 Flags /*= 0*/, 
                      const void* Data /*= nullptr*/)
{
    void* Result = nullptr;

    Assert(Arena);

    if (Size)
    {
        if ((Arena->Size - Arena->Used) >= Size)
        {
            Result = OffsetPtr(Arena->Base, Arena->Used);
            Arena->Used += Size;
            if (Alignment)
            {
                void* ResultAligned = AlignPtr(Result, Alignment);
                Arena->Used += (size_t)((u8*)ResultAligned - (u8*)Result);
                Result = ResultAligned;
            }
        
            if (Data)
            {
                memcpy(Result, Data, Size);
            }
            else if (Flags & MemPush_Clear)
            {
                memset(Result, 0, Size);
            }
        }
        else
        {
            UnhandledError("Arena out of memory");
        }
    }
    return Result;
}

template<typename T>
inline T* PushStruct(memory_arena* Arena, flags32 Flags /*= 0*/, const T* InitialData /*= nullptr*/)
{
    T* Result = (T*)PushSize(Arena, sizeof(T), alignof(T), Flags, InitialData);
    return Result;
}

template<typename T>
inline T* PushArray(memory_arena* Arena, size_t Count, flags32 Flags /*= 0*/, const T* InitialData /*= nullptr*/)
{
    T* Result = (T*)PushSize(Arena, sizeof(T) * Count, alignof(T), Flags, InitialData);
    return Result;
}

inline memory_arena_checkpoint ArenaCheckpoint(memory_arena* Arena)
{
    memory_arena_checkpoint Checkpoint = {};
    if (Arena)
    {
        Checkpoint.Used = Arena->Used;
        Checkpoint.Arena = Arena;
    }
    return(Checkpoint);
}
inline void RestoreArena(memory_arena* Arena, memory_arena_checkpoint Checkpoint)
{
    Assert(Arena == Checkpoint.Arena);
    Arena->Used = Checkpoint.Used;
}

template<typename T>
inline T* FreeListInitialize(T* Head, size_t Count)
{
    if (Count)
    {
        T* At = Head;
        for (size_t i = 0; i < Count - 1; i++)
        {
            At->Next = Head + i + 1;
            At++;
        }
        At->Next = nullptr;
    }
    return Head;
}

template<typename T>
inline T* FreeListAllocate(T*& List)
{
    T* Result = List;
    if (Result)
    {
        List = Result->Next;
        Result->Next = nullptr;
    }
    return Result;
}

template<typename T>
inline void FreeListFree(T*& List, T* Elem)
{
    if (Elem)
    {
        Elem->Next = List;
        List = Elem;
    }
}

template<typename T>
inline T* DListInitialize(T* Sentinel)
{
    Sentinel->Next = Sentinel->Prev = Sentinel;
    return Sentinel;
}

template<typename T>
inline bool DListIsEmpty(const T* Sentinel)
{
    bool Result = (Sentinel->Next == Sentinel);
    return Result;
}

template<typename T>
inline T* DListInsert(T* Sentinel, T* Elem)
{
    Elem->Next = Sentinel->Next;
    Elem->Prev = Sentinel;
    Elem->Next->Prev = Elem;
    Elem->Prev->Next = Elem;
    return Elem;
}

template<typename T>
inline T* DListRemove(T* Elem)
{
    if (Elem)
    {
        Elem->Next->Prev = Elem->Prev;
        Elem->Prev->Next = Elem->Next;
        Elem->Next = Elem->Prev = nullptr;
    }
    return Elem;
}


//
// Math
//

inline u32 RandU32(entropy32* Entropy)
{
    // NOTE(boti): XorShift32
    u32 Value = Entropy->Value;
    Value ^= Value << 13;
    Value ^= Value >> 17;
    Value ^= Value << 5;
    Entropy->Value = Value;
    return(Value);
}

inline f32 RandUnilateral(entropy32* Entropy)
{
    u32 Value = RandU32(Entropy);
    // NOTE(boti): Shove the bits into the mantissa and set the exponent to 1
    // to get a value in [1,2).
    Value = (F32_EXPONENT_BIAS << F32_EXPONENT_SHIFT) | (Value & F32_MANTISSA_MASK);
    f32 Result = *((f32*)&Value) - 1.0f; // NOTE(boti): subtract 1 to get [0,1)
    return(Result);
}

inline f32 RandBilateral(entropy32* Entropy)
{
    f32 Result = 2.0f * RandUnilateral(Entropy) - 1.0f;
    return(Result);
}

inline f32 Ratio0(f32 Numerator, f32 Denominator)
{
    f32 Result = (Denominator == 0.0f) ? 0.0f : Numerator / Denominator;
    return(Result);
}

inline constexpr u32 CeilDiv(u32 x, u32 y)
{
    u32 Result = (x / y) + ((x % y) != 0);
    return Result;
}

template<typename T>
inline constexpr T Min(T a, T b)
{
    return (a < b) ? a : b;
}

template<typename T>
inline constexpr T Max(T a, T b)
{
    return (a < b) ? b : a;
}

template<>
inline constexpr v2 Min<v2>(v2 a, v2 b)
{
    v2 Result = { Min(a.x, b.x), Min(a.y, b.y) };
    return Result;
}

template<>
inline constexpr v2 Max<v2>(v2 a, v2 b)
{
    v2 Result = { Max(a.x, b.x), Max(a.y, b.y) };
    return Result;
}

template<typename T>
inline constexpr T Clamp(T x, T e0, T e1)
{
    T Result = Min(Max(x, e0), e1);
    return Result;
}

template<typename T> inline constexpr
T Lerp(T a, T b, f32 t)
{
    T Result = (1.0f - t)*a + t*b;
    return Result;
}

inline constexpr f32 ToRadians(f32 Degrees)
{
    f32 Result = Pi * Degrees / 180.0f;
    return Result;
}
inline constexpr f32 ToDegrees(f32 Radians)
{
    f32 Result = 180.0f * Radians / Pi;
    return Result;
}

inline v2 operator-(v2 v)
{
    v2 Result = { -v.x, -v.y };
    return Result;
}

inline v2 operator*(v2 v, f32 s)
{
    v2 Result = { v.x * s, v.y * s };
    return Result;
}

inline v2 operator*(f32 s, v2 v)
{
    v2 Result = v * s;
    return Result;
}

inline v2 operator+(v2 a, v2 b)
{
    v2 Result = { a.x + b.x, a.y + b.y };
    return Result;
}

inline v2 operator-(v2 a, v2 b)
{
    v2 Result = { a.x - b.x, a.y - b.y };
    return Result;
}

inline v2& operator*=(v2& a, f32 s)
{
    a = a * s;
    return a;
}

inline v2& operator+=(v2& a, v2 b)
{
    a = a + b;
    return a;
}

inline v2& operator-=(v2& a, v2 b)
{
    a = a - b;
    return a;
}

inline f32 Dot(v2 a, v2 b)
{
    f32 Result = a.x*b.x + a.y*b.y;
    return Result;
}

inline f32 VectorLength(v2 v)
{
    f32 Result = Sqrt(Dot(v, v));
    return Result;
}

inline v2 Normalize(v2 v)
{
    f32 InvLength = 1.0f / VectorLength(v);
    v2 Result = v * InvLength;
    return Result;
}

inline v2 NOZ(v2 v)
{
    v2 Result = {};

    f32 LengthSq = Dot(v, v);
    if (LengthSq >= NOZ_Threshold)
    {
        f32 InvLength = 1.0f / Sqrt(LengthSq);
        Result = v * InvLength;
    }

    return(Result);
}

inline v3 operator-(v3 v)
{
    v3 Result = { -v.x, -v.y, -v.z };
    return Result;
}

inline v3 operator*(v3 v, f32 s)
{
    v3 Result = { v.x * s, v.y * s, v.z * s };
    return Result;
}

inline v3 operator*(f32 s, v3 v)
{
    v3 Result = v*s;
    return Result;
}

inline v3 operator+(v3 a, v3 b)
{
    v3 Result = { a.x + b.x, a.y + b.y, a.z + b.z };
    return Result;
}

inline v3 operator-(v3 a, v3 b)
{
    v3 Result = { a.x - b.x, a.y - b.y, a.z - b.z };
    return Result;
}

inline v3& operator+=(v3& a, v3 b)
{
    a = a + b;
    return a;
}

inline v3& operator-=(v3& a, v3 b)
{
    a = a - b;
    return a;
}

inline bool IsZeroVector(v3 v)
{
    bool Result = (v.x == 0.0f) && (v.y == 0.0f) && (v.z == 0.0f);
    return Result;
}

inline f32 Dot(v3 a, v3 b)
{
    f32 Result = a.x*b.x + a.y*b.y + a.z*b.z;
    return Result;
}

inline f32 VectorLength(v3 v)
{
    f32 Result = Sqrt(Dot(v, v));
    return Result;
}

inline v3 Normalize(v3 v)
{
    f32 InvLength = 1.0f / VectorLength(v);
    v3 Result = v * InvLength;
    return Result;
}

inline v3 NOZ(v3 v)
{
    v3 Result = {};

    f32 LengthSq = Dot(v, v);
    if (LengthSq >= NOZ_Threshold)
    {
        f32 InvLength = 1.0f / Sqrt(LengthSq);
        Result = v * InvLength;
    }

    return(Result);
}

inline v3 Cross(v3 a, v3 b)
{
    v3 Result = 
    {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
    return Result;
}

inline v4 operator-(v4 v)
{
    v4 Result = { -v.x, -v.y, -v.z, -v.w };
    return Result;
}
inline v4 operator*(v4 v, f32 s)
{
    v4 Result = { s*v.x, s*v.y, s*v.z, s*v.w };
    return Result;
}

inline v4 operator*(f32 s, v4 v)
{
    v4 Result = { s*v.x, s*v.y, s*v.z, s*v.w };
    return Result;
}

inline v4 operator+(v4 a, v4 b)
{
    v4 Result = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    return Result;
}

inline v4 operator-(v4 a, v4 b)
{
    v4 Result = { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    return Result;
}

inline f32 Dot(v4 a, v4 b)
{
    f32 Result = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    return Result;
}

inline v4 Normalize(v4 V)
{
    v4 Result;
    f32 InvLength = 1.0f / Sqrt(Dot(V, V));
    Result = V * InvLength;
    return(Result);
}

inline m4 M4(
    f32 m00, f32 m01, f32 m02, f32 m03,
    f32 m10, f32 m11, f32 m12, f32 m13,
    f32 m20, f32 m21, f32 m22, f32 m23,
    f32 m30, f32 m31, f32 m32, f32 m33)
{
    m4 Result = 
    {
        m00, m10, m20, m30,
        m01, m11, m21, m31,
        m02, m12, m22, m32,
        m03, m13, m23, m33,
    };
    return Result;
}

inline m4 operator*(const m4& A, const m4& B)
{
    m4 Result;

    for (int Column = 0; Column < 4; Column++)
    {
        Result.C[Column] = 
            A.X * B.C[Column].x +
            A.Y * B.C[Column].y + 
            A.Z * B.C[Column].z +
            A.P * B.C[Column].w;
    }
    return Result;
}

inline v4 operator*(const m4& M, const v4& v)
{
    v4 Result = M.X * v.x + M.Y * v.y + M.Z * v.z + M.P * v.w;
    return Result;
}

inline v4 operator*(const v4& v, const m4& M)
{
    v4 Result = { Dot(M.X, v), Dot(M.Y, v), Dot(M.Z, v), Dot(M.P, v) };
    return Result;
}

inline v3 TransformPoint(const m4& M, v3 v)
{
    v3 Result = M.X.xyz * v.x + M.Y.xyz * v.y + M.Z.xyz * v.z + M.P.xyz;
    return Result;
}

inline v3 TransformDirection(const m4& M, v3 v)
{
    v3 Result = M.X.xyz * v.x + M.Y.xyz * v.y + M.Z.xyz * v.z;
    return Result;
}

inline m4 AffineOrthonormalInverse(const m4& M)
{
    m4 Result = M4(
        M.X.x, M.X.y, M.X.z, -Dot(M.X.xyz, M.P.xyz),
        M.Y.x, M.Y.y, M.Y.z, -Dot(M.Y.xyz, M.P.xyz),
        M.Z.x, M.Z.y, M.Z.z, -Dot(M.Z.xyz, M.P.xyz),
        0.0f, 0.0f, 0.0f, 1.0f);
    return Result;
}

inline m4 AffineInverse(const m4& M)
{
    v3 X = M.X.xyz;
    v3 Y = M.Y.xyz;
    v3 Z = M.Z.xyz;
    v3 P = M.P.xyz;
    X = X * (1.0f / Dot(X, X));
    Y = Y * (1.0f / Dot(Y, Y));
    Z = Z * (1.0f / Dot(Z, Z));

    m4 Result = M4(
        X.x, X.y, X.z, -Dot(X, P),
        Y.x, Y.y, Y.z, -Dot(Y, P),
        Z.x, Z.y, Z.z, -Dot(Z, P),
        0.0f, 0.0f, 0.0f, 1.0f);
    return(Result);
}

inline m4 PerspectiveFov(f32 Fov, f32 AspectRatio, f32 NearZ, f32 FarZ)
{
    f32 FocalLength = 1.0f / Tan(0.5f * Fov);
    f32 ZRange = FarZ - NearZ;

    m4 Result = M4(
        FocalLength / AspectRatio, 0.0f, 0.0f, 0.0f,
        0.0f, FocalLength, 0.0f, 0.0f,
        0.0f, 0.0f, FarZ / ZRange, -FarZ*NearZ / ZRange,
        0.0f, 0.0f, 1.0f, 0.0f);
    return Result;
}

inline m4 Identity4()
{
    m4 Result = M4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    return(Result);
}

inline bool PointRectOverlap(v2 P, mmrect2 Rect)
{
    bool Result = (Rect.Min.x <= P.x) && (P.x < Rect.Max.x) &&
        (Rect.Min.y <= P.y) && (P.y < Rect.Max.y);
    return Result;
}

inline m4 QuaternionToM4(v4 Q)
{
    f32 x = Q.x;
    f32 y = Q.y;
    f32 z = Q.z;
    f32 w = Q.w;
    f32 x2 = x*x;
    f32 y2 = y*y;
    f32 z2 = z*z;
    f32 w2 = w*w;
    f32 xy = x*y;
    f32 xz = x*z;
    f32 wx = x*w;
    f32 yz = y*z;
    f32 wy = y*w;
    f32 wz = z*w;

    m4 Result = M4(1.0f - 2.0f * (y2 + z2), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,
                   2.0f * (xy + wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz - wx), 0.0f,
                   2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (x2 + y2), 0.0f,
                   0.0f, 0.0f, 0.0f, 1.0f);
    return Result;
}