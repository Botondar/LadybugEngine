#pragma once

/*
TODO: list
- Decals
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
#include "Renderer/Renderer.hpp"

typedef flags32 debug_flags;
enum debug_flag : debug_flags
{
    DebugFlag_DrawLights,
    DebugFlag_DrawBoundingBoxes,
    DebugFlag_DrawJoints,
};

#include "Font.hpp"
#include "Asset.hpp"
#include "World.hpp"
#include "Editor.hpp"

#define STB_IMAGE_STATIC
#define STB_IMAGE_RESIZE_STATIC
#define STB_DXT_STATIC
#include "stb/stb_image.h"
#include "stb/stb_image_resize2.h"
#include "stb/stb_dxt.h"


// snprintf
#include <cstdio>
#include <cstdarg>

struct game_state
{
    memory_arena TotalArena;
    memory_arena TransientArena;

    renderer* Renderer;
    renderer_info RendererInfo;

    u64 FrameID;

    editor_state Editor;

    static constexpr f32 DefaultExposure = 0.55f;
    render_config RenderConfig;

    // TODO(boti): Move this to game_world
    f32 ConstantFogDensity;
    f32 LinearFogDensityAtBottom;
    f32 LinearFogMinZ;
    f32 LinearFogMaxZ;

    assets* Assets;
    game_world* World;

    static constexpr u32 MaxPerfDataCount = 64u;
    f32 PerfDataLog[MaxPerfDataCount];
};

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
        Transform.X.XYZ,
        Transform.Y.XYZ,
        Transform.Z.XYZ,
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
        // N gets negated at the end of this loop
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