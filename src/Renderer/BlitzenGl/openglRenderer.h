#pragma once
#include "openglData.h"
#include "BlitCL/DynamicArray.h"
#include "Renderer/Resources/Textures/blitTextures.h"
#include "Renderer/Interface/blitRendererInterface.h"
#include "Platform/blitPlatformContext.h"

namespace BlitzenGL
{
    using GraphicsProgram = ShaderProgram;
    using ComputeProgram = ShaderProgram;

    struct IndirectDrawCommand
    {
        uint32_t  indexCount;
        uint32_t  instanceCount;
        uint32_t  firstIndex;
        uint32_t  vertexOffset;
        uint32_t  firstInstance;
    };

    struct CullData
    {
        uint32_t drawCount;

        inline CullData(uint32_t dc) 
            :drawCount{dc} 
        {}
    };

    class OpenglRenderer
    {
    public:
        bool Init(uint32_t windowWidth, uint32_t windowHeight, void* pPlatform);

        ~OpenglRenderer();

        uint8_t UploadTexture(const char* filepath);

        uint8_t SetupForRendering(BlitzenEngine::DrawContext& drawContext);

        void UpdateObjectTransform(uint32_t transformId, BlitzenEngine::MeshTransform* pTransform);

        void DrawWhileWaiting();

        void DrawFrame(BlitzenEngine::DrawContext& context);

        static OpenglRenderer* GetRendererInstance() { return s_pRenderer; }

    private:

        bool LoadDDSTextureData(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10, BlitzenPlatform::FileHandle& fileHandle,
            void* pData);

    private:

        // Program that holds the vertex and fragment shader for drawing opaque geometry
        GraphicsProgram m_opaqueGeometryGraphicsProgram;

        ComputeProgram m_initialDrawCullCompProgram;

        // This is the single vertex buffer that will be used for the scene
        GlBuffer m_vertexBuffer;

        // The vertex array will specify the 3 vertex attributes: Position, Normals, UvMapping
        VertexArray m_vertexArray;

        // This is the single index buffer that will hold indices to the vertex buffer
        GlBuffer m_indexBuffer;

        // This buffer will hold the indirect commands that will be set by the culling compute shader
        GlBuffer m_indirectDrawBuffer;

        // Holds all the transform for every object in the scene
        GlBuffer m_transformBuffer;

        // Holds all the mesh surface/primitive representations that have been loaded for the current scene
        GlBuffer m_surfaceBuffer;

        GlBuffer m_materialBuffer;

        GlBuffer m_cullDataBuffer;

        GlBuffer m_viewDataBuffer;

        // Holds all the render objects that will be retrieved in the shaders to access the surface and transform data for each object
        GlBuffer m_renderObjectBuffer;

        GlTexture m_textures[BlitzenCore::Ce_MaxTextureCount];

        uint32_t m_textureCount = 0;

        static OpenglRenderer* s_pRenderer;
    };

    uint8_t CompileShader(GlShader& shader, GLenum shaderType, const char* filepath);

    uint8_t CreateGraphicsProgram(const char* verteShaderFilepath, const char* fragmentShaderFilepath, 
    GraphicsProgram& program);

    uint8_t CreateComputeProgram(const char* shaderFilepath, ComputeProgram& program);
}

namespace BlitzenPlatform
{
    uint8_t CreateOpenglDrawContext(void* pPlatform);

    void OpenglSwapBuffers(BlitzenPlatform::PlatformContext* pPlatform);
}