# BlitzenEngine0

Game engine focused on the rendering aspect and on implementing acceleration algorithms that are calculated on the GPU.

Vulkan is the initial targeted graphics API but OpenGL has been implemented as well. For now OpenGL does not support occlusion culling or textures but it is as performant as Vulkan.

Capable of loading .obj models and .gltf scenes (with some constraints):

-To load obj models it uses the fast_obj library from thisistherk : https://github.com/thisistherk/fast_obj

-To load gltf scenes it uses the surprisingly intuitive cgltf library : https://github.com/jkuhlmann/cgltf

-For the vertices that are loaded, the incredible meshoptimizer library is used to generate indices, LODs, clusters and optimize the vertex cache. Meshoptimizer is written by Arseny Kapoulkine (zeux on github) https://github.com/zeux/meshoptimizer, whose Niagara streams on Youtube have massively influenced the renderer of this Engine (https://www.youtube.com/watch?v=BR2my8OE1Sc&list=PL0JVLUVCkk-l7CWCn3-cdftR0oajugYvd). 

Uses compute shaders to perform frustum and occlusion culling on each surface. It also loads LODs automatically for each surface loaded. With all of these active it is capable of handling scenes with millions of objects (tested on an RTX3060).

It implements a very basic, flawed camera system that moves with the mouse and WASD keys to fly around the scene.

The engine only supports DDS textures. Textures with unsupported formats can be converted offline by using something like NVIDIA's texture tools. They will otherwise fail to load.

It has only been tested on Nvidia hardware. It is uncertain, for example, if it works on any type of AMD GPU.

It supports Windows and Linux but the Linux build has not been tested as heavily as the Windows build and can only use Vulkan, not OpenGL.

The project can be built and compiled with CMake and run without any command line arguments. In its current state it will load a default scene with meshes and one gltf scene(with textures but no lighting). The Engine can load more files than the default but because it does not have an editor, the filepaths need to be specified in the source code. It also does not have a custom format for its resources yet, so loading takes time and will be unbearably long if has to load too many vertices.
