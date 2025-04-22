# BlitzenEngine0

Minimal, console Game Engine.

Its most important aspect is its Vulkan Renderer, which uses GPU draw culling with compute shaders, that set the draw commands for actual rendering. Cluster culling without mesh shaders is also being developed. Currently, it can generate per cluster commands, after doing draw culling, but it does not cull the clusters yet. Ray tracing is also a secondary feature of the renderer. The renderer can accept loaded resources, that are requested from the command line arguments(gltf scene filepaths or some mode strings that load other tests). It has shown the capability of processing 4'000'000+ objects at the same time with 60+ fps on an NVidia 3060. Important to note is that it does not graphics effects like shadow maps and the fragment shader just uses directional light and textures. It has only been tested on Nvidia hardware. It is uncertain, for example, if it would work on an AMD GPU.

-To load obj models it uses the fast_obj library from thisistherk : https://github.com/thisistherk/fast_obj

-To load gltf scenes it uses the surprisingly intuitive cgltf library : https://github.com/jkuhlmann/cgltf

-For the vertices that are loaded, the incredible meshoptimizer library is used to generate indices, LODs, clusters and optimize the vertex cache. Meshoptimizer is written by Arseny Kapoulkine (zeux on github) https://github.com/zeux/meshoptimizer. 

The engine only supports DDS textures. Textures with unsupported formats can be converted offline by using something like NVIDIA's texture tools. They will otherwise fail to load.

Other systems include a minimal camera system, dynamic objects (but not 4'000'000 of them), an event system and its own container library. Usage of Stl is minimal throughout the project.

It supports windows and linux, but development is done on a windows environment and linux stays behind sometimes.

The project can be built and compiled with CMake. Starting the Engine without command line arguments, simply opens a default, colored window. It can also take command line arguments -which should be gltf filepaths with DDS textures- in which case it will draw the gltf scene found in the filepath. There is also a special command line argument "RenderingStressTest", which will load a scene with millions of objects in "random" positions. This serves to test the acceleration algorithms mentioned earlier. Some machines might not be able to handle this, as the renderer will load 4'500'000 objects when the BLITZEN_RENDERING_STRESS_TEST preprocessor macro is defined in CMakeLists.txt (defined by default).
