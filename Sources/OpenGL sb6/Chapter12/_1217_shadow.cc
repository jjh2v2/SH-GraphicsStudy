#include <sb6.h>
#include <vmath.h>

#include <object.h>
#include <sb6ktx.h>
#include <shader.h>

#define DEPTH_TEXTURE_SIZE      4096
#define FRUSTUM_DEPTH           1000

constexpr GLfloat ones[] = { 1.0f };
constexpr GLfloat zero[] = { 0.0f };
constexpr GLfloat gray[] = { 0.1f, 0.1f, 0.1f, 0.0f };

class ShadowMapping final : public sb6::application {
protected:
    void init() override final {
        application::init();
        constexpr const char title[] = "OpenGL SuperBible - Shadow Mapping";
        memcpy(info.title, title, sizeof(title));
    }

    void startup() override final {
        LoadShaders();
        InitializeValues();

        static const char* const object_names[] = {
            "media/objects/dragon.sbm", "media/objects/sphere.sbm", 
            "media/objects/cube.sbm", "media/objects/torus.sbm"
        };

        for (auto i = 0; i < OBJECT_COUNT; i++) objects[i].obj.load(object_names[i]);
        GenerateDepthFramebuffer(&depth_fbo);

        // Generate Empty vao buffer
        glGenVertexArrays(1, &quad_vao);
        glBindVertexArray(quad_vao);
    }

    void InitializeValues() {
        const vmath::vec3 light_position = vmath::vec3(20.0f, 20.0f, 20.0f);
        const vmath::vec3 view_position  = vmath::vec3(0.0f, 0.0f, 40.0f);

        light_proj_matrix   = vmath::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 200.0f);
        light_view_matrix   = vmath::lookat(light_position, vmath::vec3(0.0f), vmath::vec3(0.0f, 1.0f, 0.0f));
        camera_view_matrix  = vmath::lookat(view_position, vmath::vec3(0.0f), vmath::vec3(0.0f, 1.0f, 0.0f));
    }

    void render(double currentTime) override final {
        static double last_time = 0.0;
        static double total_time = 0.0;

        if (!m_paused) total_time += (currentTime - last_time);
        last_time = currentTime;

        const float ratio = static_cast<float>(info.windowWidth) / static_cast<float>(info.windowHeight);
        camera_proj_matrix = vmath::perspective(50.0f, ratio, 1.0f, 200.0f);

        UpdateObjectsModelMatrix(static_cast<float>(total_time + 30.0f));

        glEnable(GL_DEPTH_TEST);
        // Get depth values from a view of light point.
        RenderShadowScene();

        switch (mode) {
        case RENDER_DEPTH:
            glDisable(GL_DEPTH_TEST);

            glBindVertexArray(quad_vao);
            glUseProgram(show_light_depth_program);

            glBindTexture(GL_TEXTURE_2D, depth_debug_show_texture);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            break;
        default: RenderScene(total_time); break;
        }
    }

    void onKey(int key, int action) override final {
        if (action) {
            switch (key) {
                case '1': mode = RENDER_FULL; break;
                case '2': mode = RENDER_LIGHT; break;
                case '3': mode = RENDER_DEPTH; break;
                case 'R': LoadShaders(); break;
                case 'P': m_paused = !m_paused; break;
            }
        }
    }

private:
    void GenerateDepthFramebuffer(unsigned* depth_fbo) {
        glGenFramebuffers(1, depth_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, *depth_fbo);

        // Depth buffer
        glGenTextures(1, &depth_texture);
        glBindTexture(GL_TEXTURE_2D, depth_texture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, DEPTH_TEXTURE_SIZE, DEPTH_TEXTURE_SIZE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_texture, 0);
        // Color buffer for displaying depth(z) values.
        glGenTextures(1, &depth_debug_show_texture);
        glBindTexture(GL_TEXTURE_2D, depth_debug_show_texture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, DEPTH_TEXTURE_SIZE, DEPTH_TEXTURE_SIZE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, depth_debug_show_texture, 0);

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void LoadShaders() {
        GenerateLightRenderProgram();
        GenerateViewRenderProgram();
        GenerateShadowShowRenderProgram();
    }

    void GenerateLightRenderProgram() {
        auto vs = sb6::shader::load("media/shaders/shadowmapping/shadowmapping-light.vs.glsl", GL_VERTEX_SHADER);
        auto fs = sb6::shader::load("media/shaders/shadowmapping/shadowmapping-light.fs.glsl", GL_FRAGMENT_SHADER);

        if (light_program) glDeleteProgram(light_program);
        light_program = glCreateProgram();
        glAttachShader(light_program, vs);
        glAttachShader(light_program, fs);
        glLinkProgram(light_program);

        glDeleteShader(vs);
        glDeleteShader(fs);

        uniforms.light.mvp = glGetUniformLocation(light_program, "mvp");
    }

    void GenerateViewRenderProgram() {
        auto vs = sb6::shader::load("media/shaders/shadowmapping/shadowmapping-camera.vs.glsl", GL_VERTEX_SHADER);
        auto fs = sb6::shader::load("media/shaders/shadowmapping/shadowmapping-camera.fs.glsl", GL_FRAGMENT_SHADER);

        if (view_program) glDeleteProgram(view_program);
        view_program = glCreateProgram();
        glAttachShader(view_program, vs);
        glAttachShader(view_program, fs);
        glLinkProgram(view_program);

        glDeleteShader(vs);
        glDeleteShader(fs);

        uniforms.view.proj_matrix = glGetUniformLocation(view_program, "proj_matrix");
        uniforms.view.mv_matrix = glGetUniformLocation(view_program, "mv_matrix");
        uniforms.view.shadow_matrix = glGetUniformLocation(view_program, "shadow_matrix");
        uniforms.view.full_shading = glGetUniformLocation(view_program, "full_shading");
    }

    void GenerateShadowShowRenderProgram() {
        auto vs = sb6::shader::load("media/shaders/shadowmapping/shadowmapping-light-view.vs.glsl", GL_VERTEX_SHADER);
        auto fs = sb6::shader::load("media/shaders/shadowmapping/shadowmapping-light-view.fs.glsl", GL_FRAGMENT_SHADER);

        if (show_light_depth_program) glDeleteProgram(show_light_depth_program);
        show_light_depth_program = glCreateProgram();

        glAttachShader(show_light_depth_program, vs);
        glAttachShader(show_light_depth_program, fs);
        glLinkProgram(show_light_depth_program);

        glDeleteShader(vs);
        glDeleteShader(fs); 
    }

    void UpdateObjectsModelMatrix(float f) {
        objects[0].model_matrix = vmath::rotate(f * 14.5f, 0.0f, 1.0f, 0.0f) *
                                  vmath::rotate(20.0f, 1.0f, 0.0f, 0.0f) *
                                  vmath::translate(0.0f, -4.0f, 0.0f);

        objects[1].model_matrix = vmath::rotate(f * 3.7f, 0.0f, 1.0f, 0.0f) *
                                  vmath::translate(sinf(f * 0.37f) * 12.0f, cosf(f * 0.37f) * 12.0f, 0.0f) *
                                  vmath::scale(2.0f);

        objects[2].model_matrix = vmath::rotate(f * 6.45f, 0.0f, 1.0f, 0.0f) *
                                  vmath::translate(sinf(f * 0.25f) * 10.0f, cosf(f * 0.25f) * 10.0f, 0.0f) *
                                  vmath::rotate(f * 99.0f, 0.0f, 0.0f, 1.0f) *
                                  vmath::scale(2.0f);

        objects[3].model_matrix = vmath::rotate(f * 5.25f, 0.0f, 1.0f, 0.0f) *
                                  vmath::translate(sinf(f * 0.51f) * 14.0f, cosf(f * 0.51f) * 14.0f, 0.0f) *
                                  vmath::rotate(f * 120.3f, 0.707106f, 0.0f, 0.707106f) *
                                  vmath::scale(2.0f);
    }

    void RenderShadowScene() {
        glBindFramebuffer(GL_FRAMEBUFFER, depth_fbo);
        glViewport(0, 0, DEPTH_TEXTURE_SIZE, DEPTH_TEXTURE_SIZE);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 2.0f);

        glUseProgram(light_program);
        static const GLenum buffs[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, buffs);
        glClearBufferfv(GL_COLOR, 0, zero); 
        glClearBufferfv(GL_DEPTH, 0, ones);

        vmath::mat4 light_vp_matrix = light_proj_matrix * light_view_matrix;
        for (auto i = 0; i < 4; i++) {
            vmath::mat4& model_matrix = objects[i].model_matrix;
            glUniformMatrix4fv(uniforms.light.mvp, 1, GL_FALSE, light_vp_matrix * model_matrix);
            objects[i].obj.render();
        }

        glDisable(GL_POLYGON_OFFSET_FILL);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void RenderScene(double currentTime) {
        static const vmath::mat4 scale_bias_matrix = vmath::mat4(vmath::vec4(0.5f, 0.0f, 0.0f, 0.0f),
                                                             vmath::vec4(0.0f, 0.5f, 0.0f, 0.0f),
                                                             vmath::vec4(0.0f, 0.0f, 0.5f, 0.0f),
                                                             vmath::vec4(0.5f, 0.5f, 0.5f, 1.0f));
        vmath::mat4 shadow_sbpv_matrix = scale_bias_matrix * light_proj_matrix * light_view_matrix;

        glViewport(0, 0, info.windowWidth, info.windowHeight);
        glClearBufferfv(GL_COLOR, 0, gray);

        glUseProgram(view_program);
        glUniformMatrix4fv(uniforms.view.proj_matrix, 1, GL_FALSE, camera_proj_matrix);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depth_texture);
        glDrawBuffer(GL_BACK);

        glClearBufferfv(GL_DEPTH, 0, ones);

        for (auto i = 0; i < 4; i++) {
            vmath::mat4& model_matrix = objects[i].model_matrix;
            const vmath::mat4 shadow_matrix = shadow_sbpv_matrix * model_matrix;
            glUniformMatrix4fv(uniforms.view.shadow_matrix, 1, GL_FALSE, shadow_matrix);
            glUniformMatrix4fv(uniforms.view.mv_matrix, 1, GL_FALSE, camera_view_matrix * model_matrix);
            glUniform1i(uniforms.view.full_shading, mode == RENDER_FULL ? 1 : 0);
            objects[i].obj.render();
        }

        glBindTexture(GL_TEXTURE_2D, 0);
    }

private:
    GLuint          light_program{};
    GLuint          view_program{};
    GLuint          show_light_depth_program{};

    struct DUniform {
        struct DLight { GLint mvp; } light;
        struct DView {
            GLint   mv_matrix;
            GLint   proj_matrix;
            GLint   shadow_matrix;
            GLint   full_shading;
        } view;
    } uniforms;

    GLuint          depth_fbo;
    GLuint          depth_texture;
    GLuint          depth_debug_show_texture;

    enum { OBJECT_COUNT = 4 };
    struct DObjectInformation {
        sb6::object     obj;
        vmath::mat4     model_matrix;
    } objects[OBJECT_COUNT];

    vmath::mat4     light_view_matrix;
    vmath::mat4     light_proj_matrix;

    vmath::mat4     camera_view_matrix;
    vmath::mat4     camera_proj_matrix;

    GLuint          quad_vao;

    enum {
        RENDER_FULL,
        RENDER_LIGHT,
        RENDER_DEPTH
    } mode = RENDER_FULL;

    bool m_paused{false};
};

DECLARE_MAIN(ShadowMapping)