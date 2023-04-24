lbfn void SetupShadowCascades(shadow_cascades* Cascades, const render_camera* Camera, v3 SunV, f32 CascadeResolution)
{
    v3 SunZ = -SunV;
    v3 SunX, SunY;
    if (Abs(Dot(SunZ, v3{ 0.0f, 0.0f, 0.0f })) < (1.0f - 1e-3f))
    {
        SunY = Normalize(Cross(SunZ, v3{ 1.0f, 0.0f, 0.0f }));
        SunX = Cross(SunY, SunZ);
    }
    else
    {
        SunX = Normalize(Cross(SunZ, v3{ 0.0f, 0.0f, -1.0f }));
        SunY = Cross(SunZ, SunX);
    }

    m4 SunTransform = M4(SunX.x, SunY.x, SunZ.x, 0.0f,
                         SunX.y, SunY.y, SunZ.y, 0.0f,
                         SunX.z, SunY.z, SunZ.z, 0.0f,
                         0.0f, 0.0f, 0.0f, 1.0f);
    m4 SunView = AffineOrthonormalInverse(SunTransform);
    m4 CameraToSun = SunView * Camera->CameraTransform;

    f32 NdTable[R_MaxShadowCascadeCount] = { 0.0f, 2.5f, 10.0f, 20.0f, };
    f32 FdTable[R_MaxShadowCascadeCount] = { 3.0f, 12.5f, 25.0f, 30.0f, };

#if 0
    for (u32 CascadeIndex = 0; CascadeIndex < MaxCascadeCount; CascadeIndex++)
    {
        f32 StartPercent = Max((CascadeIndex - 0.1f) / MaxCascadeCount, 0.0f);
        f32 EndPercent = (CascadeIndex + 1.0f) / MaxCascadeCount;
        f32 NdLinear = Camera->FarZ * StartPercent;
        f32 FdLinear = Camera->FarZ * EndPercent;

        constexpr f32 PolyExp = 2.0f;
        f32 NdPoly = Camera->FarZ * Pow(StartPercent, PolyExp);
        f32 FdPoly = Camera->FarZ * Pow(EndPercent, PolyExp);
        constexpr f32 PolyFactor = 0.9f;
        NdTable[CascadeIndex] = Lerp(NdLinear, NdPoly, PolyFactor);
        FdTable[CascadeIndex] = Lerp(FdLinear, FdPoly, PolyFactor);
    }
#endif

    m4 Cascade0InverseViewProjection;
    for (u32 CascadeIndex = 0; CascadeIndex < MaxCascadeCount; CascadeIndex++)
    {
        f32 Nd = NdTable[CascadeIndex];
        f32 Fd = FdTable[CascadeIndex];

        f32 CascadeNear = (f32)CascadeIndex / MaxCascadeCount;

        v3 CascadeBoxP[8] = 
        {
            // Near points in camera space
            { -Nd * Camera->AspectRatio / Camera->FocalLength, -Nd / Camera->FocalLength, Nd },
            { +Nd * Camera->AspectRatio / Camera->FocalLength, -Nd / Camera->FocalLength, Nd },
            { +Nd * Camera->AspectRatio / Camera->FocalLength, +Nd / Camera->FocalLength, Nd },
            { -Nd * Camera->AspectRatio / Camera->FocalLength, +Nd / Camera->FocalLength, Nd },

            // Far points in camera space
            { -Fd * Camera->AspectRatio / Camera->FocalLength, -Fd / Camera->FocalLength, Fd },
            { +Fd * Camera->AspectRatio / Camera->FocalLength, -Fd / Camera->FocalLength, Fd },
            { +Fd * Camera->AspectRatio / Camera->FocalLength, +Fd / Camera->FocalLength, Fd },
            { -Fd * Camera->AspectRatio / Camera->FocalLength, +Fd / Camera->FocalLength, Fd },
        };

        v3 CascadeBoxMin = { +FLT_MAX, +FLT_MAX, +FLT_MAX };
        v3 CascadeBoxMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

        for (u32 i = 0; i < 8; i++)
        {
            v4 P = CameraToSun * v4{ CascadeBoxP[i].x, CascadeBoxP[i].y, CascadeBoxP[i].z, 1.0f };

            CascadeBoxMin = 
            {
                Min(P.x, CascadeBoxMin.x),
                Min(P.y, CascadeBoxMin.y),
                Min(P.z, CascadeBoxMin.z),
            };
            CascadeBoxMax = 
            {
                Max(P.x, CascadeBoxMax.x),
                Max(P.y, CascadeBoxMax.y),
                Max(P.z, CascadeBoxMax.z),
            };
        }

        f32 CascadeScale = Ceil(Max(VectorLength(CascadeBoxP[0] - CascadeBoxP[6]),
                                    VectorLength(CascadeBoxP[4] - CascadeBoxP[6])));
        f32 TexelSize = CascadeScale / CascadeResolution; 

        v3 CascadeP = 
        {
            TexelSize * Floor((CascadeBoxMax.x + CascadeBoxMin.x) / (2.0f * TexelSize)),
            TexelSize * Floor((CascadeBoxMax.y + CascadeBoxMin.y) / (2.0f * TexelSize)),
            CascadeBoxMin.z,
        };

        m4 CascadeView = M4(SunX.x, SunX.y, SunX.z, -CascadeP.x,
                            SunY.x, SunY.y, SunY.z, -CascadeP.y,
                            SunZ.x, SunZ.y, SunZ.z, -CascadeP.z,
                            0.0f, 0.0f, 0.0f, 1.0f);
        m4 CascadeProjection= M4(2.0f / CascadeScale, 0.0f, 0.0f, 0.0f,
                                 0.0f, 2.0f / CascadeScale, 0.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f / (CascadeBoxMax.z - CascadeBoxMin.z), 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f);
        m4 CascadeViewProjection = CascadeProjection * CascadeView;

        if (CascadeIndex == 0)
        {
            m4 InverseProjection = CascadeProjection;
            for (u32 i = 0; i < 3; i++)
            {
                InverseProjection.E[i][i] = 1.0f / InverseProjection.E[i][i];
            }
            Cascade0InverseViewProjection = AffineOrthonormalInverse(CascadeView) * InverseProjection;
            Cascades->ViewProjection = CascadeViewProjection;
        }
        else
        {
            m4 Cascade0ToI = CascadeViewProjection * Cascade0InverseViewProjection;

            Cascades->Scales[CascadeIndex - 1] = { Cascade0ToI.E[0][0], Cascade0ToI.E[1][1], Cascade0ToI.E[2][2], 0.0f };
            Cascades->Offsets[CascadeIndex - 1] = { Cascade0ToI.P.x, Cascade0ToI.P.y, Cascade0ToI.P.z, 0.0f };
        }
        Cascades->Nd[CascadeIndex] = Nd;
        Cascades->Fd[CascadeIndex] = Fd;
    }
}