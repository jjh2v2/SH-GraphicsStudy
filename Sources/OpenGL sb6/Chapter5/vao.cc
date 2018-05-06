#include <sb6.h>
#include <cmath>
#include <vmath.h>

const GLchar* vs[] = {
    "#version 410 core                                                  \n"
    "                                                                   \n"
    "in vec4 position;                                                  \n"
    "                                                                   \n"
    "out VS_OUT                                                         \n"
    "{                                                                  \n"
    "    vec4 color;                                                    \n"
    "} vs_out;                                                          \n"
    "                                                                   \n"
    "uniform mat4 mv_matrix;                                            \n"
    "uniform mat4 proj_matrix;                                          \n"
    "                                                                   \n"
    "void main(void)                                                    \n"
    "{                                                                  \n"
    "    gl_Position = proj_matrix * mv_matrix * position;              \n"
    "    vs_out.color = position * 2.0 + vec4(0.5, 0.5, 0.5, 0.0);      \n"
    "}                                                                  \n"
};

const GLchar* fs[] = {
    "#version 410 core                                                  \n"
    "                                                                   \n"
    "out vec4 color;                                                    \n"
    "                                                                   \n"
    "in VS_OUT                                                          \n"
    "{                                                                  \n"
    "    vec4 color;                                                    \n"
    "} fs_in;                                                           \n"
    "                                                                   \n"
    "void main(void)                                                    \n"
    "{                                                                  \n"
    "    color = fs_in.color;                                           \n"
    "}                                                                  \n"
};

GLuint CompileShader() {
    auto vs_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs_id, 1, vs, nullptr);
    glCompileShader(vs_id);

    auto fs_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs_id, 1, fs, nullptr);
    glCompileShader(fs_id);

    auto program = glCreateProgram();
    glAttachShader(program, vs_id);
    glAttachShader(program, fs_id);
    glLinkProgram(program);

    glDeleteShader(vs_id);
    glDeleteShader(fs_id);
    return program;
}

class MyApplication final : public sb6::application {
public:
    void startup() override final {
        m_rendering_program = CompileShader();
        m_mv_location = glGetUniformLocation(m_rendering_program, "mv_matrix");
        m_proj_location = glGetUniformLocation(m_rendering_program, "proj_matrix");
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        constexpr GLfloat vertex_positions[] = {
            -0.25f,  0.25f, -0.25f,
            -0.25f, -0.25f, -0.25f,
            0.25f, -0.25f, -0.25f,

            0.25f, -0.25f, -0.25f,
            0.25f,  0.25f, -0.25f,
            -0.25f,  0.25f, -0.25f,

            0.25f, -0.25f, -0.25f,
            0.25f, -0.25f,  0.25f,
            0.25f,  0.25f, -0.25f,

            0.25f, -0.25f,  0.25f,
            0.25f,  0.25f,  0.25f,
            0.25f,  0.25f, -0.25f,

            0.25f, -0.25f,  0.25f,
            -0.25f, -0.25f,  0.25f,
            0.25f,  0.25f,  0.25f,

            -0.25f, -0.25f,  0.25f,
            -0.25f,  0.25f,  0.25f,
            0.25f,  0.25f,  0.25f,

            -0.25f, -0.25f,  0.25f,
            -0.25f, -0.25f, -0.25f,
            -0.25f,  0.25f,  0.25f,

            -0.25f, -0.25f, -0.25f,
            -0.25f,  0.25f, -0.25f,
            -0.25f,  0.25f,  0.25f,

            -0.25f, -0.25f,  0.25f,
            0.25f, -0.25f,  0.25f,
            0.25f, -0.25f, -0.25f,

            0.25f, -0.25f, -0.25f,
            -0.25f, -0.25f, -0.25f,
            -0.25f, -0.25f,  0.25f,

            -0.25f,  0.25f, -0.25f,
            0.25f,  0.25f, -0.25f,
            0.25f,  0.25f,  0.25f,

            0.25f,  0.25f,  0.25f,
            -0.25f,  0.25f,  0.25f,
            -0.25f,  0.25f, -0.25f
        };
        glGenBuffers(1, &m_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_positions), vertex_positions, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, nullptr);
        glEnableVertexAttribArray(0);

        m_proj_matrix = vmath::perspective(50.f, (float)info.windowWidth / info.windowHeight,
            0.1f, 1000.f);
        glEnable(GL_DEPTH_TEST);
    }

    void shutdown() override final {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteProgram(m_rendering_program);
    }

    void render(double current_time) override final {
        constexpr GLfloat back[]{ 0, 0.25f, 0, 1.f };
        glClearBufferfv(GL_COLOR, 0, back);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_rendering_program);
        glUniformMatrix4fv(m_proj_location, 1, false, m_proj_matrix);

        {
            float f = static_cast<float>(current_time) * 0.314159f;
            auto pre_matrix = 
                vmath::rotate((float)current_time * 45.0f, 0.0f, 1.0f, 0.0f) *
                vmath::rotate((float)current_time * 81.0f, 1.0f, 0.0f, 0.0f) *
                vmath::scale(0.5f);
            for (auto i = 0; i < 24; ++i) {
                {
                    auto mv_matrix = vmath::translate(sinf(2.1f * f) * 0.5f, 
                        cosf(1.7f * f) * 0.5f, 
                        sinf(1.3f * f) * cosf(1.5f * f) * 2.0f -4.0f) * pre_matrix;
                    glUniformMatrix4fv(m_mv_location, 1, false, mv_matrix);
                }
                glDrawArrays(GL_TRIANGLES, 0, 36);
                ++f;
            }
        }
    }

private:
    GLuint m_rendering_program;
    GLuint m_vao;
    GLuint m_buffer;

    GLuint m_mv_location;
    GLuint m_proj_location;

    vmath::mat4 m_proj_matrix;
};

DECLARE_MAIN(MyApplication);