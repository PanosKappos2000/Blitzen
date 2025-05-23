#include "openglRenderer.h"
#include "Platform/filesystem.h"
#include <string>
#include "Game/blitCamera.h"

namespace BlitzenGL
{
    OpenglRenderer* OpenglRenderer::s_pRenderer;

    bool OpenglRenderer::Init(uint32_t windowWidth, uint32_t windowHeight)
    {
        if(!BlitzenPlatform::CreateOpenglDrawContext())
        {
            BLIT_ERROR("Opengl failed to load")
            return false;
        }

        s_pRenderer = this;

        // Set the viewport
        glViewport(0, 0, windowWidth, windowHeight);

        // Depth test configurations for reverse infinite z	
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL);
        glDisable(GL_CULL_FACE);

        return true;
    }

    uint8_t OpenglRenderer::UploadTexture(const char* filepath) 
    {
        if (m_textureCount >= BlitzenCore::Ce_MaxTextureCount)
        {
			BLIT_ERROR("Max texture count reached");
            return 0;
        }
        
        BlitCL::StoragePointer<uint8_t, BlitzenCore::AllocationType::SmartPointer> store(128 * 1024 * 1024);

        // This is a consequence of having a shared Load image function with Vulkan
		BlitzenEngine::DDS_HEADER header;
		BlitzenEngine::DDS_HEADER_DXT10 header10;
        BlitzenPlatform::FileHandle handle;
        if(BlitzenEngine::OpenDDSImageFile(filepath, header, header10, handle) && LoadDDSTextureData(header, header10, handle, store.Data()))
        {
            // Create and bind the texture
            glGenTextures(1, &m_textures[m_textureCount].handle);
            glBindTexture(GL_TEXTURE_2D, m_textures[m_textureCount].handle);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Pass the data to the texture and generate mipmaps
            glTexImage2D(GL_TEXTURE_2D, header.dwMipMapCount, GL_RGB, header.dwWidth, 
            header.dwHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, store.Data());
            glGenerateMipmap(GL_TEXTURE_2D);

            // Increment the texture count and generate mipmaps
            m_textureCount++;
            return 1;
        }
        else
        {
			BLIT_ERROR("Failed to load texture from file: %s", filepath);
            return 0;
        }
    }

    bool OpenglRenderer::LoadDDSTextureData(BlitzenEngine::DDS_HEADER& header, 
        BlitzenEngine::DDS_HEADER_DXT10& header10, BlitzenPlatform::FileHandle& fileHandle, 
        void* pData)
    {
        size_t blockSize = BlitzenEngine::GetDDSBlockSize(header, header10);
        size_t imageSize = BlitzenEngine::GetDDSImageSizeBC(header.dwWidth, 
            header.dwHeight, header.dwMipMapCount, static_cast<unsigned int>(blockSize));

		auto file = reinterpret_cast<FILE*>(fileHandle.pHandle);

        size_t readSize = fread(pData, 1, imageSize, file);

        if (!pData)
            return false;

        if (readSize != imageSize)
            return false;

        return true;
    }

    void OpenglRenderer::UpdateObjectTransform(uint32_t transformId, BlitzenEngine::MeshTransform* pTransform)
    {
        //auto& transforms = BlitzenEngine::RenderingResources::GetRenderingResources()->transforms;
        //BlitzenCore::BlitMemCopy(transforms.Data() + trId, &newTr, sizeof(BlitzenEngine::MeshTransform));
         
    }

    uint8_t OpenglRenderer::SetupForRendering(BlitzenEngine::DrawContext& context)
    {
        // Generates the vertex array. I don't know why this needs to be here since I am not using vertex attributes, 
        // but if I don't have it, OpenGL will draw nothing -_-
        glGenVertexArrays(1, &m_vertexArray.handle);
        // Creates the vertex buffer as a storage buffer and passes it to binding t
        glGenBuffers(1, &m_vertexBuffer.handle);
        const BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices = context.m_meshes.m_vertices;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertexBuffer.handle);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BlitzenEngine::Vertex) * vertices.GetSize(), vertices.Data(), GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_vertexBuffer.handle);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0); 

        // Creates the index buffer and pass the indices to it
        glGenBuffers(1, &m_indexBuffer.handle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer.handle);
        const BlitCL::DynamicArray<uint32_t>& indices = context.m_meshes.m_indices;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * indices.GetSize(), indices.Data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glGenBuffers(1, &m_indirectDrawBuffer.handle);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectDrawBuffer.handle);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(IndirectDrawCommand) * context.m_renders.m_renderCount, nullptr, GL_STATIC_DRAW);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        // Binds the indirect draw buffer as an SSBO, so that it can be accessed by the culling shaders
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_indirectDrawBuffer.handle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_indirectDrawBuffer.handle);

        // Create the transform buffer as a storage buffer and pass it to binding 1
        auto& transforms = context.m_renders.m_transforms;
        glGenBuffers(1, &m_transformBuffer.handle);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_transformBuffer.handle);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 
            sizeof(BlitzenEngine::MeshTransform) * context.m_renders.m_transformCount, context.m_renders.m_transforms, GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_transformBuffer.handle);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Creates the primitive surface buffer as a storage buffer and passes it to binding 2
        const auto& surfaces = context.m_meshes.m_surfaces;
        glGenBuffers(1, &m_surfaceBuffer.handle);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_surfaceBuffer.handle);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BlitzenEngine::PrimitiveSurface) * surfaces.GetSize(), surfaces.Data(), GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_surfaceBuffer.handle);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Creates the render object buffer as a storage buffer and passes it to binding 3
        glGenBuffers(1, &m_renderObjectBuffer.handle);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_renderObjectBuffer.handle);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BlitzenEngine::RenderObject) * context.m_renders.m_renderCount, context.m_renders.m_renders, GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_renderObjectBuffer.handle);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Creates the material buffer as a storage buffer and passes it binding 4
        glGenBuffers(1, &m_materialBuffer.handle);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_materialBuffer.handle);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BlitzenEngine::Material) * context.m_textures.m_materialCount, context.m_textures.m_materials, GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_materialBuffer.handle);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Generate uniform buffers
        glGenBuffers(1, &m_viewDataBuffer.handle);
        glGenBuffers(1, &m_cullDataBuffer.handle);

        // Create the compute shader program that will perform initial culling operations
        if (!CreateComputeProgram("GlslShaders/InitialDrawCullShader.comp.glsl", m_initialDrawCullCompProgram))
        {
            return 0;
        }

        // Create the graphics program that will have the vertex and fragment shader attached
        if (!CreateGraphicsProgram("GlslShaders/MainVertexOutput.vert.glsl", "GlslShaders/MainFragmentOutput.frag.glsl", m_opaqueGeometryGraphicsProgram))
        {
            return 0;
        }
        
        return 1;
    }

    void OpenglRenderer::DrawFrame(BlitzenEngine::DrawContext& context)
    {
        // Update the viewport if the window has resized
        if (context.m_camera.transformData.bWindowResize)
        {
            glViewport(0, 0, GLsizei(context.m_camera.transformData.windowWidth), GLsizei(context.m_camera.transformData.windowHeight));
        }

        // Update the culling data buffer
        glBindBuffer(GL_UNIFORM_BUFFER, m_viewDataBuffer.handle);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(BlitzenEngine::CameraViewData), &context.m_camera.viewData, GL_STATIC_READ);

        CullData cull{ context.m_renders.m_renderCount};
        glBindBuffer(GL_UNIFORM_BUFFER, m_cullDataBuffer.handle);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(CullData), &(cull), GL_STATIC_READ);

        // Switches to the culling compute program
        glUseProgram(m_initialDrawCullCompProgram.handle);

        // Bind the culling data buffer to uniform binding 0
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_viewDataBuffer.handle);
        // Binds the shader data buffer to uniform binding 1
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_cullDataBuffer.handle);
        
        // Dispatches the compute shader to do GPU side culling and create the draw commands
        glDispatchCompute(context.m_renders.m_renderCount / 64 + 1, 1, 1);
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        // glClearDepth value needs to be set to 0 since the renderer is using reverse z
        glClearDepth(0.0f);
        glClearColor(0, 0.5f, 0.7f, 1);
        // Clear the depth buffer and the color buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Switch to the graphics program
        glUseProgram(m_opaqueGeometryGraphicsProgram.handle);

        // Bind the vertex attribute array to access vertices
        glBindVertexArray(m_vertexArray.handle);

        // Bind the index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer.handle);

        // Bind the indirect draw buffer
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectDrawBuffer.handle);

        // Bind the uniform buffer that will hold the shader data to binding 0
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_viewDataBuffer.handle);

        // Draw the objects with indirect commands
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, context.m_renders.m_renderCount, sizeof(IndirectDrawCommand));

        // Swaps the framebuffer
	    BlitzenPlatform::OpenglSwapBuffers();
    }

    void OpenglRenderer::DrawWhileWaiting()
    {

    }

    uint8_t CreateGraphicsProgram(const char* vertexShaderFilepath, const char* fragmentShaderFilepath, 
    GraphicsProgram& program)
    {
        // Compiles the vertex shader
        GlShader vertexShader;
        if(!CompileShader(vertexShader, GL_VERTEX_SHADER, vertexShaderFilepath))
            return 0;

        // Compiles the fragment shader
        GlShader fragmentShader;
        if(!CompileShader(fragmentShader, GL_FRAGMENT_SHADER, fragmentShaderFilepath))
            return 0;

        // Attaches the two shaders and link the graphics program
        program.handle = glCreateProgram();
        glAttachShader(program.handle, vertexShader.handle);
        glAttachShader(program.handle, fragmentShader.handle);
        glLinkProgram(program.handle);

        // Checks if the program linked succesfully
        int32_t success;
        char infoLog[512];
        glGetProgramiv(program.handle, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(program.handle, 512, nullptr, infoLog);
            BLIT_ERROR("OpenGL Graphics program link failed::%s", infoLog)
            return 0;
        }

        return 1;
    }

    uint8_t CompileShader(GlShader& shader, GLenum shaderType, const char* filepath)
    {
        // Open the file using the handle wrapper, fClose should be called by the destructor
        BlitzenPlatform::FileHandle handle;
        if(!handle.Open(filepath, BlitzenPlatform::FileModes::Read, 1))
            return 0;
        
        // This string will hold the final shader source code
        std::string shaderSource;
        // lineLen will hold the size of each line, placeholder will temporarily hold the code of each line
        size_t lineLen = 0;
        std::string placeholder;
        placeholder.resize(1000);
        char* data = (char*)placeholder.c_str();
        // Goes through every line, save the code and the size
        while(BlitzenPlatform::FilesystemReadLine(handle, 1000, &data, &lineLen))
        {
            // Appends to the final source code string only up to lineLen
            shaderSource.append(placeholder.c_str(), lineLen);
        }

        // Compiles the shader
        const char* source = shaderSource.c_str();
        shader.handle = glCreateShader(shaderType);
        glShaderSource(shader.handle, 1, &source, nullptr);
        glCompileShader(shader.handle);

        // Checks if the shader compiled succesfully
        int32_t  success = 0;
        char infoLog[512];
        glGetShaderiv(shader.handle, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(shader.handle, 512, nullptr, infoLog);
            BLIT_ERROR("GLSL_COMPILATION_ERROR::%s", infoLog);
            return 0;
        }

        return 1;
    }

    uint8_t CreateComputeProgram(const char* shaderFilepath, ComputeProgram& program)
    {
        GlShader shader;
        if(!CompileShader(shader, GL_COMPUTE_SHADER, shaderFilepath))
            return 0;

        program.handle = glCreateProgram();
        glAttachShader(program.handle, shader.handle);
        glLinkProgram(program.handle);

        int32_t success = 0;
        char infoLog[512];
        glGetProgramiv(program.handle, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(program.handle, 512, nullptr, infoLog);
            BLIT_ERROR("Opengl compute program link failed: %s", infoLog);
            return 0;
        }

        return 1;
    }

    OpenglRenderer::~OpenglRenderer()
    {
        
    }
}