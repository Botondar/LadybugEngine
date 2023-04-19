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

#include "Font.hpp"
#include "Platform.hpp"
#include "World.hpp"
#include "Debug.hpp"
#include "Editor.hpp"
#include "Renderer/VulkanRenderer.hpp"

#include <nvtt/nvtt.h>
#include <stb/stb_image.h>

// snprintf
#include <cstdio>

struct material
{
    v3 Emissive;
    rgba8 DiffuseColor;
    rgba8 BaseMaterial;
    texture_id DiffuseID;
    texture_id NormalID;
    texture_id MetallicRoughnessID;
};

struct alignas(4) push_constants
{
    m4 Transform;
    material Material;
};

struct frame_context
{
    u64 FrameIndex;
    //render_frame_context Render;
};

struct frustum
{
    union
    {
        struct
        {
            v4 Left;
            v4 Right;
            v4 Top;
            v4 Bottom;
            v4 Near;
            v4 Far;
        };
        v4 Planes[6];
    };
};

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

struct game_state
{
    memory_arena TotalArena;
    memory_arena TransientArena;

    vulkan Vulkan;
    vulkan_renderer* Renderer;

    u64 FrameID;

    debug_state Debug;
    editor_state Editor;

    font Font;

    camera Camera;
    v3 SunL;
    v3 SunV;

    texture_id DefaultDiffuseID;
    texture_id DefaultNormalID;
    texture_id DefaultMetallicRoughnessID;

    static constexpr u32 MaxMeshCount = 65536;
    u32 MeshCount;
    geometry_buffer_allocation Meshes[MaxMeshCount];
    mmbox MeshBoxes[MaxMeshCount];
    u32 MeshMaterialIndices[MaxMeshCount];

    static constexpr u32 MaxMaterialCount = 16384;
    u32 MaterialCount;
    material Materials[MaxMaterialCount];

    static constexpr u32 MaxInstanceCount = (1u << 21);
    u32 InstanceCount;
    mesh_instance Instances[MaxInstanceCount];
};

// TODO(boti): move this
struct ray
{
    v3 P;
    v3 V;
};

static bool IntersectRayBox(ray Ray, v3 P, v3 HalfExtent, m4 Transform, f32 tMax, f32* tOut);

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