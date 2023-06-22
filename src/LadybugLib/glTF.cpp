#include "glTF.hpp"

internal u32 GLTFTypeElementCounts[GLTF_TYPE_COUNT] = 
{
    [GLTF_SCALAR] = 1,
    [GLTF_VEC2]   = 2,
    [GLTF_VEC3]   = 3,
    [GLTF_VEC4]   = 4,
    [GLTF_MAT2]   = 4,
    [GLTF_MAT3]   = 9,
    [GLTF_MAT4]   = 16,
};

enum gltf_elem_flags : flags32
{
    GLTF_Flags_None = 0,
    GLTF_Required = 1 << 0,
};

internal string ParseString(json_element* Elem, gltf_elem_flags Flags);
internal b32 ParseB32(json_element* Elem, gltf_elem_flags Flags, b32 DefaultValue = false);
internal u32 ParseU32(json_element* Elem, gltf_elem_flags Flags, u32 DefaultValue = 0);
internal f32 ParseF32(json_element* Elem, gltf_elem_flags Flags, f32 DefaultValue = 0.0f);

internal void ParseGLTFVector(float* Dst, json_element* Elem, gltf_elem_flags Flags, gltf_type Type, m4 DefaultValue = {});
internal gltf_texture_info ParseTextureInfo(json_element* Elem, gltf_elem_flags Flags);
internal gltf_type ParseGLTFType(json_element* Elem, gltf_elem_flags Flags, gltf_type DefaultValue = GLTF_SCALAR);
internal gltf_alpha_mode ParseGLTFAlphaMode(json_element* Elem, gltf_elem_flags Flags, gltf_alpha_mode DefaultValue = GLTF_ALPHA_MODE_OPAQUE);
internal gltf_animation_path ParseGLTFAnimationPath(json_element* Elem, gltf_elem_flags Flags, gltf_animation_path DefaultValue = GLTF_Scale);
internal gltf_animation_interpolation ParseGLTFInterpolation(json_element* Elem, gltf_elem_flags Flags, gltf_animation_interpolation DefaultValue = GLTF_Linear);

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
    json_element* Skins       = GetElement(RootObject, "skins");
    json_element* Animations  = GetElement(RootObject, "animations");
    json_element* Cameras     = GetElement(RootObject, "cameras");

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
                ParseGLTFVector(Dst->Max.EE, GetElement(Obj, "max"), GLTF_Flags_None, Dst->Type, { 0.0f, 0.0f, 0.0f });
                ParseGLTFVector(Dst->Min.EE, GetElement(Obj, "min"), GLTF_Flags_None, Dst->Type, { 0.0f, 0.0f, 0.0f });

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
            Assert(Src->Type == json_element_type::Object);
            json_object* Obj = &Src->Object;

            Dst->URI = ParseString(GetElement(Obj, "uri"), GLTF_Flags_None);
            Dst->MimeType = ParseString(GetElement(Obj, "mimeType"), GLTF_Flags_None);
            Dst->BufferViewIndex = ParseU32(GetElement(Obj, "bufferView"), GLTF_Flags_None, U32_MAX);

            // TODO(boti)
            if (Dst->BufferViewIndex != U32_MAX)
            {
                UnimplementedCodePath;
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

            json_element* EmissiveFactor    = GetElement(&Src->Object, "emissiveFactor");

            Dst->NormalTexture      = ParseTextureInfo(GetElement(&Src->Object, "normalTexture"), GLTF_Flags_None);
            Dst->OcclusionTexture   = ParseTextureInfo(GetElement(&Src->Object, "occlusionTexture"), GLTF_Flags_None);
            Dst->EmissiveTexture    = ParseTextureInfo(GetElement(&Src->Object, "emissiveTexture"), GLTF_Flags_None);
            Dst->IsDoubleSided      = ParseB32(GetElement(&Src->Object, "doubleSided"), GLTF_Flags_None);
            Dst->AlphaMode          = ParseGLTFAlphaMode(GetElement(&Src->Object, "alphaMode"), GLTF_Flags_None, GLTF_ALPHA_MODE_OPAQUE);
            Dst->AlphaCutoff        = ParseF32(GetElement(&Src->Object, "alphaCutOff"), GLTF_Flags_None, 0.5f);
            ParseGLTFVector(Dst->EmissiveFactor.E, GetElement(&Src->Object, "emissiveFactor"), GLTF_Flags_None, GLTF_VEC3, { 0.0f, 0.0f, 0.0f });

            json_element* PBR = GetElement(&Src->Object, "pbrMetallicRoughness");
            if (PBR) 
            {
                Assert(PBR->Type == json_element_type::Object);

                ParseGLTFVector(Dst->BaseColorFactor.E, GetElement(&PBR->Object, "baseColorFactor"), GLTF_Flags_None, GLTF_VEC4, { 1.0f, 1.0f, 1.0f, 1.0f });
                Dst->MetallicFactor = ParseF32(GetElement(&PBR->Object, "metallicFactor"), GLTF_Flags_None, 1.0f);
                Dst->RoughnessFactor = ParseF32(GetElement(&PBR->Object, "roughnessFactor"), GLTF_Flags_None, 1.0f);

                Dst->BaseColorTexture = ParseTextureInfo(GetElement(&PBR->Object, "baseColorTexture"), GLTF_Flags_None);
                Dst->MetallicRoughnessTexture = ParseTextureInfo(GetElement(&PBR->Object, "metallicRoughnessTexture"), GLTF_Flags_None);
            }
            else
            {
                UnimplementedCodePath;
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
                    json_element* Targets       = GetElement(&Src->Object, "target");

                    Dst->IndexBufferIndex = ParseU32(GetElement(&Src->Object, "indices"), GLTF_Flags_None, U32_MAX);
                    Dst->MaterialIndex = ParseU32(GetElement(&Src->Object, "material"), GLTF_Flags_None, U32_MAX);
                    Dst->Topology = (gltf_topology)ParseU32(GetElement(&Src->Object, "mode"), GLTF_Flags_None, GLTF_TRIANGLES);
                    if (GetElement(&Src->Object, "target"))
                    {
                        UnimplementedCodePath;
                    }

                    if (Attributes)
                    {
                        if (Attributes->Type != json_element_type::Object)
                        {
                            UnhandledError("Invalid glTF primitve attributes type");
                        }

                        Dst->PositionIndex      = ParseU32(GetElement(&Attributes->Object, "POSITION"), GLTF_Flags_None, U32_MAX);
                        Dst->NormalIndex        = ParseU32(GetElement(&Attributes->Object, "NORMAL"), GLTF_Flags_None, U32_MAX);
                        Dst->TangentIndex       = ParseU32(GetElement(&Attributes->Object, "TANGENT"), GLTF_Flags_None, U32_MAX);
                        Dst->ColorIndex         = ParseU32(GetElement(&Attributes->Object, "COLOR_0"), GLTF_Flags_None, U32_MAX);
                        Dst->TexCoordIndex[0]   = ParseU32(GetElement(&Attributes->Object, "TEXCOORD_0"), GLTF_Flags_None, U32_MAX);
                        Dst->TexCoordIndex[1]   = ParseU32(GetElement(&Attributes->Object, "TEXCOORD_1"), GLTF_Flags_None, U32_MAX);
                        Dst->JointsIndex        = ParseU32(GetElement(&Attributes->Object, "JOINTS_0"), GLTF_Flags_None, U32_MAX);
                        Dst->WeightsIndex       = ParseU32(GetElement(&Attributes->Object, "WEIGHTS_0"), GLTF_Flags_None, U32_MAX);

                        // TODO(boti);
                        if (GetElement(&Attributes->Object, "JOINTS_1")) UnimplementedCodePath;
                        if (GetElement(&Attributes->Object, "WEIGHTS_1")) UnimplementedCodePath;
                    }
                    else
                    {
                        UnhandledError("Missing glTF primitive attributes");
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
    }

    
    if (Animations)
    {
        Assert(Animations->Type == json_element_type::Array);
        GLTF->AnimationCount = Animations->Array.ElementCount;
        GLTF->Animations = PushArray<gltf_animation>(Arena, GLTF->AnimationCount, MemPush_Clear);

        for (u32 AnimationIndex = 0; AnimationIndex < GLTF->AnimationCount; AnimationIndex++)
        {
            gltf_animation* Animation = GLTF->Animations + AnimationIndex;
            json_element* Elem = Animations->Array.Elements + AnimationIndex;
            Assert(Elem->Type == json_element_type::Object);

            Animation->Name = ParseString(GetElement(&Elem->Object, "name"), GLTF_Flags_None);
                
            json_element* Channels = GetElement(&Elem->Object, "channels");
            if (Channels)
            {
                Assert((Channels->Type == json_element_type::Array) &&
                       (Channels->Array.ElementCount > 0));

                Animation->ChannelCount = Channels->Array.ElementCount;
                Animation->Channels = PushArray<gltf_animation_channel>(Arena, Animation->ChannelCount, MemPush_Clear);
                    
                for (u32 ChannelIndex = 0; ChannelIndex < Animation->ChannelCount; ChannelIndex++)
                {
                    json_element* Channel = Channels->Array.Elements + ChannelIndex;
                    Assert(Channel->Type == json_element_type::Object);
                    Animation->Channels[ChannelIndex].SamplerIndex = ParseU32(GetElement(&Channel->Object, "sampler"), GLTF_Required);
                        
                    json_element* Target = GetElement(&Channel->Object, "target");
                    Assert(Target && Target->Type == json_element_type::Object);

                    // NOTE(boti): the node is technically not required to be present by the spec, but if it's missing,
                    // an extension might define it, which we don't currently handle.
                    Animation->Channels[ChannelIndex].Target.NodeIndex = ParseU32(GetElement(&Target->Object, "node"), GLTF_Required);
                    Animation->Channels[ChannelIndex].Target.Path = ParseGLTFAnimationPath(GetElement(&Target->Object, "path"), GLTF_Required);
                }
            }
            else
            {
                UnhandledError("Missing channels from glTF animation");
            }

            json_element* Samplers = GetElement(&Elem->Object, "samplers");
            if (Samplers)
            {
                Assert((Samplers->Type == json_element_type::Array) &&
                       (Samplers->Array.ElementCount > 0));

                Animation->SamplerCount = Channels->Array.ElementCount;
                Animation->Samplers = PushArray<gltf_animation_sampler>(Arena, Animation->ChannelCount, MemPush_Clear);

                for (u32 SamplerIndex = 0; SamplerIndex < Animation->SamplerCount; SamplerIndex++)
                {
                    json_element* Sampler = Samplers->Array.Elements + SamplerIndex;
                    Assert(Sampler->Type == json_element_type::Object);

                    Animation->Samplers[SamplerIndex].InputAccessorIndex = ParseU32(GetElement(&Sampler->Object, "input"), GLTF_Required);
                    Animation->Samplers[SamplerIndex].OutputAccessorIndex = ParseU32(GetElement(&Sampler->Object, "output"), GLTF_Required);
                    Animation->Samplers[SamplerIndex].Interpolation = ParseGLTFInterpolation(GetElement(&Sampler->Object, "interpolation"), GLTF_Flags_None, GLTF_Linear);
                }
            }
            else
            {
                UnhandledError("Missing samplers from glTF animation");
            }
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
            json_element* Mesh          = GetElement(&Src->Object, "mesh");
            json_element* Weights       = GetElement(&Src->Object, "weights");

            Dst->CameraIndex = ParseU32(GetElement(&Src->Object, "camera"), GLTF_Flags_None, U32_MAX);
            Dst->SkinIndex = ParseU32(GetElement(&Src->Object, "skin"), GLTF_Flags_None, U32_MAX);
            Dst->MeshIndex = ParseU32(GetElement(&Src->Object, "mesh"), GLTF_Flags_None, U32_MAX);

            ParseGLTFVector(Dst->Transform.EE, GetElement(&Src->Object, "matrix"), GLTF_Flags_None, GLTF_MAT4,
                            M4(1.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 0.0f, 1.0f));

            json_element* Scale         = GetElement(&Src->Object, "scale");
            json_element* Rotation      = GetElement(&Src->Object, "rotation");
            json_element* Translation   = GetElement(&Src->Object, "translation");
            ParseGLTFVector(Dst->Scale.E, Scale, GLTF_Flags_None, GLTF_VEC3, { 1.0f, 1.0f, 1.0f });
            ParseGLTFVector(Dst->Rotation.E, Rotation, GLTF_Flags_None, GLTF_VEC4, { 0.0f, 0.0f, 0.0f, 1.0f });
            ParseGLTFVector(Dst->Translation.E, Translation, GLTF_Flags_None, GLTF_VEC3, { 0.0f, 0.0f, 0.0f });
            if (Scale || Rotation || Translation)
            {
                Dst->IsTRS = true;
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
                    Dst->Children[i] = ParseU32(Child, GLTF_Required);
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

    GLTF->DefaultSceneIndex = ParseU32(Scene, GLTF_Flags_None, 0);

    return Result;
}


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
    f32 Result = DefaultValue;
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

internal void ParseGLTFVector(float* Dst, json_element* Elem, gltf_elem_flags Flags, gltf_type Type, m4 DefaultValue /*= {}*/)
{
    u32 Count = GLTFTypeElementCounts[Type];
    for (u32 i = 0; i < Count; i++)
    {
        Dst[i] = DefaultValue.EE[i];
    }

    if (Elem)
    {
        Assert(Elem->Type == json_element_type::Array);
        json_array* Array = &Elem->Array;
        Assert(Array->ElementCount == Count);

        for (u32 i = 0; i < Count; i++)
        {
            Dst[i] = ParseF32(Array->Elements + i, GLTF_Required);
        }
    }
    else if (HasFlag(Flags, GLTF_Required))
    {
        UnhandledError("Missing required glTF element");
    }
}

internal gltf_alpha_mode ParseGLTFAlphaMode(json_element* Elem, gltf_elem_flags Flags, gltf_alpha_mode DefaultValue /*= GLTF_ALPHA_MODE_OPAQUE*/)
{
    gltf_alpha_mode Result = DefaultValue;
    if (Elem)
    {
        if (Elem->Type != json_element_type::String)
        {
            UnhandledError("Invalid glTF element type");
        }

        if      (StringEquals(&Elem->String, "OPAQUE")) Result = GLTF_ALPHA_MODE_OPAQUE;
        else if (StringEquals(&Elem->String, "MASK"))   Result = GLTF_ALPHA_MODE_MASK;
        else if (StringEquals(&Elem->String, "BLEND"))  Result = GLTF_ALPHA_MODE_BLEND;
        else 
        {
            UnhandledError("Invalid glTF alpha mode value");
        }
    }
    else if (HasFlag(Flags, GLTF_Required))
    {
        UnhandledError("Missing required glTF element");
    }
    return(Result);
}

internal gltf_texture_info ParseTextureInfo(json_element* Elem, gltf_elem_flags Flags)
{
    gltf_texture_info Result = { U32_MAX, 0, 1.0f };
    if (Elem)
    {
        Assert(Elem->Type == json_element_type::Object);
        Result.TextureIndex = ParseU32(GetElement(&Elem->Object, "index"), GLTF_Required);
        Result.TexCoordIndex = ParseU32(GetElement(&Elem->Object, "texCoord"), GLTF_Flags_None, 0);
        Result.Scale = ParseF32(GetElement(&Elem->Object, "scale"), GLTF_Flags_None, 1.0f);
    }
    else if (HasFlag(Flags, GLTF_Required))
    {
        UnhandledError("Missing required glTF element");
    }
    return(Result);
}

internal gltf_animation_path ParseGLTFAnimationPath(json_element* Elem, gltf_elem_flags Flags, gltf_animation_path DefaultValue /*= GLTF_Scale*/)
{
    gltf_animation_path Result = DefaultValue;

    if (Elem)
    {
        Assert(Elem->Type == json_element_type::String);
        if      (StringEquals(&Elem->String, "weights"))     Result = GLTF_Weights;
        else if (StringEquals(&Elem->String, "scale"))       Result = GLTF_Scale;
        else if (StringEquals(&Elem->String, "rotation"))    Result = GLTF_Rotation;
        else if (StringEquals(&Elem->String, "translation")) Result = GLTF_Translation;
        else
        {
            UnhandledError("Invalid glTF animation path value");
        }
    }
    else if (HasFlag(Flags, GLTF_Required))
    {
        UnhandledError("Missing required glTF element");
    }
    return(Result);
}

internal gltf_animation_interpolation ParseGLTFInterpolation(json_element* Elem, gltf_elem_flags Flags, gltf_animation_interpolation DefaultValue /*= GLTF_Linear*/)
{
    gltf_animation_interpolation Result = DefaultValue;
    if (Elem)
    {
        Assert(Elem->Type == json_element_type::String);
        if      (StringEquals(&Elem->String, "LINEAR")) Result = GLTF_Linear;
        else if (StringEquals(&Elem->String, "STEP")) Result = GLTF_Step;
        else if (StringEquals(&Elem->String, "CUBICSPLINE")) Result = GLTF_CubicSpline;
        else
        {
            UnhandledError("Invalid glTF interpolation value");
        }
    }
    else if (HasFlag(Flags, GLTF_Required))
    {
        UnhandledError("Missing required glTF element");
    }
    return(Result);
}

internal u64 GLTF_GetElementSize(gltf_component_type ComponentType, gltf_type Type)
{
    u64 Result = 1;
    switch (ComponentType)
    {
        case GLTF_SBYTE:
        case GLTF_UBYTE: 
        { 
            Result = 1; 
        } break;
        case GLTF_SSHORT:
        case GLTF_USHORT: 
        {
            Result = 2; 
        } break;
        case GLTF_SINT:
        case GLTF_UINT:
        case GLTF_FLOAT: 
        {
            Result = 4; 
        } break;
        default:
        {
            UnhandledError("Invalid glTF accessor component type");
        } break;
    }

    Result *= GLTFTypeElementCounts[Type];
    return Result;
}

lbfn gltf_iterator MakeGLTFAttribIterator(gltf* GLTF, gltf_accessor* Accessor, buffer* Buffers)
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