#include <cmath>
#include <chrono>
#include <random>
#include <sb6.h>
#include <sb6ktx.h>
#include <vmath.h>

const GLchar* vs[] = {
    "#version 430 core                                                      \n"
    "layout (location = 0) in int alien_index;                              \n"
    "out VS_OUT {                                                           \n"
    "    flat int alien;                                                    \n"
    "    vec2 tc;                                                           \n"
    "} vs_out;                                                              \n"
    "                                                                       \n"
    "struct droplet_t {                                                     \n"
    "    float x_offset;                                                    \n"
    "    float y_offset;                                                    \n"
    "    float orientation;                                                 \n"
    "    float unused;                                                      \n"
    "};                                                                     \n"
    "                                                                       \n"
    "layout (std140) uniform droplets {                                     \n"
    "    droplet_t droplet[256];                                            \n"
    "};                                                                     \n"
    "                                                                       \n"
    "void main(void) {                                                      \n"
    "    const vec2[4] position = vec2[4](vec2(-0.5, -0.5),                 \n"
    "                                     vec2( 0.5, -0.5),                 \n"
    "                                     vec2(-0.5,  0.5),                 \n"
    "                                     vec2( 0.5,  0.5));                \n"
    "    vs_out.tc = position[gl_VertexID].xy + vec2(0.5);                  \n"
    "    float co = cos(droplet[alien_index].orientation);                  \n"
    "    float so = sin(droplet[alien_index].orientation);                  \n"
    "    mat2 rot = mat2(vec2(co, so),                                      \n"
    "                    vec2(-so, co));                                    \n"
    "    vec2 pos = 0.25 * rot * position[gl_VertexID];                     \n"
    "    gl_Position = vec4(pos.x + droplet[alien_index].x_offset,          \n"
    "                       pos.y + droplet[alien_index].y_offset,          \n"
    "                       0.5, 1.0);                                      \n"
    "    vs_out.alien = alien_index % 64;                                   \n"
    "}                                                                      \n"
};

const GLchar* fs[] = {
    "#version 430 core                                                      \n"
    "layout (location = 0) out vec4 color;                                  \n"
    "in VS_OUT                                                              \n"
    "{                                                                      \n"
    "    flat int alien;                                                    \n"
    "    vec2 tc;                                                           \n"
    "} fs_in;                                                               \n"
    "                                                                       \n"
    "uniform sampler2DArray tex_aliens;                                     \n"
    "void main(void)                                                        \n"
    "{                                                                      \n"
    "    color = texture(tex_aliens, vec3(fs_in.tc, float(fs_in.alien)));   \n"
    "}                                                                      \n"
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

        auto array_texture = sb6::ktx::file::load("media/textures/aliens.ktx");
        glBindTexture(GL_TEXTURE_2D_ARRAY, array_texture);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        glGenBuffers(1, &m_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, m_buffer);
        glBufferData(GL_UNIFORM_BUFFER, 256 * 16, nullptr, GL_DYNAMIC_DRAW);

        std::default_random_engine lng{};
        lng.seed(std::chrono::system_clock::now().time_since_epoch().count());
        for (int i = 0; i < 256; i++) {
            float random_float      = static_cast<float>((lng() % 200)) / 200;
            droplet_x_offset[i]     = random_float * 2.0f - 1.0f;
            droplet_rot_speed[i]    = (random_float + 0.5f) * ((i & 1) ? -3.0f : 3.0f);
            droplet_fall_speed[i]   = random_float + 0.2f;
        }

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void shutdown() override final {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteProgram(m_rendering_program);
    }

    void render(double current_time) override final {
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(m_rendering_program);
        glBindVertexArray(m_vao);

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_buffer);
        auto buffer_ptr = (vmath::vec4*) glMapBufferRange(GL_UNIFORM_BUFFER,
            0, 256 * 16,
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        auto f = static_cast<float>(current_time);
        for (auto i = 0u; i < 256; ++i) {
            buffer_ptr[i][0] = droplet_x_offset[i];
            buffer_ptr[i][1] = 2.0f - fmodf((f + float(i)) * droplet_fall_speed[i], 4.31f);
            buffer_ptr[i][2] = f * droplet_rot_speed[i];
            buffer_ptr[i][3] = 0.0f;
        }
        glUnmapBuffer(GL_UNIFORM_BUFFER);

        for (auto i = 0; i < 256; ++i) {
            glVertexAttribI1i(0, i);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }

private:
    GLuint m_rendering_program;
    GLuint m_vao;
    GLuint m_buffer;

    float droplet_x_offset[256];
    float droplet_rot_speed[256];
    float droplet_fall_speed[256];
};

DECLARE_MAIN(MyApplication);