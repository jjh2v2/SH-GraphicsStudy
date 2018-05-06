#include <sb6.h>
#include <vmath.h>
#include <sb6ktx.h>
#include <shader.h>
#include <object.h>

#include <string>

struct CTransformBuffer {
    vmath::mat4 proj_matrix;
    vmath::mat4 mv_matrix[16];
};


class MultiLayerDonuts final : public sb6::application {
public:
    void init() override final {
        application::init();

        constexpr const char title[]{"Multi-layered rendering"};
        memcpy(info.title, title, sizeof(title));
    }

    void startup(void) override final {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        LoadShaders();

        constexpr const char torus_mesh_path[]{"media/objects/torus.sbm"};
        obj.load(torus_mesh_path);

        glGenBuffers(1, &m_transform_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, m_transform_ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(CTransformBuffer), NULL, GL_DYNAMIC_DRAW);

        glGenTextures(1, &array_texture);
        glBindTexture(GL_TEXTURE_2D_ARRAY, array_texture);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 256, 256, 16);

        glGenTextures(1, &array_depth);
        glBindTexture(GL_TEXTURE_2D_ARRAY, array_depth);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT32, 256, 256, 16);

        glGenFramebuffers(1, &m_layered_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_layered_fbo);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, array_texture, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, array_depth, 0);
    }

    void render(double t) override final {
        constexpr GLfloat gray[]    { 0.1f, 0.1f, 0.1f, 1.0f };

        UpdateMatrixBuffers(t);
        RenderLayerFragments();

        // On-screen rendering.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // Set to draw pixels into back-buffer. (a.k.a double buffering)
        glDrawBuffer(GL_BACK);

        glUseProgram(program_showlayers);

        glViewport(0, 0, info.windowWidth, info.windowHeight);
        glClearBufferfv(GL_COLOR, 0, gray);

        glBindTexture(GL_TEXTURE_2D_ARRAY, array_texture);
        glDisable(GL_DEPTH_TEST);

        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 16);

        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        glBindVertexArray(0);
    }

    void shutdown(void) noexcept override final {
        glDeleteProgram(program_showlayers);
        glDeleteProgram(program_gslayers);
        glDeleteBuffers(1, &m_transform_ubo);
        glDeleteVertexArrays(1, &vao);
    }

    void onKey(int key, int action) override final {
        if (!action) return;

        switch (key) { 
            case '1': 
            case '2': mode = key - '1'; break;
            case 'R': LoadShaders(); break;
            case 'M': mode = (mode + 1) % 2; break;
        }
    }

    void LoadShaders() {
        InstantiateTotalRenderShader();
        InstantiateLayerRenderShader();
    }

    void InstantiateTotalRenderShader() {
        if (program_showlayers) glDeleteProgram(program_showlayers);

        program_showlayers = glCreateProgram();

        const GLuint vs = sb6::shader::load("media/shaders/gslayers/showlayers.vs.glsl", GL_VERTEX_SHADER);
        const GLuint fs = sb6::shader::load("media/shaders/gslayers/showlayers.fs.glsl", GL_FRAGMENT_SHADER);

        glAttachShader(program_showlayers, vs);
        glAttachShader(program_showlayers, fs);
        glLinkProgram(program_showlayers);

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    void InstantiateLayerRenderShader() {
        if (program_gslayers)
            glDeleteProgram(program_gslayers);

        program_gslayers = glCreateProgram();

        const GLuint vs = sb6::shader::load("media/shaders/gslayers/gslayers.vs.glsl", GL_VERTEX_SHADER);
        const GLuint gs = sb6::shader::load("media/shaders/gslayers/gslayers.gs.glsl", GL_GEOMETRY_SHADER);
        const GLuint fs = sb6::shader::load("media/shaders/gslayers/gslayers.fs.glsl", GL_FRAGMENT_SHADER);

        glAttachShader(program_gslayers, vs);
        glAttachShader(program_gslayers, gs);
        glAttachShader(program_gslayers, fs);

        glLinkProgram(program_gslayers);

        glDeleteShader(vs);
        glDeleteShader(gs);
        glDeleteShader(fs);
    }

    void UpdateMatrixBuffers(double t) {
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_transform_ubo);

        auto buffer = static_cast<CTransformBuffer*>(
            glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(CTransformBuffer), 
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

        buffer->proj_matrix = vmath::perspective(50.0f, 1.0f, 0.1f, 1000.0f); // proj_matrix;

        for (auto i = 0; i < 16; i++) {
            float fi = (float)(i + 12) / 16.0f;
            buffer->mv_matrix[i] = vmath::translate(0.0f, 0.0f, -4.0f) *
                                   vmath::rotate((float)t * 25.0f * fi, 0.0f, 0.0f, 1.0f) *
                                   vmath::rotate((float)t * 30.0f * fi, 1.0f, 0.0f, 0.0f);
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }

    void RenderLayerFragments() {
        // Off-screen rendering.
        glBindFramebuffer(GL_FRAMEBUFFER, m_layered_fbo);
        // Set to draw rendered pixels into GL_COLOR_ATTACHMENT0~.
        constexpr GLenum ca0 = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &ca0);
        glViewport(0, 0, 256, 256);

        constexpr GLfloat black[]   { 0.0f, 0.0f, 0.0f, 1.0f };
        constexpr GLfloat one       { 1.0f };
        glClearBufferfv(GL_COLOR, 0, black);
        glClearBufferfv(GL_DEPTH, 0, &one);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glUseProgram(program_gslayers);
        obj.render();
    }

private:
    GLuint      program_gslayers = 0;
    GLuint      program_showlayers = 0;
    GLuint      vao = 0;
    int         mode = 0;
    GLuint      m_transform_ubo = 0;
    GLuint      m_layered_fbo = 0;
    GLuint      array_texture = 0;
    GLuint      array_depth = 0;

    sb6::object obj;
};

DECLARE_MAIN(MultiLayerDonuts);
