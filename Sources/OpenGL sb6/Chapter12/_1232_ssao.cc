#include <sb6.h>
#include <shader.h>
#include <object.h>
#include <vmath.h>

static inline float random_float() {
    static unsigned int seed = 0x13371337;
    float res;
    unsigned int tmp;

    seed *= 16807;
    tmp = seed ^ (seed >> 4) ^ (seed << 15);
    *((unsigned int *)&res) = (tmp >> 9) | 0x3F800000;

    return (res - 1.0f);
}

class Ssao final : public sb6::application {
    void startup() override final;

    void render(double currentTime) override final;

    void onKey(int key, int action) override final {
        if (action) {
            switch (key) {
            case 'N': weight_by_angle = !weight_by_angle; break;
            case 'R': randomize_points = !randomize_points; break;
            case 'S': point_count++; break;
            case 'X': point_count--; break;
            case 'Q': show_shading = !show_shading; break;
            case 'W': show_ao = !show_ao; break;
            case 'A': ssao_radius += 0.01f; break;
            case 'Z': ssao_radius -= 0.01f; break;
            case 'P': paused = !paused; break;
            case 'L': load_shaders(); break;
            }
        }
    }

    void init() override final {
        application::init();
        constexpr const char title[] = "OpenGL SuperBible - Screen-Space Ambient Occlusion";
        memcpy(info.title, title, sizeof(title));
    }

    void load_shaders();

    GLuint render_program{};
    GLuint ssao_program{};
    bool paused{ false };

    GLuint render_fbo;
    GLuint fbo_textures[3];
    GLuint quad_empty_vao;
    GLuint points_buffer;

    sb6::object object;
    sb6::object cube;

    struct DUniform {
        struct DRender {
            GLint mv_matrix;
            GLint proj_matrix;
            GLint shading_level;
        } render;
        struct DSsaoCalculate {
            GLint ssao_level;
            GLint object_level;
            GLint ssao_radius;
            GLint weight_by_angle;
            GLint randomize_points;
            GLint point_count;
        } ssao;
    } uniforms;

    bool show_shading{ true };
    bool show_ao{ true };
    float ssao_level{ 1.0f };
    float ssao_radius{ 0.05f };
    bool weight_by_angle{ true };
    bool randomize_points{ true };
    unsigned int point_count{ 10 };

    struct SAMPLE_POINTS {
        vmath::vec4 point[256];
        vmath::vec4 random_vectors[256];
    };

    void GenerateRenderFramebufferComponents() {
        glGenFramebuffers(1, &render_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);
        glGenTextures(3, fbo_textures);

        glBindTexture(GL_TEXTURE_2D, fbo_textures[0]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, 2048, 2048);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindTexture(GL_TEXTURE_2D, fbo_textures[1]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 2048, 2048);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindTexture(GL_TEXTURE_2D, fbo_textures[2]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 2048, 2048);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fbo_textures[0], 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, fbo_textures[1], 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo_textures[2], 0);

        static const GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, draw_buffers);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void InitializePointDirectionVectors(SAMPLE_POINTS& point_data) {
        for (auto i = 0; i < 256; i++) {
            do {
                point_data.point[i][0] = random_float() * 2.0f - 1.0f;
                point_data.point[i][1] = random_float() * 2.0f - 1.0f;
                point_data.point[i][2] = random_float(); //  * 2.0f - 1.0f;
                point_data.point[i][3] = 0.0f;
            } while (length(point_data.point[i]) > 1.0f);
            normalize(point_data.point[i]);
        }
    }

    void InitializePointRadiusRandomVectors(SAMPLE_POINTS& point_data) {
        for (auto i = 0; i < 256; i++) {
            point_data.random_vectors[i][0] = random_float();
            point_data.random_vectors[i][1] = random_float();
            point_data.random_vectors[i][2] = random_float();
            point_data.random_vectors[i][3] = random_float();
        }
    }
};

void Ssao::startup() {
    load_shaders();

    GenerateRenderFramebufferComponents();
    glGenVertexArrays(1, &quad_empty_vao);
    glBindVertexArray(quad_empty_vao);

    object.load("media/objects/dragon.sbm");
    cube.load("media/objects/cube.sbm");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glGenBuffers(1, &points_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, points_buffer);
    SAMPLE_POINTS point_data;
    InitializePointDirectionVectors(point_data);
    InitializePointRadiusRandomVectors(point_data);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(SAMPLE_POINTS), &point_data, GL_STATIC_DRAW);
}

void Ssao::render(double currentTime) {
    static const GLfloat black[] = {0.0f, 0.0f, 0.0f, 0.0f};
    static const GLfloat one = 1.0f;
    static double last_time = 0.0;
    static double total_time = 0.0;

    using Mat4 = vmath::mat4;
    using Vec3 = vmath::vec3;
    using vmath::translate;
    using vmath::rotate;
    using vmath::scale;

    if (!paused) total_time += (currentTime - last_time);
    last_time = currentTime;

    glViewport(0, 0, info.windowWidth, info.windowHeight);

    // Render ---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*
    glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);
    glEnable(GL_DEPTH_TEST);

    glClearBufferfv(GL_COLOR, 0, black);
    glClearBufferfv(GL_COLOR, 1, black);
    glClearBufferfv(GL_DEPTH, 0, &one);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, points_buffer);

    glUseProgram(render_program);
    glUniform1f(uniforms.render.shading_level, show_shading ? (show_ao ? 0.7f : 1.0f) : 0.0f);
    Mat4 proj_matrix = vmath::perspective(50.0f, (float)info.windowWidth / (float)info.windowHeight, 0.1f, 1000.0f);
    glUniformMatrix4fv(uniforms.render.proj_matrix, 1, GL_FALSE, proj_matrix);

    // Dragon
    const float f = static_cast<float>(total_time);
    Mat4 mv_matrix = translate(0.f, -5.f, 0.f) * rotate(f * 5.f, 0.f, 1.f, 0.f) * Mat4::identity();
    const Mat4 lookat_matrix = lookat(Vec3(0.0f, 3.0f, 15.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(uniforms.render.mv_matrix, 1, GL_FALSE, lookat_matrix * mv_matrix);
    object.render();

    // Floor
    mv_matrix = translate(0.f, -4.5f, 0.f) * rotate(f * 5.f, 0.f, 1.f, 0.f) * scale(4000.f, .1f, 4000.f) *
        Mat4::identity();
    glUniformMatrix4fv(uniforms.render.mv_matrix, 1, GL_FALSE, lookat_matrix * mv_matrix);
    cube.render();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);

    // SSAO ---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*
    glUseProgram(ssao_program);

    glUniform1f(uniforms.ssao.ssao_radius, ssao_radius * float(info.windowWidth) / 1000.0f);
    glUniform1f(uniforms.ssao.ssao_level, show_ao ? (show_shading ? 0.3f : 1.0f) : 0.0f);
    glUniform1i(uniforms.ssao.randomize_points, randomize_points ? 1 : 0);
    glUniform1ui(uniforms.ssao.point_count, point_count);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo_textures[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, fbo_textures[1]);

    glBindVertexArray(quad_empty_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Ssao::load_shaders() {
    GLuint shaders[2];

    shaders[0] = sb6::shader::load("media/shaders/ssao/render.vs.glsl", GL_VERTEX_SHADER);
    shaders[1] = sb6::shader::load("media/shaders/ssao/render.fs.glsl", GL_FRAGMENT_SHADER);

    if (render_program) glDeleteProgram(render_program);
    render_program = sb6::program::link_from_shaders(shaders, 2, true);

    uniforms.render.mv_matrix = glGetUniformLocation(render_program, "mv_matrix");
    uniforms.render.proj_matrix = glGetUniformLocation(render_program, "proj_matrix");
    uniforms.render.shading_level = glGetUniformLocation(render_program, "shading_level");

    shaders[0] = sb6::shader::load("media/shaders/ssao/ssao.vs.glsl", GL_VERTEX_SHADER);
    shaders[1] = sb6::shader::load("media/shaders/ssao/ssao.fs.glsl", GL_FRAGMENT_SHADER);

    if (ssao_program) glDeleteProgram(ssao_program);
    ssao_program = sb6::program::link_from_shaders(shaders, 2, true);

    uniforms.ssao.ssao_radius = glGetUniformLocation(ssao_program, "ssao_radius");
    uniforms.ssao.ssao_level = glGetUniformLocation(ssao_program, "ssao_level");
    uniforms.ssao.object_level = glGetUniformLocation(ssao_program, "object_level");
    uniforms.ssao.weight_by_angle = glGetUniformLocation(ssao_program, "weight_by_angle");
    uniforms.ssao.randomize_points = glGetUniformLocation(ssao_program, "randomize_points");
    uniforms.ssao.point_count = glGetUniformLocation(ssao_program, "point_count");
}

DECLARE_MAIN(Ssao)