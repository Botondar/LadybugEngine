#pragma once

//#include <Core.hpp>
//#include <Memory.hpp>
//#include <String.hpp>

enum class json_element_type : u32
{
    Null = 0,
    Boolean,
    Number,
    String,
    Object,
    Array,
};

struct json_element;

struct json_number;
struct json_array;
struct json_object;

enum class json_number_type : u32
{
    U32,
    S32,
    F32,
};

struct json_number
{
    json_number_type Type;
    union
    {
        u32 U32;
        s32 S32;
        f32 F32;
    };

    u32 AsU32() const;
    s32 AsS32() const;
    f32 AsF32() const;
};

struct json_array
{
    u64 ElementCount;
    json_element* Elements;
};

struct json_object
{
    u64 ElementCount;
    string* Keys;
    json_element* Elements;
};

struct json_element
{
    json_element_type Type;
    union
    {
        b32 Boolean;
        json_number Number;
        string String;
        json_array Array;
        json_object Object;
    };
};

lbfn json_element* GetElement(json_object* Object, const char* Name);

lbfn json_element* ParseJSON(const void* Data, u64 DataSize, memory_arena* Arena);