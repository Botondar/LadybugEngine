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

#define LB_Concat_(A, B) A##B
#define LB_Concat(A, B) LB_Concat_(A, B)

#include <cstdint>
#include <cstddef>
#include <cfloat>
#include <cassert>
#include <cstdlib>

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
typedef u64 flags64;

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

#define KiB(x) (1024*(x))
#define MiB(x) (1024*KiB(x))
#define GiB(x) (1024llu*MiB(x))
// ^NOTE(boti): we cast GiBs to 64 bits because that's probably what you want

#define Assert(...) assert(__VA_ARGS__)
#define InvalidCodePath Assert(!"Invalid code path")
#define InvalidDefaultCase default: InvalidCodePath; break

#if DEVELOPER
#define Verify(...) assert(__VA_ARGS__)
#define UnhandledError(msg) Assert(!msg)
#define UnimplementedCodePath Assert(!"Unimplemented code path")
#else
#define Verify(...) static_assert(false, "Verify in non-developer build")
#define UnhandledError(...) static_assert(false, "Unhandled error")
#define UnimplementedCodePath static_assert(false, "Unimplemented code path")
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

inline umm Align(umm Value, umm Alignment)
{
    umm Result = (Value + Alignment - 1) & ~(Alignment - 1);
    return(Result);
}

inline u32 Align(u32 Value, u32 Alignment)
{
    u32 Result = (Value + Alignment - 1) & ~(Alignment - 1);
    return(Result);
}

inline void* AlignPtr(void* Base, umm Alignment)
{
    void* Result = (void*)Align((umm)Base, Alignment);
    return(Result);
}

inline void* OffsetPtr(void* Base, umm Size)
{
    void* Result = (void*)((u8*)Base + Size);
    return(Result);
}

inline const void* OffsetPtr(const void* Base, umm Size)
{
    const void* Result = (const void*)((u8*)Base + Size);
    return(Result);
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

typedef flags32 memory_push_flags;
enum memory_push_flag_bits : memory_push_flags
{
    MemPush_None = 0,
    MemPush_Clear = 1 << 0,
};

struct memory_arena
{
    umm Size;
    umm Used;
    void* Base;
};

inline memory_arena InitializeArena(size_t Size, void* Base);

struct memory_arena_checkpoint
{
    umm Used;
    memory_arena* Arena;
};

inline memory_arena_checkpoint ArenaCheckpoint(memory_arena* Arena);
inline void RestoreArena(memory_arena* Arena, memory_arena_checkpoint Checkpoint);
inline void ResetArena(memory_arena* Arena);

#define PushStruct(Arena, Flags, Type) (Type*)PushSize_(Arena, Flags, sizeof(Type), alignof(Type))
#define PushArray(Arena, Flags, Type, Count) (Type*)PushSize_(Arena, Flags, (umm)(Count) * sizeof(Type), alignof(Type))
inline void* PushSize_(memory_arena* Arena, memory_push_flags Flags, umm Size, umm Alignment);

#define DList_Initialize(pSentinel, Next, Prev) \
    (pSentinel)->Prev = (pSentinel)->Next = (pSentinel)

#define DList_IsEmpty(pSentinel, Next, Prev) ((pSentinel)->Next == pSentinel)

#define DList_InsertFront(pSentinel, pElem, Next, Prev) \
    (pElem)->Next = (pSentinel)->Next; \
    (pElem)->Prev = (pSentinel); \
    (pElem)->Next->Prev = (pElem); \
    (pElem)->Prev->Next = (pElem)

#define DList_Remove(pElem, Next, Prev) \
    (pElem)->Next->Prev = (pElem)->Prev; \
    (pElem)->Prev->Next = (pElem)->Next; \
    (pElem)->Next = (pElem)->Prev = nullptr


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

union v2u;
union v3u;
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


inline constexpr f32 Abs(f32 x) { return x < 0.0f ? -x : x; }
inline constexpr f32 Sgn(f32 x) 
{ 
    f32 Result = 0.0f;
    if      (x < 0.0f) Result = -1.0f;
    else if (x > 0.0f) Result = +1.0f;
    return(Result);
}

inline f32 Ratio0(f32 Numerator, f32 Denominator);
inline f32 Modulo(f32 x, f32 d) { return fmodf(x, d); }
inline f32 Modulo0(f32 x, f32 d);
inline constexpr u32 CeilDiv(u32 x, u32 y);

inline f32 Floor(f32 x) { return floorf(x); }
inline f32 Ceil(f32 x) { return ceilf(x); }
inline f32 Round(f32 x) { return roundf(x); }

inline f32 Square(f32 x) { return x*x; }
inline f32 Sqrt(f32 x) { return sqrtf(x); }
inline f32 Exp2(f32 x) { return exp2f(x); }
inline f32 Log2(f32 x) { return log2f(x); }

inline f32 Sin(f32 x) { return sinf(x); }
inline f32 Cos(f32 x) { return cosf(x); }
inline f32 Tan(f32 x) { return tanf(x); }

inline f32 ASin(f32 x) { return asinf(x); }
inline f32 ACos(f32 x) { return acosf(x); }
inline f32 ATan2(f32 y, f32 x) { return atan2f(y, x); }

inline f32 Ln(f32 x) { return logf(x); }
inline f32 Pow(f32 a, f32 b) { return powf(a, b); }

template<typename T> inline constexpr 
T Min(T a, T b);
template<typename T> inline constexpr 
T Max(T a, T b);

inline constexpr v2 Min(v2 a, v2 b);
inline constexpr v2 Max(v2 a, v2 b);
inline constexpr v3 Min(v3 A, v3 B);
inline constexpr v3 Max(v3 A, v3 B);

template<typename T> inline constexpr 
T Clamp(T x, T e0, T e1);

template<typename T> inline constexpr
T Lerp(T a, T b, f32 t);

inline constexpr f32 ToRadians(f32 Degrees);
inline constexpr f32 ToDegrees(f32 Radians);

inline bool PointRectOverlap(v2 P, mmrect2 Rect);

union v2u
{
    u32 E[2];
    struct { u32 X, Y; };
};

union v3u
{
    u32 E[3];
    struct { u32 X, Y, Z; };
};

union v2
{
    f32 E[2];
    struct { f32 X, Y; };
};

union v3
{
    f32 E[3];
    struct { f32 X, Y, Z; };
    struct 
    { 
        v2 XY;
        f32 Ignored0_;
    };
};

union v4
{
    f32 E[4];
    struct { f32 X, Y, Z, W; };
    struct 
    {
        v3 XYZ;
        f32 Ignored0_;
    };
};

// NOTE(boti): column major matrix order:
//   [ 0, 4,  8, 12 ]
//   [ 1, 5,  9, 13 ]
//   [ 2, 6, 10, 14 ]
//   { 3, 7, 11, 15 ]
union m3
{
    f32 E[3][3];
    f32 EE[9];
    struct
    {
        v3 X, Y, Z;
    };
};

union m3x4
{
    f32 E[4][3];
    f32 EE[12];
    struct
    {
        v3 X, Y, Z, P;
    };
};

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
inline v2 Hadamard(v2 a, v2 b);

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
inline v3 Hadamard(v3 a, v3 b);
inline v3 Projection(v3 A, v3 N);
inline v3 Rejection(v3 A, v3 N);
inline v3 PlaneProjection(v3 A, v3 N);

inline v4 operator-(v4 v);
inline v4 operator*(v4 v, f32 s);
inline v4 operator*(f32 s, v4 v);
inline v4 operator+(v4 a, v4 b);
inline v4 operator-(v4 a, v4 b);
inline v4& operator+=(v4& a, v4 b);
inline v4& operator-=(v4& a, v4 b);
inline v4& operator*=(v4& v, f32 s);

inline f32 Dot(v4 a, v4 b);
inline v4 Normalize(v4 V);
// NOTE(boti): This is _not_ an slerp
inline v4 QLerp(v4 A, v4 B, f32 t);

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

inline v4 QMul(v4 A, v4 B);
inline m4 QuaternionToM4(v4 Q);
inline v4 QuatFromAxisAngle(v3 Axis, f32 Angle);

//
// Random
//
struct entropy32
{
    u32 Value;
};

inline u32 RandU32(entropy32* Entropy);
inline f32 RandUnilateral(entropy32* Entropy);
inline f32 RandBilateral(entropy32* Entropy);
inline f32 RandBetween(entropy32* Entropy, f32 Minimum, f32 Maximum);
inline v2 RandInUnitCircle(entropy32* Entropy);

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

inline void* 
PushSize_(memory_arena* Arena, memory_push_flags Flags, umm Size, umm Alignment)
{
    void* Result = nullptr;
    if (Size)
    {
        umm EffectiveAt = Alignment ? Align(Arena->Used, Alignment) : Arena->Used;
        if (EffectiveAt + Size <= Arena->Size)
        {
            Result = OffsetPtr(Arena->Base, EffectiveAt);
            Arena->Used = EffectiveAt + Size;
        
            if (Flags & MemPush_Clear)
            {
                memset(Result, 0, Size);
            }
        }
        else
        {
#if DEVELOPER
            UnhandledError("Arena out of memory");
#endif
        }
    }
    return Result;
}

//
// Random
//

inline u32 RandU32(entropy32* Entropy)
{
    // NOTE(boti): PCG32
    Entropy->Value = Entropy->Value * 747796405u + 2891336453u;
    u32 Result = ((Entropy->Value >> ((Entropy->Value >> 28u) + 4u)) ^ Entropy->Value) * 277803737u;
    Result = (Result >> 22u) ^ Result;
    return(Result);
}

inline f32 RandUnilateral(entropy32* Entropy)
{
    u32 Value = RandU32(Entropy);
    // NOTE(boti): Shove the bits into the mantissa and set the exponent to 1 to get a value in [1,2).
    Value = (F32_EXPONENT_BIAS << F32_EXPONENT_SHIFT) | (Value & F32_MANTISSA_MASK);
    f32 Result = *((f32*)&Value) - 1.0f; // NOTE(boti): subtract 1 to get [0,1)
    return(Result);
}

inline f32 RandBilateral(entropy32* Entropy)
{
    f32 Result = 2.0f * RandUnilateral(Entropy) - 1.0f;
    return(Result);
}

inline f32 RandBetween(entropy32* Entropy, f32 Minimum, f32 Maximum)
{
    f32 Result = (Maximum - Minimum) * RandUnilateral(Entropy) + Minimum;
    return(Result);
}

inline v2 RandInUnitCircle(entropy32* Entropy)
{
    f32 Angle = 2.0f * Pi * RandUnilateral(Entropy);
    f32 R = Sqrt(RandUnilateral(Entropy));

    v2 Result = R * v2{ Cos(Angle), Sin(Angle) };
    return(Result);
}

//
// Math
//

inline f32 Modulo0(f32 x, f32 d)
{
    f32 Result = (d == 0.0f) ? 0.0f : Modulo(x, d);
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
    return(Result);
}

inline constexpr umm CeilDiv(umm x, umm y)
{
    umm Result = (x / y) + ((x % y) != 0);
    return(Result);
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

inline constexpr v2 Min(v2 A, v2 B)
{
    v2 Result = { Min(A.X, B.X), Min(A.Y, B.Y) };
    return(Result);
}

inline constexpr v2 Max(v2 A, v2 B)
{
    v2 Result = { Max(A.X, B.X), Max(A.Y, B.Y) };
    return(Result);
}

inline constexpr v3 Min(v3 A, v3 B)
{
    v3 Result = { Min(A.X, B.X), Min(A.Y, B.Y), Min(A.Z, B.Z) };
    return(Result);
}
inline constexpr v3 Max(v3 A, v3 B)
{
    v3 Result = { Max(A.X, B.X), Max(A.Y, B.Y), Max(A.Z, B.Z) };
    return(Result);
}

template<typename T>
inline constexpr T Clamp(T x, T e0, T e1)
{
    T Result = Min(Max(x, e0), e1);
    return(Result);
}

template<typename T> inline constexpr
T Lerp(T a, T b, f32 t)
{
    T Result = (1.0f - t)*a + t*b;
    return(Result);
}

inline constexpr f32 ToRadians(f32 Degrees)
{
    f32 Result = Pi * Degrees / 180.0f;
    return(Result);
}
inline constexpr f32 ToDegrees(f32 Radians)
{
    f32 Result = 180.0f * Radians / Pi;
    return(Result);
}

inline v2 operator-(v2 V)
{
    v2 Result = { -V.X, -V.Y };
    return(Result);
}

inline v2 operator*(v2 V, f32 s)
{
    v2 Result = { V.X * s, V.Y * s };
    return(Result);
}

inline v2 operator*(f32 s, v2 V)
{
    v2 Result = V * s;
    return(Result);
}

inline v2 operator+(v2 A, v2 B)
{
    v2 Result = { A.X + B.X, A.Y + B.Y };
    return(Result);
}

inline v2 operator-(v2 A, v2 B)
{
    v2 Result = { A.X - B.X, A.Y - B.Y };
    return(Result);
}

inline v2& operator*=(v2& A, f32 s)
{
    A = A * s;
    return(A);
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

inline f32 Dot(v2 A, v2 B)
{
    f32 Result = A.X*B.X + A.Y*B.Y;
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

inline v2 Hadamard(v2 A, v2 B)
{
    v2 Result = { A.X*B.X, A.Y*B.Y };
    return(Result);
}

inline v3 Projection(v3 A, v3 N)
{
    v3 Result = Dot(A, N) * N;
    return(Result);
}

inline v3 Rejection(v3 A, v3 N)
{
    v3 Result = A - Dot(A, N) * N;
    return(Result);
}

inline v3 operator-(v3 V)
{
    v3 Result = { -V.X, -V.Y, -V.Z };
    return(Result);
}

inline v3 operator*(v3 V, f32 s)
{
    v3 Result = { V.X * s, V.Y * s, V.Z * s };
    return(Result);
}

inline v3 operator*(f32 s, v3 V)
{
    v3 Result = V*s;
    return(Result);
}

inline v3 operator+(v3 A, v3 B)
{
    v3 Result = { A.X + B.X, A.Y + B.Y, A.Z + B.Z };
    return(Result);
}

inline v3 operator-(v3 A, v3 B)
{
    v3 Result = { A.X - B.X, A.Y - B.Y, A.Z - B.Z };
    return(Result);
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

inline bool IsZeroVector(v3 V)
{
    bool Result = (V.X == 0.0f) && (V.Y == 0.0f) && (V.Z == 0.0f);
    return(Result);
}

inline f32 Dot(v3 A, v3 B)
{
    f32 Result = A.X*B.X + A.Y*B.Y + A.Z*B.Z;
    return(Result);
}

inline f32 VectorLength(v3 V)
{
    f32 Result = Sqrt(Dot(V, V));
    return(Result);
}

inline v3 Normalize(v3 v)
{
    f32 InvLength = 1.0f / VectorLength(v);
    v3 Result = v * InvLength;
    return(Result);
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

inline v3 Cross(v3 A, v3 B)
{
    v3 Result = 
    {
        A.Y * B.Z - A.Z * B.Y,
        A.Z * B.X - A.X * B.Z,
        A.X * B.Y - A.Y * B.X,
    };
    return(Result);
}

inline v3 Hadamard(v3 A, v3 B)
{
    v3 Result = { A.X*B.X, A.Y*B.Z, A.Z*B.Z };
    return(Result);
}

inline v4 operator-(v4 V)
{
    v4 Result = { -V.X, -V.Y, -V.Z, -V.W };
    return Result;
}
inline v4 operator*(v4 V, f32 s)
{
    v4 Result = { s*V.X, s*V.Y, s*V.Z, s*V.W };
    return Result;
}

inline v4 operator*(f32 s, v4 V)
{
    v4 Result = { s*V.X, s*V.Y, s*V.Z, s*V.W };
    return Result;
}

inline v4 operator+(v4 A, v4 B)
{
    v4 Result = { A.X + B.X, A.Y + B.Y, A.Z + B.Z, A.W + B.W };
    return Result;
}

inline v4 operator-(v4 A, v4 B)
{
    v4 Result = { A.X - B.X, A.Y - B.Y, A.Z - B.Z, A.W - B.W };
    return Result;
}

inline v4& operator+=(v4& a, v4 b)
{
    a = a + b;
    return(a);
}
inline v4& operator-=(v4& a, v4 b)
{
    a = a - b;
    return(a);
}

inline v4& operator*=(v4& v, f32 s)
{
    v = v * s;
    return(v);
}

inline f32 Dot(v4 A, v4 B)
{
    f32 Result = A.X*B.X + A.Y*B.Y + A.Z*B.Z + A.W*B.W;
    return Result;
}

inline v4 Normalize(v4 V)
{
    v4 Result;
    f32 InvLength = 1.0f / Sqrt(Dot(V, V));
    Result = V * InvLength;
    return(Result);
}

inline v4 QLerp(v4 A, v4 B, f32 t)
{
    if (Dot(A, B) < 0.0f)
    {
        B = -B;
    }

    v4 Result = Normalize(Lerp(A, B, t));
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
            A.X * B.C[Column].X +
            A.Y * B.C[Column].Y + 
            A.Z * B.C[Column].Z +
            A.P * B.C[Column].W;
    }
    return Result;
}

inline v4 operator*(const m4& M, const v4& V)
{
    v4 Result = M.X * V.X + M.Y * V.Y + M.Z * V.Z + M.P * V.W;
    return Result;
}

inline v4 operator*(const v4& v, const m4& M)
{
    v4 Result = { Dot(M.X, v), Dot(M.Y, v), Dot(M.Z, v), Dot(M.P, v) };
    return Result;
}

inline v3 TransformPoint(const m4& M, v3 V)
{
    v3 Result = M.X.XYZ * V.X + M.Y.XYZ * V.Y + M.Z.XYZ * V.Z + M.P.XYZ;
    return Result;
}

inline v3 TransformDirection(const m4& M, v3 V)
{
    v3 Result = M.X.XYZ * V.X + M.Y.XYZ * V.Y + M.Z.XYZ * V.Z;
    return Result;
}

inline m4 AffineOrthonormalInverse(const m4& M)
{
    m4 Result = M4(
        M.X.X, M.X.Y, M.X.Z, -Dot(M.X.XYZ, M.P.XYZ),
        M.Y.X, M.Y.Y, M.Y.Z, -Dot(M.Y.XYZ, M.P.XYZ),
        M.Z.X, M.Z.Y, M.Z.Z, -Dot(M.Z.XYZ, M.P.XYZ),
        0.0f, 0.0f, 0.0f, 1.0f);
    return Result;
}

inline m4 AffineInverse(const m4& M)
{
    v3 X = M.X.XYZ;
    v3 Y = M.Y.XYZ;
    v3 Z = M.Z.XYZ;
    v3 P = M.P.XYZ;
    X = X * (1.0f / Dot(X, X));
    Y = Y * (1.0f / Dot(Y, Y));
    Z = Z * (1.0f / Dot(Z, Z));

    m4 Result = M4(
        X.X, X.Y, X.Z, -Dot(X, P),
        Y.X, Y.Y, Y.Z, -Dot(Y, P),
        Z.X, Z.Y, Z.Z, -Dot(Z, P),
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
    bool Result = (Rect.Min.X <= P.X) && (P.X < Rect.Max.X) &&
        (Rect.Min.Y <= P.Y) && (P.Y < Rect.Max.Y);
    return(Result);
}

inline v4 QMul(v4 A, v4 B)
{
    v4 Result;
    Result.X = A.W*B.X + B.W*A.X + A.Y*B.Z - A.Z*B.Y;
    Result.Y = A.W*B.Y - A.X*B.Z + A.Y*B.W + A.Z*B.X;
    Result.Z = A.W*B.Z + A.X*B.Y - A.Y*B.X + A.Z*B.W;
    Result.W = A.W*B.W - A.X*B.X - A.Y*B.Y - A.Z*B.Z;
    return(Result);
}

inline v4 QuatFromAxisAngle(v3 Axis, f32 Angle)
{
    f32 C = Cos(0.5f * Angle);
    f32 S = Sin(0.5f * Angle);
    v4 Result = { S * Axis.X, S * Axis.Y, S * Axis.Z, C };
    return(Result);
}

inline m4 QuaternionToM4(v4 Q)
{
    f32 x = Q.X;
    f32 y = Q.Y;
    f32 z = Q.Z;
    f32 w = Q.W;
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