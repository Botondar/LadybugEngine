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

// NOTE(boti): The results of these lbfnts are normalized to the em square, 
//             multiply the result by the render size if you want to use it for layout.

// Get the enclosing rectangle for the text including the full ascent+descent
inline mmrect2 GetTextRect(const font* Font, const char* Text, font_layout_type Layout = font_layout_type::Baseline);

// TODO(boti): This should _NOT_ be inline!
// Currently the renderer depends on fonts, but this shouldn't be the case, the caller should be able to 
// set the texture for rendering and all text rendering should be on the user side (not on the renderer)
inline  mmrect2 GetTextRect(const font* Font, const char* Text, font_layout_type Layout /*= font_layout_type::Baseline*/)
{
    mmrect2 Result = 
    {
        .Min = { +10000.0f, -Font->Ascent },
        .Max = { -10000.0f, +Font->Descent },
    };

    v2 P = { 0.0f, 0.0f };
    for (const char* At = Text; *At; At++)
    {
        f32 MinX = 0.0f;
        f32 MaxX = 0.0f;
        f32 AdvanceX = 0.0f;
        if (*At == '\n')
        {
            P.x = 0.0f;
            P.y += Font->BaselineDistance;
            Result.Max.y += Font->BaselineDistance;
        }
        else
        {
            const font_glyph* Glyph = Font->Glyphs + Font->CharMapping[*At].GlyphIndex;
            MinX = Glyph->P0.x;
            MaxX = Glyph->P1.x;
            AdvanceX += Glyph->AdvanceX;
        }
        
        Result.Min.x = Min(Result.Min.x, P.x + Min(MinX, 0.0f));
        Result.Max.x = Max(Result.Max.x, P.x + Max(MaxX, AdvanceX));

        P.x += AdvanceX;
    }

    if (Layout == font_layout_type::Top)
    {
        Result.Min.y += Font->Ascent;
        Result.Max.y += Font->Ascent;
    }
    else if (Layout == font_layout_type::Bottom)
    {
        Result.Min.y -= Font->Descent;
        Result.Max.y -= Font->Descent;
    }

    return Result;
}