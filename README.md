# BlitzenEngine0

Game engine focused on the rendering aspect and on implementing acceleration algorithms that are calculated on the GPU.

Vulkan is the initial targeted graphics API but OpenGL has been implemented as well. For now OpenGL does not support occlusion culling or textures but it is as performant as Vulkan.

Capable of loading .obj models and .gltf scenes (with some constraints):

-To load obj models it uses the fast_obj library from thisistherk : https://github.com/thisistherk/fast_obj

-To load gltf scenes it uses the surprisingly intuitive cgltf library : https://github.com/jkuhlmann/cgltf

-For the vertices that are loaded, the incredible meshoptimizer library is used to generate indices, LODs, clusters and optimize the vertex cache. Meshoptimizer is written by Arseny Kapoulkine (zeux on github) https://github.com/zeux/meshoptimizer. 

Uses compute shaders to perform frustum and occlusion culling on each surface. It also loads LODs automatically for each surface loaded. With all of these active it is capable of handling scenes with millions of objects (tested on an RTX3060).

It implements a very basic, flawed camera system that moves with the mouse and WASD keys to fly around the scene.

The engine only supports DDS textures. Textures with unsupported formats can be converted offline by using something like NVIDIA's texture tools. They will otherwise fail to load.

It has only been tested on Nvidia hardware. It is uncertain, for example, if it works on any type of AMD GPU.

It supports Windows and Linux but the Linux build has not been tested as heavily as the Windows build and can only use Vulkan, not OpenGL.

The project can be built and compiled with CMake. Starting the Engine without command line arguments, simply opens a default, colored window. It can also take command line arguments -which should be gltf filepaths with DDS textures- in which case it will draw the gltf scene found in the filepath. There is also a special command line argument "RenderingStressTest", which will load a scene with millions of objects in "random" positions. This serves to test the acceleration algorithms mentioned earlier. Some machines might not be able to handle this, as the renderer will load 4'500'000 objects when the BLITZEN_RENDERING_STRESS_TEST preprocessor macro is defined in CMakeLists.txt (defined by default).
