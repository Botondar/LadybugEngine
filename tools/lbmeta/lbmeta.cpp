#define DEVELOPER 1
#include "LadybugLib/Core.hpp"
#include "LadybugLib/String.hpp"

enum token_type : u32
{
    Token_Unknown = 0,

    Token_KeywordOrIdentifier,
    Token_Number,
    Token_String,
    Token_Char,
    Token_Special,
    Token_Preprocessor,

    //
    // Punctuation marks
    //

    // Triple
    Token_TripleDot,
    Token_PointerToMemberAccess,
    Token_LeftShiftAssign,
    Token_RightShiftAssign,
    Token_Spaceship,

    // Double
    Token_DoublePound,
    Token_DoubleColon,
    Token_DereferenceAccess,
    Token_AddAssign,
    Token_SubAssign,
    Token_MulAssign,
    Token_DivAssign,
    Token_ModAssign,
    Token_XorAssign,
    Token_AndAssign,
    Token_OrAssign,
    Token_Equal,
    Token_NotEqual,
    Token_LessEqual,
    Token_GreaterEqual,
    Token_LogicalAnd,
    Token_LogicalOr,
    Token_LeftShift,
    Token_RightShift,
    Token_Increment,
    Token_Decrement,

    // Single
    Token_Semicolon,
    Token_Colon,
    Token_Comma,
    Token_Dot,
    Token_Pound,
    Token_QuestionMark,
    Token_ExclamationMark,
    Token_Assign,
    Token_OpenBrace,
    Token_CloseBrace,
    Token_OpenParen,
    Token_CloseParen,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_BitNot,
    Token_BitXor,
    Token_BitAnd,
    Token_BitOr,
    Token_Add,
    Token_Sub,
    Token_Asterisk,
    Token_Slash,
    Token_Modulo,
    Token_Less,
    Token_Greater,

    Token_Eof,

    Token_Count,

    Token_Punct3_First          = Token_TripleDot,
    Token_Punct3_OnePastLast    = Token_Spaceship + 1,
    Token_Punct2_First          = Token_DoublePound,
    Token_Punct2_OnePastLast    = Token_Decrement + 1,
    Token_Punct1_First          = Token_Semicolon,
    Token_Punct1_OnePastLast    = Token_Greater + 1,
};

static const char* PunctuationMarks[Token_Count] = 
{
    [Token_TripleDot]               = "...",
    [Token_PointerToMemberAccess]   = "->*",
    [Token_LeftShiftAssign]         = "<<=",
    [Token_RightShiftAssign]        = ">>=",
    [Token_Spaceship]               = "<=>",

    [Token_DoublePound]         = "##",
    [Token_DoubleColon]         = "::",
    [Token_DereferenceAccess]   = "->",
    [Token_AddAssign]           = "+=",
    [Token_SubAssign]           = "-=",
    [Token_MulAssign]           = "*=",
    [Token_DivAssign]           = "/=",
    [Token_ModAssign]           = "%=",
    [Token_XorAssign]           = "^=",
    [Token_AndAssign]           = "&=",
    [Token_OrAssign]            = "|=",
    [Token_Equal]               = "==",
    [Token_NotEqual]            = "!=",
    [Token_LessEqual]           = "<=",
    [Token_GreaterEqual]        = ">=",
    [Token_LogicalAnd]          = "&&",
    [Token_LogicalOr]           = "||",
    [Token_LeftShift]           = "<<",
    [Token_RightShift]          = ">>",
    [Token_Increment]           = "++",
    [Token_Decrement]           = "--",

    [Token_Semicolon]           = ";",
    [Token_Colon]               = ":",
    [Token_Comma]               = ",",
    [Token_Dot]                 = ".",
    [Token_Pound]               = "#",
    [Token_QuestionMark]        = "?",
    [Token_ExclamationMark]     = "!",
    [Token_Assign]              = "=",
    [Token_OpenBrace]           = "{",
    [Token_CloseBrace]          = "}",
    [Token_OpenParen]           = "(",
    [Token_CloseParen]          = ")",
    [Token_OpenBracket]         = "[",
    [Token_CloseBracket]        = "]",
    [Token_BitNot]              = "~",
    [Token_BitXor]              = "^",
    [Token_BitAnd]              = "&",
    [Token_BitOr]               = "|",
    [Token_Add]                 = "+",
    [Token_Sub]                 = "-",
    [Token_Asterisk]            = "*",
    [Token_Slash]               = "/",
    [Token_Modulo]              = "%",
    [Token_Less]                = "<",
    [Token_Greater]             = ">",
};

// NOTE(boti): Partial precedence for enum parsing
static const int OperatorPrecedence[Token_Count] = 
{
    [Token_Asterisk]    = 5,
    [Token_Slash]       = 5,
    [Token_Modulo]      = 5,
    [Token_Add]         = 6,
    [Token_Sub]         = 6,
    [Token_LeftShift]   = 7,
    [Token_RightShift]  = 7,
    [Token_BitAnd]      = 11,
    [Token_BitXor]      = 12,
    [Token_BitOr]       = 13,
};

struct token
{
    token_type Type;
    b32 IsComplete;
    string String;
};

struct tokenizer
{
    umm Count;
    char* At;
    b32 PutBackFlag;
    token LastToken;
};

inline char Peek(tokenizer* Tokenizer);
inline char PeekAhead(tokenizer* Tokenizer, umm Count);
inline void Skip(tokenizer* Tokenizer);
inline char GetAndAdvance(tokenizer* Tokenizer);
inline char AdvanceAndGet(tokenizer* Tokenizer);

inline void PutBackLast(tokenizer* Tokenizer);
static token NextToken(tokenizer* Tokenizer);

//
// Implementation
//
inline char Peek(tokenizer* Tokenizer)
{
    char Result = 0;
    if (Tokenizer->Count)
    {
        Result = Tokenizer->At[0];
    }
    return(Result);
}

inline char PeekAhead(tokenizer* Tokenizer, umm Count)
{
    char Result = 0;
    if (Count < Tokenizer->Count)
    {
        Result = Tokenizer->At[Count];
    }
    return(Result);
}

inline void Skip(tokenizer* Tokenizer)
{
    if (Tokenizer->Count)
    {
        Tokenizer->At++;
        Tokenizer->Count--;
    }
}

inline char GetAndAdvance(tokenizer* Tokenizer)
{
    char Result = 0;
    if (Tokenizer->Count)
    {
        Result = *Tokenizer->At++;
        Tokenizer->Count--;
    }
    return(Result);
}

inline char AdvanceAndGet(tokenizer* Tokenizer)
{
    char Result = 0;
    if (Tokenizer->Count)
    {
        Tokenizer->At++;
        if (--Tokenizer->Count)
        {
            Result = *Tokenizer->At;
        }
    }
    return(Result);
}

inline void PutBackLast(tokenizer* Tokenizer)
{
    Assert(!Tokenizer->PutBackFlag);
    Tokenizer->PutBackFlag = true;
}

static token
NextToken(tokenizer* Tokenizer)
{
    token Token = {};

    if (Tokenizer->PutBackFlag)
    {
        Tokenizer->PutBackFlag = false;
        return(Tokenizer->LastToken);
    }

    b32 HadComment = false;
    do
    {
        HadComment = false;
        while (IsSpace(Peek(Tokenizer)))
        {
            Skip(Tokenizer);
        }

        char Buff[2] = { PeekAhead(Tokenizer, 0), PeekAhead(Tokenizer, 1) };
        if (Buff[0] == '/' && Buff[1] == '/')
        {
            HadComment = true;
            while (Peek(Tokenizer) && Peek(Tokenizer) != '\n')
            {
                Skip(Tokenizer);
            }
        }
        else if (Buff[0] == '/' && Buff[1] == '*')
        {
            HadComment = true;
            // NOTE(boti): Skipping twice prevents accidentally ending the comment on the /*/ case
            Skip(Tokenizer);
            Skip(Tokenizer);

            for (;;)
            {
                if (!Peek(Tokenizer))
                {
                    break;
                }
                else if (Peek(Tokenizer) == '*')
                {
                    Skip(Tokenizer);
                    if (Peek(Tokenizer) == '/')
                    {
                        Skip(Tokenizer);
                        break;
                    }
                }
                Skip(Tokenizer);
            }
        }
    } while (HadComment);

    Token.String.String = Tokenizer->At;
    Token.String.Length = 1;
    char Ch = Peek(Tokenizer);

    switch (Ch)
    {
        case 0:
        {
            Token.Type = Token_Eof;
            Token.IsComplete = true;
            Token.String = {};
        } break;
        case '#':
        {
            Token.Type = Token_Preprocessor;
            while (!Token.IsComplete)
            {
                Ch = AdvanceAndGet(Tokenizer);

                // TODO(boti): new line escape
                if (!Ch)
                {
                    break;
                }
                else if (Ch == '\n')
                {
                    Token.IsComplete = true;
                    Skip(Tokenizer);
                }
                else
                {
                    Token.String.Length++;
                }
            }
        } break;
        case '"':
        {
            Token.Type = Token_String;
            while (!Token.IsComplete)
            {
                Ch = AdvanceAndGet(Tokenizer);

                // TODO(boti): Escape chars
                if (!Ch)
                {
                    break;
                }
                else if (Ch == '"')
                {
                    Token.IsComplete = true;
                    Skip(Tokenizer);
                }
                Token.String.Length++;
            }
        } break;
        case '\'':
        {
            Token.Type = Token_Char;
            while (!Token.IsComplete)
            {
                Ch = AdvanceAndGet(Tokenizer);

                // TODO(boti): Escape chars
                if (!Ch)
                {
                    break;
                }
                else if (Ch == '\'')
                {
                    Token.IsComplete = true;
                    Skip(Tokenizer);
                }
                Token.String.Length++;
            }
        } break;

        default:
        {
            if (IsAlpha(Ch) || Ch == '_')
            {
                Token.Type = Token_KeywordOrIdentifier;
                while (!Token.IsComplete)
                {
                    Ch = AdvanceAndGet(Tokenizer);
                    if (!Ch)
                    {
                        break;
                    }
                    else if (!IsAlpha(Ch) && !IsDigit(Ch) && Ch != '_')
                    {
                        Token.IsComplete = true;
                    }
                    else
                    {
                        Token.String.Length++;
                    }
                }
            }
            else if (IsDigit(Ch))
            {
                Token.Type = Token_Number;

                for (;;)
                {
                    Ch = AdvanceAndGet(Tokenizer);
                    if (!Ch)
                    {
                        break;
                    }
                    else if (!IsDigit(Ch))
                    {
                        break;
                    }
                    else
                    {
                        Token.String.Length++;
                    }
                }

                if (Ch == '.')
                {
                    Token.String.Length++;
                    for (;;)
                    {
                        Ch = AdvanceAndGet(Tokenizer);
                        if (!Ch)
                        {
                            break;
                        }
                        else if (!IsDigit(Ch))
                        {
                            break;
                        }
                        else
                        {
                            Token.String.Length++;
                        }
                    }

                    if (Ch == 'e')
                    {
                        UnimplementedCodePath;
                    }

                    if (Ch == 'f' || Ch == 'F')
                    {
                        Token.String.Length++;
                        Skip(Tokenizer);
                    }
                }
                else
                {
                    bool IsUnsigned = false;
                    u32 LongCount = 0;

                    while (!IsUnsigned || (LongCount == 0))
                    {
                        if (Ch == 'u' || Ch == 'U')
                        {
                            if (IsUnsigned)
                            {
                                UnhandledError("Multiple unsigned suffixes");
                            }
                            IsUnsigned = true;
                        }
                        else if (Ch == 'l' || Ch == 'L')
                        {
                            LongCount++;
                            Token.String.Length++;
                            Ch = AdvanceAndGet(Tokenizer);
                            if (Ch == 'l' || Ch == 'L')
                            {
                                LongCount++;
                            }
                        }
                        else
                        {
                            break;
                        }

                        Token.String.Length++;
                        Ch = AdvanceAndGet(Tokenizer);
                    }
                }
            }
            else
            {
                char Chars[3] = { Ch, PeekAhead(Tokenizer, 1), PeekAhead(Tokenizer, 2) };
                // Punctuation marks
                for (u32 TokenIndex = Token_Punct3_First; TokenIndex < Token_Punct3_OnePastLast; TokenIndex++)
                {
                    if ((Chars[0] == PunctuationMarks[TokenIndex][0]) &&
                        (Chars[1] == PunctuationMarks[TokenIndex][1]) &&
                        (Chars[2] == PunctuationMarks[TokenIndex][2]))
                    {
                        Token.Type = (token_type)TokenIndex;
                        Token.IsComplete = true;
                        Token.String.Length = 3;
                        Skip(Tokenizer);
                        Skip(Tokenizer);
                        Skip(Tokenizer);
                        break;
                    }
                }

                if (!Token.IsComplete)
                {
                    for (u32 TokenIndex = Token_Punct2_First; TokenIndex < Token_Punct2_OnePastLast; TokenIndex++)
                    {
                        if ((Chars[0] == PunctuationMarks[TokenIndex][0]) &&
                            (Chars[1] == PunctuationMarks[TokenIndex][1]))
                        {
                            Token.Type = (token_type)TokenIndex;
                            Token.IsComplete = true;
                            Token.String.Length = 2;
                            Skip(Tokenizer);
                            Skip(Tokenizer);
                            break;
                        }
                    }

                    if (!Token.IsComplete)
                    {
                        for (u32 TokenIndex = Token_Punct1_First; TokenIndex < Token_Punct1_OnePastLast; TokenIndex++)
                        {
                            if (Chars[0] == PunctuationMarks[TokenIndex][0])
                            {
                                Token.Type = (token_type)TokenIndex;
                                Token.IsComplete = true;
                                Token.String.Length = 1;
                                Skip(Tokenizer);
                                break;
                            }
                        }
                    }
                }
            }
        } break;
    }

    Tokenizer->LastToken = Token;
    return(Token);
}

#include <cstdio>

int main(int ArgCount, char** Args)
{
    const char* FilePath = "src/Renderer/Renderer.hpp";

    umm ArenaSize = MiB(64);
    memory_arena Arena = InitializeArena(ArenaSize, malloc(ArenaSize));

    FILE* File = fopen(FilePath, "rb");
    if (File)
    {
        fseek(File, 0, SEEK_END);
        umm FileSize = (umm)ftell(File);
        char* Data = (char*)PushSize_(&Arena, 0, FileSize, 64);
        if (Data)
        {
            fseek(File, 0, SEEK_SET);
            if (fread(Data, 1, FileSize, File) == FileSize)
            {
                tokenizer Tokenizer = { .Count = FileSize, .At = Data };

                bool IsParsing = true;
                while (IsParsing)
                {
                    token Token = NextToken(&Tokenizer);
                    switch (Token.Type)
                    {
                        case Token_Eof:
                        {
                            IsParsing = false;
                        } break;
                        case Token_KeywordOrIdentifier:
                        {
                            if (StringEquals(Token.String, "enum"))
                            {
                                token Name = NextToken(&Tokenizer);
                                if (Name.Type == Token_KeywordOrIdentifier)
                                {
                                    Token = NextToken(&Tokenizer);
                                    string UnderlyingType = {};

                                    if (Token.Type == Token_Colon)
                                    {
                                        Token = NextToken(&Tokenizer);
                                        // TODO(boti): multi-token types
                                        if (Token.Type == Token_KeywordOrIdentifier)
                                        {
                                            UnderlyingType = Token.String;
                                            Token = NextToken(&Tokenizer);
                                        }
                                        else
                                        {
                                            UnhandledError("Invalid token underlying type");
                                        }
                                    }

                                    if (Token.Type == Token_Semicolon)
                                    {
                                        // Forward declaration, ignored
                                    }
                                    else if (Token.Type == Token_OpenBrace)
                                    {
                                        fprintf(stdout, "enum %.*s : %.*s\n", 
                                                (int)Name.String.Length, Name.String.String,
                                                (int)UnderlyingType.Length, UnderlyingType.String);
                                        fprintf(stdout, "{\n");

                                        u32 CurrentEnumValue = 0;
                                        for (Token = NextToken(&Tokenizer); Token.Type != Token_CloseBrace; Token = NextToken(&Tokenizer))
                                        {
                                            if (Token.Type == Token_Eof)
                                            {
                                                UnhandledError("Unexpected eof in enum");
                                                break;
                                            }
                                            else if (Token.Type == Token_KeywordOrIdentifier)
                                            {
                                                string EnumName = Token.String;
                                                Token = NextToken(&Tokenizer);
                                                if (Token.Type == Token_Assign)
                                                {
                                                    constexpr u32 MaxExpressionTokenCount = 256;
                                                    u32 ExpressionTokenCount = 0;
                                                    token Expression[MaxExpressionTokenCount];
                                                    for (;;)
                                                    {
                                                        Token = NextToken(&Tokenizer);
                                                        if (Token.Type == Token_Eof)
                                                        {
                                                            UnhandledError("Unexpected eof in enum");
                                                        }
                                                        else if (Token.Type == Token_Comma)
                                                        {
                                                            break;
                                                        }
                                                        else
                                                        {
                                                            Assert(ExpressionTokenCount < MaxExpressionTokenCount);
                                                            Expression[ExpressionTokenCount++] = Token;
                                                        }
                                                    }

                                                    for (u32 It = 0; It < ExpressionTokenCount; It++)
                                                    {
                                                        token* ExpressionToken = Expression + It;
                                                        Assert(ExpressionToken->Type != Token_KeywordOrIdentifier);
                                                        fprintf(stdout, "%.*s", (int)ExpressionToken->String.Length, ExpressionToken->String.String);
                                                    }
                                                    fprintf(stdout, "\n");
                                                }
                                                else if (Token.Type == Token_Comma)
                                                {
                                                    // Ignored
                                                }
                                                else
                                                {
                                                    UnhandledError("Unexpected token type in enum");
                                                }

                                                fprintf(stdout, "\t%.*s = %u,\n", (int)EnumName.Length, EnumName.String, CurrentEnumValue);
                                                CurrentEnumValue += 1;
                                            }
                                            else
                                            {
                                                UnhandledError("Unexpected token type in enum");
                                            }
                                        }

                                        fprintf(stdout, "}\n");
                                    }
                                    else
                                    {
                                        UnhandledError("Invalid enum");
                                    }
                                }
                                else
                                {
                                    UnhandledError("Invalid enum identifier");
                                }
                            }
                        } break;
                        default:
                        {
                            if (Token.String.Length)
                            {
                                Assert(Token.String.String[Token.String.Length - 1] != '\n');
                            }
                            //fprintf(stdout, "[%3u] %.*s\n", Token.Type, (int)Token.String.Length, Token.String.String);
                        } break;
                    }
                }
            }
            else
            {
                fprintf(stderr, "Failed to read file \"%s\"\n", FilePath);
            }
        }
        else
        {
            fprintf(stderr, "Failed to allocate memory\n");
        }
        fclose(File);
    }
    else
    {
        fprintf(stderr, "Failed to open file \"%s\"\n", FilePath);
    }
    return(0);
}