# LadybugEngine
Vulkan-based renderer for x64 Windows.

## Requirements
- Windows operating system on x64 architecture.
- [Microsoft Visual Studio](https://visualstudio.microsoft.com/downloads/) installation including Desktop development with C++:
    - Windows SDK
    - C++ Clang tools for Windows x64/x86
- [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) with `VULKAN_SDK` environment variable set and the SDK's `Bin` directory in `PATH`
## Building
- Open the x64 Developer Command Prompt for Visual Studio.
- Navigate to the repository's directory.
- Run `nmake`
## Running
Run `build\Win_LadybugEngine.exe` from the _root directory of the repository_ (i.e. _not_ the build directory).

## Project structure
The program is divided into subsystems, each of which uses the STUB (single translation unit build) compilation model. These are as follows:
- Windows platform layer (.exe): 
    - Contains the entry-point of the program
    - Handles window creation
    - Listens to input and passes it to the application
    - Provides platform specific functionality (memory, file IO, etc.)
- Application/Game (.dll):
    - Contains the logic of the program in a platform-independent manner.
- Renderer (.dll/.obj):
    - Contains graphics API specific code.

A few select header files are used as interfaces between the subsystems:
- `src/Platform.hpp` contains the interface between the platform layer and the application code. Also available to the renderer.
- `src/Renderer/Renderer.hpp` contains the interface between the application and renderer code.

The folder `src/LadybugLib/` contains utilities available to all subsystems.

## Renderer features
- Forward+ pipeline with 2.5D tiled light binning
- High-dynamic range rendering
- Physically-based shading model
- Cascaded shadow maps
- Volumetric directional light
- Mesh skinning and animation
- Additive particle effects (w/ multiple billboard modes)
- Screen-space ambient occlusion
- Bloom post-process effect
- Tonemapping
- Immediate-mode 2D rendering

## Renderer architecture
While the renderer is based on Vulkan, it does not leak Vulkan-specific information into the rest of the application code. Instead it's divided into a frontend (which is the interface between the app and the renderer) and a backend (which performs the rendering and makes the actual Vulkan calls)

### Frontend
The renderer provides multiple "stacks" or "buckets" as its interface. These buckets contain lightweight render commands that describe the contents of the game world to the renderer.

The commands in each bucket are executed in the order they were submitted, but the buckets themselves are executed in a fixed order by the renderer, defined by the functionality that the specific bucket implements.

Helper functions are provided by the front-end that aid in filling and bounds-checking the buckets.

### Backend
While some of the buckets in the frontend live purely on the CPU-side, some are mapped directly to BAR or shared memory, and as such don't need to be explicitly transferred on the GPU's timeline.
With that in mind, the GPU timeline looks as follows:
1. Upload light source data to the GPU (performing coarse frustum light culling on the CPU)
2. Perform mesh skinning and write the results into a dedicated vertex buffer.
3. Render the depth buffer and a specialized "structure buffer" which contains the camera-space depth and its screen-space x and y derivatives.
4. Render SSAO buffer
5. Blur SSAO buffer
6. Bin light sources into screen-space tiles
7. Render shadow maps
8. Render the scene with lighting (including the volumetric sun-light) 
9. Render sky
10. Render particles
11. Perform post-process bloom effect
12. Blend the bloom into the final image, perform tonemapping, and draw the result to the swapchain
13. Draw 3D UI elements on the swapchain
14. Draw 2D UI elements on the swapchain
15. Present