#pragma once

#ifndef LADYBUG_ENGINE
#include <cinttypes>
typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);
#endif

#define LBFNT_MAKE_VERSION(major, minor) (((u32)major) << 16) | ((u32)(minor) & 0xFFFFu)

#define LBFNT_VERSION_MAJOR(version) (u32)((version) >> 16)
#define LBFNT_VERSION_MINOR(version) (u32)((version) & 0xFFFFu)

#define LBFNT_CURRENT_VERSION LBFNT_MAKE_VERSION(0, 1)

constexpr u64 LBFNT_FILE_TAG = 0x6c622020666e7420;
#pragma pack(push, 1)

struct lbfnt_charmap
{
    u32 Codepoint;
    u32 GlyphIndex;
};

struct lbfnt_glyph
{
    // Texture location
    f32 u0;
    f32 v0;
    f32 u1;
    f32 v1;

    // Layout information
    f32 AdvanceX;
    f32 OriginY;

    // NOTE(boti): using the bounds we get from the glyph run outline seems to give
    //             the same results as using the bearing data + font info, but certain characters seem incorrect
#if 1
    f32 BoundsLeft;
    f32 BoundsTop;
    f32 BoundsRight;
    f32 BoundsBottom;
#else
    f32 LeftBearing;
    f32 TopBearing;
    f32 RightBearing;
    f32 BottomBearing;
#endif
};

struct lbfnt_font_info
{
    f32 RasterHeight;
    f32 Ascent;
    f32 Descent;
    f32 BaselineDistance;

#if 0
    f32 GlyphBoxLeft;
    f32 GlyphBoxRight;
    f32 GlyphBoxTop;
    f32 GlyphBoxBottom;
    //f32 GlyphOffsetX;
    //f32 GlyphOffsetY;
    f32 GlyphWidth;
    f32 GlyphHeight;
#endif
};

struct lbfnt_bitmap
{
    u32 Width;
    u32 Height;
    u8 Bitmap[]; // NOTE(boti): 8-bit alpha-only bitmap
};

struct lbfnt_file
{
    u64 FileTag;
    u32 Version;

    // NOTE(boti): We'll want to expand this in the future, but for now
    //             we directly map the 0-255 codepoints
    //             and allocate enough storage in the file to hold all the glyphs, 
    //             even if all characters map to a unique glyph.
    lbfnt_font_info FontInfo;
    lbfnt_charmap CharacterMap[256];
    u32 GlyphCount;
    lbfnt_glyph Glyphs[256];

    lbfnt_bitmap Bitmap;
};
#pragma pack(pop)