// Temporarily opengl is only available on WindowsS
#if _MSC_VER
#include "openglRenderer.h"
#include "Platform/filesystem.h"

const char *vertexShaderSource = "#version 450 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

const char* fragmentShaderSource = "#version 330 core\n"
                                    "out vec4 FragColor;\n"
                                    "void main()\n"
                                    "{\n"
                                    "FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
                                    "}";

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

        return 1;
    }

    void OpenglRenderer::SetupForRendering()
    {
        BlitML::vec3 vertices[3];
        vertices[0] = BlitML::vec3(-0.5f, 0.f, 0.f);
        vertices[1] = BlitML::vec3(0.f, 1.f, 0.f);
        vertices[2] = BlitML::vec3(0.5f, 0.f, 0.f);

        glGenBuffers(1, &m_vertexBuffer);
        glGenVertexArrays(1, &m_indexBuffer);

        glBindVertexArray(m_indexBuffer);

        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(BlitML::vec3), &vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BlitML::vec3), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        BLIT_ASSERT(CreateGraphicsProgram("VulkanShaders/MainObjectShader.vert.glsl.spv", "VulkanShaders/MainObjectShader.frag.glsl.spv", 
        m_opaqueGeometryGraphicsProgram))
    }

    void OpenglRenderer::DrawFrame()
    {
        glClearColor(0, 0.5f, 0.7f, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(m_opaqueGeometryGraphicsProgram);
        glBindVertexArray(m_indexBuffer);
        glDrawArrays(GL_TRIANGLES, 0, 3);

	    BlitzenPlatform::OpenglSwapBuffers();
    }

    uint8_t CreateGraphicsProgram(const char* vertexShaderFilepath, const char* fragmentShaderFilepath, 
    GraphicsProgram& program)
    {
        // Compiles the fragment shader
        GlShader fragmentShader;
        if(!CompileShader(fragmentShader, GL_FRAGMENT_SHADER, fragmentShaderFilepath))
            return 0;

        // Compiles the vertex shader
        GlShader vertexShader;
        if(!CompileShader(vertexShader, GL_VERTEX_SHADER, vertexShaderFilepath))
            return 0;

        // Attaches the two shaders and link the graphics program
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
            
        size_t filesize = 0;
        uint8_t* pBytes = nullptr;
        if(!BlitzenPlatform::FilesystemReadAllBytes(handle, &pBytes, &filesize))
            return 0;
        BlitzenPlatform::CloseFile(handle);

        // Compile the vertex shader
        shader = glCreateShader(shaderType);
        glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, pBytes, filesize);
        const char* entryPoint = "main";
        glSpecializeShader(shader, (const GLchar*)entryPoint, 0, nullptr, nullptr);

        // Check if the vertex shader compiled succesfully
        int32_t  success = 0;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            BLIT_ERROR("GLSL_COMPILATION_ERROR::%s", infoLog);
            return 0;
        }

        return 1;
    }
}
#endif