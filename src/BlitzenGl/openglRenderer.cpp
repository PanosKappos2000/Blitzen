// Temporarily opengl is only available on WindowsS
#if _MSC_VER
#include "openglRenderer.h"
#include "Platform/filesystem.h"
#include <string>

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

        // Enable depth testing
        glEnable(GL_DEPTH_TEST);

        return 1;
    }

    uint8_t OpenglRenderer::SetupForRendering(BlitzenEngine::RenderingResources* pResources)
    {
        // Creates the vertex buffer and vertex attributes 
        glGenVertexArrays(1, &m_vertexArray);
        glGenBuffers(1, &m_vertexBuffer);

        // Passes the vertices to the vertex buffer
        BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices = pResources->vertices;
        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(BlitzenEngine::Vertex) * vertices.GetSize(), vertices.Data(), GL_STATIC_DRAW);

        // Specify vertex attribute: position at binding 0
        glBindVertexArray(m_vertexArray);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BlitzenEngine::Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        // Specify vertex attribute: textureMapU at binding 1
        glVertexAttribPointer(1, 1, GL_HALF_FLOAT, GL_FALSE, sizeof(BlitzenEngine::Vertex), (void*)offsetof(BlitzenEngine::Vertex, uvX));
        glEnableVertexAttribArray(1);
        // Specify vertex attribute: textureMapV at binding 2
        glVertexAttribPointer(2, 1, GL_HALF_FLOAT, GL_FALSE, sizeof(BlitzenEngine::Vertex), (void*)offsetof(BlitzenEngine::Vertex, uvY));
        glEnableVertexAttribArray(2);
        // Specify vertex attribute: normalX at binding 3
        glVertexAttribPointer(3, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(BlitzenEngine::Vertex), (void*)offsetof(BlitzenEngine::Vertex, normalX));
        glEnableVertexAttribArray(3);
        // Specify vertex attribute: normalY at binding 4
        glVertexAttribPointer(4, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(BlitzenEngine::Vertex), (void*)offsetof(BlitzenEngine::Vertex, normalY));
        glEnableVertexAttribArray(4);
        // Specify vertex attribute: normalZ at binding 5
        glVertexAttribPointer(5, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(BlitzenEngine::Vertex), (void*)offsetof(BlitzenEngine::Vertex, normalZ));
        glEnableVertexAttribArray(5);

        // Unbinds the vertex buffer and the vertex attribute array
        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0);

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

        //glGenBuffers(1, &m_cullingDataBuffer);
        //glBindBuffer(GL_)

        if(!CreateGraphicsProgram("GlslShaders/MainVertexOutput.vert.glsl", "GlslShaders/MainFragmentOutput.frag.glsl", 
        m_opaqueGeometryGraphicsProgram))
            return 0;

        if(!CreateComputeProgram("GlslShaders/InitialDrawCullShader.comp.glsl", m_initialDrawCullCompProgram))
            return 0;
        
        return 1;
    }

    void OpenglRenderer::DrawFrame(BlitzenEngine::RenderContext& context)
    {
        BlitzenEngine::CullingData& cullData = context.cullingData;

        glUseProgram(m_initialDrawCullCompProgram);
        glDispatchCompute(cullData.drawCount / 64 + 1, 1, 1);

        glClearColor(0, 0.5f, 0.7f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind the graphics shader program
        glUseProgram(m_opaqueGeometryGraphicsProgram);

        // Bind the vertex attribute array for vertex position
        glBindVertexArray(m_vertexArray);

        // Bind the index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);

        // Bind the indirect draw buffer
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectDrawBuffer);

        // Temporary test code
        int vertexColorLocation = glGetUniformLocation(m_opaqueGeometryGraphicsProgram, "projectionView");
        glUniformMatrix4fv(vertexColorLocation, 1, GL_FALSE, &context.globalShaderData.projectionView[0]);

        // Draw the objects with indirect commands
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 5000, sizeof(IndirectDrawCommand));

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
        BlitzenPlatform::FileHandle handle;
        if(!BlitzenPlatform::OpenFile(filepath, BlitzenPlatform::FileModes::Read, 1, handle))
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

        glDeleteBuffers(1, &m_renderObjectBuffer);
        glDeleteBuffers(1, &m_surfaceBuffer);
        glDeleteBuffers(1, &m_transformBuffer);
        glDeleteBuffers(1, &m_indirectDrawBuffer);
        glDeleteBuffers(1, &m_indexBuffer);
        glDeleteBuffers(1, &m_vertexBuffer);

        glDeleteVertexArrays(1, &m_vertexArray);
    }
}
#endif