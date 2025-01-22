# BlitzenEngine0
Focused on 3D graphics real time rendering and using GPU-driven techniques to render large scenes with good performance. 
Only uses Vulkan for now.

Capable of loading .obj models and .gltf scenes with some constraints. Uses compute shaders to perform frustum and occlusion culling on each surface. 
It also loads LODs automatically for each surface loaded, but their implementation is faulty on large objects. With all of these active it is capable of handling scenes with millions of objects (tested on an RTX3060).

It uses a very basic, default camera that moves with the mouse and WASD keys to fly around the scene.

The engine has been tested on an rtx3060 for both windows and linux. 
It also succesfully ran on an old Windows laptop of unknown specification. The performance was obviously worse but bearable despite the old hardware and how demanding the default scene is on the GPU.
Beyond that, it has not been tested on a variety of hardware. For example, I have no idea if it works on AMD GPUs.
The device that runs Blitzen will also need Vulkan drivers as the renderer only supports Vulkan at the moment.
