OUT = build
SRC = src

.SUFFIXES : .glsl .comp

COMMON = -nologo -std:c++20 -Zi -EHsc -MD -arch:AVX2
FLOAT_ENV = -fp:except- -fp:strict
OPTIMIZATION = -Od -Oi
INCLUDES = -I$(SRC) -I$(VULKAN_SDK)/Include/
DEFINES = -DDEVELOPER=1 -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX
WARNINGS = -WX -W4 -wd4100 -wd4189 -wd4200 -wd4201 -wd4505

CXX_FLAGS = $(COMMON) $(FLOAT_ENV) $(INCLUDES) $(DEFINES) $(WARNINGS)

LINK_FLAGS = -LIBPATH:"$(VULKAN_SDK)/Lib/" -LIBPATH:lib/

LIBS = kernel32.lib user32.lib gdi32.lib Shell32.lib advapi32.lib vulkan-1.lib nvtt30106.lib

SRC_BASE = "$(SRC)/*.cpp" "$(SRC)/*.hpp"
SRC_RENDERER = "$(SRC)/Renderer/*.cpp" "$(SRC)/Renderer/*.hpp"
SRC_LBLIB = "$(SRC)/LadybugLib/*.hpp" "$(SRC)/LadybugLib/*.cpp"
SRC_ALL = $(SRC_BASE) $(SRC_RENDERER) $(SRC_LBLIB)

GAME_EXPORT = -EXPORT:Game_UpdateAndRender

SHADERS = build/blit.vs build/blit.fs build/shader.vs build/shader.fs build/prepass.vs build/prepass.fs \
    build/shadow.vs build/shadow.fs build/sky.vs build/sky.fs build/ui.vs build/ui.fs \
    build/gizmo.vs build/gizmo.fs \
    build/downsample_bloom.cs build/upsample_bloom.cs build/ssao.cs build/ssao_blur.cs

all: "$(OUT)/Win_LadybugEngine.exe" "$(OUT)/game.dll" $(SHADERS)

clean: 
    @del /q build\*.*

"$(OUT)/Win_LadybugEngine.exe": $(SRC_ALL)
    @cl $(CXX_FLAGS) "src/Win_LadybugEngine.cpp" -Fe: "$@" -Fo: "$(OUT)/" -Fd: "$(OUT)/" $(LIBS) -link $(LINK_FLAGS)
"$(OUT)/game.dll": $(SRC_ALL)
    @cl $(CXX_FLAGS) "src/LadybugEngine.cpp" -Fo: "$(OUT)/" -Fd: "$(OUT)/" $(LIBS) -link -DLL -OUT:$@ $(LINK_FLAGS) $(GAME_EXPORT)

{$(SRC)/Shader/}.glsl{$(OUT)/}.vs:
    glslc --target-env=vulkan1.3 -DVS -fshader-stage=vert -o "$(OUT)/$(@B).vs" "$<"

{$(SRC)/Shader/}.glsl{$(OUT)/}.fs:
    glslc --target-env=vulkan1.3 -DFS -fshader-stage=frag -o "$(OUT)/$(@B).fs" "$<"

{$(SRC)/Shader/}.comp{$(OUT)/}.cs:
    glslc --target-env=vulkan1.3 -DCS -fshader-stage=comp -o "$(OUT)/$(@B).cs" "$<"