#pragma once

constexpr u32 MaxCascadeCount = 4;

struct render_camera
{
    m4 CameraTransform;
    m4 ViewTransform;
    m4 ProjectionTransform;
    m4 ViewProjectionTransform;

    f32 AspectRatio;
    f32 FocalLength;
    f32 NearZ;
    f32 FarZ;
};

struct shadow_cascades
{
    m4 ViewProjection;
    v4 Scales[MaxCascadeCount - 1];
    v4 Offsets[MaxCascadeCount - 1];
    f32 Nd[MaxCascadeCount];
    f32 Fd[MaxCascadeCount];
};

void SetupShadowCascades(shadow_cascades* Cascades, const render_camera* Camera, v3 SunV, f32 CascadeResolution);