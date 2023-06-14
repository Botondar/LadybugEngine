#pragma once

//#include <Core.hpp>
//#include <JSON.hpp>

enum gltf_type : u32
{
    GLTF_SCALAR,
    GLTF_VEC2,
    GLTF_VEC3,
    GLTF_VEC4,
    GLTF_MAT2,
    GLTF_MAT3,
    GLTF_MAT4,

    GLTF_TYPE_COUNT,
};

enum gltf_component_type : u32
{
    GLTF_SBYTE  = 5120,
    GLTF_UBYTE  = 5121,
    GLTF_SSHORT = 5122,
    GLTF_USHORT = 5123,
    GLTF_UINT   = 5124,
    GLTF_SINT   = 5125,
    GLTF_FLOAT  = 5126,
};

enum gltf_topology : u32
{
    GLTF_POINTS         = 0,
    GLTF_LINES          = 1,
    GLTF_LINE_LOOP      = 2,
    GLTF_LINE_STRIP     = 3,
    GLTF_TRIANGLES      = 4,
    GLTF_TRIANGLE_STRIP = 5,
    GLTF_TRIANGLE_FAN   = 6,
};

enum gltf_filter : u32
{
    GLTF_FILTER_NEAREST                 = 9728,
    GLTF_FILTER_LINEAR                  = 9729,
    GLTF_FILTER_NEREAST_MIPMAP_NEAREST  = 9984,
    GLTF_FILTER_LINEAR_MIPMAP_NEAREST   = 9985,
    GLTF_FILTER_NEAREST_MIPMAMP_LINEAR  = 9986,
    GLTF_FILTER_LINEAR_MIPMAP_LINEAR    = 9987,
};

enum gltf_wrap : u32
{
    GLTF_WRAP_REPEAT          = 10497,
    GLTF_WRAP_CLAMP_TO_EDGE   = 33071,
    GLTF_WRAP_MIRRORED_REPEAT = 33648,
};

enum gltf_alpha_mode : u32
{
    GLTF_ALPHA_MODE_OPAQUE = 0,
    GLTF_ALPHA_MODE_MASK,
    GLTF_ALPHA_MODE_BLEND,
};


struct gltf_buffer
{
    string URI;
    u32 ByteLength;
};

struct gltf_buffer_view
{
    u32 BufferIndex;
    u32 Offset;
    u32 Size;
    u32 Stride;
};

struct gltf_accessor
{
    u32 BufferView;
    u32 ByteOffset;
    u32 Count;
    gltf_component_type ComponentType;
    gltf_type Type;
    b32 IsNormalized;
    b32 IsSparse;
    //Sparse;
    //string Name;
    //Extensions;
    //Extras;

    // NOTE(boti): Technically the type of Min/Max is defined by the component type,
    //             but they're used for bounding boxes which means they're going to be floats
    //             most of the time.
    m4 Max; 
    m4 Min;
};

struct gltf_mesh_primitive
{
    u32 PositionIndex;
    u32 NormalIndex;
    u32 ColorIndex;
    u32 TangentIndex;
    u32 TexCoordIndex[2];

    u32 IndexBufferIndex;
    u32 MaterialIndex;

    gltf_topology Topology;
};

struct gltf_mesh
{
    u32 PrimitiveCount;
    gltf_mesh_primitive* Primitives;
};

struct gltf_sampler
{
    gltf_filter MagFilter;
    gltf_filter MinFilter;
    gltf_wrap WrapU;
    gltf_wrap WrapV;
};

struct gltf_texture
{
    u32 SamplerIndex;
    u32 ImageIndex;
};

struct gltf_image
{
    string URI;
    string MimeType;
    u32 BufferViewIndex; // NOTE(boti): this is only valid when URI is missing
};

struct gltf_material
{
    b32 IsDoubleSided;
    gltf_alpha_mode AlphaMode;
    f32 AlphaCutoff;
    v3 EmissiveFactor;

    v4 BaseColorFactor;
    u32 BaseColorTextureIndex;
    u32 BaseColorTexCoordIndex;

    f32 MetallicFactor;
    f32 RoughnessFactor;

    u32 MetallicRoughnessTextureIndex;
    u32 MetallicRoughnessTexCoordIndex;

    u32 NormalTextureIndex;
    u32 NormalTexCoordIndex;
    f32 NormalTexCoordScale;

    u32 OcclusionTextureIndex;
    u32 OcclusionTexCoordIndex;
    f32 OcclusionStrength;

    u32 EmissiveTextureIndex;
    u32 EmmisiveTexCoordIndex;
};

struct gltf_node
{
    b32 IsTRS;
    m4 Transform;
    v4 Rotation;
    v3 Scale;
    v3 Translation;

    u32 CameraIndex;
    u32 SkinIndex;
    u32 MeshIndex;
    u32 ChildrenCount;
    u32* Children;
};

struct gltf_scene
{
    u32 RootNodeCount;
    u32* RootNodes;
};

struct gltf
{
    u32 BufferCount;
    gltf_buffer* Buffers;
    u32 BufferViewCount;
    gltf_buffer_view* BufferViews;
    u32 AccessorCount;
    gltf_accessor* Accessors;
    u32 MeshCount;
    gltf_mesh* Meshes;
    u32 SamplerCount;
    gltf_sampler* Samplers;
    u32 TextureCount;
    gltf_texture* Textures;
    u32 ImageCount;
    gltf_image* Images;
    u32 MaterialCount;
    gltf_material* Materials;
    u32 NodeCount;
    gltf_node* Nodes;
    u32 SceneCount;
    gltf_scene* Scenes;

    u32 DefaultSceneIndex;
};

lbfn bool ParseGLTF(gltf* GLTF, json_element* Root, memory_arena* Arena);

struct gltf_iterator
{
    gltf* GLTF;
    gltf_accessor* Accessor;

    u64 ElementSize;
    u64 Stride;
    u64 AtIndex; // in # of elements
    u64 Count;
    u8* At;

    template<typename T>
    T Get() const;

    gltf_iterator& operator++();

    operator bool() const;
};

lbfn gltf_iterator MakeGLTFAttribIterator(gltf* GLTF, 
                                              gltf_accessor* Accessor, 
                                              buffer* Buffers);