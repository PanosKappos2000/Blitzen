# BlitzenEngine0

Game engine focused on the rendering aspect and implementing acceleration algorithms performed on the GPU.

Vulkan is the only supported graphics API for now.

Capable of loading .obj models and .gltf scenes with some constraints. 

Uses compute shaders to perform frustum and occlusion culling on each surface. It also loads LODs automatically for each surface loaded. With all of these active it is capable of handling scenes with millions of objects (tested on an RTX3060).

It uses a very basic, default camera that moves with the mouse and WASD keys to fly around the scene.

The engine loads textures from .gtlf files but they have to be DDS textures. Textures with unsupported formats can be converted offline by using something like NVIDIA's texture tools.

The engine has not been tested for a variety of hardware. For example, it is uncertain if it works on any type of AMD GPU

The application supports Windows and Linux but it has been a few weeks since it was last tested on linux.
