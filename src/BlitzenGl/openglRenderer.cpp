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
        BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices = pResources->vertices;
        glGenVertexArrays(1, &m_vertexArray);
        glGenBuffers(1, &m_vertexBuffer);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(m_vertexArray);

        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(BlitzenEngine::Vertex) * vertices.GetSize(), vertices.Data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BlitzenEngine::Vertex), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0);

        pIndices = &pResources->indices;
        glGenBuffers(1, &m_indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * pIndices->GetSize(), pIndices->Data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        BlitzenEngine::PrimitiveSurface& surface = pResources->surfaces[0];
        IndirectDrawCommand command{surface.meshLod[0].indexCount, 1, surface.meshLod[0].firstIndex, surface.vertexOffset, 0};
        glGenBuffers(1, &m_indirectDrawBuffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectDrawBuffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(IndirectDrawCommand), &command,  GL_STATIC_DRAW);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

        BlitzenEngine::MeshTransform transform;
        transform.orientation = BlitML::quat(0);
        transform.pos = BlitML::vec3(0, 0, -70.f);
        transform.scale = 1.f;
        glGenBuffers(1, &m_transformBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_transformBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BlitzenEngine::MeshTransform), &transform, GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_transformBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        if(!CreateGraphicsProgram("GlslShaders/MainVertexOutput.vert.glsl", "GlslShaders/MainFragmentOutput.frag.glsl", 
        m_opaqueGeometryGraphicsProgram))
            return 0;
        
        return 1;
    }

    void OpenglRenderer::DrawFrame(BlitzenEngine::RenderContext& context)
    {
        glClearColor(0, 0.5f, 0.7f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_opaqueGeometryGraphicsProgram);
        glBindVertexArray(m_vertexArray);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectDrawBuffer);

        int vertexColorLocation = glGetUniformLocation(m_opaqueGeometryGraphicsProgram, "projectionView");
        glUniformMatrix4fv(vertexColorLocation, 1, GL_FALSE, &context.globalShaderData.projectionView[0]);

        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 1, sizeof(IndirectDrawCommand));

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
            BLIT_ERROR("OpenGL Graphics program compilation failed::%s", infoLog)
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
}
#endif