glTF Import Pipeline (Geometry + Textures)

Source files:
	•	Renderer/Scene/gltfScene.h
	•	Renderer/Scene/gltfScene.cpp

Overview

Blitzen’s glTF importer treats the glTF as a scene bundle: geometry, materials, and textures are translated into Blitzen’s resource layout under a per-scene folder (e.g., WorldResources/sponza/). Textures are expected in DDS format for fast runtime upload; the importer does not convert images at import time.

Texture handling (current behavior)
	1.	URI rewrite to .dds
For each cgltf_texture, an absolute/relative path is built from the glTF’s directory and replace the original extension with .dds.
	•	This means all textures must be pre-converted outside the engine (e.g., with NVIDIA Texture Tools, texconv, etc.).
	•	If a matching .dds file does not exist, the import fails early.
	2.	Copy into WorldResources
Resolved .dds files are copied to the project’s resource bucket under a scene-named subfolder, e.g.:
WorldResources/
  sponza/
    texture0.dds
    texture1.dds
    texture2.dds
    ...
The importer assigns stable, sequential names (texture0.dds, texture1.dds, …) in the order discovered during import.
This makes it easier for texture names to be saved when adding the scene to a map.

3.	Persist texture count
The total number of textures discovered/copied for the scene is written to the nodes binary file alongside other scene metadata. Downstream loaders can use this count to:
	•	Preallocate texture descriptors/handles,
	•	Validate material indices,
	•	Resolve texture indices to textureN.dds filenames within the scene folder.