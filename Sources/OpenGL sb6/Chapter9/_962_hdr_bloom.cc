#include <sb6.h>
#include <vmath.h>
#include <sb6ktx.h>
#include <shader.h>
#include <object.h>

enum {
    MAX_SCENE_WIDTH     = 2048,
    MAX_SCENE_HEIGHT    = 2048,
    SPHERE_COUNT        = 32,
};

class CHdrBloom final : public sb6::application {
public:
    void init() override final {
        application::init();

        constexpr char title[] = "OpenGL SuperBible - HDR Bloom";
        memcpy(info.title, title, sizeof(title));
    }

    void startup(void) override final {
        LoadShaders();
        object.load("media/objects/sphere.sbm");

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Initialize Render part
        InitializeRenderFrameBufferObject(&render_fbo);
        constexpr GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, buffers);

        // Initailize Filter part
        glGenFramebuffers(2, &filter_fbo[0]);
        glGenTextures(2, &tex_filter[0]);
        for (auto i = 0; i < 2; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, filter_fbo[i]);
            glBindTexture(GL_TEXTURE_2D, tex_filter[i]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, 
                i ? MAX_SCENE_WIDTH : MAX_SCENE_HEIGHT, i ? MAX_SCENE_HEIGHT : MAX_SCENE_WIDTH);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_filter[i], 0);
            glDrawBuffers(1, buffers);
        }

        // Initialize et sec trac
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glGenBuffers(1, &ubo_transform);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo_transform);
        glBufferData(GL_UNIFORM_BUFFER, (2 + SPHERE_COUNT) * sizeof(vmath::mat4), NULL, GL_DYNAMIC_DRAW);

        InitializeColorOfBalls();
    }

    void shutdown(void) override final {
        glDeleteProgram(program_render);
        glDeleteProgram(program_filter);
        glDeleteProgram(program_resolve);
        glDeleteVertexArrays(1, &vao);
        glDeleteTextures(1, &tex_src);
    }

    void render(double currentTime) override final {
        constexpr GLfloat black[]   = { 0.0f, 0.0f, 0.0f, 1.0f };
        constexpr GLfloat one       = 1.0f;
        static double last_time     = 0.0;
        static double total_time    = 0.0;

        if (!paused)
            total_time += (currentTime - last_time);
        last_time = currentTime;

        glViewport(0, 0, info.windowWidth, info.windowHeight);

        glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);
        glClearBufferfv(GL_COLOR, 0, black); // GL_COLOR_ATTACHMENT0 of render_fbo
        glClearBufferfv(GL_COLOR, 1, black); // GL_COLOR_ATTACHMENT1 of render_fbo
        glClearBufferfv(GL_DEPTH, 0, &one);  // GL_DEPTH_COMPONENT32F of render_fbo

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // Render using program_render
        glUseProgram(program_render);

        UpdateTransformMatrixes(ubo_transform, static_cast<float>(total_time));

        glBindBufferBase(GL_UNIFORM_BUFFER, 1, ubo_material);
        glUniform1f(uniforms.scene.bloom_thresh_min, bloom_thresh_min);
        glUniform1f(uniforms.scene.bloom_thresh_max, bloom_thresh_max);

        // Instancing
        object.render(SPHERE_COUNT);

        glDisable(GL_DEPTH_TEST);

        // Render using program_filter
        DrawFilterWithGaussian();

        // Render using program_resolve
        glUseProgram(program_resolve);

        glUniform1f(uniforms.resolve.exposure, exposure);
        if (show_prefilter) {
            glUniform1f(uniforms.resolve.bloom_factor, 0.0f);
            glUniform1f(uniforms.resolve.scene_factor, 1.0f);
        }
        else {
            glUniform1f(uniforms.resolve.bloom_factor, show_bloom ? bloom_factor : 0.0f);
            glUniform1f(uniforms.resolve.scene_factor, show_scene ? 1.0f : 0.0f);
        }

        // Default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0); 
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_filter[1]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, show_prefilter ? tex_brightpass : tex_scene);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void onKey(int key, int action) override final {
        if (!action) return;

        switch (key) {
            case '1':
            case '2':
            case '3': mode = key - '1'; break;

            case 'B': show_bloom = !show_bloom; break;
            case 'V': show_scene = !show_scene; break;
            case 'A': bloom_factor += 0.1f; break;
            case 'Z': bloom_factor -= 0.1f; break;
            case 'S': bloom_thresh_min += 0.1f; break;
            case 'X': bloom_thresh_min -= 0.1f; break;
            case 'D': bloom_thresh_max += 0.1f; break;
            case 'C': bloom_thresh_max -= 0.1f; break;
            case 'R': LoadShaders(); break;
            case 'N': show_prefilter = !show_prefilter; break;
            case 'M': mode = (mode + 1) % 3; break;
            case 'P': paused = !paused; break;
            case GLFW_KEY_KP_ADD: exposure *= 1.1f; break;
            case GLFW_KEY_KP_SUBTRACT: exposure /= 1.1f; break;
            default: break;
        }
    }

private:
    void LoadShaders() {
        struct DShaders {  
            GLuint vs;
            GLuint fs;
        } shaders;

        // Render shader 
        if (program_render) glDeleteProgram(program_render);

        shaders.vs = sb6::shader::load("media/shaders/hdrbloom/hdrbloom-scene.vs.glsl", GL_VERTEX_SHADER);
        shaders.fs = sb6::shader::load("media/shaders/hdrbloom/hdrbloom-scene.fs.glsl", GL_FRAGMENT_SHADER);
        program_render = sb6::program::link_from_shaders(&shaders.vs, 2, true);

        uniforms.scene.bloom_thresh_min = glGetUniformLocation(program_render, "bloom_thresh_min");
        uniforms.scene.bloom_thresh_max = glGetUniformLocation(program_render, "bloom_thresh_max");

        // Filtering shader 
        if (program_filter) glDeleteProgram(program_filter);

        shaders.vs = sb6::shader::load("media/shaders/hdrbloom/hdrbloom-filter.vs.glsl", GL_VERTEX_SHADER);
        shaders.fs = sb6::shader::load("media/shaders/hdrbloom/hdrbloom-filter.fs.glsl", GL_FRAGMENT_SHADER);
        program_filter = sb6::program::link_from_shaders(&shaders.vs, 2, true);

        // Resolving shader
        if (program_resolve) glDeleteProgram(program_resolve);

        shaders.vs = sb6::shader::load("media/shaders/hdrbloom/hdrbloom-resolve.vs.glsl", GL_VERTEX_SHADER);
        shaders.fs = sb6::shader::load("media/shaders/hdrbloom/hdrbloom-resolve.fs.glsl", GL_FRAGMENT_SHADER);
        program_resolve = sb6::program::link_from_shaders(&shaders.vs, 2, true);

        uniforms.resolve.exposure = glGetUniformLocation(program_resolve, "exposure");
        uniforms.resolve.bloom_factor = glGetUniformLocation(program_resolve, "bloom_factor");
        uniforms.resolve.scene_factor = glGetUniformLocation(program_resolve, "scene_factor");
    }

    void InitializeRenderFrameBufferObject(GLuint* render_fbo) {
        glGenFramebuffers(1, render_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, *render_fbo);

        glGenTextures(1, &tex_scene);
        glBindTexture(GL_TEXTURE_2D, tex_scene);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, MAX_SCENE_WIDTH, MAX_SCENE_HEIGHT);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_scene, 0);

        glGenTextures(1, &tex_brightpass);
        glBindTexture(GL_TEXTURE_2D, tex_brightpass);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, MAX_SCENE_WIDTH, MAX_SCENE_HEIGHT);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tex_brightpass, 0);

        glGenTextures(1, &tex_depth);
        glBindTexture(GL_TEXTURE_2D, tex_depth);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, MAX_SCENE_WIDTH, MAX_SCENE_HEIGHT);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex_depth, 0);
    }

    void InitializeColorOfBalls() {
        struct material {
            vmath::vec3     diffuse_color;
            unsigned int    : 32;           // pad
            vmath::vec3     specular_color;
            float           specular_power;
            vmath::vec3     ambient_color;
            unsigned int    : 32;           // pad
        };

        glGenBuffers(1, &ubo_material);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo_material);
        glBufferData(GL_UNIFORM_BUFFER, SPHERE_COUNT * sizeof(material), NULL, GL_STATIC_DRAW);

        auto m = static_cast<material*>(
            glMapBufferRange(GL_UNIFORM_BUFFER, 0, SPHERE_COUNT * sizeof(material), 
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)
        );
        float ambient = 0.002f;
        for (auto i = 0; i < SPHERE_COUNT; i++) {
            float fi = 3.14159267f * (float)i / 8.0f;
            m[i].diffuse_color  = vmath::vec3(sinf(fi) * 0.5f + 0.5f, sinf(fi + 1.345f) * 0.5f + 0.5f, sinf(fi + 2.567f) * 0.5f + 0.5f);
            m[i].specular_color = vmath::vec3(2.8f, 2.8f, 2.9f);
            m[i].specular_power = 30.0f;
            m[i].ambient_color  = vmath::vec3(ambient * 0.025f);
            ambient *= 1.5f;
        }
        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }

    void UpdateTransformMatrixes(GLuint ubo_transform, float t) {
        struct DTransformsT {
            vmath::mat4 mat_proj;
            vmath::mat4 mat_view;
            vmath::mat4 mat_model[SPHERE_COUNT];
        }; 

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo_transform);
        auto transforms = static_cast<DTransformsT*>(
            glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(DTransformsT), 
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)
        );

        transforms->mat_proj = vmath::perspective(50.0f, (float)info.windowWidth / (float)info.windowHeight, 1.0f, 1000.0f);
        transforms->mat_view = vmath::translate(0.0f, 0.0f, -20.0f);
        for (auto i = 0; i < SPHERE_COUNT; i++) {
            float fi = 3.141592f * (float)i / 16.0f;
            float r = (i & 2) ? 0.6f : 1.5f;
            transforms->mat_model[i] = vmath::translate(cosf(t + fi) * 5.0f * r, sinf(t + fi * 4.0f) * 4.0f, sinf(t + fi) * 5.0f * r);
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }

    void DrawFilterWithGaussian() {
        glUseProgram(program_filter);

        glBindVertexArray(vao);

        glBindFramebuffer(GL_FRAMEBUFFER, filter_fbo[0]);
        glBindTexture(GL_TEXTURE_2D, tex_brightpass);
        glViewport(0, 0, info.windowHeight, info.windowWidth);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindFramebuffer(GL_FRAMEBUFFER, filter_fbo[1]);
        glBindTexture(GL_TEXTURE_2D, tex_filter[0]);
        glViewport(0, 0, info.windowWidth, info.windowHeight);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

private:
    GLuint      tex_src;

    GLuint      render_fbo;
    GLuint      filter_fbo[2];

    GLuint      tex_scene;
    GLuint      tex_brightpass;
    GLuint      tex_depth;
    GLuint      tex_filter[2];

    GLuint      program_render{};
    GLuint      program_filter{};
    GLuint      program_resolve{};
    GLuint      vao{};
    float       exposure { 1.0f };
    int         mode{};
    bool        paused{ false };
    float       bloom_factor{ 1.0f };
    bool        show_bloom{ true };
    bool        show_scene{ true };
    bool        show_prefilter{ false };
    float       bloom_thresh_min{ 0.8f };
    float       bloom_thresh_max{ 1.2f };

    struct {
        struct {
            int bloom_thresh_min;
            int bloom_thresh_max;
        } scene;
        struct {
            int exposure;
            int bloom_factor;
            int scene_factor;
        } resolve;
    } uniforms;

    GLuint      ubo_transform;
    GLuint      ubo_material;

    sb6::object object;
};

DECLARE_MAIN(CHdrBloom);