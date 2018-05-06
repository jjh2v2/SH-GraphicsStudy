#include <sb6.h>
#include <cmath>

const GLchar* vs[] = {
    "#version 430 core \n"
    "layout (location = 0) in vec4 offset; \n"
    "\n"
    "void main() { \n"
    "const vec4 vertices[3] = vec4[3]( vec4(0.25f, -0.25f, 0.f, 1.0f), "
    "vec4(-0.25f, -0.25f, 0.f, 1.0f), vec4(0.25f, 0.25f, 0.f, 1.0f) ); \n"
    "gl_Position = vertices[gl_VertexID] + offset; \n" 
    "}"
};

const GLchar* tcs[] = {
    "#version 430 core                                                                  \n"
    "                                                                                   \n"
    "layout (vertices = 3) out;                                                         \n"
    "                                                                                   \n"
    "void main(void)                                                                    \n"
    "{                                                                                  \n"
    "    if (gl_InvocationID == 0)                                                      \n"
    "    {                                                                              \n"
    "        gl_TessLevelInner[0] = 5.0;                                                \n"
    "        gl_TessLevelOuter[0] = 5.0;                                                \n"
    "        gl_TessLevelOuter[1] = 5.0;                                                \n"
    "        gl_TessLevelOuter[2] = 5.0;                                                \n"
    "    }                                                                              \n"
    "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;      \n"
    "}                                                                                  \n"
};

const GLchar* tes[] = {
    "#version 430 core                                           \n"
    "                                                            \n"
    "layout (triangles, equal_spacing, cw) in;                   \n"
    "                                                            \n"
    "void main(void)                                             \n"
    "{                                                           \n"
    "    gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) + \n"
    "                  (gl_TessCoord.y * gl_in[1].gl_Position) + \n"
    "                  (gl_TessCoord.z * gl_in[2].gl_Position);  \n"
    "}                                                           \n"
};

const GLchar* fs[] = {
    "#version 430 core\n"
    "out vec4 color;\n"
    "void main(void) {\n"
    "color = vec4(sin(gl_FragCoord.x * 0.25) * 0.5 + 0.5, \n"
    "             cos(gl_FragCoord.y * 0.25) * 0.5 + 0.5, \n"
    "             sin(gl_FragCoord.x * 0.15) * cos(gl_FragCoord.y * 0.15), 1.f); }"
};

GLuint CompileShader() {
    auto vs_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs_id, 1, vs, nullptr);
    glCompileShader(vs_id);

    auto fs_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs_id, 1, fs, nullptr);
    glCompileShader(fs_id);

    auto tcs_id = glCreateShader(GL_TESS_CONTROL_SHADER);
    glShaderSource(tcs_id, 1, tcs, nullptr);
    glCompileShader(tcs_id);

    auto tes_id = glCreateShader(GL_TESS_EVALUATION_SHADER);
    glShaderSource(tes_id, 1, tes, nullptr);
    glCompileShader(tes_id);

    auto program = glCreateProgram();
    glAttachShader(program, vs_id);
    glAttachShader(program, tcs_id);
    glAttachShader(program, tes_id);
    glAttachShader(program, fs_id);
    glLinkProgram(program);

    glDeleteShader(vs_id);
    glDeleteShader(tcs_id);
    glDeleteShader(tes_id);
    glDeleteShader(fs_id);
    return program;
}

class MyApplication final : public sb6::application {
public:
    void startup() override final {
        m_rendering_program = CompileShader();
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
    }

    void shutdown() override final {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteProgram(m_rendering_program);
    }

    void render(double current_time) override final {
        const GLfloat red[] { 
            static_cast<float>(sin(current_time)) * .5f + .5f, 
            static_cast<float>(cos(current_time)) * .5f + .5f, 
            0.f, 1.f };
        glClearBufferfv(GL_COLOR, 0, red);

        glUseProgram(m_rendering_program);
        GLfloat attrib[] {
            static_cast<float>(sin(current_time)) * .5f,
            static_cast<float>(cos(current_time)) * .6f,
            0.f, 0.f };
        glVertexAttrib4fv(0, attrib);

        glDrawArrays(GL_PATCHES, 0, 3);
    }

private:
    GLuint m_rendering_program;
    GLuint m_vao;
};

DECLARE_MAIN(MyApplication);