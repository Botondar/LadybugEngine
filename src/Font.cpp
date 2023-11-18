
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
            P.X = 0.0f;
            P.Y += Font->BaselineDistance;
            Result.Max.Y += Font->BaselineDistance;
        }
        else
        {
            const font_glyph* Glyph = Font->Glyphs + Font->CharMapping[*At].GlyphIndex;
            MinX = Glyph->P0.X;
            MaxX = Glyph->P1.X;
            AdvanceX += Glyph->AdvanceX;
        }
        
        Result.Min.X = Min(Result.Min.X, P.X + Min(MinX, 0.0f));
        Result.Max.X = Max(Result.Max.X, P.X + Max(MaxX, AdvanceX));

        P.X += AdvanceX;
    }

    if (Layout == font_layout_type::Top)
    {
        Result.Min.Y += Font->Ascent;
        Result.Max.Y += Font->Ascent;
    }
    else if (Layout == font_layout_type::Bottom)
    {
        Result.Min.Y -= Font->Descent;
        Result.Max.Y -= Font->Descent;
    }

    return Result;
}