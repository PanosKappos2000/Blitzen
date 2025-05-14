# Blitzen

Minimal Game Engine.

Vulkan renderer utilizing GPU driven techniques, to cull away objects with compute shaders. Capable of draw frustum and occlusion culling + LOD selection. Includes cluster mode(not mesh shaders) but it is still in development. Supports Window and Linux, but it is more performant on Windows, where it has shown the capability of processing 4'000'000+ objects at the same time with 60+ fps on an NVidia 3060. In terms of graphics effects, it uses bindless textures with directional light for now. Used to support very basic RayTracing but the mode is suspended for now.

Direct2D 12 renderer similar to Vulkan. Does not do occlusion culling yet. Ignores transparent objects for now. Better support for double buffering. Additional Support for indirect instancing. Only works on Windows. Draw occlusion culling and cluster culling are in development. Very similar capabilities to the Vulkan renderer for the most part

Support for OpenGL exists but it's suspended for now.

Vulkan and D3D12 have on only been tested on NVidia hardware.

The "frontend" of the renderer can load Gltf scenes and Obj models and saves their data in a format that is mostly shared between the backend renderers.
-To load obj models it uses the fast_obj library from thisistherk : https://github.com/thisistherk/fast_obj
-To load gltf scenes it uses the surprisingly intuitive cgltf library : https://github.com/jkuhlmann/cgltf
-For the vertices that are loaded, the incredible meshoptimizer library is used to generate indices, LODs, clusters and optimize the vertex cache. Meshoptimizer is written by Arseny Kapoulkine (zeux on github) https://github.com/zeux/meshoptimizer. 
-Only supports DDS textures for now. But other formats can be converted offline. The gltf loader will automatically replace the extension with .dds. Other formats will fail to load.

Aside from the renderer, the engine includes a minimal camera for testing, event and input systems in case more interaction needs to be added, dynamic update objects (tested with 1'000 rotating meshes), its own minimal container library (mostly for academic reasons, but the additional control is also nice) and its own math library (though it does not do anything special, like SIMD...yet).

The project can be built and compiled with CMake(hopefully...). Command line arguments can be gltf filepath or special mode like "RenderingStressTest" and "InstancingStressTest" (better to have a decent GPU for those modes though). It support windows and linux, but Linux may stay behind sometimes when making changes to the platform layer.
