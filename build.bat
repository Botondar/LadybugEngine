@echo off

IF NOT EXIST build ( mkdir build )

set CompilerOptions=-nologo -std:c++20 -Zi -EHsc -MD  -fp:except- -fp:contract -fp:precise
set CompiletOptions=-arch:AVX2 -O2 -Oi %CompilerOptions%
set CompilerOptions=-I"src/" -I%VULKAN_SDK%\Include\ %CompilerOptions%
set CompilerOptions=-DWIN32_LEAN_AND_MEAN -DDEVELOPER=1 -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX %CompilerOptions%
set CompilerOptions=-WX -W4 -wd4100 -wd4189 -wd4200 -wd4201 -wd4505 %CompilerOptions%
set CompilerOptions=%CompilerOptions%
set Libraries=kernel32.lib user32.lib gdi32.lib Shell32.lib advapi32.lib vulkan-1.lib nvtt30106.lib
set LinkerOptions=/link /LIBPATH:%VULKAN_SDK%\Lib\ /LIBPATH:lib/

cl %CompilerOptions% "src/Win_LadybugEngine.cpp" -Fe"build/Win_LadybugEngine.exe" %Libraries% %LinkerOptions%

build-shaders