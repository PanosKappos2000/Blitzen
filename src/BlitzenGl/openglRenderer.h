#pragma once
#include "openglData.h"

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
        uint8_t occlusionEnabled;
        uint8_t lodEnabled;

        inline CullData(uint32_t dc, uint8_t oc, uint8_t lod) 
            :drawCount{dc}, occlusionEnabled{oc}, lodEnabled{lod} 
        {}
    };

    class OpenglRenderer
    {
    public:
        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight);

        uint8_t UploadTexture(void* pData, const char* filepath);

        uint8_t SetupForRendering(BlitzenEngine::RenderingResources* pResources, 
            float& pyramidWidth, float& pyramidHeight
        );

        void DrawFrame(BlitzenEngine::DrawContext& context);

        void Shutdown();

    private:

        bool LoadDDSTextureData(BlitzenEngine::DDS_HEADER& header,
            BlitzenEngine::DDS_HEADER_DXT10& header10, BlitzenPlatform::FileHandle& fileHandle,
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

        GlTexture m_textures[BlitzenEngine::ce_maxTextureCount];

        uint32_t m_textureCount = 0;
    };

    uint8_t CompileShader(GlShader& shader, GLenum shaderType, const char* filepath);

    uint8_t CreateGraphicsProgram(const char* verteShaderFilepath, const char* fragmentShaderFilepath, 
    GraphicsProgram& program);

    uint8_t CreateComputeProgram(const char* shaderFilepath, ComputeProgram& program);
}

namespace BlitzenPlatform
{
    uint8_t CreateOpenglDrawContext();

    void OpenglSwapBuffers();
}