#include <sb6.h>
#include <vmath.h>

#include <object.h>
#include <shader.h>

#define _0x00 0x00, 0x00, 0x00

class CellShading final : public sb6::application {
protected:
    void init() override final {
        application::init();
        constexpr char title[] = "OpenGL SuperBible - Toon Shading";
        memcpy(info.title, title, sizeof(title));
    }

    void startup() override final {
        LoadShaders();
        object.load("media/objects/torus_nrms_tc.sbm");

        constexpr GLubyte toon_tex_data[] = {
            0x22, _0x00, 0x44, _0x00, 
            0x66, _0x00, 0x88, _0x00, 
            0xAA, _0x00, 0xCC, _0x00,
            0xFF, _0x00,
        };

        glGenTextures(1, &tex_toon);
        glBindTexture(GL_TEXTURE_1D, tex_toon);
        constexpr int count = sizeof(toon_tex_data) / 4;
        // GL_RGB8 has 32bits, width : Specifies the width of the texture, in texels (not byte unit, but count).
        glTexStorage1D(GL_TEXTURE_1D, 1, GL_RGB8, count);
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, count, GL_RGBA, GL_UNSIGNED_BYTE, toon_tex_data);

        // Important (GL_NEAREST)
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }

    void render(double currentTime) override final {
        static const GLfloat gray[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        static const GLfloat ones[] = { 1.0f };

        glClearBufferfv(GL_COLOR, 0, gray);
        glClearBufferfv(GL_DEPTH, 0, ones);

        glBindTexture(GL_TEXTURE_1D, tex_toon);

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
        glDeleteTextures(1, &tex_toon);
    }

    void LoadShaders() {
        using sb6::shader::load;
        auto vs = load("media/shaders/toonshading/toonshading.vs.glsl", GL_VERTEX_SHADER);
        auto fs = load("media/shaders/toonshading/toonshading.fs.glsl", GL_FRAGMENT_SHADER);

        if (render_prog) glDeleteProgram(render_prog);
        render_prog = glCreateProgram();

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
                default: break;
            }
        }
    }

protected:
    GLuint          render_prog{};
    GLuint          tex_toon{};

    struct DUniform {
        GLint       mv_matrix;
        GLint       proj_matrix;
    } uniforms = {};

    sb6::object     object;
};

DECLARE_MAIN(CellShading)