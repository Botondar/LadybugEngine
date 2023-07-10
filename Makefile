OUT = build
SRC = src

.SUFFIXES : .glsl .comp

COMMON = -nologo -std:c++20 -Zi -EHsc -MD -arch:AVX2
FLOAT_ENV = -fp:except- -fp:strict
OPTIMIZATION = -Od -Oi
INCLUDES = -I$(SRC) -I$(VULKAN_SDK)/Include/
DEFINES = -DDEVELOPER=1 -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX
WARNINGS_MSVC = -WX -W4 -wd4100 -wd4189 -wd4200 -wd4201 -wd4505
WARNINGS_CLANG = -Wno-unused-function -Wno-unused-variable -Wno-unused-lambda-capture -Wno-unused-value -Wno-missing-field-initializers -Wno-char-subscripts -Wno-missing-braces -Wno-c99-designator
WARNINGS = $(WARNINGS_MSVC) $(WARNINGS_CLANG)

CXX_FLAGS = $(COMMON) $(FLOAT_ENV) $(INCLUDES) $(DEFINES) $(WARNINGS)

LINK_FLAGS = -LIBPATH:"$(VULKAN_SDK)/Lib/" -LIBPATH:lib/
SHADER_FLAGS = -O --target-env=vulkan1.3

LIBS = kernel32.lib user32.lib gdi32.lib Shell32.lib advapi32.lib vulkan-1.lib nvtt30106.lib

SRC_BASE = "$(SRC)/*.cpp" "$(SRC)/*.hpp"
SRC_RENDERER = "$(SRC)/Renderer/*.cpp" "$(SRC)/Renderer/*.hpp"
SRC_LBLIB = "$(SRC)/LadybugLib/*.hpp" "$(SRC)/LadybugLib/*.cpp"
SRC_ALL = $(SRC_BASE) $(SRC_RENDERER) $(SRC_LBLIB)

GAME_EXPORT = -EXPORT:Game_UpdateAndRender

SHADERS = build/blit.vs build/blit.fs build/shader.vs build/shader.fs build/prepass.vs build/prepass.fs \
    build/shadow.vs build/shadow.fs build/sky.vs build/sky.fs build/ui.vs build/ui.fs \
    build/gizmo.vs build/gizmo.fs \
    build/downsample_bloom.cs build/upsample_bloom.cs build/ssao.cs build/ssao_blur.cs \
    build/fx.vs build/fx.fs \
    build/quad.vs build/quad.fs \
    build/skin.cs \
    build/skinned.vs build/skinned.fs

all: "$(OUT)/Win_LadybugEngine.exe" "$(OUT)/game.dll" "$(OUT)/renderer.obj" $(SHADERS)

clean: 
    @del /q build\*.*

"$(OUT)/renderer.obj": $(SRC_LBLIB) $(SRC_RENDERER)
    @clang-cl $(CXX_FLAGS) -c "src/Renderer/Renderer.cpp" -Fo$@ -Fd"$(OUT)/"
"$(OUT)/Win_LadybugEngine.exe": $(SRC_ALL)
    @clang-cl $(CXX_FLAGS) "src/Win_LadybugEngine.cpp" -Fe$@ -Fo"$(OUT)/" -Fd"$(OUT)/" $(LIBS) -link $(LINK_FLAGS)
"$(OUT)/game.dll": $(SRC_ALL) "$(OUT)/renderer.obj"
    @clang-cl $(CXX_FLAGS) "src/LadybugEngine.cpp" -Fo"$(OUT)/" -Fd"$(OUT)/" $(LIBS) "$(OUT)/renderer.obj" -link -DLL -OUT:$@ $(LINK_FLAGS) $(GAME_EXPORT)

{$(SRC)/Shader/}.glsl{$(OUT)/}.vs:
    glslc $(SHADER_FLAGS) -DVS -fshader-stage=vert -o "$(OUT)/$(@B).vs" "$<"

{$(SRC)/Shader/}.glsl{$(OUT)/}.fs:
    glslc $(SHADER_FLAGS) -DFS -fshader-stage=frag -o "$(OUT)/$(@B).fs" "$<"

{$(SRC)/Shader/}.comp{$(OUT)/}.cs:
    glslc $(SHADER_FLAGS) -DCS -fshader-stage=comp -o "$(OUT)/$(@B).cs" "$<"