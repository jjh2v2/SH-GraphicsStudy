#include <utility>
// sb6 libraries
#include <sb6.h>
#include <vmath.h>
#include <shader.h>

class MultiScissor final : public sb6::application {
public:
    void init() override final {
        constexpr const char k_title[] { "Multiple Scissors" };
        sb6::application::init();
        memcpy(info.title, k_title, sizeof(k_title));
    }

    void startup() override final {
        SetupShaderProgram();
        SetupUniformLocations();

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        SetupBuffers();
        glBindVertexArray(0);

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }

    void render(double current_time) override final {
        // Clear buffer values.
        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, info.windowWidth, info.windowHeight);

        constexpr GLfloat depth_init_value { 1.0f };
        constexpr GLfloat black_color[] { 0.f, 0.f, 0.f, 0.f };
        glClearBufferfv(GL_COLOR, 0, black_color);
        glClearBufferfv(GL_DEPTH, 0, &depth_init_value);

        // Turn on scissor test.
        glEnable(GL_SCISSOR_TEST);

        const int scissor_width   { (7 * info.windowWidth) / 16 };
        const int scissor_height  { (7 * info.windowHeight) / 16 };

        // Set scissor rectangle onto viewport buffer.
        glScissorIndexed(0, 0, 0, scissor_width, scissor_height);
        glScissorIndexed(1, info.windowWidth - scissor_width, 0, scissor_width, scissor_height);
        glScissorIndexed(2, 0, info.windowHeight - scissor_height, scissor_width, scissor_height);
        glScissorIndexed(3, info.windowWidth - scissor_width, info.windowHeight - scissor_height, 
                         scissor_width, scissor_height);
        // Render
        glUseProgram(m_program);
        glBindVertexArray(m_vao);
        UpdateProjectionMatrix(current_time);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
    }

    void shutdown() override final {
        glDeleteBuffers(1, &m_position_buffer);
        glDeleteBuffers(1, &m_index_buffer);
        glDeleteBuffers(1, &m_uniform_buffer);
        glDeleteVertexArrays(1, &m_vao);
        glDeleteProgram(m_program);
    }

private:
    void SetupShaderProgram() {
        constexpr const char* k_vs_path { "Chapter9/_921.vert" };
        constexpr const char* k_gs_path { "Chapter9/_921.geom" };
        constexpr const char* k_fs_path { "Chapter9/_921.frag" };

        auto vert_shader { sb6::shader::load(k_vs_path, GL_VERTEX_SHADER) };
        auto geom_shader { sb6::shader::load(k_gs_path, GL_GEOMETRY_SHADER) };
        auto frag_shader { sb6::shader::load(k_fs_path, GL_FRAGMENT_SHADER) };

        m_program = glCreateProgram();
        glAttachShader(m_program, vert_shader);
        glAttachShader(m_program, geom_shader);
        glAttachShader(m_program, frag_shader);
        glLinkProgram(m_program);

        glDeleteShader(vert_shader);
        glDeleteShader(geom_shader);
        glDeleteShader(frag_shader);
    }

    void SetupUniformLocations() {
        m_mv_location.second    = glGetUniformLocation(m_program, m_mv_location.first);
        m_proj_location.second  = glGetUniformLocation(m_program, m_proj_location.first);
    }

    void SetupBuffers() {
        constexpr GLushort vertex_indices[] {
            0, 1, 2,
            2, 1, 3,
            2, 3, 4,
            4, 3, 5,
            4, 5, 6,
            6, 5, 7,
            6, 7, 0,
            0, 7, 1,
            6, 0, 2,
            2, 4, 6,
            7, 5, 3,
            7, 3, 1
        };

        constexpr GLfloat vertex_positions[] {
            -0.25f, -0.25f, -0.25f,
            -0.25f,  0.25f, -0.25f,
             0.25f, -0.25f, -0.25f,
             0.25f,  0.25f, -0.25f,
             0.25f, -0.25f,  0.25f,
             0.25f,  0.25f,  0.25f,
            -0.25f, -0.25f,  0.25f,
            -0.25f,  0.25f,  0.25f,
        };

        glGenBuffers(1, &m_position_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_position_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_positions), vertex_positions, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        glGenBuffers(1, &m_index_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertex_indices), vertex_indices, GL_STATIC_DRAW);

        glGenBuffers(1, &m_uniform_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, m_uniform_buffer);
        glBufferData(GL_UNIFORM_BUFFER, 4 * sizeof(vmath::mat4), nullptr, GL_DYNAMIC_DRAW);
    }

    void UpdateProjectionMatrix(double current_time) {
        static const vmath::mat4 proj_matrix = 
            vmath::perspective(50.0f, (float)info.windowWidth / (float)info.windowHeight, 0.1f, 1000.0f);

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_uniform_buffer);
        // Map projection * view matrix for four scissored screen.
        auto mv_matrix_array{ 
            static_cast<vmath::mat4*>(
                glMapBufferRange(GL_UNIFORM_BUFFER, 0, 4 * sizeof(vmath::mat4),
                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)
            )
        };

        for (auto i = 0; i < 4; i++) {
            mv_matrix_array[i] = proj_matrix *
                                 vmath::translate(0.0f, 0.0f, -2.0f) *
                                 vmath::rotate((float)current_time * 45.0f * (float)(i + 1), 0.0f, 1.0f, 0.0f) *
                                 vmath::rotate((float)current_time * 81.0f * (float)(i + 1), 1.0f, 0.0f, 0.0f);
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }

private:
    GLuint  m_program{ 0 };
    GLuint  m_vao{ 0 };
    GLuint  m_position_buffer{ 0 };
    GLuint  m_index_buffer{ 0 };
    GLuint  m_uniform_buffer{ 0 };

    std::pair<const char*, GLint> m_mv_location     { "mv_matrix", 0 };
    std::pair<const char*, GLint> m_proj_location   { "proj_matrix", 0 };
};

DECLARE_MAIN(MultiScissor)