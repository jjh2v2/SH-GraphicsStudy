#include <sb6.h>
#include <vmath.h>

#include <object.h>
#include <sb6ktx.h>
#include <shader.h>

constexpr unsigned k_fbo_size = 2048u;
constexpr unsigned k_frustum_depth = 1000u;

class DepthOfField final : public sb6::application {
protected:
    void init() override final {
        application::init();
        constexpr const char title[] = "Depth Of Field Example";
        memcpy(info.title, title, sizeof(title));
    }

    void startup() override final {
        LoadShaders();

        InitializeObjectInformation();

        using vmath::perspective;
        camera_proj_matrix = perspective(50.0f, (float)info.windowWidth / (float)info.windowHeight, 2.0f, 300.0f);

        GenerateDepthRenderFramebufferComponents();

        // Generate texture just for temporary texture of processing.
        glGenTextures(1, &temp_tex);
        glBindTexture(GL_TEXTURE_2D, temp_tex);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, k_fbo_size, k_fbo_size);

        // Empty VAO
        glGenVertexArrays(1, &quad_vao);
        glBindVertexArray(quad_vao);
    }

    void InitializeObjectInformation() {
        const vmath::vec4 object_colors[] {
            vmath::vec4(1.0f, 0.7f, 0.8f, 1.0f), vmath::vec4(0.7f, 0.8f, 1.0f, 1.0f),
            vmath::vec4(0.3f, 0.9f, 0.4f, 1.0f), vmath::vec4(0.6f, 0.4f, 0.9f, 1.0f),
            vmath::vec4(0.8f, 0.2f, 0.1f, 1.0f),
        };

        for (auto i = 0; i < 5; i++) {
            objects[i].obj.load("media/objects/dragon.sbm");
            objects[i].diffuse_albedo = object_colors[i];
        }
    }

    void GenerateDepthRenderFramebufferComponents() {
        glGenFramebuffers(1, &depth_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, depth_fbo);

        glGenTextures(1, &depth_tex);
        glBindTexture(GL_TEXTURE_2D, depth_tex);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, k_fbo_size, k_fbo_size);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenTextures(1, &color_tex);
        glBindTexture(GL_TEXTURE_2D, color_tex);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, k_fbo_size, k_fbo_size);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_tex, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_tex, 0);

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void render(double currentTime) override final {
        static double last_time = 0.0;
        static double total_time = 0.0;

        if (!paused) 
            total_time += (currentTime - last_time);
        last_time = currentTime;

        UpdateMatrixes((float)total_time + 30.f);

        // Render and process
        glEnable(GL_DEPTH_TEST);
        RenderScene(total_time);
        ProcessGaussianFilter();
        glDisable(GL_DEPTH_TEST);

        // Final render
        FinalRender();
    }

    void UpdateMatrixes(float f) {
        objects[0].model_matrix = vmath::translate(5.0f, 0.0f, 20.0f) *
                                  vmath::rotate(f * 14.5f, 0.0f, 1.0f, 0.0f) *
                                  vmath::rotate(20.0f, 1.0f, 0.0f, 0.0f) *
                                  vmath::translate(0.0f, -4.0f, 0.0f);

        objects[1].model_matrix = vmath::translate(-5.0f, 0.0f, 0.0f) *
                                  vmath::rotate(f * 14.5f, 0.0f, 1.0f, 0.0f) *
                                  vmath::rotate(20.0f, 1.0f, 0.0f, 0.0f) *
                                  vmath::translate(0.0f, -4.0f, 0.0f);

        objects[2].model_matrix = vmath::translate(-15.0f, 0.0f, -20.0f) *
                                  vmath::rotate(f * 14.5f, 0.0f, 1.0f, 0.0f) *
                                  vmath::rotate(20.0f, 1.0f, 0.0f, 0.0f) *
                                  vmath::translate(0.0f, -4.0f, 0.0f);

        objects[3].model_matrix = vmath::translate(-25.0f, 0.0f, -40.0f) *
                                  vmath::rotate(f * 14.5f, 0.0f, 1.0f, 0.0f) *
                                  vmath::rotate(20.0f, 1.0f, 0.0f, 0.0f) *
                                  vmath::translate(0.0f, -4.0f, 0.0f);

        objects[4].model_matrix = vmath::translate(-35.0f, 0.0f, -60.0f) *
                                  vmath::rotate(f * 14.5f, 0.0f, 1.0f, 0.0f) *
                                  vmath::rotate(20.0f, 1.0f, 0.0f, 0.0f) *
                                  vmath::translate(0.0f, -4.0f, 0.0f);
    }

    void RenderScene(double currentTime);

    void ProcessGaussianFilter() {
        glUseProgram(filter_program);
        // First (row)
        glBindImageTexture(0, color_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(1, temp_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        glDispatchCompute(info.windowHeight, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        // Second (column)
        glBindImageTexture(0, temp_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(1, color_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        glDispatchCompute(info.windowWidth, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // Wait until the process is ended.
    }

    void FinalRender() {
        glUseProgram(display_program);
        glUniform1f(uniforms.dof.focal_distance, focal_distance);
        glUniform1f(uniforms.dof.focal_depth, focal_depth);
        glBindVertexArray(quad_vao);
        glBindTexture(GL_TEXTURE_2D, color_tex);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void onKey(int key, int action) override final {
            if (action) {
            switch (key) {
                case 'Q': focal_distance *= 1.1f; break;
                case 'A': focal_distance /= 1.1f; break;
                case 'W': focal_depth *= 1.1f; break;
                case 'S': focal_depth /= 1.1f; break;
                case 'R': LoadShaders(); break;
                case 'P': paused = !paused; break;
                default: break;
            }
        }
    }

    void LoadShaders();

private:
    GLuint          view_program{};
    GLuint          filter_program{};
    GLuint          display_program{};

    struct DUniform {
        struct {
            GLint   focal_distance;
            GLint   focal_depth;
        } dof;
        struct {
            GLint   mv_matrix;
            GLint   proj_matrix;
            GLint   full_shading;
            GLint   diffuse_albedo;
        } view;
    } uniforms;

    GLuint          depth_fbo;
    GLuint          depth_tex;
    GLuint          color_tex;
    GLuint          temp_tex;
    float           focal_distance = 40.f;
    float           focal_depth = 50.f;

    struct DObject {
        sb6::object     obj;
        vmath::mat4     model_matrix;
        vmath::vec4     diffuse_albedo;
    } objects[5];

    vmath::mat4     camera_view_matrix = lookat(vmath::vec3(0, 0, 40.f), vmath::vec3(0.0f), vmath::vec3(0.0f, 1.0f, 0.0f));
    vmath::mat4     camera_proj_matrix;

    GLuint          quad_vao{};

    bool paused{false};
};

void DepthOfField::RenderScene(double currentTime) {
    static const GLfloat ones[] = { 1.0f };
    static const GLfloat gray[] = { 0.1f, 0.1f, 0.1f, 0.0f };
    static const GLenum attachments[] = { GL_COLOR_ATTACHMENT0 };

    glBindFramebuffer(GL_FRAMEBUFFER, depth_fbo);

    glDrawBuffers(1, attachments);
    glViewport(0, 0, info.windowWidth, info.windowHeight);
    glClearBufferfv(GL_COLOR, 0, gray);
    glClearBufferfv(GL_DEPTH, 0, ones);

    glUseProgram(view_program);
    glUniformMatrix4fv(uniforms.view.proj_matrix, 1, GL_FALSE, camera_proj_matrix);

    for (auto i = 0; i < 5; i++) {
        glUniformMatrix4fv(uniforms.view.mv_matrix, 1, GL_FALSE, camera_view_matrix * objects[i].model_matrix);
        glUniform3fv(uniforms.view.diffuse_albedo, 1, objects[i].diffuse_albedo);
        objects[i].obj.render();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DepthOfField::LoadShaders() {
    GLuint shaders[2];

    // Render shader
    shaders[0] = sb6::shader::load("media/shaders/dof/render.vs.glsl", GL_VERTEX_SHADER);
    shaders[1] = sb6::shader::load("media/shaders/dof/render.fs.glsl", GL_FRAGMENT_SHADER);

    if (view_program) glDeleteProgram(view_program);
    view_program = sb6::program::link_from_shaders(shaders, 2, true);

    uniforms.view.proj_matrix = glGetUniformLocation(view_program, "proj_matrix");
    uniforms.view.mv_matrix = glGetUniformLocation(view_program, "mv_matrix");
    uniforms.view.full_shading = glGetUniformLocation(view_program, "full_shading");
    uniforms.view.diffuse_albedo = glGetUniformLocation(view_program, "diffuse_albedo");

    // Compute shader 
    shaders[0] = sb6::shader::load("media/shaders/dof/gensat.cs.glsl", GL_COMPUTE_SHADER);

    if (filter_program) glDeleteProgram(filter_program);
    filter_program = sb6::program::link_from_shaders(shaders, 1, true);

    // Display shader
    shaders[0] = sb6::shader::load("media/shaders/dof/display.vs.glsl", GL_VERTEX_SHADER);
    shaders[1] = sb6::shader::load("media/shaders/dof/display.fs.glsl", GL_FRAGMENT_SHADER);

    if (display_program) glDeleteProgram(display_program);
    display_program = sb6::program::link_from_shaders(shaders, 2, true);

    uniforms.dof.focal_distance = glGetUniformLocation(display_program, "focal_distance");
    uniforms.dof.focal_depth = glGetUniformLocation(display_program, "focal_depth");
}

DECLARE_MAIN(DepthOfField)