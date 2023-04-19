@echo off

FOR /F %%G IN ('dir /s /b "src/shader"') DO (
    IF %%~xG==.glsl (
        glslc --target-env=vulkan1.3 -DVS -fshader-stage=vert -o "build/%%~nG.vs" %%G
        glslc --target-env=vulkan1.3 -DFS -fshader-stage=frag -o "build/%%~nG.fs" %%G
    ) ELSE IF %%~xG==.comp (
       glslc --target-env=vulkan1.3 -DCS -fshader-stage=comp -o "build/%%~nG.cs" %%G
    )
)