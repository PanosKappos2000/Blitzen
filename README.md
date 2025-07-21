# Blitzen Engine

Blitzen is a purpose-built game engine designed to explore and understand the full scope of building large-scale, interactive 3D worlds from the ground up. Even thought its primary goal is deep understanding of video games / interactive 3D graphics, it's written with the dream that it will one day create its own large-scale, interactive 3D world.

This engine is not a framework, a toolkit, or a plugin. It is an ongoing, low-level effort to write every core system—from rendering and physics to world simulation and entity logic—from scratch. The goal is not only performance, but deep technical ownership and clarity over how a modern game engine works behind the scenes.

It is inspired by real-world game engines that favor minimalism, explicit control, and full-system integration over convenience or feature bloat. This is not because engines that do this are any worse, but because it is actually much harder to maintain. 

Dependencies are avoided unless absolutely necessary, and when they are used, they are meant to be replaced with Blitzen's own implementation later, with a few exceptions.



# Platform Layer

Blitzen is designed with multi-platform support in mind, with active development and testing on Windows and Linux.

    On Windows, Blitzen defaults to Direct3D 12 for rendering, unless explicitly configured to use another backend.

    On Linux, Vulkan is the primary rendering API and the basis for future cross-platform consistency.

At present, Linux support is temporarily out of order. Development continues on Windows as the primary platform.

Both platforms have been tested on a NVIDIA RTX 3060, with Windows consistently outperforming Linux, even with both using Vulkan. In complex, stress-tested scenarios, the Windows build delivers significantly higher performance—highlighting the maturity and optimization advantages of the Windows graphics stack, even outside of Direct3D.



# Rendering and Graphics APIs

Blitzen uses a compile-time abstraction layer to support multiple graphics APIs. Currently, Direct3D 12 is the only backend fully compatible with Blitzen's increasingly modular renderer architecture.

    Vulkan support exists but is undergoing a temporary hiatus. It will be revived once Blitzen reaches a more complete feature set.

    OpenGL was previously supported but is now abandoned due to its limitations in GPU-driven rendering and general architectural constraints. Its revival is unlikely unless a compelling reason presents itself.

Blitzen’s renderer is built with GPU-driven rendering at its core. It uses compute shaders extensively to perform simulation and culling, followed by a small number of indirect draw calls to feed the GPU its final workload. This model helps reduce CPU overhead and allows scalability to very large scenes.

While mesh shaders are not currently in the roadmap, Blitzen includes a cluster-based rendering mode (still in development) that leverages compute and traditional vertex shaders to approximate similar benefits. The renderer is also tightly integrated with other subsystems such as collision resolution, which is planned to have a GPU-accelerated mode for experimentation and performance comparison against CPU-based solutions.

At this stage, the renderer prioritizes performance and scalability over visual fidelity. Support for advanced graphical features and visual effects will come later—once the foundational systems and real-time performance are robust enough to justify their inclusion.



# Resident System (Blitzen's attempt at ECS)

Blitzen features a custom Resident System, its own attempt at an Entity Component System (ECS). Rather than following a traditional object-oriented or inheritance-based model, Blitzen uses a Structure of Arrays (SOA) approach. This design is aimed at GPU compatibility and cache efficiency, making it well-suited for large-scale, real-time simulations.

In this model:

    A Resident is simply an index that can reference one or more SOA arrays containing world data—such as transforms, velocities, or colliders.

    There are no fully encapsulated entities. Instead, each system operates directly on the data arrays, and residents selectively populate relevant components.

    Not all residents have entries in every array. It’s up to the logic systems to determine which data is relevant to which resident.

This system is minimal and flexible, but currently lacks strong encapsulation and safety mechanisms typically found in mature ECS frameworks. 

Blitzen is also developing a lightweight scripting interface that leverages this system. Scripts receive a resident ID and operate on the corresponding SOA data using specialized access functions. This bridges low-level performance with high-level logic expression and will continue to evolve as the engine matures.



# Alba Stella – Collision Engine

Alba Stella is Blitzen's custom-built collision engine, developed entirely from scratch with a focus on tight, optimized game interactions. It is currently under active development and designed to integrate deeply with the resident system and GPU-driven architecture.

The collision system begins with a broad phase, where residents are spatially sorted using their positions on the X and Z axes. These positions are mapped into grid cells:

    Static residents are placed into a cell during initialization and are not checked again.

    Dynamic residents (referred to as World Variables) are updated every frame, allowing for efficient movement tracking.

With this structure, final collision checks are only performed between residents that occupy the same spatial cell—dramatically reducing the number of comparisons and enabling large-scale interaction at high speed.

Collider data structures are built with SIMD and compute shader execution in mind. Whether the CPU uses compiler intrinsics or the GPU handles it through parallel dispatch, Alba Stella is designed for high performance at all layers.

Though still incomplete, the system already demonstrates strong early results, accurately detecting collisions 5000 dynamic entities. A wide range of improvements and features are planned, including tighter GPU integration, advanced resolution techniques, and broader support for collision-based gameplay logic.



# Dasher – Blitzen’s Editor

Dasher is Blitzen’s in-engine editor, currently in an early and non-functional state. It relies on Vulkan for rendering, which is temporarily out of service, and uses Dear ImGui for its UI layer (planning to replace with fully custom lib). While functional in the past, it is no longer maintained in its current form.

Despite its current state, Dasher is a crucial part of Blitzen’s future. An interactive editor is essential for real-time world building and debugging, and there are concrete plans to revive and replace the current system with a more permanent solution. The long-term goal is to transition Blitzen away from being purely a console-driven application into a full visual development environment.



# Resource Loading

Blitzen supports GLTF and OBJ meshes, along with DDS textures. Mesh loading is currently handled through external libraries, while DDS texture reading is implemented manually due to its simpler format. These mesh loading libraries are among the few dependencies in Blitzen that are unlikely to ever be replaced with custom solutions, due to their complexity and reliability.

At present, Blitzen reloads all external resources on each engine startup. However, a custom binary resource format is under development to dramatically improve load times and enable powerful features such as:

    Faster boot through preprocessed data

    World partitioning via streaming

    Support for multiple map regions

    Resource baking on first load

Special thanks to the meshoptimizer library and Arseny Kapoulkine, whose open source work has been instrumental to Blitzen’s rendering performance. Meshoptimizer is used for LOD generation and vertex cache optimization, and is also serving as the blueprint for a future in-house resource optimizer currently in development. Thank you, legend.



# Future Systems

Several important modules are planned for future development or are in such early stages that their presence is currently hidden within internal code. 

Dasher – Engine Editor: 
A visual editor is essential for large-scale game development. 

Rapid File (Name subject to change) – Custom Binary Resource Format:
This system will handle loading and streaming of binary-formatted resources. 

Bilbo – Custom Build System

A planned custom build system, likely written in Python. Not in development. Blitzen currently uses CMake.

Blitzen Generator – Mesh Rendering Optimization Library:
This in-house library aims to replace the external meshoptimizer tool. It will handle LOD generation, vertex/index reordering for cache efficiency, and GPU-driven rendering data prep. It’s a long-term project to internalize more of the performance-critical mesh pipeline.

Unnamed Low-Level Audio Engine:
A minimal, dependency-light sound system is planned to handle 3D spatial audio. Not in development. Blitzen is currently mute.

Magnificent Father – Lighting Engine:
This name might be given to directional lighting. Hardcoded directional lighting is the only thing that exists right now.

Job System – Low-Level Threading and Task Scheduling:
Blitzen plans to include a low-level job system to maximize multithreading efficiency. 



# Other Tools

Blitzen also includes several internal utility libraries developed to support engine-wide needs while avoiding external dependencies. Notable examples: 

BlitCL – Container Library

BlitCL is a lightweight alternative to the STL, developed to provide dynamic arrays, auto-cleanup pointers (not quite smart), and other basic containers. It played a large role in Blitzen’s early systems and is still used in offline contexts today. Runtime use is limited to avoid unnecessary kernel calls and maximize performance. While functional, it remains an experimental and evolving tool.

BlitML – Math Library
BlitML is Blitzen’s custom math library. It focuses on compatibility with GPU-side representations and is gradually being upgraded to support SIMD operations. Its integration with the renderer and simulation systems makes it a central—but intentionally minimal—piece of the engine.



# Acknowledgments

Blitzen would not be possible without the following open source projects and the people behind them. Deep gratitude to the developers who share their tools and knowledge freely with the community.

    cgltf {https://github.com/jkuhlmann/cgltf}
    A single-file GLTF 2.0 loader by Johannes Kuhlmann. Lightweight, dependency-free, and a great fit for Blitzen’s custom resource handling.

    fast_obj {https://github.com/thisistherk/fast_obj}
    A simple and fast Wavefront OBJ loader. Ideal for lightweight mesh import and fast integration.

    Dear ImGui {https://github.com/ocornut/imgui}
    An immediate-mode GUI toolkit by Omar Cornut. Currently used in Blitzen’s editor for debugging and visualization.

    meshoptimizer {https://github.com/zeux/meshoptimizer}
    A mesh processing and LOD generation library by Arseny Kapoulkine. Blitzen uses this to improve GPU vertex cache performance and LOD generation for loaded meshes.

# Special Thanks

A personal thanks to Arseny Kapoulkine for making performance-focused graphics accessible. His work on meshoptimizer, alongside his blog posts and open source efforts, had a significant impact on Blitzen’s rendering direction and its aspirations for GPU-driven design.