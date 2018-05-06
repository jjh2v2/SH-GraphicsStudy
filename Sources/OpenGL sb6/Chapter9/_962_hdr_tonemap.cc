#include <sb6.h>
#include <sb6ktx.h>
#include <shader.h>

enum class EMode : char {
    NAIVE = 0,
    EXPOSURE = 1,
    ADAPTIVE = 2,
};

class HdrTonemap final : public sb6::application {
public:
    void init() override final {
        application::init();

        constexpr char title[] = "OpenGL SuperBible - HDR Tone Mapping";
        memcpy(info.title, title, sizeof(title));
    }

    void startup(void) override final {
        // Load texture from file
        tex_src = sb6::ktx::file::load("media/textures/treelights_2k.ktx");

        // Now bind it to the context using the GL_TEXTURE_2D binding point
        glBindTexture(GL_TEXTURE_2D, tex_src);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        load_shaders();
    }

    void shutdown(void) override final {
        glDeleteProgram(program_adaptive);
        glDeleteProgram(program_exposure);
        glDeleteProgram(program_naive);
        glDeleteVertexArrays(1, &vao);
        glDeleteTextures(1, &tex_src);
        glDeleteTextures(1, &tex_lut);
    }

    void render(double t) override final {
        glViewport(0, 0, info.windowWidth, info.windowHeight);

        constexpr GLfloat black[] { 0.0f, 0.25f, 0.0f, 1.0f };
        glClearBufferfv(GL_COLOR, 0, black);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_src);

        switch (mode) {
        case EMode::NAIVE: glUseProgram(program_naive); break;
        case EMode::EXPOSURE:
            glUseProgram(program_exposure);
            glUniform1f(uniforms.exposure.exposure, exposure);
            break;
        case EMode::ADAPTIVE: glUseProgram(program_adaptive); break;
        default: glUseProgram(0); break;
        }
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void onKey(int key, int action) override final {
        if (!action) return;

        switch (key) {
        case '1': mode = EMode::NAIVE; break;
        case '2': mode = EMode::EXPOSURE; break;
        case '3': mode = EMode::ADAPTIVE; break;
        case 'R': load_shaders(); break;
        case '0': exposure *= 1.1f; break;
        case '9': exposure /= 1.1f; break;
        default: break;
        }
    }

    void load_shaders() {
        GLuint vs;
        GLuint fs;

        if (program_naive) glDeleteProgram(program_naive);
        program_naive = glCreateProgram();

        vs = sb6::shader::load("media/shaders/hdrtonemap/tonemap.vs.glsl", GL_VERTEX_SHADER);
        fs = sb6::shader::load("media/shaders/hdrtonemap/tonemap_naive.fs.glsl", GL_FRAGMENT_SHADER);

        glAttachShader(program_naive, vs);
        glAttachShader(program_naive, fs);
        glLinkProgram(program_naive);

        glDeleteShader(fs);

        if (program_adaptive) glDeleteProgram(program_adaptive);
        program_adaptive = glCreateProgram();

        fs = sb6::shader::load("media/shaders/hdrtonemap/tonemap_adaptive.fs.glsl", GL_FRAGMENT_SHADER);

        glAttachShader(program_adaptive, vs);
        glAttachShader(program_adaptive, fs);
        glLinkProgram(program_adaptive);

        glDeleteShader(fs);

        if (program_exposure) glDeleteProgram(program_exposure);
        program_exposure = glCreateProgram();

        fs = sb6::shader::load("media/shaders/hdrtonemap/tonemap_exposure.fs.glsl", GL_FRAGMENT_SHADER);

        glAttachShader(program_exposure, vs);
        glAttachShader(program_exposure, fs);
        glLinkProgram(program_exposure);

        uniforms.exposure.exposure = glGetUniformLocation(program_exposure, "exposure");

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

private:
    GLuint      tex_src{};
    GLuint      tex_lut{};

    GLuint      program_naive{};
    GLuint      program_exposure{};
    GLuint      program_adaptive{};
    GLuint      vao{};
    float       exposure = 1.0f;
    EMode       mode = EMode::NAIVE;

    struct {
        struct {
            int exposure;
        } exposure;
    } uniforms{};
};

DECLARE_MAIN(HdrTonemap);