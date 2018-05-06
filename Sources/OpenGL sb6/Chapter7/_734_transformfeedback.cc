#include <array>
#include <memory>

#include <sb6.h>
#include <vmath.h>
#include <shader.h>

enum BUFFER_TYPE_t {
    POSITION_A, POSITION_B,
    VELOCITY_A, VELOCITY_B,
    CONNECTION
};

constexpr const char k_title[]{ "Spring-mass simuation" };
constexpr const char* k_vs_update_path{ "media/shaders/springmass/update.vs.glsl" };
constexpr const char* k_vs_render_path{ "media/shaders/springmass/render.vs.glsl" };
constexpr const char* k_fs_render_path{ "media/shaders/springmass/render.fs.glsl" };

constexpr unsigned k_point_x    { 50u };
constexpr unsigned k_point_y    { 50u };
constexpr unsigned k_point_total{ k_point_x * k_point_y };
constexpr unsigned k_total_connection{ (k_point_x - 1) * k_point_y + (k_point_y - 1) * k_point_x };
constexpr unsigned k_lines{ (k_point_x - 1) * k_point_y + (k_point_y - 1) * k_point_x };

constexpr const GLfloat black[] { 0.0f, 0.0f, 0.0f, 0.0f };

template <typename _Ty>
constexpr unsigned total_byte_size = sizeof(_Ty) * k_point_total;

class SpringMass final : public sb6::application {
public:
    void init() override final {
        sb6::application::init();
        memcpy(info.title, k_title, sizeof(k_title));
    }

    void startup() override final {
        LoadShaders();
        MakeInitialPositionWithConnection();

        glGenTextures(2, m_pos_tbo.data());
        glBindTexture(GL_TEXTURE_BUFFER, m_pos_tbo[0]);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_vbo[POSITION_A]);
        glBindTexture(GL_TEXTURE_BUFFER, m_pos_tbo[1]);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_vbo[POSITION_B]);

        // Generate Index buffer for line element drawing.
        glGenBuffers(1, &m_index_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, k_lines * 2 * sizeof(int), NULL, GL_STATIC_DRAW);

        int* e = (int*)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, k_lines * 2 * sizeof(int), 
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        for (auto j = 0; j < k_point_y; j++) {
            for (auto i = 0; i < k_point_x - 1; i++) {
                *e = i + j * k_point_x;
                e++;
                *e = 1 + i + j * k_point_y;
                e++;
            }
        }

        for (auto i = 0; i < k_point_x; i++) {
            for (auto j = 0; j < k_point_y - 1; j++) {
                *e = i + j * k_point_x;
                e++;
                *e = k_point_x + i + j * k_point_x;
                e++;
            }
        }

        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    }

    void shutdown() override final {
        glDeleteProgram(m_update_program);
        glDeleteProgram(m_render_program);
        glDeleteBuffers(5, m_vbo.data());
        glDeleteBuffers(2, m_pos_tbo.data());
        glDeleteBuffers(1, &m_index_buffer);
        glDeleteVertexArrays(2, m_vao.data());
    }

    void render(double dt) override final {
        // Render update shader program till frontend stages.
        glUseProgram(m_update_program);
        glEnable(GL_RASTERIZER_DISCARD);

        for (auto i = m_iterations_per_frame; i != 0; --i) {
            glBindVertexArray(m_vao[m_iteration_index & 1]);
            glBindTexture(GL_TEXTURE_BUFFER, m_pos_tbo[m_iteration_index & 1]);
            m_iteration_index++;
            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_vbo[POSITION_A + (m_iteration_index & 1)]);
            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_vbo[VELOCITY_A + (m_iteration_index & 1)]);
            // Begin transfrom feedback buffer storing.
            glBeginTransformFeedback(GL_POINTS);
            glDrawArrays(GL_POINTS, 0, k_point_total);
            glEndTransformFeedback();
        }

        glDisable(GL_RASTERIZER_DISCARD);

        // Render actual render shader program.
        glViewport(0, 0, info.windowWidth, info.windowHeight);
        glClearBufferfv(GL_COLOR, 0, black);
        glUseProgram(m_render_program);

        if (m_draw_points) {
            glPointSize(4.0f);
            glDrawArrays(GL_POINTS, 0, k_point_total);
        }

        if (m_draw_lines) {
            glLineWidth(2.f);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
            glDrawElements(GL_LINES, k_total_connection * 2, GL_UNSIGNED_INT, NULL);
        }
    }

private:
    GLuint m_update_program{ 0 };
    GLuint m_render_program{ 0 };

    GLuint m_index_buffer;
    std::array<GLuint, 2> m_vao;
    std::array<GLuint, 5> m_vbo;
    std::array<GLuint, 2> m_pos_tbo;

    GLuint m_iteration_index{ 0u };
    const int m_iterations_per_frame{ 16 };
    const bool m_draw_points{ true };
    const bool m_draw_lines { true };

private:
    void onKey(int key, int action) override final {};

    void LoadShaders() {
        // Initialize Update Shader Program.
        // If there is already update shader program, delete it.
        if (m_update_program) glDeleteProgram(m_update_program);
        m_update_program = glCreateProgram();

        {
            GLuint vertex_shader    { sb6::shader::load(k_vs_update_path, GL_VERTEX_SHADER) };
            glAttachShader(m_update_program, vertex_shader);
            const char* transform_feedback_varyings[] { "tf_position_mass", "tf_velocity" };
            glTransformFeedbackVaryings(m_update_program, 2, transform_feedback_varyings, GL_SEPARATE_ATTRIBS);
            glLinkProgram(m_update_program);
            glDeleteShader(vertex_shader);
        }

        // Initialize Render Shader Program.
        if (m_render_program) glDeleteProgram(m_render_program);
        m_render_program = glCreateProgram();

        {
            GLuint vertex_shader    { sb6::shader::load(k_vs_render_path, GL_VERTEX_SHADER) };
            GLuint fragment_shader  { sb6::shader::load(k_fs_render_path, GL_FRAGMENT_SHADER) };
            glAttachShader(m_render_program, vertex_shader);
            glAttachShader(m_render_program, fragment_shader);
            glLinkProgram(m_render_program);
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);
        }
    }

    void MakeInitialPositionWithConnection() {
        auto initial_positions  { std::make_unique<vmath::vec4[]>(k_point_total) };
        auto initial_velocities { std::make_unique<vmath::vec3[]>(k_point_total) };
        auto connection_vectors { std::make_unique<vmath::ivec4[]>(k_point_total) };

        for (auto y = 0u, n = 0u; y < k_point_y; ++y) {
            float fy{ static_cast<float>(y) / k_point_y };

            for (auto x = 0u; x < k_point_x; ++x) {
                float fx{ static_cast<float>(x) / k_point_x };

                initial_positions[n] = vmath::vec4(
                    (fx - .5f) * static_cast<float>(k_point_x),
                    (fy - .5f) * static_cast<float>(k_point_y),
                    .6f * sinf(fx) * cosf(fy),
                    1.f);
                initial_velocities[n] = vmath::vec3(.0f);
                connection_vectors[n] = vmath::ivec4(-1);

                if (y != (k_point_y - 1)) {
                    if (x != 0)                 connection_vectors[n][0] = n - 1;
                    if (x != (k_point_x - 1))   connection_vectors[n][2] = n + 1;
                    if (y != 0)                 connection_vectors[n][1] = n - k_point_x;
                    if (y != (k_point_y - 1))   connection_vectors[n][3] = n + k_point_x;
                }
                ++n;
            }
        }

        glGenVertexArrays(2, m_vao.data());
        glGenBuffers(5, m_vbo.data());

        for (auto i = 0; i < 2; i++) {
            glBindVertexArray(m_vao[i]);

            glBindBuffer(GL_ARRAY_BUFFER, m_vbo[POSITION_A + i]);
            glBufferData(GL_ARRAY_BUFFER, total_byte_size<vmath::vec4>, initial_positions.get(), GL_DYNAMIC_COPY);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ARRAY_BUFFER, m_vbo[VELOCITY_A + i]);
            glBufferData(GL_ARRAY_BUFFER, total_byte_size<vmath::vec3>, initial_velocities.get(), GL_DYNAMIC_COPY);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ARRAY_BUFFER, m_vbo[CONNECTION]);
            glBufferData(GL_ARRAY_BUFFER, total_byte_size<vmath::ivec4>, connection_vectors.get(), GL_STATIC_DRAW);
            glVertexAttribIPointer(2, 4, GL_INT, 0, NULL);
            glEnableVertexAttribArray(2);
        }
    }
};

DECLARE_MAIN(SpringMass);