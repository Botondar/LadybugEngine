OUT = build
SRC = src

.SUFFIXES : .glsl .comp

COMMON = -nologo -std:c++20 -Zi -EHsc -MT -arch:AVX2
FLOAT_ENV = -fp:except- -fp:strict
OPTIMIZATION = -O2 -Oi
VULKAN_INCLUDE = -I$(VULKAN_SDK)/Include/
INCLUDES = -I$(SRC)
DEFINES = -DDEVELOPER=1 -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX
WARNINGS_MSVC = -WX -W4 -wd4100 -wd4189 -wd4200 -wd4201 -wd4505
WARNINGS_CLANG = -Wshadow -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-lambda-capture -Wno-unused-value -Wno-missing-field-initializers -Wno-char-subscripts -Wno-missing-braces -Wno-c99-designator
WARNINGS = $(WARNINGS_MSVC) $(WARNINGS_CLANG)

TRANSLATION_UNITS = -DLB_TranslationUnitCount=3 \
    -DLB_TranslationUnit_PlatformLayer=0 \
    -DLB_TranslationUnit_Game=1 \
    -DLB_TranslationUnit_Renderer=2

CXX_FLAGS = $(COMMON) $(FLOAT_ENV) $(INCLUDES) $(DEFINES) $(WARNINGS) $(TRANSLATION_UNITS)
LINK_FLAGS = -INCREMENTAL:NO

SHADER_FLAGS = -g -O --target-env=vulkan1.3 -std=460core -fhlsl-offsets

LIBS = kernel32.lib user32.lib gdi32.lib Shell32.lib advapi32.lib Ole32.lib Winmm.lib

SRC_BASE = "$(SRC)/*.cpp" "$(SRC)/*.hpp"
SRC_RENDERER = "$(SRC)/Renderer/*.cpp" "$(SRC)/Renderer/*.hpp"
SRC_LBLIB = "$(SRC)/LadybugLib/*.hpp" "$(SRC)/LadybugLib/*.cpp"
SRC_ALL = $(SRC_BASE) $(SRC_RENDERER) $(SRC_LBLIB) "$(SRC)/Shader/ShaderInterop.h"

GAME_EXPORT = -EXPORT:Game_UpdateAndRender
RENDERER_EXPORT = \
    -EXPORT:CreateRenderer \
    -EXPORT:AllocateGeometry \
    -EXPORT:AllocateTexture \
    -EXPORT:BeginRenderFrame \
    -EXPORT:EndRenderFrame

TOOLS = "$(OUT)/lbmeta.exe"

SHADERS = \
    build/blit.vs build/blit.fs \
    build/shading_forward.vs build/shading_forward.fs \
    build/shading_transparent.vs build/shading_transparent.fs \
    build/prepass.vs build/prepass.fs \
    build/prepass_alphatest.vs build/prepass_alphatest.fs \
    build/shadow.vs build/shadow.fs \
    build/shadow_alphatest.vs build/shadow_alphatest.fs \
    build/sky.vs build/sky.fs \
    build/ui.vs build/ui.fs \
    build/gizmo.vs build/gizmo.fs \
    build/bloom_downsample.cs build/bloom_upsample.cs \
    build/ssao.cs build/ssao_blur.cs \
    build/quad.vs build/quad.fs \
    build/skin.cs \
    build/light_bin.cs \
    build/shading_visibility.cs \
    build/sky.cs \
    build/environment_brdf.cs

all: "$(OUT)/" "$(OUT)/Win_LadybugEngine.exe" "$(OUT)/game.dll" "$(OUT)/vulkan_renderer.dll" $(SHADERS) $(TOOLS)

clean: 
    @del /q $(OUT)\*.*

"$(OUT)/":
	@if not exist $@ mkdir $@

"$(OUT)/lbmeta.exe": "$(OUT)/" $(SRC_LBLIB) "tools/lbmeta/*"
    @clang-cl -Wno-c99-designator -std:c++20 -Zi -I"src/" -D_CRT_SECURE_NO_WARNINGS "tools/lbmeta/lbmeta.cpp" -Fe$@ -Fo"$(OUT)/" -Fd"$(OUT)/"

"$(OUT)/vulkan_renderer.dll": "$(OUT)/" $(SRC_LBLIB) $(SRC_RENDERER)
    @clang-cl $(CXX_FLAGS) $(VULKAN_INCLUDE) -DLB_TranslationUnit=LB_TranslationUnit_Renderer  "src/Renderer/VulkanRenderer.cpp" -Fo"$(OUT)/" -Fd"$(OUT)/" vulkan-1.lib -link -DLL -OUT:$@ $(LINK_FLAGS) $(RENDERER_EXPORT) -LIBPATH:$(VULKAN_SDK)/Lib/
"$(OUT)/Win_LadybugEngine.exe": "$(OUT)/" $(SRC_ALL)
    @clang-cl $(CXX_FLAGS) $(VULKAN_INCLUDE) -DLB_TranslationUnit=LB_TranslationUnit_PlatformLayer "src/Win_LadybugEngine.cpp" -Fe$@ -Fo"$(OUT)/" -Fd"$(OUT)/" $(LIBS) vulkan-1.lib -link $(LINK_FLAGS) -LIBPATH:$(VULKAN_SDK)/Lib/
"$(OUT)/game.dll":"$(OUT)/" $(SRC_ALL)
    @clang-cl $(CXX_FLAGS) -DLB_TranslationUnit=LB_TranslationUnit_Game "src/LadybugEngine.cpp" -Fo"$(OUT)/" -Fd"$(OUT)/" $(LIBS) -link -DLL -OUT:$@ $(LINK_FLAGS) $(GAME_EXPORT)

{$(SRC)/Shader/}.glsl{$(OUT)/}.vs:
    glslc $(SHADER_FLAGS) -DVS -fshader-stage=vert -o "$(OUT)/$(@B).vs" "$<"

{$(SRC)/Shader/}.glsl{$(OUT)/}.fs:
    glslc $(SHADER_FLAGS) -DFS -fshader-stage=frag -o "$(OUT)/$(@B).fs" "$<"

{$(SRC)/Shader/}.comp{$(OUT)/}.cs:
    glslc $(SHADER_FLAGS) -DCS -fshader-stage=comp -o "$(OUT)/$(@B).cs" "$<"