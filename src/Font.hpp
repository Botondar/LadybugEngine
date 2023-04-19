#pragma once

// TODO(boti): move this
struct font_glyph
{
    v2 UV0;
    v2 UV1;

    v2 P0;
    v2 P1;

    f32 AdvanceX;
    f32 OriginY;
};

struct font_char_mapping
{
    u32 Codepoint;
    u32 GlyphIndex;
};

struct font
{
    f32 RasterHeight;
    f32 Ascent;
    f32 Descent;
    f32 BaselineDistance;

    static constexpr u32 CharCount = 256;
    font_char_mapping CharMapping[CharCount];

    static constexpr u32 MaxGlyphCount = 256;
    u32 GlyphCount;
    font_glyph Glyphs[MaxGlyphCount];
};

enum class font_layout_type : u32
{
    Baseline = 0,
    Top,
    Bottom,
};

// NOTE(boti): The results of these lbfns are  normalized to the em square, 
//             multiply the result by the render size if you want to use it for layout.

// Get the enclosing rectangle for the text including the full ascent+descent
lbfn mmrect2 GetTextRect(const font* Font, const char* Text, font_layout_type Layout = font_layout_type::Baseline);