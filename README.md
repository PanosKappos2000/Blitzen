# BlitzenEngine0

Game engine focused on the rendering aspect and implementing acceleration algorithms performed on the GPU.

Vulkan is the initial targeted graphics API but OpenGL has been implemented as well. For now it does not support occlusion culling or textures but it is as performant as Vulkan.

Capable of loading .obj models and .gltf scenes with some constraints. 

Uses compute shaders to perform frustum and occlusion culling on each surface. It also loads LODs automatically for each surface loaded. With all of these active it is capable of handling scenes with millions of objects (tested on an RTX3060).

It implements a very basic, flawed camera system that moves with the mouse and WASD keys to fly around the scene.

The engine only supports DDS textures. Textures with unsupported formats can be converted offline by using something like NVIDIA's texture tools.

It has not been tested for a variety of hardware. For example, it is uncertain if it works on any type of AMD GPU

It supports Windows and Linux but it has been a few weeks since it was last tested on linux. It is possible that the addition of OpenGL has broken the UNIX build.
