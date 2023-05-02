#pragma once

inline bool IsSpace(char C);
inline bool IsDigit(char C);

inline bool CopyZStringToBuffer(char* Dst, const char* Src, u64* SizePtr);

struct string
{
    u64 Length;
    char* String;
};

// Checks string with zero-terminated string for equality
// Only returns true if the strings also have equal length
inline bool StringEquals(const string* String, const char* ZString);

struct string_view
{
    const char* At;
    u64 Size;
};

inline char Peek(string_view* View);
inline char Advance(string_view* View);

inline u64 EatWhitespace(string_view* View);


inline bool MatchZAndAdvance(string_view* View, const char* ZString);

//
// Implementation
//

inline bool IsSpace(char C)
{
    bool Result = 
        (C == ' ') ||
        (C == '\t') ||
        (C == '\n') ||
        (C == '\r');
    return Result;
}

inline bool IsDigit(char C)
{
    bool Result = (C >= '0') && (C <= '9');
    return Result;
}

inline bool CopyZStringToBuffer(char* Dst, const char* Src, u64* SizePtr)
{
    bool Result = false;

    if (SizePtr)
    {
        u64 Size = *SizePtr;
        u64 Count = Size;
        char* DstAt = Dst;
        const char* SrcAt = Src;
        while (Count--)
        {
            char Ch = *Src++;
            *Dst++ = Ch;
            if (Ch == 0) 
            {
                Result = true;
                break;
            }
        }

        if (!Result)
        {
            Dst[Size - 1] = 0;
        }
        *SizePtr = Count + 1;
    }

    return(Result);
}

inline bool StringEquals(const string* String, const char* ZString)
{
    bool Result = false;

    if (ZString)
    {
        u64 i;
        for (i = 0; i < String->Length; i++)
        {
            if (ZString[i] != String->String[i])
            {
                break;
            }
            else if (ZString[i] == 0)
            {
                break;
            }
        }

        if (i == String->Length)
        {
            Result = (ZString[i] == 0);
        }
    }
    else
    {
        Result = (String->Length == 0);
    }

    return Result;
}

inline char Peek(string_view* View)
{
    char Result = 0;
    if (View->Size)
    {
        Result = *View->At;
    }
    return Result;
}

inline char Advance(string_view* View)
{
    char Result = 0;
    if (View->Size)
    {
        Result = *View->At++;
        View->Size--;
    }
    return Result;
}

inline u64 EatWhitespace(string_view* View)
{
    u64 Result = 0;

    while (IsSpace(Peek(View)))
    {
        Advance(View);
        Result++;
    }

    return Result;
}

inline bool MatchZAndAdvance(string_view* View, const char* ZString)
{
    bool Result = true;

    const char* Start = View->At;
    u64 StartSize = View->Size;

    for (u64 i = 0; ZString[i]; i++)
    {
        if (Advance(View) != ZString[i])
        {
            Result = false;
            View->At = Start;
            View->Size = StartSize;
            break;
        }
    }
    return Result;
}