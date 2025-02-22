#include "openglRenderer.h"
#include "Platform/filesystem.h"
#include <string>
#include "Game/blitCamera.h"

namespace BlitzenGL
{
    uint8_t OpenglRenderer::Init(uint32_t windowWidth, uint32_t windowHeight)
    {
        if(!BlitzenPlatform::CreateOpenglDrawContext())
        {
            BLIT_ERROR("Opengl failed to load")
            return 0;
        }

        // Set the viewport
        glViewport(0, 0, windowWidth, windowHeight);

        // Configure the depth test	
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL);
        glDisable(GL_CULL_FACE);

        return 1;
    }

    uint8_t OpenglRenderer::UploadTexture(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10, 
    void* pData, const char* filepath) 
    {
        if(m_textureCount >= BlitzenEngine::ce_maxTextureCount)
            return 0;
        
        BlitCL::StoragePointer<uint8_t, BlitzenCore::AllocationType::SmartPointer> store(128 * 1024 * 1024);

        // This is a consequence of having a shared Load image function with Vulkan
        unsigned int placeholder = 0;
        if(BlitzenEngine::LoadDDSImage(filepath, header, header10, placeholder, BlitzenEngine::RendererToLoadDDS::Opengl, store.Data()))
        {
            // Create and bind the texture
            glGenTextures(1, &m_textures[m_textureCount]);
            glBindTexture(GL_TEXTURE_2D, m_textures[m_textureCount]);

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
            return 0;
    }

    uint8_t OpenglRenderer::SetupForRendering(BlitzenEngine::RenderingResources* pResources, float& pyramidWidth, float& pyramidHeight)
    {
        // Generates the vertex array. I don't know why this needs to be here since I am not using vertex attributes, 
        // but if I don't have it, OpenGL will draw nothing -_-
        glGenVertexArrays(1, &m_vertexArray);
        // Creates the vertex buffer as a storage buffer and passes it to binding t
        glGenBuffers(1, &m_vertexBuffer);
        BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices = pResources->vertices;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertexBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BlitzenEngine::Vertex) * vertices.GetSize(), vertices.Data(), GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_vertexBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0); 

        // Creates the index buffer and pass the indices to it
        glGenBuffers(1, &m_indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
        BlitCL::DynamicArray<uint32_t>& indices = pResources->indices;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * indices.GetSize(), indices.Data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Creates the indirect draw buffer. It will be as big as the draw count. It will initially be empty but it will be filled by the culling shaders
        glGenBuffers(1, &m_indirectDrawBuffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectDrawBuffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(IndirectDrawCommand) * pResources->renderObjectCount, nullptr,  GL_STATIC_DRAW);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        // Binds the indirect draw buffer as an SSBO, so that it can be accessed by the culling shaders
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_indirectDrawBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_indirectDrawBuffer);

        // Create the transform buffer as a storage buffer and pass it to binding 1
        BlitCL::DynamicArray<BlitzenEngine::MeshTransform>& transforms = pResources->transforms;
        glGenBuffers(1, &m_transformBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_transformBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BlitzenEngine::MeshTransform) * transforms.GetSize(), 
        transforms.Data(), GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_transformBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Creates the primitive surface buffer as a storage buffer and passes it to binding 2
        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& surfaces = pResources->surfaces;
        glGenBuffers(1, &m_surfaceBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_surfaceBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BlitzenEngine::PrimitiveSurface) * surfaces.GetSize(),
        surfaces.Data(), GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_surfaceBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Creates the render object buffer as a storage buffer and passes it to binding 3
        glGenBuffers(1, &m_renderObjectBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_renderObjectBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BlitzenEngine::RenderObject) * pResources->renderObjectCount, pResources->renders, GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_renderObjectBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Creates the material buffer as a storage buffer and passes it binding 4
        glGenBuffers(1, &m_materialBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_materialBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BlitzenEngine::Material) * pResources->materialCount, pResources->materials, GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_materialBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Generate uniform buffers
        glGenBuffers(1, &m_viewDataBuffer);
        glGenBuffers(1, &m_cullDataBuffer);

        // Create the graphics program that will have the vertex and fragment shader attached
        if(!CreateGraphicsProgram("GlslShaders/MainVertexOutput.vert.glsl", "GlslShaders/MainFragmentOutput.frag.glsl", 
        m_opaqueGeometryGraphicsProgram))
            return 0;

        // Create the compute shader program that will perform initial culling operations
        if(!CreateComputeProgram("GlslShaders/InitialDrawCullShader.comp.glsl", m_initialDrawCullCompProgram))
            return 0;
        
        return 1;
    }

    void OpenglRenderer::DrawFrame(BlitzenEngine::DrawContext& context)
    {
        BlitzenEngine::Camera* pCamera = reinterpret_cast<BlitzenEngine::Camera*>(context.pCamera);
        // Update the viewport if the window has resized
        if(pCamera->transformData.windowResize)
        {
            glViewport(0, 0, 
            static_cast<GLsizei>(pCamera->transformData.windowWidth), 
            static_cast<GLsizei>(pCamera->transformData.windowHeight));
        }

        // Update the culling data buffer
        glBindBuffer(GL_UNIFORM_BUFFER, m_viewDataBuffer);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(BlitzenEngine::CameraViewData), &(pCamera->viewData), GL_STATIC_READ);

        CullData cull(context.drawCount, context.bOcclusionCulling, context.bLOD);
        glBindBuffer(GL_UNIFORM_BUFFER, m_cullDataBuffer);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(CullData), &(cull), GL_STATIC_READ);

        // Switches to the culling compute program
        glUseProgram(m_initialDrawCullCompProgram);

        // Bind the culling data buffer to uniform binding 0
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_viewDataBuffer);
        // Binds the shader data buffer to uniform binding 1
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_cullDataBuffer);
        
        // Dispatches the compute shader to do GPU side culling and create the draw commands
        glDispatchCompute(context.drawCount / 64 + 1, 1, 1);
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        // glClearDepth value needs to be set to 0 since the renderer is using reverse z
        glClearDepth(0.0f);
        glClearColor(0, 0.5f, 0.7f, 1);
        // Clear the depth buffer and the color buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Switch to the graphics program
        glUseProgram(m_opaqueGeometryGraphicsProgram);

        // Bind the vertex attribute array to access vertices
        glBindVertexArray(m_vertexArray);

        // Bind the index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);

        // Bind the indirect draw buffer
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectDrawBuffer);

        // Bind the uniform buffer that will hold the shader data to binding 0
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_viewDataBuffer);

        // Draw the objects with indirect commands
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, context.drawCount, sizeof(IndirectDrawCommand));

        // Swaps the framebuffer
	    BlitzenPlatform::OpenglSwapBuffers();
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
        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        // Checks if the program linked succesfully
        int32_t success;
        char infoLog[512];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            BLIT_ERROR("OpenGL Graphics program link failed::%s", infoLog)
            return 0;
        }

        // Deletes the shaders, they are no longer needed
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

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
        shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        // Checks if the shader compiled succesfully
        int32_t  success = 0;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            BLIT_ERROR("GLSL_COMPILATION_ERROR::%s", infoLog);
            glDeleteShader(shader);
            return 0;
        }

        return 1;
    }

    uint8_t CreateComputeProgram(const char* shaderFilepath, ComputeProgram& program)
    {
        GlShader shader;
        if(!CompileShader(shader, GL_COMPUTE_SHADER, shaderFilepath))
            return 0;

        program = glCreateProgram();
        glAttachShader(program, shader);
        glLinkProgram(program);

        int32_t success = 0;
        char infoLog[512];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            BLIT_ERROR("Opengl compute program link failed: %s", infoLog);
            glDeleteProgram(program);
            glDeleteShader(shader);
            return 0;
        }

        glDeleteShader(shader);
        return 1;
    }

    void OpenglRenderer::Shutdown()
    {
        glDeleteProgram(m_opaqueGeometryGraphicsProgram);
        glDeleteProgram(m_initialDrawCullCompProgram);

        glDeleteBuffers(1, &m_viewDataBuffer);
        glDeleteBuffers(1, &m_cullDataBuffer);

        glDeleteBuffers(1, &m_materialBuffer);
        glDeleteBuffers(1, &m_renderObjectBuffer);
        glDeleteBuffers(1, &m_surfaceBuffer);
        glDeleteBuffers(1, &m_transformBuffer);
        glDeleteBuffers(1, &m_indirectDrawBuffer);
        glDeleteBuffers(1, &m_indexBuffer);
        glDeleteBuffers(1, &m_vertexBuffer);

        glDeleteVertexArrays(1, &m_vertexArray);
    }
}