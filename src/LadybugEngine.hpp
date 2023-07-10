#pragma once

/*
- Render pass cleanup:
    - Certain render passes (SSAO, Bloom, etc.) should have struct definitions that
      contain their inputs/parameters, descriptor definitions, etc.
- Animation:
    - Separate vertex buffer in the geometry heap for animated meshes
    - Skinning pass on GPU: 
        - Writes data into the static geometry vertex buffer w/o joint 
          weights/indices (specialized section in vertbuffer?)
    - Skinned mesh data can be referenced as static geometry
- Decals
- Multiple lights/shadows
- Visibility buffer rendering
- Transparents
*/
#include "LadybugLib/Core.hpp"
#include "LadybugLib/Intrinsics.hpp"
#include "LadybugLib/String.hpp"

#include "LadybugLib/lbfnt.hpp"
#include "LadybugLib/JSON.hpp"
#include "LadybugLib/glTF.hpp"

#include "Platform.hpp"
#include "Font.hpp"
#include "World.hpp"
#include "Debug.hpp"
#include "Editor.hpp"
#include "Renderer/Renderer.hpp"

#include <nvtt/nvtt.h>
#include <stb/stb_image.h>

// snprintf
#include <cstdio>
#include <cstdarg>

lbfn frustum FrustumFromCamera(const camera* Camera);
lbfn bool IntersectFrustum(const frustum* Frustum, const mmbox* Box);

//
// Mesh
//
struct mesh
{
    u32 VertexCount;
    u32 IndexCount;
    vertex* VertexData;
    vert_index* IndexData;
    mmbox Box;
};

struct mesh_instance
{
    u32 MeshID;
    m4 Transform;
};

struct animation_transform
{
    v4 Rotation;
    v3 Position;
    v3 Scale;
};

struct skinned_mesh_instance
{
    u32 MeshID;
    u32 SkinID;
    u32 CurrentAnimationID;
    b32 DoAnimation;
    f32 AnimationCounter;
    m4 Transform;
};

struct skin
{
    static constexpr u32 MaxJointCount = 256;
    u32 JointCount;
    m4 InverseBindMatrices[MaxJointCount];
    animation_transform BindPose[MaxJointCount];
    u32 JointParents[MaxJointCount];
};

struct animation_key_frame
{
    animation_transform JointTransforms[skin::MaxJointCount];
};

struct animation
{
    u32 SkinID;
    u32 KeyFrameCount;
    f32 MinTimestamp;
    f32 MaxTimestamp;
    f32* KeyFrameTimestamps;
    animation_key_frame* KeyFrames;
};

struct assets
{
    memory_arena* Arena;

    texture_id DefaultDiffuseID;
    texture_id DefaultNormalID;
    texture_id DefaultMetallicRoughnessID;

    texture_id DefaultFontTextureID;
    font DefaultFont;

    static constexpr u32 MaxMeshCount = 1u << 16;
    static constexpr u32 MaxMaterialCount = 1u << 14;
    static constexpr u32 MaxSkinCount = 1u << 14;
    static constexpr u32 MaxAnimationCount = 1u << 16;
    u32 MeshCount;
    u32 MaterialCount;
    u32 SkinCount;
    u32 AnimationCount;

    geometry_buffer_allocation Meshes[MaxMeshCount];
    mmbox MeshBoxes[MaxMeshCount];
    u32 MeshMaterialIndices[MaxMeshCount];

    material Materials[MaxMaterialCount];

    skin Skins[MaxSkinCount];

    animation Animations[MaxAnimationCount];
};

struct game_world
{
    b32 IsLoaded;
    camera Camera;
    v3 SunL;
    v3 SunV;

    static constexpr u32 MaxInstanceCount = 1u << 21;
    u32 InstanceCount;
    u32 SkinnedInstanceCount;
    mesh_instance Instances[MaxInstanceCount];
    skinned_mesh_instance SkinnedInstances[MaxInstanceCount];
};

struct game_state
{
    memory_arena TotalArena;
    memory_arena TransientArena;

    vulkan Vulkan;
    renderer* Renderer;

    u64 FrameID;

    debug_state Debug;
    editor_state Editor;

    post_process_params PostProcessParams;

    assets* Assets;
    game_world* World;
};

// TODO(boti): move this

static m4 TransformToM4(animation_transform Transform);

struct ray
{
    v3 P;
    v3 V;
};

static bool IntersectRayBox(ray Ray, v3 P, v3 HalfExtent, m4 Transform, f32 tMax, f32* tOut);

static m4 TransformToM4(animation_transform Transform)
{
    m4 Result;

    m4 S = M4(Transform.Scale.x, 0.0f, 0.0f, 0.0f,
              0.0f, Transform.Scale.y, 0.0f, 0.0f,
              0.0f, 0.0f, Transform.Scale.z, 0.0f,
              0.0f, 0.0f, 0.0f, 1.0f);
    m4 R = QuaternionToM4(Transform.Rotation);
    m4 T = M4(1.0f, 0.0f, 0.0f, Transform.Position.x,
              0.0f, 1.0f, 0.0f, Transform.Position.y,
              0.0f, 0.0f, 1.0f, Transform.Position.z,
              0.0f, 0.0f, 0.0f, 1.0f);

    Result = T * R * S;
    return(Result);
}

static bool IntersectRayBox(ray Ray, v3 P, v3 HalfExtent, m4 Transform, f32 tMax, f32* tOut)
{
    bool Result = false;

    constexpr f32 Tolerance = 1e-7f;

    P = TransformPoint(Transform, P);

    v3 Axes[3] = 
    {
        Transform.X.xyz,
        Transform.Y.xyz,
        Transform.Z.xyz,
    };
    f32 AxisLengths[3] = 
    {
        VectorLength(Axes[0]),
        VectorLength(Axes[1]),
        VectorLength(Axes[2]),
    };

    for (u32 i = 0; i < 3; i++) 
    {
        Axes[i] = Axes[i] * (1.0f / AxisLengths[i]);
        HalfExtent.E[i] = HalfExtent.E[i] * AxisLengths[i];
    }

    for (u32 i = 0; i < 3; i++)
    {
        u32 j = (i + 1) % 3;
        u32 k = (i + 2) % 3;

        v3 N = Axes[i];
        // Two checks for the positive and negative planes along the current axis,
        // N gets negated at the of this loop
        for (u32 n = 0; n < 2; n++)
        {
            f32 NdotP = Dot(Ray.P, N);
            f32 NdotV = Dot(Ray.V, N);
            f32 d = -Dot(P, N) - HalfExtent.E[i];

            if (Abs(NdotV) >= Tolerance)
            {
                f32 t = (-NdotP - d) / NdotV;
                if (t < tMax && t > 0.0f)
                {
                    v3 HitP = Ray.P + Ray.V * t;
                    v3 DeltaP = HitP - P;
                    f32 Proj1 = Abs(Dot(DeltaP, Axes[j]));
                    f32 Proj2 = Abs(Dot(DeltaP, Axes[k]));

                    if (Proj1 < HalfExtent.E[j] && Proj2 < HalfExtent.E[k])
                    {
                        *tOut = t;
                        tMax = t;
                        Result = true;
                    }
                }
            }

            N = -N;
        }
    }

    return Result;
}