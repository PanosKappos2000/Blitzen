# BlitzenEngine0.C
Focused on 3D graphics real time rendering and using GPU-driven techniques to render large scenes with good performance. 
Only uses Vulkan for now.

It is now capable of rendering over 1 million high poly meshes (tested with 1'400'000 stanford bunnies and 100'000 kitten meshes).
It manages to do so by performing frustum culling, using LODs and (to a less significant extent) occlusion culling. All done in compute shaders.
Frustum culling and LODs are crucial for performance. Switching any of them off tanks performance significantly.
Occlusion culling is not as functional and gives very little gain. Because of this I have to cheat and allow frustum culling to have very low draw distance, reducing objects on the z axis.

*It has now been tested with even 4'000'000 objects but there are memory allocation validation errors for Vulkan.

The engine has a default camera than can move and rotate around the scene. It has a problem where, when the frame rate approaches 1000 fps, the rotation slows to a crawl becuase of delta time. This can be fixed with VSync.

The engine has been tested on an rtx3060 for both windows and linux. 
It also succesfully ran on an old Windows laptop of unknown specification. The performance was obviously worse but bearable despite the old hardware and how demanding the default scene is on the GPU.
Beyond that, it has not been tested on a variety of hardware. For example, I have no idea if it works on AMD GPUs.
The device that runs Blitzen will also need Vulkan drivers as the renderer only supports Vulkan at the moment.
