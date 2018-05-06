#include <sb6.h>
#include <vmath.h>

#include <object.h>
#include <sb6ktx.h>
#include <shader.h>

#include <cstdio>

class BumpMapping final : public sb6::application {
protected:
    void init() override final {
        application::init();

        constexpr char title[] = "OpenGL SuperBible - Bump Mapping";
        memcpy(info.title, title, sizeof(title));
    }

    void startup() override final {
        LoadShaders();

        using sb6::ktx::file::load;
        glActiveTexture(GL_TEXTURE0);
        textures.color = load("media/textures/ladybug_co.ktx");
        glActiveTexture(GL_TEXTURE1);
        textures.normals = load("media/textures/ladybug_nm.ktx");

        object.load("media/objects/ladybug.sbm");

        proj_matrix = vmath::perspective(50.0f, (float)info.windowWidth / (float)info.windowHeight, 0.1f, 1000.0f);
    }

    void render(double currentTime) override final {
        static const GLfloat zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        static const GLfloat gray[] = { 0.1f, 0.1f, 0.1f, 0.0f };
        static const GLfloat ones[] = { 1.0f };
        static double last_time = 0.0;
        static double total_time = 0.0;

        if (!paused) total_time += (currentTime - last_time);
        last_time = currentTime;


        glClearBufferfv(GL_COLOR, 0, gray);
        glClearBufferfv(GL_DEPTH, 0, ones);

        glViewport(0, 0, info.windowWidth, info.windowHeight);
        glEnable(GL_DEPTH_TEST);

        glUseProgram(program);

        glUniformMatrix4fv(uniforms.proj_matrix, 1, GL_FALSE, proj_matrix);

        vmath::mat4 mv_matrix = vmath::translate(0.0f, -0.2f, -5.5f) *
                                vmath::rotate(14.5f, 1.0f, 0.0f, 0.0f) *
                                vmath::rotate(-20.0f, 0.0f, 1.0f, 0.0f) *
                                vmath::mat4::identity();
        glUniformMatrix4fv(uniforms.mv_matrix, 1, GL_FALSE, mv_matrix);
        const auto f = static_cast<float>(total_time);
        glUniform3fv(uniforms.light_pos, 1, vmath::vec3(40.0f * sinf(f), 30.0f + 20.0f * cosf(f), 40.0f));

        object.render();
    }

    void onKey(int key, int action) override final {
        if (action) {
            switch (key) {
                case 'R': LoadShaders(); break;
                case 'S': MakeScreenshot(); break;
                case 'P': paused = !paused; break;
                default: break;
            }
        }       
    }

    void LoadShaders() {
        auto vs = sb6::shader::load("media/shaders/bumpmapping/bumpmapping.vs.glsl", GL_VERTEX_SHADER);
        auto fs = sb6::shader::load("media/shaders/bumpmapping/bumpmapping.fs.glsl", GL_FRAGMENT_SHADER);

        if (program) glDeleteProgram(program);
        program = glCreateProgram();

        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        uniforms.mv_matrix = glGetUniformLocation(program, "mv_matrix");
        uniforms.proj_matrix = glGetUniformLocation(program, "proj_matrix");
        uniforms.light_pos = glGetUniformLocation(program, "light_pos");
    }

    void MakeScreenshot();

    vmath::mat4     proj_matrix;
    GLuint          program{};

    struct {
        GLuint      color;
        GLuint      normals;
    } textures;

    struct {
        GLint       mv_matrix;
        GLint       proj_matrix;
        GLint       light_pos;
    } uniforms;

    bool            paused{ false };
    sb6::object     object;
};

void BumpMapping::MakeScreenshot() {
    int row_size = ((info.windowWidth * 3 + 3) & ~3);
    int data_size = row_size * info.windowHeight;
    unsigned char * data = new unsigned char [data_size];

#pragma pack (push, 1)
    struct {
        unsigned char identsize;    // Size of following ID field
        unsigned char cmaptype;     // Color map type 0 = none
        unsigned char imagetype;    // Image type 2 = rgb
        short cmapstart;            // First entry in palette
        short cmapsize;             // Fumber of entries in palette
        unsigned char cmapbpp;      // Number of bits per palette entry
        short xorigin;              // X origin
        short yorigin;              // Y origin
        short width;                // Width in pixels
        short height;               // Height in pixels
        unsigned char bpp;          // Bits per pixel
        unsigned char descriptor;   // Descriptor bits
    } tga_header;
#pragma pack (pop)

    glReadPixels(0, 0,                                  // Origin
                 info.windowWidth, info.windowHeight,   // Size
                 GL_BGR, GL_UNSIGNED_BYTE,              // Format, type
                 data);                                 // Data

    memset(&tga_header, 0, sizeof(tga_header));
    tga_header.imagetype = 2;
    tga_header.width = (short)info.windowWidth;
    tga_header.height = (short)info.windowHeight;
    tga_header.bpp = 24;

    FILE * f_out = fopen("screenshot.tga", "wb");
    fwrite(&tga_header, sizeof(tga_header), 1, f_out);
    fwrite(data, data_size, 1, f_out);
    fclose(f_out);

    delete [] data;
}

DECLARE_MAIN(BumpMapping)