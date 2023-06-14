#include "glTF.hpp"

enum gltf_elem_flags : flags32
{
    GLTF_Flags_None = 0,

    GLTF_Required = 1 << 0,
};

internal string ParseString(json_element* Elem, gltf_elem_flags Flags);
internal b32 ParseB32(json_element* Elem, gltf_elem_flags Flags, b32 DefaultValue = false);
internal u32 ParseU32(json_element* Elem, gltf_elem_flags Flags, u32 DefaultValue = 0);
internal f32 ParseF32(json_element* Elem, gltf_elem_flags Flags, f32 DefaultValue = 0.0f);
internal gltf_type ParseGLTFType(json_element* Elem, gltf_elem_flags Flags, gltf_type DefaultValue = GLTF_SCALAR);
internal m4 ParseGLTFVector(json_element* Elem, gltf_elem_flags Flags, gltf_type Type, m4 DefaultValue = {});

internal string ParseString(json_element* Elem, gltf_elem_flags Flags)
{
    string Result = {};
    if (Elem)
    {
        Assert(Elem->Type == json_element_type::String);
        Result = Elem->String;
    }
    else if (HasFlag(Flags, GLTF_Required))
    {
        UnhandledError("Missing required glTF element");
    }
    return(Result);
}

internal b32 ParseB32(json_element* Elem, gltf_elem_flags Flags, b32 DefaultValue /*= false*/)
{
    b32 Result = DefaultValue;
    if (Elem)
    {
        Assert(Elem->Type == json_element_type::Boolean);
        Result = Elem->Boolean;
    }
    else if (HasFlag(Flags, GLTF_Required))
    {
        UnhandledError("Missing required glTF element");
    }
    return(Result);
}

internal u32 ParseU32(json_element* Elem, gltf_elem_flags Flags, u32 DefaultValue /*= 0*/)
{
    u32 Result = DefaultValue;
    if (Elem)
    {
        Assert(Elem->Type == json_element_type::Number);
        Result = Elem->Number.AsU32();
    }
    else if (HasFlag(Flags, GLTF_Required))
    {
        UnhandledError("Missing required glTF element");
    }
    return(Result);
}

internal f32 ParseF32(json_element* Elem, gltf_elem_flags Flags, f32 DefaultValue /*= 0.0f*/)
{
    u32 Result = DefaultValue;
    if (Elem)
    {
        Assert(Elem->Type == json_element_type::Number);
        Result = Elem->Number.AsF32();
    }
    else if (HasFlag(Flags, GLTF_Required))
    {
        UnhandledError("Missing required glTF element");
    }
    return(Result);
}

internal gltf_type ParseGLTFType(json_element* Elem, gltf_elem_flags Flags, gltf_type DefaultValue /*= GLTF_SCALAR*/)
{
    gltf_type Result = DefaultValue;
    if (Elem)
    {
        Assert(Elem->Type == json_element_type::String);
        if      (StringEquals(&Elem->String, "SCALAR")) Result = GLTF_SCALAR;
        else if (StringEquals(&Elem->String, "VEC2"))   Result = GLTF_VEC2;
        else if (StringEquals(&Elem->String, "VEC3"))   Result = GLTF_VEC3;
        else if (StringEquals(&Elem->String, "VEC4"))   Result = GLTF_VEC4;
        else if (StringEquals(&Elem->String, "MAT2"))   Result = GLTF_MAT2;
        else if (StringEquals(&Elem->String, "MAT3"))   Result = GLTF_MAT3;
        else if (StringEquals(&Elem->String, "MAT4"))   Result = GLTF_MAT4;
        else
        {
            UnhandledError("Invalid accessor type");
        }
    }
    else if (HasFlag(Flags, GLTF_Required))
    {
        UnhandledError("Missing required glTF element");
    }
    return(Result);
}

internal m4 ParseGLTFVector(json_element* Elem, gltf_elem_flags Flags, gltf_type Type, m4 DefaultValue /*= {}*/)
{
    m4 Result = DefaultValue;
    if (Elem)
    {
        constexpr u32 CountTable[GLTF_TYPE_COUNT] = 
        {
            [GLTF_SCALAR] = 1,
            [GLTF_VEC2]   = 2,
            [GLTF_VEC3]   = 3,
            [GLTF_VEC4]   = 4,
            [GLTF_MAT2]   = 4,
            [GLTF_MAT3]   = 9,
            [GLTF_MAT4]   = 16,
        };
        u32 Count = CountTable[Type];

        Assert(Elem->Type == json_element_type::Array);
        json_array* Array = &Elem->Array;
        Assert(Array->ElementCount == Count);

        for (u32 i = 0; i < Count; i++)
        {
            Result.EE[i] = ParseF32(Array->Elements + i, GLTF_Required);
        }
    }
    else if (HasFlag(Flags, GLTF_Required))
    {
        UnhandledError("Missing required glTF element");
    }
    return(Result);
}

lbfn bool ParseGLTF(gltf* GLTF, json_element* Root, memory_arena* Arena)
{
    // TODO(boti): We're not correctly setting the return value when encountering an error !!!
    bool Result = true;
    *GLTF = {};

    if (!Root || Root->Type != json_element_type::Object)
    {
        return false;
    }

    json_object* RootObject = &Root->Object;

    json_element* Accessors   = GetElement(RootObject, "accessors");
    json_element* BufferViews = GetElement(RootObject, "bufferViews");
    json_element* Samplers    = GetElement(RootObject, "samplers");
    json_element* Textures    = GetElement(RootObject, "textures");
    json_element* Images      = GetElement(RootObject, "images");
    json_element* Buffers     = GetElement(RootObject, "buffers");
    json_element* Materials   = GetElement(RootObject, "materials");
    json_element* Meshes      = GetElement(RootObject, "meshes");
    json_element* Nodes       = GetElement(RootObject, "nodes");
    json_element* Scenes      = GetElement(RootObject, "scenes");
    json_element* Scene       = GetElement(RootObject, "scene");

    if (Buffers)
    {
        if (Buffers->Type != json_element_type::Array)
        {
            UnhandledError("Invalid GLTF \"buffers\" type");
        }

        GLTF->BufferCount = (u32)Buffers->Array.ElementCount;
        GLTF->Buffers = PushArray<gltf_buffer>(Arena, GLTF->BufferCount, MemPush_Clear);
        for (u32 i = 0; i < GLTF->BufferCount; i++)
        {
            gltf_buffer* Dst = GLTF->Buffers + i;
            json_element* Src = Buffers->Array.Elements + i;
            if (Src->Type == json_element_type::Object)
            {
                json_object* Obj = &Src->Object;
                Dst->URI = ParseString(GetElement(Obj, "uri"), GLTF_Required);
                Dst->ByteLength = ParseU32(GetElement(Obj, "byteLength"), GLTF_Required);
            }
            else
            {
                UnhandledError("Invalid GLTF buffer type");
            }
        }
    }
    else
    {
        UnhandledError("Missing GLTF buffers");
    }

    if (BufferViews)
    {
        if (BufferViews->Type != json_element_type::Array)
        {
            UnhandledError("Invalid GLTF \"bufferViews\" type");
        }

        GLTF->BufferViewCount = (u32)BufferViews->Array.ElementCount;
        GLTF->BufferViews = PushArray<gltf_buffer_view>(Arena, GLTF->BufferViewCount, MemPush_Clear);
        for (u32 i = 0; i < GLTF->BufferViewCount; i++)
        {
            gltf_buffer_view* Dst = GLTF->BufferViews + i;
            json_element* Src = BufferViews->Array.Elements + i;
            if (Src->Type == json_element_type::Object)
            {
                json_object* Obj = &Src->Object;
                Dst->BufferIndex = ParseU32(GetElement(Obj, "buffer"), GLTF_Required);
                Dst->Offset = ParseU32(GetElement(Obj, "byteOffset"), GLTF_Flags_None, 0);
                Dst->Size = ParseU32(GetElement(Obj, "byteLength"), GLTF_Required);
                // TODO(boti): GLTF doesn't require a stride so we should have a
                //             sensible default value here
                Dst->Stride = ParseU32(GetElement(Obj, "byteStride"), GLTF_Flags_None, 0);
                //json_element* Target = GetElement(&Src->Object, "target");
            }
            else
            {
                UnhandledError("Invalid GLTF bufferView type");
            }
        }
    }
    else
    {
        UnhandledError("Missing GLTF bufferViews");
    }

    if (Accessors)
    {
        if (Accessors->Type != json_element_type::Array)
        {
            UnhandledError("Invalid GLTF \"accessors\" type");
        }

        GLTF->AccessorCount = (u32)Accessors->Array.ElementCount;
        GLTF->Accessors = PushArray<gltf_accessor>(Arena, GLTF->AccessorCount, MemPush_Clear);
        for (u32 i = 0; i < GLTF->AccessorCount; i++)
        {
            gltf_accessor* Dst = GLTF->Accessors + i;
            json_element* Src = Accessors->Array.Elements + i;
            if (Src->Type == json_element_type::Object)
            {
                json_object* Obj = &Src->Object;
                Dst->BufferView = ParseU32(GetElement(Obj, "bufferView"), GLTF_Flags_None, U32_MAX);
                Dst->ByteOffset = ParseU32(GetElement(Obj, "byteOffset"), GLTF_Flags_None, U32_MAX);
                Dst->ComponentType = (gltf_component_type)ParseU32(GetElement(Obj, "componentType"), GLTF_Required);
                Dst->IsNormalized = ParseB32(GetElement(Obj, "normalized"), GLTF_Flags_None, false);
                Dst->Count = ParseU32(GetElement(Obj, "count"), GLTF_Required);
                Dst->Type = ParseGLTFType(GetElement(Obj, "type"), GLTF_Required);
                Dst->Max = ParseGLTFVector(GetElement(Obj, "max"), GLTF_Flags_None, Dst->Type, { 0.0f, 0.0f, 0.0f });
                Dst->Min = ParseGLTFVector(GetElement(Obj, "min"), GLTF_Flags_None, Dst->Type, { 0.0f, 0.0f, 0.0f });

                if (GetElement(&Src->Object, "sparse"))
                {
                    // TODO(boti)
                    UnimplementedCodePath;
                }
            }
            else
            {
                UnhandledError("Invalid GLTF accessor type");
            }
        }
    }
    else
    {
        UnhandledError("Missing GLTF accessors");
    }

    if (Samplers)
    {
        if (Samplers->Type != json_element_type::Array)
        {
            UnhandledError("Invalid GLTF samplers type");
        }

        GLTF->SamplerCount = (u32)Samplers->Array.ElementCount;
        GLTF->Samplers = PushArray<gltf_sampler>(Arena, GLTF->SamplerCount, MemPush_Clear);
        for (u32 i = 0; i < GLTF->SamplerCount; i++)
        {
            gltf_sampler* Dst = GLTF->Samplers + i;
            json_element* Src = Samplers->Array.Elements + i;
            if (Src->Type == json_element_type::Object)
            {
                json_object* Obj = &Src->Object;
                Dst->MagFilter = (gltf_filter)ParseU32(GetElement(Obj, "magFilter"), GLTF_Flags_None, GLTF_FILTER_LINEAR);
                Dst->MinFilter = (gltf_filter)ParseU32(GetElement(Obj, "minFilter"), GLTF_Flags_None, GLTF_FILTER_NEAREST);
                Dst->WrapU = (gltf_wrap)ParseU32(GetElement(Obj, "wrapS"), GLTF_Flags_None, GLTF_WRAP_REPEAT);
                Dst->WrapV = (gltf_wrap)ParseU32(GetElement(Obj, "wrapT"), GLTF_Flags_None, GLTF_WRAP_REPEAT);
            }
            else
            {
                UnhandledError("Invalid GLTF sampler type");
            }
        }
    }
    else
    {

    }

    if (Textures)
    {
        if (Textures->Type != json_element_type::Array)
        {
            UnhandledError("Invalid glTF \"textures\" type");
        }

        GLTF->TextureCount = (u32)Textures->Array.ElementCount;
        GLTF->Textures = PushArray<gltf_texture>(Arena, GLTF->TextureCount, MemPush_Clear);
        for (u32 i = 0; i < GLTF->TextureCount; i++)
        {
            gltf_texture* Dst = GLTF->Textures + i;
            json_element* Src = Textures->Array.Elements + i;
            if (Src->Type == json_element_type::Object)
            {
                json_object* Obj = &Src->Object;
                Dst->SamplerIndex = ParseU32(GetElement(Obj, "sampler"), GLTF_Flags_None, U32_MAX);
                Dst->ImageIndex = ParseU32(GetElement(Obj, "source"), GLTF_Flags_None, U32_MAX);
            }
            else
            {
                UnhandledError("Invalid glTF texture type");
            }
        }
    }

    if (Images)
    {
        if (Images->Type != json_element_type::Array)
        {
            UnhandledError("Invalid glTF \"images\" type");
        }

        GLTF->ImageCount = (u32)Images->Array.ElementCount;
        GLTF->Images = PushArray<gltf_image>(Arena, GLTF->ImageCount, MemPush_Clear);
        for (u32 i = 0; i < GLTF->ImageCount; i++)
        {
            gltf_image* Dst = GLTF->Images + i;
            json_element* Src = Images->Array.Elements + i;

            json_element* URI           = GetElement(&Src->Object, "uri");
            json_element* MimeType      = GetElement(&Src->Object, "mimeType");
            json_element* BufferView    = GetElement(&Src->Object, "bufferView");

            if (URI)
            {
                Assert(URI->Type == json_element_type::String);
                Dst->URI = URI->String;
            }

            if (MimeType)
            {
                Assert(MimeType->Type == json_element_type::String);
                Dst->MimeType = MimeType->String;
            }

            if (BufferView)
            {
                UnimplementedCodePath;
            }
            else
            {
                Dst->BufferViewIndex = U32_MAX;
            }
        }
    }

    if (Materials)
    {
        if (Materials->Type != json_element_type::Array)
        {
            UnhandledError("Invalid glTF \"materials\" type");
        }

        GLTF->MaterialCount = (u32)Materials->Array.ElementCount;
        GLTF->Materials = PushArray<gltf_material>(Arena, GLTF->MaterialCount, MemPush_Clear);
        for (u32 i = 0; i < GLTF->MaterialCount; i++)
        {
            gltf_material* Dst = GLTF->Materials + i;
            json_element* Src = Materials->Array.Elements + i;
            if (Src->Type != json_element_type::Object)
            {
                UnhandledError("Invalid glTF material type");
            }

            json_element* PBR               = GetElement(&Src->Object, "pbrMetallicRoughness");
            json_element* NormalTexture     = GetElement(&Src->Object, "normalTexture");
            json_element* OcclusionTexture  = GetElement(&Src->Object, "occlusionTexture");
            json_element* EmissiveTexture   = GetElement(&Src->Object, "emissiveTexture");
            json_element* EmissiveFactor    = GetElement(&Src->Object, "emissiveFactor");
            json_element* AlphaMode         = GetElement(&Src->Object, "alphaMode");
            json_element* AlphaCutoff       = GetElement(&Src->Object, "alphaCutoff");
            json_element* DoubleSided       = GetElement(&Src->Object, "doubleSided");

            if (PBR) 
            {
                Assert(PBR->Type == json_element_type::Object);
                json_element* BaseColorFactor           = GetElement(&PBR->Object, "baseColorFactor");
                json_element* BaseColorTexture          = GetElement(&PBR->Object, "baseColorTexture");
                json_element* MetallicFactor            = GetElement(&PBR->Object, "metallicFactor");
                json_element* RoughnessFactor           = GetElement(&PBR->Object, "roughnessFactor");
                json_element* MetallicRoughnessTexture  = GetElement(&PBR->Object, "metallicRoughnessTexture");

                if (BaseColorFactor)
                {
                    Assert(BaseColorFactor->Type == json_element_type::Array);
                    Assert(BaseColorFactor->Array.ElementCount == 4);

                    for (u32 ElemIndex = 0; ElemIndex < BaseColorFactor->Array.ElementCount; ElemIndex++)
                    {
                        json_element* Elem = BaseColorFactor->Array.Elements + ElemIndex;
                        Assert(Elem->Type == json_element_type::Number);
                        Dst->BaseColorFactor.E[ElemIndex] = Elem->Number.AsF32();
                    }
                }
                else
                {
                    Dst->BaseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
                }

                if (BaseColorTexture)
                {
                    Assert(BaseColorTexture->Type == json_element_type::Object);
                    json_element* TexIndex      = GetElement(&BaseColorTexture->Object, "index");
                    json_element* TexCoordIndex = GetElement(&BaseColorTexture->Object, "texCoord");

                    if (TexIndex)
                    {
                        Assert(TexIndex->Type == json_element_type::Number);
                        Dst->BaseColorTextureIndex = TexIndex->Number.AsU32();
                    }
                    else
                    {
                        UnhandledError("glTF Missing texture index from baseColorTexture");
                    }

                    if (TexCoordIndex)
                    {
                        Assert(TexCoordIndex->Type == json_element_type::Number);
                        Dst->BaseColorTexCoordIndex = TexCoordIndex->Number.AsU32();
                    }
                    else
                    {
                        Dst->BaseColorTexCoordIndex = 0;
                    }
                }
                else
                {
                    Dst->BaseColorTextureIndex = U32_MAX;
                    Dst->BaseColorTexCoordIndex = 0;
                }

                if (MetallicFactor)
                {
                    if (MetallicFactor->Type != json_element_type::Number) UnhandledError("Invalid glTF metallicFactor type");
                    Dst->MetallicFactor = MetallicFactor->Number.AsF32();
                }
                else
                {
                    Dst->MetallicFactor = 1.0f;
                }

                if (RoughnessFactor)
                {
                    if (RoughnessFactor->Type != json_element_type::Number) UnhandledError("Invalid glTF roughnessFactor type");
                    Dst->RoughnessFactor = RoughnessFactor->Number.AsF32();
                }
                else
                {
                    Dst->RoughnessFactor = 1.0f;
                }

                if (MetallicRoughnessTexture)
                {
                    if (MetallicRoughnessTexture->Type == json_element_type::Object) 
                    {
                        json_element* TextureIndex = GetElement(&MetallicRoughnessTexture->Object, "index");
                        json_element* TexCoordIndex = GetElement(&MetallicRoughnessTexture->Object, "texCoord");

                        if (TextureIndex)
                        {
                            if (TextureIndex->Type != json_element_type::Number) UnhandledError("Invalid glTF texture index type");
                            Dst->MetallicRoughnessTextureIndex = TextureIndex->Number.AsU32();
                        }
                        else
                        {
                            UnhandledError("Missing glTF metallicRoughnessTexture.index");
                        }

                        if (TexCoordIndex)
                        {
                            if (TexCoordIndex->Type != json_element_type::Number) UnhandledError("Invalid glTF texcoord index type");
                            Dst->MetallicRoughnessTexCoordIndex = TexCoordIndex->Number.AsU32();
                        }
                        else
                        {
                            Dst->MetallicRoughnessTexCoordIndex = 0;
                        }
                    }
                    else 
                    {
                        UnhandledError("Invalid glTF metallicRoughnessTexture type");
                    }
                }
                else
                {
                    Dst->MetallicRoughnessTextureIndex = U32_MAX;
                    Dst->MetallicRoughnessTexCoordIndex = 0;
                }
            }
            else
            {
                UnimplementedCodePath;
            }

            if (NormalTexture)
            {
                Assert(NormalTexture->Type == json_element_type::Object);
                json_element* TexIndex      = GetElement(&NormalTexture->Object, "index");
                json_element* TexCoordIndex = GetElement(&NormalTexture->Object, "texCoord");
                json_element* TexCoordScale = GetElement(&NormalTexture->Object, "scale");

                if (TexIndex)
                {
                    Assert(TexIndex->Type == json_element_type::Number);
                    Dst->NormalTextureIndex = TexIndex->Number.AsU32();
                }
                else
                {
                    UnhandledError("glTF Missing texture index from normalTexture");
                }

                if (TexCoordIndex)
                {
                    Assert(TexCoordIndex->Type == json_element_type::Number);
                    Dst->NormalTexCoordIndex = TexCoordIndex->Number.AsU32();
                }
                else 
                {
                    Dst->NormalTexCoordIndex = 0;
                }

                if (TexCoordScale)
                {
                    Assert(TexCoordScale->Type == json_element_type::Number);
                    Dst->NormalTexCoordScale = TexCoordScale->Number.AsF32();
                }
                else
                {
                    Dst->NormalTexCoordScale = 1.0f;
                }
            }
            else
            {
                Dst->NormalTextureIndex = U32_MAX;
                Dst->NormalTexCoordIndex = 0;
                Dst->NormalTexCoordScale = 1.0f;
            }

            if (OcclusionTexture)
            {
                UnimplementedCodePath;
            }
            else
            {
            }

            if (EmissiveTexture)
            {
                UnimplementedCodePath;
            }
            else
            {
            }

            if (DoubleSided)
            {
                Assert(DoubleSided->Type == json_element_type::Boolean);
                Dst->IsDoubleSided = DoubleSided->Boolean;
            }
            else
            {
                Dst->IsDoubleSided = false;
            }

            if (AlphaMode)
            {
                Assert(AlphaMode->Type == json_element_type::String);
                if (StringEquals(&AlphaMode->String, "OPAQUE"))
                {
                    Dst->AlphaMode = GLTF_ALPHA_MODE_OPAQUE;
                }
                else if (StringEquals(&AlphaMode->String, "MASK"))
                {
                    Dst->AlphaMode = GLTF_ALPHA_MODE_MASK;
                }
                else if (StringEquals(&AlphaMode->String, "BLEND"))
                {
                    Dst->AlphaMode = GLTF_ALPHA_MODE_BLEND;
                }
            }
            else
            {
                Dst->AlphaMode = GLTF_ALPHA_MODE_OPAQUE;
            }

            if (AlphaCutoff)
            {
                Assert(AlphaCutoff->Type == json_element_type::Number);
                Dst->AlphaCutoff = AlphaCutoff->Number.AsF32();
            }
            else
            {
                Dst->AlphaCutoff = 0.5f;
            }

            if (EmissiveFactor)
            {
                Assert(EmissiveFactor->Type == json_element_type::Array);
                Assert(EmissiveFactor->Array.ElementCount == 3);

                for (u32 ElemIndex = 0; ElemIndex < EmissiveFactor->Array.ElementCount; ElemIndex++)
                {
                    json_element* Elem = EmissiveFactor->Array.Elements + ElemIndex;
                    Assert(Elem->Type == json_element_type::Number);

                    Dst->EmissiveFactor.E[ElemIndex] = Elem->Number.AsF32();
                }
            }
            else
            {
                Dst->EmissiveFactor = { 0.0f, 0.0f, 0.0f };
            }
        }
    }

    if (Meshes)
    {
        if (Meshes->Type != json_element_type::Array)
        {
            UnhandledError("Invalid glTF \"meshes\" type");
        }

        GLTF->MeshCount = (u32)Meshes->Array.ElementCount;
        GLTF->Meshes = PushArray<gltf_mesh>(Arena, GLTF->MeshCount, MemPush_Clear);
        for (u32 MeshIndex = 0; MeshIndex < GLTF->MeshCount; MeshIndex++)
        {
            gltf_mesh* MeshDst = GLTF->Meshes + MeshIndex;
            json_element* MeshSrc = Meshes->Array.Elements + MeshIndex;
            if (MeshSrc->Type != json_element_type::Object)
            {
                UnhandledError("Invalid glTF mesh type");
            }

            json_element* Primitives = GetElement(&MeshSrc->Object, "primitives");
            json_element* Weights    = GetElement(&MeshSrc->Object, "weights");

            if (Primitives)
            {
                if (Primitives->Type != json_element_type::Array)
                {
                    UnhandledError ("Invalid glTF primitives type");
                }

                MeshDst->PrimitiveCount = (u32)Primitives->Array.ElementCount;
                MeshDst->Primitives = PushArray<gltf_mesh_primitive>(Arena, MeshDst->PrimitiveCount, MemPush_Clear);

                for (u32 PrimitiveIndex = 0; PrimitiveIndex < MeshDst->PrimitiveCount; PrimitiveIndex++)
                {
                    gltf_mesh_primitive* Dst = MeshDst->Primitives + PrimitiveIndex;
                    json_element* Src = Primitives->Array.Elements + PrimitiveIndex;
                    if (Src->Type != json_element_type::Object)
                    {
                        UnhandledError("Invalid glTF primitive type");
                    }

                    json_element* Attributes    = GetElement(&Src->Object, "attributes");
                    json_element* Indices       = GetElement(&Src->Object, "indices");
                    json_element* Material      = GetElement(&Src->Object, "material");
                    json_element* Topology      = GetElement(&Src->Object, "mode");
                    json_element* Targets       = GetElement(&Src->Object, "target");

                    if (Attributes)
                    {
                        if (Attributes->Type != json_element_type::Object)
                        {
                            UnhandledError("Invalid glTF primitve attributes type");
                        }

                        Dst->PositionIndex    = U32_MAX;
                        Dst->NormalIndex      = U32_MAX;
                        Dst->ColorIndex       = U32_MAX;
                        Dst->TangentIndex     = U32_MAX;
                        Dst->TexCoordIndex[0] = U32_MAX;
                        Dst->TexCoordIndex[1] = U32_MAX;

                        for (u32 i = 0; i < Attributes->Object.ElementCount; i++)
                        {
                            string* Key = Attributes->Object.Keys + i;
                            json_element* Attribute = Attributes->Object.Elements + i;

                            if (Attribute->Type != json_element_type::Number)
                            {
                                UnhandledError("Invalid glTF primitive attribute type");
                            }

                            if (StringEquals(Key, "POSITION"))
                            {
                                Dst->PositionIndex = Attribute->Number.AsU32();
                            }
                            else if (StringEquals(Key, "NORMAL"))
                            {
                                Dst->NormalIndex = Attribute->Number.AsU32();
                            }
                            else if (StringEquals(Key, "TANGENT"))
                            {
                                Dst->TangentIndex = Attribute->Number.AsU32();
                            }
                            else if (StringEquals(Key, "COLOR_0"))
                            {
                                Dst->ColorIndex = Attribute->Number.AsU32();
                            }
                            else if (StringEquals(Key, "TEXCOORD_0"))
                            {
                                Dst->TexCoordIndex[0] = Attribute->Number.AsU32();
                            }
                            else if (StringEquals(Key, "TEXCOORD_1"))
                            {
                                Dst->TexCoordIndex[1] = Attribute->Number.AsU32();
                            }
                            else if (StringEquals(Key, "JOINTS_0"))
                            {
                                UnimplementedCodePath;
                            }
                            else if (StringEquals(Key, "WEIGHTS_0"))
                            {
                                UnimplementedCodePath;
                            }
                            else
                            {
                                UnhandledError("Invalid glTF primitive attribute");
                            }
                        }
                    }
                    else
                    {
                        UnhandledError("Missing glTF primitive attributes");
                    }

                    if (Indices)
                    {
                        if (Indices->Type != json_element_type::Number)
                        {
                            UnhandledError("Invalid glTF primitive indices type");
                        }
                        Dst->IndexBufferIndex = Indices->Number.AsU32();
                    }
                    else
                    {
                        Dst->IndexBufferIndex = U32_MAX;
                    }

                    if (Material)
                    {
                        if (Material->Type != json_element_type::Number)
                        {
                            UnhandledError("Invalid glTF primitive material type");
                        }
                        Dst->MaterialIndex = Material->Number.AsU32();
                    }
                    else
                    {
                        Dst->MaterialIndex = U32_MAX;
                    }

                    if (Topology)
                    {
                        if (Topology->Type != json_element_type::Number)
                        {
                            UnhandledError("Invalid glTF primitive topology type");
                        }
                        Dst->Topology = (gltf_topology)Topology->Number.AsU32();
                    }
                    else
                    {
                        Dst->Topology = GLTF_TRIANGLES;
                    }

                    if (Targets)
                    {
                        UnimplementedCodePath;
                    }
                }
            }
            else
            {
                UnhandledError("Missing primitives from glTF mesh");
            }

            if (Weights)
            {
                UnimplementedCodePath;
            }
        }

        if (Nodes)
        {
            if (Nodes->Type != json_element_type::Array)
            {
                UnhandledError("Invalid glTF \"nodes\" type");
            }

            GLTF->NodeCount = (u32)Nodes->Array.ElementCount;
            GLTF->Nodes = PushArray<gltf_node>(Arena, GLTF->NodeCount, MemPush_Clear);
            for (u32 NodeIndex = 0; NodeIndex < GLTF->NodeCount; NodeIndex++)
            {
                gltf_node* Dst = GLTF->Nodes + NodeIndex;
                json_element* Src = Nodes->Array.Elements + NodeIndex;
                if (Src->Type != json_element_type::Object)
                {
                    UnhandledError("Invalid glTF node type");
                }

                json_element* Camera        = GetElement(&Src->Object, "camera");
                json_element* Children      = GetElement(&Src->Object, "children");
                json_element* Skin          = GetElement(&Src->Object, "skin");
                json_element* Matrix        = GetElement(&Src->Object, "matrix");
                json_element* Mesh          = GetElement(&Src->Object, "mesh");
                json_element* Rotation      = GetElement(&Src->Object, "rotation");
                json_element* Scale         = GetElement(&Src->Object, "scale");
                json_element* Translation   = GetElement(&Src->Object, "translation");
                json_element* Weights       = GetElement(&Src->Object, "weights");

                if (Matrix)
                {
                    if (Matrix->Type != json_element_type::Array)
                    {
                        UnhandledError("Invalid glTF matrix type");
                    }

                    if (Matrix->Array.ElementCount != 16)
                    {
                        UnhandledError("Invalid glTF matrix element count");
                    }

                    for (u32 i = 0; i < 16; i++)
                    {
                        json_element* Elem = Matrix->Array.Elements + i;
                        if (Elem->Type != json_element_type::Number)
                        {
                            UnhandledError("Invalid glTF matrix element type");
                        }

                        Dst->Transform.E[i / 4][i % 4] = Elem->Number.AsF32();
                    }
                }
                else
                {
                    Dst->Transform = M4(1.0f, 0.0f, 0.0f, 0.0f,
                                        0.0f, 1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f, 0.0f,
                                        0.0f, 0.0f, 0.0f, 1.0f);
                }

                if (Rotation)
                {
                    if (Rotation->Type != json_element_type::Array)
                    {
                        UnhandledError("Invalid glTF rotation type");
                    }

                    if (Rotation->Array.ElementCount != 4)
                    {
                        UnhandledError("Invalid glTF rotation element count");
                    }

                    Dst->IsTRS = true;
                    for (u32 i = 0; i < 4; i++)
                    {
                        json_element* Elem = Rotation->Array.Elements + i;
                        if (Elem->Type != json_element_type::Number)
                        {
                            UnhandledError("Invalid glTF rotation element type");
                        }

                        Dst->Rotation.E[i] = Elem->Number.AsF32();
                    }
                }
                else
                {
                    Dst->Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
                }

                if (Scale)
                {
                    if (Scale->Type != json_element_type::Array)
                    {
                        UnhandledError("Invalid glTF scale type");
                    }

                    if (Scale->Array.ElementCount != 3)
                    {
                        UnhandledError("Invalid glTF scale element count");
                    }

                    Dst->IsTRS = true;
                    for (u32 i = 0; i < 3; i++)
                    {
                        json_element* Elem = Scale->Array.Elements + i;
                        if (Elem->Type != json_element_type::Number)
                        {
                            UnhandledError("Invalid glTF scale element type");
                        }

                        Dst->Scale.E[i] = Elem->Number.AsF32();
                    }
                }
                else
                {
                    Dst->Scale = { 1.0f, 1.0f, 1.0f };
                }

                if (Translation)
                {
                    if (Translation->Type != json_element_type::Array)
                    {
                        UnhandledError("Invalid glTF translation type");
                    }

                    if (Translation->Array.ElementCount != 3)
                    {
                        UnhandledError("Invalid glTF translation element count");
                    }

                    Dst->IsTRS = true;
                    for (u32 i = 0; i < 3; i++)
                    {
                        json_element* Elem = Translation->Array.Elements + i;
                        if (Elem->Type != json_element_type::Number)
                        {
                            UnhandledError("Invalid glTF translation element type");
                        }

                        Dst->Translation.E[i] = Elem->Number.AsF32();
                    }
                }
                else
                {
                    Dst->Translation = { 0.0f, 0.0f, 0.0f };
                }

                if (Skin)
                {
                    UnimplementedCodePath;
                }
                else
                {
                    Dst->SkinIndex = U32_MAX;
                }

                if (Camera)
                {
                    UnimplementedCodePath;
                }
                else
                {
                    Dst->CameraIndex = U32_MAX;
                }

                if (Mesh)
                {
                    if (Mesh->Type != json_element_type::Number)
                    {
                        UnhandledError("Invalid glTF node mesh type");
                    }

                    Dst->MeshIndex = Mesh->Number.AsU32();
                }

                if (Children)
                {
                    if (Children->Type != json_element_type::Array)
                    {
                        UnhandledError("Invalid glTF node children type");
                    }

                    Dst->ChildrenCount = (u32)Children->Array.ElementCount;
                    Dst->Children = PushArray<u32>(Arena, Dst->ChildrenCount, MemPush_Clear);

                    for (u32 i = 0; i < Dst->ChildrenCount; i++)
                    {
                        json_element* Child = Children->Array.Elements + i;
                        if (Child->Type != json_element_type::Number)
                        {
                            UnhandledError("Invalid glTF node child type");
                        }

                        Dst->Children[i] = Child->Number.AsU32();
                    }
                }
            }
        }

        if (Scenes)
        {
            if (Scenes->Type != json_element_type::Array)
            {
                UnhandledError("Invalid glTF \"scenes\" type");
            }

            GLTF->SceneCount = (u32)Scenes->Array.ElementCount;
            GLTF->Scenes = PushArray<gltf_scene>(Arena, GLTF->SceneCount, MemPush_Clear);
            for (u32 SceneIndex = 0; SceneIndex < GLTF->SceneCount; SceneIndex++)
            {
                gltf_scene* Dst = GLTF->Scenes + SceneIndex;
                json_element* Src = Scenes->Array.Elements + SceneIndex;
                if (Src->Type != json_element_type::Object)
                {
                    UnhandledError("Invalid glTF scene type");
                }

                json_element* SceneNodes = GetElement(&Src->Object, "nodes");
                if (SceneNodes)
                {
                    if (SceneNodes->Type != json_element_type::Array)
                    {
                        UnhandledError("Invalid glTF scene nodes type");
                    }

                    Dst->RootNodeCount = (u32)SceneNodes->Array.ElementCount;
                    Dst->RootNodes = PushArray<u32>(Arena, Dst->RootNodeCount, MemPush_Clear);
                    for (u32 i = 0; i < Dst->RootNodeCount; i++)
                    {
                        json_element* Node = SceneNodes->Array.Elements + i;
                        if (Node->Type != json_element_type::Number)
                        {
                            UnhandledError("Invalid glTF scene node type");
                        }

                        Dst->RootNodes[i] = Node->Number.AsU32();
                    }
                }
            }
        }

        if (Scene)
        {
            if (Scene->Type != json_element_type::Number)
            {
                UnhandledError("Invalid glTF scene type");
            }

            GLTF->DefaultSceneIndex = Scene->Number.AsU32();
        }
    }

    return Result;
}

internal u64 GLTF_GetElementSize(gltf_component_type ComponentType, gltf_type Type)
{
    u64 Result = 1;
    switch (ComponentType)
    {
        case GLTF_SBYTE:
        case GLTF_UBYTE: 
            Result = 1; 
            break;
        case GLTF_SSHORT:
        case GLTF_USHORT: 
            Result = 2; 
            break;
        case GLTF_SINT:
        case GLTF_UINT:
        case GLTF_FLOAT: 
            Result = 4; 
            break;
        default:
            UnhandledError("Invalid glTF accessor component type");
            break;
    }

    switch (Type)
    {
        case GLTF_SCALAR:   Result *= 1; break;
        case GLTF_VEC2:     Result *= 2; break;
        case GLTF_VEC3:     Result *= 3; break;
        case GLTF_VEC4:     Result *= 4; break;
        case GLTF_MAT2:     Result *= 4; break;
        case GLTF_MAT3:     Result *= 9; break;
        case GLTF_MAT4:     Result *= 16; break;
        default:
            UnhandledError("Invalid glTF accessor type");
            break;
    }
    return Result;
}

lbfn gltf_iterator MakeGLTFAttribIterator(gltf* GLTF, 
                                              gltf_accessor* Accessor, 
                                              buffer* Buffers)
{
    gltf_iterator It = {};
    It.GLTF = GLTF;
    if (Accessor)
    {
        It.Accessor = Accessor;
        if (Accessor->BufferView >= GLTF->BufferViewCount)
        {
            UnhandledError("Invalid glTF bufferView index");
        }

        gltf_buffer_view* BufferView = GLTF->BufferViews + Accessor->BufferView;
        if (BufferView->BufferIndex >= GLTF->BufferCount)
        {
            UnhandledError("Invalid glTF buffer index");
        }

        buffer* Buffer = Buffers + BufferView->BufferIndex;
        if (BufferView->Offset + BufferView->Size > Buffer->Size)
        {
            UnhandledError("Invalid glTF bufferView range");
        }

        u64 ElementSize = GLTF_GetElementSize(Accessor->ComponentType, Accessor->Type);
        u64 Stride = BufferView->Stride ? BufferView->Stride : ElementSize;

        if ((BufferView->Offset + Accessor->ByteOffset + Accessor->Count * Stride) > Buffer->Size)
        {
            UnhandledError("Invalid glTF accessor range");
        }

        It.ElementSize = ElementSize;
        It.Stride = Stride;
        It.AtIndex = 0;
        It.Count = Accessor->Count;
        It.At = (u8*)OffsetPtr(Buffer->Data, Accessor->ByteOffset + BufferView->Offset);
    }
    return It;
}

template<typename T>
T gltf_iterator::Get() const
{
    T Result = {};
    if (At)
    {
        if (AtIndex >= Count)
        {
            UnhandledError("Out of bounds");
            return Result;
        }
        if (sizeof(T) != ElementSize) UnhandledError("Incompatible elements");
        memcpy(&Result, At, Min(ElementSize, sizeof(T)));
    }
    return Result;
}

gltf_iterator& gltf_iterator::operator++()
{
    if (AtIndex + 1 <= Count)
    {
        AtIndex++;
        At += Stride;
    }
    return (*this);
}

gltf_iterator::operator bool() const
{
    bool Result = (!At) || (AtIndex < Count);
    return Result;
}