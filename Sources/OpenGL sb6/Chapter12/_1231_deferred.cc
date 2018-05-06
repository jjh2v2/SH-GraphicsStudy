#include <sb6.h>
#include <vmath.h>

#include <object.h>
#include <shader.h>
#include <sb6ktx.h>

#ifndef _WIN32
#define _WIN32 0
#endif

enum {
    MAX_DISPLAY_WIDTH       = 2048,
    MAX_DISPLAY_HEIGHT      = 2048,
    NUM_LIGHTS              = 64,
    NUM_INSTANCES           = (15 * 15)
};

class DeferredShading final : public sb6::application {
public:
    DeferredShading() : render_program_nm(0), render_program(0), light_program(0), use_nm(true), paused(false) {}

protected:
    void init() override final {
        application::init();

        constexpr const char title[] = "OpenGL SuperBible - Deferred Shading";
        memcpy(info.title, title, sizeof(title));
    }

    void startup() override final {
        LoadShaders();

        object.load("media/objects/ladybug.sbm");
        using sb6::ktx::file::load;
        tex_nm      = load("media/textures/ladybug_nm.ktx");
        tex_diffuse = load("media/textures/ladybug_co.ktx");

        GenerateGBuffer();

        glGenVertexArrays(1, &fs_quad_empty_vao);
        glBindVertexArray(fs_quad_empty_vao);

        glGenBuffers(1, &light_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, light_ubo);
        glBufferData(GL_UNIFORM_BUFFER, NUM_LIGHTS * sizeof(DLight), NULL, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &render_transform_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, render_transform_ubo);
        glBufferData(GL_UNIFORM_BUFFER, (2 + NUM_INSTANCES) * sizeof(vmath::mat4), NULL, GL_DYNAMIC_DRAW);
    }

    void RenderGBuffer(float t) {
        static const GLuint uint_zeros[] = { 0, 0, 0, 0 };
        static const GLfloat float_zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        static const GLfloat float_ones[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

        glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
        glViewport(0, 0, info.windowWidth, info.windowHeight);
        glDrawBuffers(2, draw_buffers);
        glClearBufferuiv(GL_COLOR, 0, uint_zeros);
        glClearBufferuiv(GL_COLOR, 1, uint_zeros);
        glClearBufferfv(GL_DEPTH, 0, float_ones);

        UpdateMatrixesRenderTransformUbo(t);

        glUseProgram(use_nm ? render_program_nm : render_program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_diffuse);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_nm);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        object.render(NUM_INSTANCES);
    }

    void UpdateMatrixesRenderTransformUbo(float t) {
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, render_transform_ubo);
        vmath::mat4* matrices = reinterpret_cast<vmath::mat4*>(glMapBufferRange(GL_UNIFORM_BUFFER, 0, 
            (2 + NUM_INSTANCES) * sizeof(vmath::mat4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

        matrices[0] = vmath::perspective(50.0f, (float)info.windowWidth / (float)info.windowHeight, 0.1f, 1000.0f);
        const float d = (sinf(t * 0.131f) + 2.0f) * 0.15f;
        vmath::vec3 eye_pos = vmath::vec3(d * 120.0f * sinf(t * 0.11f), 5.5f, d * 120.0f * cosf(t * 0.01f));
        matrices[1] = vmath::lookat(eye_pos, vmath::vec3(0.0f, -20.0f, 0.0f), vmath::vec3(0.0f, 1.0f, 0.0f));

        for (auto j = 0; j < 15; j++) {
            for (auto i = 0; i < 15; i++) {
                matrices[j * 15 + i + 2] = vmath::translate((i - 7.5f) * 7.0f, 0.0f, (j - 7.5f) * 11.0f);
            }
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }

    void UpdateMatrixesLightUbo(float t) {
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, light_ubo);
        DLight* lights = reinterpret_cast<DLight *>(glMapBufferRange(GL_UNIFORM_BUFFER, 0, 
            NUM_LIGHTS * sizeof(DLight), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

        for (auto i = 0; i < NUM_LIGHTS; i++) {
            float i_f = ((float)i - 7.5f) * 0.1f + 0.3f;
            lights[i].position = vmath::vec3(100.0f * sinf(t * 1.1f + (5.0f * i_f)) * cosf(t * 2.3f + (9.0f * i_f)),
                                             15.0f,
                                             100.0f * sinf(t * 1.5f + (6.0f * i_f)) * cosf(t * 1.9f + (11.0f * i_f))); // 300.0f * sinf(t * i_f * 0.7f) * cosf(t * i_f * 0.9f) - 600.0f);
            lights[i].color = vmath::vec3(cosf(i_f * 14.0f) * 0.5f + 0.8f,
                                          sinf(i_f * 17.0f) * 0.5f + 0.8f,
                                          sinf(i_f * 13.0f) * cosf(i_f * 19.0f) * 0.5f + 0.8f);
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }

    void render(double currentTime) override final {
        static double last_time = 0.0;
        static double total_time = 0.0;

        if (!paused) total_time += (currentTime - last_time);
        else {
            if constexpr (_WIN32) Sleep(10);
        }

        last_time = currentTime;

        RenderGBuffer(static_cast<float>(total_time));

        // Final Render
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, info.windowWidth, info.windowHeight);
        glDrawBuffer(GL_BACK);
        glDisable(GL_DEPTH_TEST);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbuffer_tex[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gbuffer_tex[1]);

        if (vis_mode == EVisibilityMode::VIS_OFF)
            glUseProgram(light_program);
        else {
            glUseProgram(vis_program);
            glUniform1i(loc_vis_mode, static_cast<int>(vis_mode));
        }

        UpdateMatrixesLightUbo(static_cast<float>(total_time));

        glBindVertexArray(fs_quad_empty_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // Release bind status.
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void shutdown() override final {
        glDeleteTextures(3, &gbuffer_tex[0]);
        glDeleteFramebuffers(1, &gbuffer);
        glDeleteProgram(render_program);
        glDeleteProgram(light_program);
    }

    void onKey(int key, int action) override final {
        if (action) {
            switch (key) {
                case 'R': LoadShaders(); break;
                case 'P': paused = !paused; break;
                case 'N': use_nm = !use_nm; break;
                case '1': vis_mode = EVisibilityMode::VIS_OFF; break;
                case '2': vis_mode = EVisibilityMode::VIS_NORMALS; break;
                case '3': vis_mode = EVisibilityMode::VIS_WS_COORDS; break;
                case '4': vis_mode = EVisibilityMode::VIS_DIFFUSE; break;
                case '5': vis_mode = EVisibilityMode::VIS_META; break;
                default: break;
            }
        }
    }

private:
    void LoadShaders() {
        GLuint vs, fs;

        vs = sb6::shader::load("media/shaders/deferredshading/render.vs.glsl", GL_VERTEX_SHADER);
        fs = sb6::shader::load("media/shaders/deferredshading/render.fs.glsl", GL_FRAGMENT_SHADER);

        if (render_program) glDeleteProgram(render_program);
        render_program = glCreateProgram();
        glAttachShader(render_program, vs);
        glAttachShader(render_program, fs);
        glLinkProgram(render_program);

        glDeleteShader(vs);
        glDeleteShader(fs);

        vs = sb6::shader::load("media/shaders/deferredshading/render-nm.vs.glsl", GL_VERTEX_SHADER);
        fs = sb6::shader::load("media/shaders/deferredshading/render-nm.fs.glsl", GL_FRAGMENT_SHADER);

        render_program_nm = glCreateProgram();
        glAttachShader(render_program_nm, vs);
        glAttachShader(render_program_nm, fs);
        glLinkProgram(render_program_nm);

        glDeleteShader(vs);
        glDeleteShader(fs);

        vs = sb6::shader::load("media/shaders/deferredshading/light.vs.glsl", GL_VERTEX_SHADER);
        fs = sb6::shader::load("media/shaders/deferredshading/light.fs.glsl", GL_FRAGMENT_SHADER);

        if (light_program) glDeleteProgram(light_program);
        light_program = glCreateProgram();
        glAttachShader(light_program, vs);
        glAttachShader(light_program, fs);
        glLinkProgram(light_program);

        glDeleteShader(fs);

        fs = sb6::shader::load("media/shaders/deferredshading/render-vis.fs.glsl", GL_FRAGMENT_SHADER);

        vis_program = glCreateProgram();
        glAttachShader(vis_program, vs);
        glAttachShader(vis_program, fs);
        glLinkProgram(vis_program);

        loc_vis_mode = glGetUniformLocation(vis_program, "vis_mode");

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    void GenerateGBuffer() {
        glGenFramebuffers(1, &gbuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);

        glGenTextures(3, gbuffer_tex);

        // gbuffer_tex[0] saves three 16bits normal into R 32bit and G 16bit, 
        // 16bits albedo colors into G latter 16bit and B 
        // and 32bit material index as final item.
        glBindTexture(GL_TEXTURE_2D, gbuffer_tex[0]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32UI, MAX_DISPLAY_WIDTH, MAX_DISPLAY_HEIGHT); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // gbuffer_tex[1] saves three 32bits floating point world coordinate and 32bit specular power value.
        glBindTexture(GL_TEXTURE_2D, gbuffer_tex[1]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, MAX_DISPLAY_WIDTH, MAX_DISPLAY_HEIGHT); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Just depth buffer. :)
        glBindTexture(GL_TEXTURE_2D, gbuffer_tex[2]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, MAX_DISPLAY_WIDTH, MAX_DISPLAY_HEIGHT); 

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer_tex[0], 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gbuffer_tex[1], 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gbuffer_tex[2], 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

private:
    GLuint      gbuffer;
    GLuint      gbuffer_tex[3];
    GLuint      fs_quad_empty_vao;

    sb6::object object;

    GLuint      render_program;
    GLuint      render_program_nm;
    GLuint      render_transform_ubo;

    GLuint      light_program;
    GLuint      light_ubo;

    GLuint      vis_program;
    GLint       loc_vis_mode;

    GLuint      tex_diffuse;
    GLuint      tex_nm;

    bool        use_nm;
    bool        paused;

    enum class EVisibilityMode : int {
        VIS_OFF = 0,
        VIS_NORMALS,
        VIS_WS_COORDS,
        VIS_DIFFUSE,
        VIS_META
    };
    EVisibilityMode vis_mode = EVisibilityMode::VIS_OFF;

#pragma pack (push, 1)
    struct DLight {
        vmath::vec3         position;
        unsigned int        : 32;       // pad0
        vmath::vec3         color;
        unsigned int        : 32;       // pad1
    };
#pragma pack (pop)
};

DECLARE_MAIN(DeferredShading)