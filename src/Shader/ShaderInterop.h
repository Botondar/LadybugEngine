#if defined(__cplusplus)

#else

#   define PI 3.14159265359

#   define u32      uint
#   define f32      float
#   define v2       vec2
#   define v3       vec3
#   define v4       vec4
#   define m2       mat2
#   define m3       mat3
#   define m4       mat4
#   define v2u      uvec2

#   define rgba8               u32
#   define renderer_texture_id u32

#endif

//
// Config
//
#define R_AlphaTestThreshold 0.5f

#define R_TileSizeX 16
#define R_TileSizeY 16
#define R_MaxLightCountPerTile 255
#define R_LuminanceThreshold 1e-2f
#define R_MaxLightCount 16384
#define R_MaxShadowCount 64

struct light
{
    v4 P;
    v4 E;
};

struct screen_tile
{
    u32 LightCount;
    u32 LightIndices[R_MaxLightCountPerTile];
};

struct per_frame
{
    m4 CameraTransform;
    m4 View;
    m4 Projection;
    m4 InverseProjection;
    m4 ViewProjection;

    m4 CascadeViewProjections[4];

    m4 ShadowViewProjections[6*R_MaxShadowCount];

    f32 CascadeMinDistances[4];
    f32 CascadeMaxDistances[4];
    v3 CascadeScales[3];
    v3 CascadeOffsets[3];

    f32 FocalLength;
    f32 AspectRatio;
    f32 NearZ;
    f32 FarZ;

    v3 CameraP;
    v3 SunV;
    v3 SunL;

    v2 ScreenSize;
    v2u TileCount;

    u32 LightCount;

};

struct renderer_material
{
    v3 Emissive;
    rgba8 DiffuseColor;
    rgba8 BaseMaterial;
    renderer_texture_id DiffuseID;
    renderer_texture_id NormalID;
    renderer_texture_id MetallicRoughnessID;
};