#pragma once
#include "Renderer/BlitzenVulkan/Context/vulkanRenderer.h"
#include "Renderer/BlitzenGL/openglRenderer.h"
#include "Renderer/BlitzenDX12/Context/dx12Renderer.h"
#include "BlitCL/blitSmartPointer.h"

namespace BlitzenEngine
{
    #if defined(linux)
        using Renderer = BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenVulkan::VulkanRenderer*;

        using RendererType = BlitzenVulkan::VulkanRenderer;

        using RenderingLoadingContextMesh = BlitzenVulkan::LoadingContextMesh;

        using RenderingLoadingContextRenderObjects = BlitzenVulkan::LoadingContextRenderObjects;

    #elif defined(_WIN32) && defined(BLIT_VK_FORCE)

        using Renderer = BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenVulkan::VulkanRenderer*;

        using RendererType = BlitzenVulkan::VulkanRenderer;

        using RenderingLoadingContextMesh = BlitzenVulkan::LoadingContextMesh;

        using RenderingLoadingContextRenderObjects = BlitzenVulkan::LoadingContextRenderObjects;

    #elif defined(_WIN32) && defined(BLIT_GL_LEGACY_OVERRIDE) 

        using Renderer = BlitCL::SmartPointer<BlitzenGL::OpenglRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenGL::OpenglRenderer*;

		using RendererType = BlitzenGL::OpenglRenderer;

        using RenderingLoadingContextMesh = BlitzenGL::LoadingContextMesh;

        using RenderingLoadingContextRenderObjects = BlitzenGL::LoadingContextRenderObjects;

    #elif defined(_WIN32)

        using Renderer = BlitCL::SmartPointer<BlitzenDX12::Dx12Renderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenDX12::Dx12Renderer*;

        using RendererType = BlitzenDX12::Dx12Renderer;

        using RenderingLoadingContextMesh = BlitzenDX12::LoadingContextMesh;

        using RenderingLoadingContextRenderObjects = BlitzenDX12::LoadingContextRenderObjects;

    #else

        static_assert(true);

    #endif
    
    // API initialization. Requests handles, preallocates buffers and checks that the host can succesfully run the chosen renderer
    uint8_t StartupRenderer(RendererPtrType pRenderer, uint32_t windowWidth, uint32_t windowHeight, BlitzenPlatform::PlatformContext* pPlatform);

    // Post-loading function. CPU side resources passed to GPU side buffers (or whichever type of handle is used)
    // Views for resources also placed
    uint8_t UploadResourcesToGPU(RendererPtrType pRenderer, DrawContext& drawContext);

    // Singular texture upload
    uint8_t UploadTextureToGPU(RendererPtrType pRenderer, void* pTextureData, const char* filepath);

    uint8_t AllocateLoadingContextMesh(RendererPtrType pRenderer ,RenderingLoadingContextMesh& ctx);

    // Copies a mesh's primitives to a staging buffer. Count should not have sizeof(type) included. It's done inside.
    uint8_t UploadToMeshPrimitiveStagingBuffer(RenderingLoadingContextMesh& ctx, PrimitiveSurface* primitives, uint32_t count);
    // Copies a mesh's LODs to a staging buffer. Count should not have sizeof(type) included. It's done inside.
    uint8_t UploadToLODDataStagingBuffer(RenderingLoadingContextMesh& ctx, LodData* LODs, uint32_t count);
    // Copies a mesh's vertex positions to a staging buffer. Count should not have sizeof(type) included. It's done inside.
    uint8_t UploadToVertexPositionsStagingBuffer(RenderingLoadingContextMesh& ctx, VtxPos* vtxPositions, uint32_t count);
    // Copies a mesh's vertex normals to a staging buffer. Count should not have sizeof(type) included. It's done inside.
    uint8_t UploadToVertexNormalsStagingBuffer(RenderingLoadingContextMesh& ctx, VtxNormals* vtxNormals, uint32_t count);
    // Copies a mesh's vertex tangents to a staging buffer. Count should not have sizeof(type) included. It's done inside.
    uint8_t UploadToVertexTangentsStagingBuffer(RenderingLoadingContextMesh& ctx, VtxTangents* vtxTangents, uint32_t count);
    // Copies a mesh's vertex texture coordinates to a staging buffer. Count should not have sizeof(type) included. It's done inside.
    uint8_t UploadToVertexTextureCoordinatesStagingBuffer(RenderingLoadingContextMesh& ctx, VtxTexCoords* vtxTexCoords, uint32_t count);
    // Copies a mesh's vertex indices to a staging buffer. Count should have sizeof(type) included. It's done inside.
    uint8_t UploadToVertexIndicesStagingBuffer(RenderingLoadingContextMesh& ctx, uint32_t* indices, uint32_t count);
    // Copies a mesh's cluster vertices to a staging buffer. Count should not have sizeof(type) included. It's done inside.
    uint8_t UploadToClusterVerticesStagingBuffer(RenderingLoadingContextMesh& ctx, ClusterVertices* clusterVertices, uint32_t count);
    // Copies a mesh's cluster spheres to a staging buffer. Count should not have sizeof(type) included. It's done inside.
    uint8_t UploadToClusterSpheresStagingBuffer(RenderingLoadingContextMesh& ctx, ClusterSphere* clusterSpheres, uint32_t count);
    // Copies a mesh's cluster cones to a staging buffer. Count should not have sizeof(type) included. It's done inside.
    uint8_t UploadToClusterConesStagingBuffer(RenderingLoadingContextMesh& ctx, ClusterCone* clusterCones, uint32_t count);
    // Copies a mesh's cluster indices to a staging buffer. Count shoud not have sizeof(type) include. It's done inside.
    uint8_t UploadToClusterIndicesStagingBuffer(RenderingLoadingContextMesh& ctx, uint32_t* clusterIndices, uint32_t count);

    uint8_t AllocateLoadingContextRenderObjects(RendererPtrType pRenderer, RenderingLoadingContextRenderObjects& ctx);

    uint8_t UploadToRenderObjectStagingBuffer(RenderingLoadingContextRenderObjects& ctx, RenderObject* renderObjects, uint32_t renderCount);
    uint8_t UploadToDynamicRenderObjectStagingBuffer(RenderingLoadingContextRenderObjects& ctx, RenderObject* renderObjects, uint32_t renderCount);
    uint8_t UploadToWorldTransformStagingBuffer(RenderingLoadingContextRenderObjects& ctx, MeshTransform* transforms, uint32_t transformCount);
    uint8_t UploadToCPUTransformStagingBuffer(RenderingLoadingContextRenderObjects& ctx, CPU_TRANSFORM* transforms, uint32_t transformCount);

    uint8_t UploadToRenderObjectStagingBuffer_MKII(RendererPtrType pRenderer, RenderingLoadingContextRenderObjects& ctx, RenderObject* renderObjects, uint32_t renderCount);
    uint8_t UploadToDynamicRenderObjectStagingBuffer_MKII(RendererPtrType pRenderer, RenderingLoadingContextRenderObjects& ctx, RenderObject* renderObjects, uint32_t renderCount);
    uint8_t UploadToWorldTransformStagingBuffer_MKII(RendererPtrType pRenderer, RenderingLoadingContextRenderObjects& ctx, MeshTransform* transforms, uint32_t transformCount);
    uint8_t UploadToCPUTransformStagingBuffer_MKII(RendererPtrType pRenderer, RenderingLoadingContextRenderObjects& ctx, CPU_TRANSFORM* transforms, uint32_t transformCount);
	uint8_t UploadToBoundingSphereStagingBuffer_MKII(RendererPtrType pRenderer, RenderingLoadingContextRenderObjects& ctx, BoundingSphere* boundingSpheres, uint32_t sphereCount);

    uint8_t UploadNewGeometryDataToSSBOs(RendererPtrType pRenderer, RenderingLoadingContextRenderObjects& instanceData, RenderingLoadingContextMesh& resourceData);

    enum class MESH_RESOURCES_STAGING_BUFFER_RESOURCE_TYPE : uint8_t
    {
        VERTEX_POSITIONS,
        VERTEX_NORMALS,
        VERTEX_TANGENTS,
        VERTEX_TEXTURE,
        VERTEX_INDICES,
        CLUSTER_VERTICES,
        CLUSTER_SPHERES,
        CLUSTER_CONES,
        CLUSTER_INDICES,
        MESH_PRIMITIVES,
        LEVELS_OF_DETAIL
    };
    uint8_t UploadMeshPrimitiveResourcesToStagingBufferGeneral(RenderingLoadingContextMesh& ctx, BLIT_STRAIGHTHANDLE pData, uint32_t count, MESH_RESOURCES_STAGING_BUFFER_RESOURCE_TYPE resourceType);

    // Finalizes resources so that the renderer is ready for culling and rendering
    void PrepareRendererForRuntime(RendererPtrType pRenderer);

    enum class RENDERER_FENCE_TYPE : uint8_t
    {
        GRAPHICS,
        COMPUTE,
        TRANSFER
    };
    // Custom CPU fence. Meant to stop renderer command recording until a desired event.
    void PlaceRendererFence(RendererPtrType pRenderer, RENDERER_FENCE_TYPE type);

    // Gives new camera values to the shader. Static object culling can be dispatched after this
    void UpdateRendererView(RendererPtrType pRenderer, CameraViewData& camera, bool isFrustumFrozen);
    
    // Generates a Hierarchical depth buffer by copying the depth target. Allow for occlusion culling. Should be called before transparent object are drawn
    void GenerateHI_Z_MAP(RendererPtrType pRenderer);

    // Should be called after game logic is done. Copies movement data to shader buffer. Dynamic object culling can be dispatched after this
    void UpdateRendererTransforms(RendererPtrType pRenderer, CPU_TRANSFORM* pTransforms, uint32_t transformCount);

    enum class BLIT_CULL_TYPE : uint8_t
    {
        NO_CULL,
        DRAW_CULL_DEFAULT,
        DRAW_CULL_INSTANCED,
        DRAW_CULL_TEMPORAL_OCCLUSION,
        CLUSTER_CULL_DEFAULT
    };
    struct CULL_CONTEXT
    {
        WORLD_RESIDENTS* m_pResidents{ nullptr };
        BLIT_CULL_TYPE m_cullType{ BLIT_CULL_TYPE::NO_CULL };
        RENDER_OBJECT_TYPE m_workType{ RENDER_OBJECT_TYPE::OPAQUE_STATIC };
        uint32_t m_workCount{ 0 };
    };
    // Culls a group of renders using compute shader. Prepares draw commands for rendering
    void DispatchCullingShaders(RendererPtrType pRenderer, const CULL_CONTEXT& cullContext);

    // Needs to be called before the first render pass to define vieport
    void SetupForFirstRenderPass(RendererPtrType pRenderer);

    void ChangeCullingBuffersToReadbackMode(RendererPtrType pRenderer);

    struct SHADER_GAME_LOGIC_UPDATES
    {
        CPU_TRANSFORM* pGpuTransorms;
        uint32_t m_transformCount;
    };
    // Copies updates that were made in the shader back to the GPU. 
    // Stops compute commands but resets them inside, no need to call BeginGPUCommands after this.
    void RequestGameLogicUpdatesFromShader(RendererPtrType, SHADER_GAME_LOGIC_UPDATES& outUpdate);

    enum class BLIT_RENDER_TYPE : uint8_t
    {
        RENDER_OPAQUE,
        RENDER_DYNAMIC,
        RENDER_INSTANCED,
        RENDER_TRANSPARENT
    };
    struct RENDER_CONTEXT
    {
        BLIT_RENDER_TYPE m_renderType{ BLIT_RENDER_TYPE::RENDER_OPAQUE };
    };
    void RenderObjects(RendererPtrType pRenderer, RENDER_CONTEXT& renderContext);

    void RenderTerrain(RendererPtrType pRenderer, uint32_t terrainCount);

#if !defined(NDEBUG)
    void RENDER_BOUNDING_SPHERES_DEBUG(RendererPtrType pRenderer);
#else
    #define RENDER_BOUNDING_SPHERES_DEBUG(pRenderer)
#endif

    void FinalizeRendering(RendererPtrType pRenderer);

    void PresentRender(RendererPtrType pRenderer, uint32_t waitCount);

    // Should be called by event system to update renderer surface resources when a window resize event is encountered
    BlitML::vec2 UpdateRendererWindowData(RendererPtrType pRenderer, uint32_t newWidth, uint32_t newHeight, BlitzenPlatform::PlatformContext* pPlatform);

    enum class RENDERER_IDLE_MODE : uint8_t
    {
        TRIANGLE,
        BLITZEN_LOGO
    };
    void RendererWorkIdle(RendererPtrType pRendrer, RENDERER_IDLE_MODE mode);

    uint8_t UploadRendererIdleWorkResources(RendererPtrType pRenderer, RENDERER_IDLE_MODE mode);

    enum class BMPR_COMMAND_LIST_TYPE : uint8_t
    {
        GRAPHICS,
        TRANSFER,
        COMPUTE
    };
    void BeginGPUCommands(RendererPtrType pRenderer, BMPR_COMMAND_LIST_TYPE mode);
    void EndGPUCommands(RendererPtrType pRenderer, BMPR_COMMAND_LIST_TYPE type);
}