#include <sb6.h>
#include <vmath.h>

#include <object.h>
#include <sb6ktx.h>
#include <shader.h>

class RimLight final : public sb6::application {
protected:
    void init() override final {
        application::init();
        static const char title[] = "OpenGL SuperBible - Rim Lighting";
        memcpy(info.title, title, sizeof(title));
    }

    void startup() override final {
        LoadShaders();
        object.load("media/objects/dragon.sbm");

        glEnable(GL_CULL_FACE);
        //glCullFace(GL_FRONT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        proj_matrix = vmath::perspective(50.0f, (float)info.windowWidth / (float)info.windowHeight, 0.1f, 1000.0f);
    }

    void render(double currentTime) override final {
        static const GLfloat background_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        static const GLfloat one = 1.0f;
        static double last_time = 0.0;
        static double total_time = 0.0;

        if (!paused) total_time += (currentTime - last_time);
        last_time = currentTime;

        glViewport(0, 0, info.windowWidth, info.windowHeight);
        glClearBufferfv(GL_COLOR, 0, background_color);
        glClearBufferfv(GL_DEPTH, 0, &one);

        glUseProgram(program);

        glUniformMatrix4fv(uniforms.proj_matrix, 1, GL_FALSE, proj_matrix);

        auto f = static_cast<float>(total_time);
        vmath::mat4 mv_matrix = vmath::translate(0.0f, -5.0f, -20.0f) *
                                vmath::rotate(f * 5.0f, 0.0f, 1.0f, 0.0f) *
                                vmath::mat4::identity();
        glUniformMatrix4fv(uniforms.mv_matrix, 1, GL_FALSE, mv_matrix);

        glUniform3fv(uniforms.rim_color, 1, rim_enable ? rim_color : vmath::vec3(0.0f));
        glUniform1f(uniforms.rim_power, rim_power);

        object.render();
    }

    virtual void shutdown() override final {
        object.free();
        glDeleteProgram(program);
    }

    void onKey(int key, int action) override final {
        if (action) {
            switch (key) {
                case 'Q': rim_color[0] += 0.1f; break;
                case 'W': rim_color[1] += 0.1f; break;
                case 'E': rim_color[2] += 0.1f; break;
                case 'A': rim_color[0] -= 0.1f; break;
                case 'S': rim_color[1] -= 0.1f; break;
                case 'D': rim_color[2] -= 0.1f; break;
                case 'R': rim_power *= 1.5f; break;
                case 'F': rim_power /= 1.5f; break;
                case 'Z': rim_enable = !rim_enable; break;

                case 'P': paused = !paused; break;
                case 'L': LoadShaders(); break;
            }
        }
    }

    void LoadShaders() {
        auto vs = sb6::shader::load("media/shaders/rimlight/render.vs.glsl", GL_VERTEX_SHADER);
        auto fs = sb6::shader::load("media/shaders/rimlight/render.fs.glsl", GL_FRAGMENT_SHADER);

        if (program != 0) glDeleteProgram(program);
        program = glCreateProgram();

        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        uniforms.mv_matrix = glGetUniformLocation(program, "mv_matrix");
        uniforms.proj_matrix = glGetUniformLocation(program, "proj_matrix");
        uniforms.rim_color = glGetUniformLocation(program, "rim_color");
        uniforms.rim_power = glGetUniformLocation(program, "rim_power");

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

private:
    GLuint          program{};
    struct {
        GLint           mv_matrix;
        GLint           proj_matrix;
        GLint           rim_color;
        GLint           rim_power;
    } uniforms;

    vmath::mat4         proj_matrix;
    vmath::mat4         mat_rotation { vmath::mat4::identity() };
    vmath::vec3         rim_color {0.3f, 0.3f, 0.3f};

    sb6::object         object;
    float               rim_power = 2.5f;
    bool                rim_enable = true;
    bool                paused = false;
};

DECLARE_MAIN(RimLight)