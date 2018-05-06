#include <sb6.h>
#include <vmath.h>

#include <object.h>
#include <shader.h>
#include <sb6ktx.h>

class Gloss final : public sb6::application {
protected:
    void init() override final {
        application::init();

        constexpr const char title[] = "OpenGL SuperBible - Per-Pixel Gloss";
        memcpy(info.title, title, sizeof(title));
    }

    void startup() override final {
        object.load("media/objects/torus_nrms_tc.sbm");

        glActiveTexture(GL_TEXTURE0);
        tex_envmap = sb6::ktx::file::load("media/textures/envmaps/mountains3d.ktx");

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glActiveTexture(GL_TEXTURE1);
        tex_glossmap = sb6::ktx::file::load("media/textures/pattern1.ktx");

        LoadShaders();

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }

    void render(double currentTime) override final {
        static const GLfloat gray[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        static const GLfloat ones[] = { 1.0f };

        glClearBufferfv(GL_COLOR, 0, gray);
        glClearBufferfv(GL_DEPTH, 0, ones);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, tex_envmap);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_glossmap);

        glViewport(0, 0, info.windowWidth, info.windowHeight);
        glUseProgram(render_prog);

        vmath::mat4 proj_matrix = vmath::perspective(60.0f, (float)info.windowWidth / (float)info.windowHeight, 0.1f, 1000.0f);
        vmath::mat4 mv_matrix = vmath::translate(0.0f, 0.0f, -3.0f) *
                                vmath::rotate((float)currentTime * 13.75f, 0.0f, 1.0f, 0.0f) *
                                vmath::rotate((float)currentTime * 7.75f, 0.0f, 0.0f, 1.0f) *
                                vmath::rotate((float)currentTime * 15.3f, 1.0f, 0.0f, 0.0f);

        glUniformMatrix4fv(uniforms.mv_matrix, 1, GL_FALSE, mv_matrix);
        glUniformMatrix4fv(uniforms.proj_matrix, 1, GL_FALSE, proj_matrix);

        object.render();
    }

    void shutdown() override final {
        glDeleteProgram(render_prog);
        glDeleteTextures(1, &tex_envmap);
    }

    void LoadShaders() {
        if (render_prog) glDeleteProgram(render_prog);
        render_prog = glCreateProgram();

        auto vs = sb6::shader::load("media/shaders/perpixelgloss/perpixelgloss.vs.glsl", GL_VERTEX_SHADER);
        auto fs = sb6::shader::load("media/shaders/perpixelgloss/perpixelgloss.fs.glsl", GL_FRAGMENT_SHADER);

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
                case 'R': LoadShaders();
                    break;
            }
        }
    }

protected:
    GLuint          render_prog{0};

    GLuint          tex_envmap;
    GLuint          tex_glossmap;

    struct {
        GLint       mv_matrix;
        GLint       proj_matrix;
    } uniforms;

    sb6::object     object;
};

DECLARE_MAIN(Gloss)