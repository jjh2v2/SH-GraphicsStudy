#include <sb6.h>
#include <vmath.h>

#include <object.h>
#include <shader.h>
#include <sb6ktx.h>

class EnvironmentSphere final : public sb6::application {
protected:
    void init() override final {
        application::init();
        constexpr const char title[] = "OpenGL SuperBible - Spherical Environment Map";
        memcpy(info.title, title, sizeof(title));
    }

    void startup() override final {
        using sb6::ktx::file::load;
        envmaps[0] = load("media/textures/envmaps/spheremap1.ktx");
        envmaps[1] = load("media/textures/envmaps/spheremap2.ktx");
        envmaps[2] = load("media/textures/envmaps/spheremap3.ktx");
        tex_envmap = envmaps[envmap_index];

        object.load("media/objects/dragon.sbm");
        LoadShaders();

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }

    void render(double current_time) override final {
        constexpr GLfloat gray[] = { 0.2f, 0.2f, 0.2f, 1.0f };
        constexpr GLfloat ones[] = { 1.0f };

        glClearBufferfv(GL_COLOR, 0, gray);
        glClearBufferfv(GL_DEPTH, 0, ones);
        glBindTexture(GL_TEXTURE_2D, tex_envmap);

        glViewport(0, 0, info.windowWidth, info.windowHeight);

        glUseProgram(render_prog);

        vmath::mat4 proj_matrix = vmath::perspective(60.0f, (float)info.windowWidth / (float)info.windowHeight, 0.1f, 1000.0f);
        const auto t = static_cast<float>(current_time);
        vmath::mat4 mv_matrix = vmath::translate(0.0f, 0.0f, -15.0f) *
                                vmath::rotate(t, 1.0f, 0.0f, 0.0f) *
                                vmath::rotate(t * 1.1f, 0.0f, 1.0f, 0.0f) *
                                vmath::translate(0.0f, -4.0f, 0.0f);

        glUniformMatrix4fv(uniforms.mv_matrix, 1, GL_FALSE, mv_matrix);
        glUniformMatrix4fv(uniforms.proj_matrix, 1, GL_FALSE, proj_matrix);

        object.render();
    }

    void shutdown() override final {
        glDeleteProgram(render_prog);
        glDeleteTextures(3, envmaps);
    }

    void LoadShaders() {
        if (render_prog) glDeleteProgram(render_prog);
        render_prog = glCreateProgram();

        auto vs = sb6::shader::load("media/shaders/envmapsphere/render.vs.glsl", GL_VERTEX_SHADER);
        auto fs = sb6::shader::load("media/shaders/envmapsphere/render.fs.glsl", GL_FRAGMENT_SHADER);
        glAttachShader(render_prog, vs);
        glAttachShader(render_prog, fs);
        glLinkProgram(render_prog);

        glDeleteShader(vs);
        glDeleteShader(fs);

        uniforms.mv_matrix = glGetUniformLocation(render_prog, "mv_matrix");
        uniforms.proj_matrix = glGetUniformLocation(render_prog, "proj_matrix");
    }

    void onKey(int key, int action) override final {
        if (action) {
            switch (key) {
                case 'R': LoadShaders(); break;
                case 'E':
                    envmap_index = (envmap_index + 1) % 3;
                    tex_envmap = envmaps[envmap_index];
                    break;
                default: break;
            }
        }
    }

protected:
    GLuint          render_prog;

    GLuint          tex_envmap;
    GLuint          envmaps[3];
    int             envmap_index = 0;

    struct {
        GLint       mv_matrix;
        GLint       proj_matrix;
    } uniforms;

    sb6::object     object;
};

DECLARE_MAIN(EnvironmentSphere)