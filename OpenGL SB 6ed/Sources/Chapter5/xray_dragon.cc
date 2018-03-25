#include <cmath>
#include <chrono>
#include <random>

#include <object.h>
#include <shader.h>
#include <sb6.h>
#include <sb6ktx.h>
#include <vmath.h>

constexpr unsigned size = 800;
constexpr unsigned size_2 = size * size;

struct uniforms_block
{
    vmath::mat4     mv_matrix;
    vmath::mat4     view_matrix;
    vmath::mat4     proj_matrix;
};

class MyApplication final : public sb6::application {
public:
    void startup() override final;
    void render(double current_time) override final;

    void shutdown() override final {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteProgram(m_clear_program);
        glDeleteProgram(m_append_program);
        glDeleteProgram(m_resolve_program);
    }

private:
    GLuint m_clear_program;
    GLuint m_append_program;
    GLuint m_resolve_program;

    GLuint m_uniforms_buffer;
    GLuint m_fragment_buffer;
    GLuint m_head_pointer_image;
    GLuint m_atomic_counter_buffer;
    GLuint m_vao;
    GLuint m_buffer;

    GLuint m_uniform_mvp;

    sb6::object m_object;

    void CompileShader();
};

void MyApplication::startup() {
    CompileShader();
    glGenBuffers(1, &m_uniforms_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, m_uniforms_buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(uniforms_block), NULL, GL_DYNAMIC_DRAW);

    m_object.load("media/objects/dragon.sbm");

    glGenBuffers(1, &m_fragment_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fragment_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size_2 * 16, NULL, GL_DYNAMIC_COPY);

    glGenBuffers(1, &m_atomic_counter_buffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomic_counter_buffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, 4, NULL, GL_DYNAMIC_COPY);

    glGenTextures(1, &m_head_pointer_image);
    glBindTexture(GL_TEXTURE_2D, m_head_pointer_image);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, size, size);

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
}

void MyApplication::render(double current_time) {
    static const GLfloat zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    static const GLfloat gray[] = { 0.1f, 0.1f, 0.1f, 0.0f };
    static const GLfloat ones[] = { 1.0f };
    const float f = (float)current_time;

    glViewport(0, 0, info.windowWidth, info.windowHeight);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(m_clear_program);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glUseProgram(m_append_program);

    vmath::mat4 model_matrix = vmath::scale(7.0f);
    vmath::vec3 view_position = vmath::vec3(cosf(f * 0.35f) * 120.0f, cosf(f * 0.4f) * 30.0f, sinf(f * 0.35f) * 120.0f);
    vmath::mat4 view_matrix = vmath::lookat(view_position,
                                            vmath::vec3(0.0f, 30.0f, 0.0f),
                                            vmath::vec3(0.0f, 1.0f, 0.0f));

    vmath::mat4 mv_matrix = view_matrix * model_matrix;
    vmath::mat4 proj_matrix = vmath::perspective(50.0f,
                                                 (float)info.windowWidth / (float)info.windowHeight,
                                                 0.1f,
                                                 1000.0f);

    glUniformMatrix4fv(m_uniform_mvp, 1, GL_FALSE, proj_matrix * mv_matrix);

    static const unsigned int zero = 0;
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomic_counter_buffer);
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(zero), &zero);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_fragment_buffer);

    glBindImageTexture(0, m_head_pointer_image, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    m_object.render();

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(m_resolve_program);

    glBindVertexArray(m_vao);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void MyApplication::CompileShader() {
    GLuint shaders[2];
    shaders[0] = sb6::shader::load("media/shaders/fragmentlist/clear.vs.glsl", GL_VERTEX_SHADER);
    shaders[1] = sb6::shader::load("media/shaders/fragmentlist/clear.fs.glsl", GL_FRAGMENT_SHADER);

    if (m_clear_program)
        glDeleteProgram(m_clear_program);
    m_clear_program = sb6::program::link_from_shaders(shaders, 2, true);

    shaders[0] = sb6::shader::load("media/shaders/fragmentlist/append.vs.glsl", GL_VERTEX_SHADER);
    shaders[1] = sb6::shader::load("media/shaders/fragmentlist/append.fs.glsl", GL_FRAGMENT_SHADER);

    if (m_append_program)
        glDeleteProgram(m_append_program);
    m_append_program = sb6::program::link_from_shaders(shaders, 2, true);

    m_uniform_mvp = glGetUniformLocation(m_append_program, "mvp");

    shaders[0] = sb6::shader::load("media/shaders/fragmentlist/resolve.vs.glsl", GL_VERTEX_SHADER);
    shaders[1] = sb6::shader::load("media/shaders/fragmentlist/resolve.fs.glsl", GL_FRAGMENT_SHADER);

    if (m_resolve_program)
        glDeleteProgram(m_resolve_program);
    m_resolve_program = sb6::program::link_from_shaders(shaders, 2, true);
}

DECLARE_MAIN(MyApplication);