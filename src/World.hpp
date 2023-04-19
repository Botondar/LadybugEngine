#pragma once

struct camera
{
    v3 P;
    f32 Yaw;
    f32 Pitch;
    f32 NearZ;
    f32 FarZ;
    f32 FieldOfView;
};

lbfn m4 GetLocalTransform(const camera* Camera);
lbfn m4 GetTransform(const camera* Camera);
