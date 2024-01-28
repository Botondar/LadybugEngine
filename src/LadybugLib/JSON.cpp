#include "JSON.hpp"

//
// Tokenizer interface
//

enum class json_token_type : u32
{
    Undefined = 0,
    // Structural tokens
    OpenBrace, // Curly brace
    CloseBrace,
    OpenBracket, // Square bracket
    CloseBracket,
    Colon,
    Comma,

    String,
    Number,
    Literal,
    Eof,
};

enum class json_token_literal : u32 
{
    True,
    False,
    Null,
};

struct json_token_number
{
    u64 Significand;
    s32 Exponent;
    u32 SignBit;

    s32 ToS32() const;
    u32 ToU32() const;
    f32 ToF32() const;

    json_number ToNumber() const;
};

struct json_token
{
    json_token_type Type;
    b32 IsValid;
    u64 Length;
    const char* Begin;
    union
    {
        json_token_number Number;
        json_token_literal LiteralType;
    };
};

struct json_tokenizer
{
    u64 Length;
    const char* At;

    u64 DEBUGLineIndex;
};

internal json_token GetToken(json_tokenizer* Tokenizer);

//
// Internal parser functions
//

internal bool VerifyObjectAndCountElements(json_tokenizer* Tokenizer, u64* Count);
internal bool VerifyArrayAndCountElements(json_tokenizer* Tokenizer, u64* Count);

internal bool ParseElement(json_tokenizer* Tokenizer, json_element* Element, memory_arena* Arena);

//
// Parser implementation
//

lbfn json_element* ParseJSON(const void* Data, u64 DataSize, memory_arena* Arena)
{
    json_element* Root = nullptr;
    memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Arena);

    Root = PushStruct(Arena, MemPush_Clear, json_element);

    json_tokenizer Tokenizer = { DataSize, (const char*)Data };
    if (ParseElement(&Tokenizer, Root, Arena))
    {
        if (Root->Type != json_element_type::Object &&
            Root->Type != json_element_type::Array)
        {
            Root = nullptr;
        }
    }
    else
    {
        Root = nullptr;
    }
    
    if (!Root)
    {
        RestoreArena(Arena, Checkpoint);
    }

    return Root;
}

internal bool ParseElement(json_tokenizer* Tokenizer, json_element* Element, memory_arena* Arena)
{
    bool Result = true;

    *Element = {};
    json_token Token = GetToken(Tokenizer);
    if (Token.IsValid)
    {
        if (Token.Type == json_token_type::Literal)
        {
            if (Token.LiteralType == json_token_literal::Null) 
            {
                Element->Type = json_element_type::Null;
            }
            else if (Token.LiteralType == json_token_literal::True)
            {
                Element->Type = json_element_type::Boolean;
                Element->Boolean = true;
            }
            else if (Token.LiteralType == json_token_literal::False)
            {
                Element->Type = json_element_type::Boolean;
                Element->Boolean = false;
            }
            else
            {
                // NOTE(boti): If we got here something's wrong, 
                //             the tokenizer should've already failed.
                InvalidCodePath;
                Result = false;
            }
        }
        else if (Token.Type == json_token_type::Number)
        {
            Element->Type = json_element_type::Number;
            Element->Number = Token.Number.ToNumber();
        }
        else if (Token.Type == json_token_type::String)
        {
            // TODO(boti): Escape char conversion
            Element->Type = json_element_type::String;
            Element->String.Length = Token.Length;
            Element->String.String = PushArray(Arena, 0, char, Token.Length);
            memcpy(Element->String.String, Token.Begin, Token.Length);
        }
        else if (Token.Type == json_token_type::OpenBrace)
        {
            Element->Type = json_element_type::Object;

            json_tokenizer ObjectTokenizer = *Tokenizer;
            bool IsObjectValid = VerifyObjectAndCountElements(&ObjectTokenizer, &Element->Object.ElementCount);
            if (IsObjectValid)
            {
                Element->Object.Keys = PushArray(Arena, 0, string, Element->Object.ElementCount);
                Element->Object.Elements = PushArray(Arena, 0, json_element, Element->Object.ElementCount);

                for (u64 i = 0; i < Element->Object.ElementCount; i++)
                {
                    if (i > 0)
                    {
                        Token = GetToken(Tokenizer);
                        if (Token.Type != json_token_type::Comma)
                        {
                            // NOTE(boti): VerifyObjectAndCountElements() should've caught this
                            InvalidCodePath;
                            Result = false;
                            break;
                        }
                    }

                    Token = GetToken(Tokenizer);
                    if (Token.Type != json_token_type::String) 
                    {
                        Result = false;
                        break;
                    }

                    // TODO(boti): escape char conversion
                    Element->Object.Keys[i].Length = Token.Length;
                    Element->Object.Keys[i].String = PushArray(Arena, 0, char, Token.Length);
                    memcpy(Element->Object.Keys[i].String, Token.Begin, Token.Length);

                    Token = GetToken(Tokenizer);
                    if (Token.Type != json_token_type::Colon)
                    {
                        Result = false;
                        break;
                    }

                    if (!ParseElement(Tokenizer, Element->Object.Elements + i, Arena))
                    {
                        Result = false;
                        break;
                    }
                }

                Token = GetToken(Tokenizer);
                if (Token.Type != json_token_type::CloseBrace)
                {
                    // NOTE(boti): VerifyObjectAndCountElements() should've caught this
                    InvalidCodePath;
                    Result = false;
                }
            }
            else
            {
                Result = false;
            }
        }
        else if (Token.Type == json_token_type::OpenBracket)
        {
            Element->Type = json_element_type::Array;

            json_tokenizer ArrayTokenizer = *Tokenizer;
            bool IsArrayValid = VerifyArrayAndCountElements(&ArrayTokenizer, &Element->Array.ElementCount);
            if (IsArrayValid)
            {
                Element->Array.Elements = PushArray(Arena, 0, json_element, Element->Array.ElementCount);
                for (u64 i = 0; i < Element->Array.ElementCount; i++)
                {
                    if (i > 0)
                    {
                        Token = GetToken(Tokenizer);
                        if (Token.Type != json_token_type::Comma)
                        {
                            // NOTE(boti): VerifyArrayAndCountElements() should've caught this
                            InvalidCodePath;
                            Result = false;
                            break;
                        }
                    }

                    if (!ParseElement(Tokenizer, Element->Array.Elements + i, Arena))
                    {
                        Result = false;
                        break;
                    }
                }
                
                Token = GetToken(Tokenizer);
                if (Token.Type != json_token_type::CloseBracket)
                {
                    // NOTE(boti): VerifyArrayAndCountElements() should've caught this
                    InvalidCodePath;
                    Result = false;
                }
            }
            else
            {
                Result = false;
            }
        }
        else
        {
            Result = false;
        }
    }

    return Result;
}

internal bool VerifyObjectAndCountElements(json_tokenizer* Tokenizer, u64* OutCount)
{
    bool Result = false;
    u64 Count = 0;

    for (;;)
    {
        json_token Token = GetToken(Tokenizer);
        if (Token.Type == json_token_type::CloseBrace)
        {
            Result = true;
            break;
        }

        if (Count > 0)
        {
            if (Token.Type == json_token_type::Comma) Token = GetToken(Tokenizer);
            else break;
        }

        if (Token.Type != json_token_type::String) break;

        Token = GetToken(Tokenizer);
        if (Token.Type != json_token_type::Colon) break;

        Token = GetToken(Tokenizer);
        if (Token.Type == json_token_type::Literal || 
            Token.Type == json_token_type::Number ||
            Token.Type == json_token_type::String)
        {
            Count++;
        }
        else if (Token.Type == json_token_type::OpenBrace)
        {
            u64 Depth = 1;
            while (Depth)
            {
                Token = GetToken(Tokenizer);

                if (Token.Type == json_token_type::Eof) break;
                else if (Token.Type == json_token_type::OpenBrace) Depth++;
                else if (Token.Type == json_token_type::CloseBrace) Depth--;
            }

            if (Depth == 0) Count++;
            else break;
        }
        else if (Token.Type == json_token_type::OpenBracket)
        {
            u64 Depth = 1;
            while (Depth)
            {
                Token = GetToken(Tokenizer);

                if (Token.Type == json_token_type::Eof) break;
                else if (Token.Type == json_token_type::OpenBracket) Depth++;
                else if (Token.Type == json_token_type::CloseBracket) Depth--;
            }

            if (Depth == 0) Count++;
            else break;
        }
        else
        {
            break;
        }
    }

    if (Result)
    {
        *OutCount = Count;
    }

    return Result;
}

internal bool VerifyArrayAndCountElements(json_tokenizer* Tokenizer, u64* OutCount)
{
    bool Result = false;
    
    u64 Count = 0;
    for (;;)
    {
        json_token Token = GetToken(Tokenizer);
        if (Token.Type == json_token_type::CloseBracket)
        {
            Result = true;
            break;
        }

        if (Count > 0)
        {
            if (Token.Type == json_token_type::Comma) Token = GetToken(Tokenizer);
            else break;
        }

        if (Token.Type == json_token_type::Literal ||
            Token.Type == json_token_type::Number ||
            Token.Type == json_token_type::String)
        {
            Count++;
        }
        else if (Token.Type == json_token_type::OpenBrace)
        {
            u64 Depth = 1;
            while (Depth)
            {
                Token = GetToken(Tokenizer);
                if (Token.Type == json_token_type::Eof) break;
                else if (Token.Type == json_token_type::OpenBrace) Depth++;
                else if (Token.Type == json_token_type::CloseBrace) Depth--;
            }

            if (Depth == 0) Count++;
            else break;
        }
        else if (Token.Type == json_token_type::OpenBracket)
        {
            u64 Depth = 1;

            while (Depth)
            {
                Token = GetToken(Tokenizer);
                if (Token.Type == json_token_type::Eof) break;
                else if (Token.Type == json_token_type::OpenBracket) Depth++;
                else if (Token.Type == json_token_type::CloseBracket) Depth--;
            }

            if (Depth == 0) Count++;
            else break;
        }
        else
        {
            break;
        }
    }

    if (Result)
    {
        *OutCount = Count;
    }

    return Result;
}

//
// Tokenizer implementation
//

s32 json_token_number::ToS32() const
{
    Assert(Exponent == 0);
    s32 Result = SignBit ? -((s32)Significand) : (s32)Significand;
    return Result;
}

u32 json_token_number::ToU32() const 
{
    Assert(Exponent == 0);
    Assert(SignBit == 0);
    u32 Result = (u32)Significand;
    return Result;
};

f32 json_token_number::ToF32() const 
{
    // TODO(boti): Verify the precision of this
    f32 Result = (f32)Significand;
    Result *= Pow(10.0f, (f32)Exponent);
    Result = SignBit ? -Result : Result;
    return Result;
};

json_number json_token_number::ToNumber() const
{
    json_number Result = {};
    if (Exponent != 0)
    {
        Result.Type = json_number_type::F32;
        Result.F32 = ToF32();
    }
    else if (SignBit != 0)
    {
        Result.Type = json_number_type::S32;
        Result.S32 = ToS32();
    }
    else 
    {
        Result.Type = json_number_type::U32;
        Result.U32 = ToU32();
    }
    return Result;
}


internal json_token GetToken(json_tokenizer* Tokenizer)
{
    json_token Token = {};

    while (Tokenizer->Length && IsSpace(Tokenizer->At[0]))
    {
        if (Tokenizer->At[0] == '\n')
        {
            Tokenizer->DEBUGLineIndex++;
        }
        Tokenizer->Length--;
        Tokenizer->At++;
    }

    if (Tokenizer->Length)
    {
        // Set the initial values for the common (single-char) token case
        Token.IsValid = true;
        Token.Length = 1;
        Token.Begin = Tokenizer->At;
        char Ch = *Tokenizer->At++;
        Tokenizer->Length--;
        switch (Ch)
        {
            case '{': Token.Type = json_token_type::OpenBrace; break;
            case '}': Token.Type = json_token_type::CloseBrace; break;
            case '[': Token.Type = json_token_type::OpenBracket; break;
            case ']': Token.Type = json_token_type::CloseBracket; break;
            case ',': Token.Type = json_token_type::Comma; break;
            case ':': Token.Type = json_token_type::Colon; break;
            default:
            {
                Token.IsValid = false;

                if (Ch == '"')
                {
                    Token.Type = json_token_type::String;
                    Token.Length = 0;
                    Token.Begin = Tokenizer->At;

                    while (Tokenizer->Length)
                    {
                        if (Tokenizer->At[0] == '"' && Tokenizer->At[-1] != '\\')
                        {
                            Tokenizer->At++;
                            Tokenizer->Length--;
                            Token.IsValid = true;
                            break;
                        }

                        Tokenizer->At++;
                        Tokenizer->Length--;
                        Token.Length++;
                    }
                }
                else if (IsDigit(Ch) || Ch == '-')
                {
                    Token.Type = json_token_type::Number;
                    if (Ch == '-')
                    { 
                        Token.Number.SignBit = 1;
                        Ch = *Tokenizer->At++;
                        Tokenizer->Length--;
                    }

                    if (IsDigit(Ch))
                    {
                        Token.IsValid = true;

                        Token.Number.Significand *= 10;
                        Token.Number.Significand += (u32)(Ch - '0');

                        // TODO(boti): detect overflow
                        while (Tokenizer->Length && IsDigit(Tokenizer->At[0]))
                        {
                            Token.Number.Significand *= 10;
                            Token.Number.Significand += (u32)(Tokenizer->At[0] - '0');
                            Token.Length++;
                            Tokenizer->At++;
                            Tokenizer->Length--;
                        }

                        if (Tokenizer->At[0] == '.')
                        {
                            Token.IsValid = false;

                            Token.Length++;
                            Tokenizer->At++;
                            Tokenizer->Length--;

                            if (IsDigit(Tokenizer->At[0]))
                            {
                                Token.IsValid = true;

                                while (Tokenizer->Length && IsDigit(Tokenizer->At[0]))
                                {
                                    Token.Number.Exponent -= 1;
                                    Token.Number.Significand *= 10;
                                    Token.Number.Significand += (u32)(Tokenizer->At[0] - '0');
                                    Token.Length++;
                                    Tokenizer->At++;
                                    Tokenizer->Length--;
                                }
                            }
                        }

                        if (Tokenizer->At[0] == 'e' || Tokenizer->At[0] == 'E')
                        {
                            Token.IsValid = false;

                            Token.Length++;
                            *Tokenizer->At++;
                            Tokenizer->Length--;

                            if (Tokenizer->Length)
                            {
                                s32 ExponentSign = 1;
                                if (Tokenizer->At[0] == '-')
                                {
                                    ExponentSign = -1;
                                    Token.Length++;
                                    Tokenizer->At++;
                                    Tokenizer->Length--;
                                }
                                else if (Ch == '+')
                                {
                                    Token.Length++;
                                    Tokenizer->At++;
                                    Tokenizer->Length--;
                                }

                                if (IsDigit(Tokenizer->At[0]))
                                {
                                    Token.IsValid = true;

                                    s32 Exponent = 0;
                                    while (Tokenizer->Length && IsDigit(Tokenizer->At[0]))
                                    {
                                        Token.Number.Exponent *= 10;
                                        Token.Number.Exponent += (s32)(Tokenizer->At[0] - '0');
                                        Token.Length++;
                                        Ch = *Tokenizer->At++;
                                        Tokenizer->Length--;
                                    }

                                    Token.Number.Exponent += ExponentSign * Exponent;
                                }
                            }
                        }
                    }
                }
                else
                {
                    auto StartsWith = [](u64 Length, const char* Str, const char* Comparand) -> bool
                    {
                        bool Result = true;

                        const char* A = Str;
                        const char* B = Comparand;
                        while (Length-- && *B)
                        {
                            if (*A != *B)
                            {
                                Result = false;
                                break;
                            }
                            A++;
                            B++;
                        }

                        if (*B != 0)
                        {
                            Result = false;
                        }

                        return Result;
                    };

                    if (StartsWith(Tokenizer->Length + 1, Tokenizer->At - 1, "true"))
                    {
                        Token.Type = json_token_type::Literal;
                        Token.IsValid = true;
                        Token.Length = 4;
                        Token.LiteralType = json_token_literal::True;
                        Tokenizer->At += 3;
                        Tokenizer->Length -= 3;
                    }
                    else if (StartsWith(Tokenizer->Length + 1, Tokenizer->At - 1, "false"))
                    {
                        Token.Type = json_token_type::Literal;
                        Token.Length = 5;
                        Token.IsValid = true;
                        Token.LiteralType = json_token_literal::False;
                        Tokenizer->At += 4;
                        Tokenizer->Length -= 4;
                    }
                    else if (StartsWith(Tokenizer->Length + 1, Tokenizer->At - 1, "null"))
                    {
                        Token.Type = json_token_type::Literal;
                        Token.IsValid = true;
                        Token.Length = 4;
                        Token.LiteralType = json_token_literal::Null;
                        Tokenizer->At += 4;
                        Tokenizer->Length -= 4;
                    }
                }
            } break;
        }
    }
    else
    {
        Token.Type = json_token_type::Eof;
        Token.IsValid = true;
    }

    return Token;
}

//
// JSON interface implementation
//

u32 json_number::AsU32() const
{
    u32 Result = 0;
    switch(Type)
    {
        case json_number_type::U32: Result = U32; break;
        case json_number_type::S32: Result = (u32)S32; break;
        case json_number_type::F32: Result = (u32)F32; break;
    }
    return Result;
}
s32 json_number::AsS32() const
{
    s32 Result = 0;
    switch(Type)
    {
        case json_number_type::U32: Result = (s32)U32; break;
        case json_number_type::S32: Result = S32; break;
        case json_number_type::F32: Result = (s32)F32; break;
    }
    return Result;
}
f32 json_number::AsF32() const
{
    f32 Result = 0.0f;
    switch(Type)
    {
        case json_number_type::U32: Result = (f32)U32; break;
        case json_number_type::S32: Result = (f32)S32; break;
        case json_number_type::F32: Result = F32; break;
    }
    return Result;
}

lbfn json_element* GetElement(json_object* Object, const char* Name)
{
    json_element* Result = nullptr;

    for (u64 i = 0; i < Object->ElementCount; i++)
    {
        if (StringEquals(Object->Keys[i], Name))
        {
            Result = Object->Elements + i;
            break;
        }
    }

    return Result;
}