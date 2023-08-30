
lbfn mmrect2 GetTextRect(const font* Font, const char* Text, font_layout_type Layout /*= font_layout_type::Baseline*/)
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