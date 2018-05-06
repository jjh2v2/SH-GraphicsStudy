#include <sb6.h>
#include <shader.h>
#include <object.h>
#include <vmath.h>

enum {
    NUM_DRAWS = 50000
};

// Be used in GL_DRAW_INDIRECT_BUFFER
// Must follow next struct element orders.
// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawArraysIndirect.xhtml
struct DrawArraysIndirectCommand {
    GLuint  count;
    GLuint  primCount;
    GLuint  first;
    GLuint  baseInstance;
};

class multidrawindirect_app : public sb6::application {
public:
    multidrawindirect_app()
        : render_program(0), mode(MODE_MULTIDRAW), paused(false), vsync(false) {}

    void init() {
        static const char title[] = "OpenGL SuperBible - Asteroids";
        sb6::application::init();
        memcpy(info.title, title, sizeof(title));
    }

    void startup();
    void render(double currentTime);

protected:
    void load_shaders();
    void onKey(int key, int action);

    GLuint              render_program;
    sb6::object         object;

    GLuint              indirect_draw_buffer;
    GLuint              draw_index_buffer;

    struct {
        GLint           time;
        GLint           view_matrix;
        GLint           proj_matrix;
        GLint           viewproj_matrix;
    } uniforms;

    enum MODE {
        MODE_FIRST,
        MODE_MULTIDRAW = 0,
        MODE_SEPARATE_DRAWS,
        MODE_MAX = MODE_SEPARATE_DRAWS
    };

    MODE                mode;
    bool                paused;
    bool                vsync;
};

void multidrawindirect_app::startup() {
    int i;

    load_shaders();
    // I don't know what object.load() does but seems that it load object's vertex informations
    // and implictly generate VAO, VBO used in shader and bind them to location = 0, and 1.
    object.load("media/objects/asteroids.sbm");

    constexpr int32_t indirect_buffer_byte_size = NUM_DRAWS * sizeof(DrawArraysIndirectCommand);
    glGenBuffers(1, &indirect_draw_buffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_draw_buffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, indirect_buffer_byte_size, nullptr, GL_STATIC_DRAW);

    {
        auto cmd = static_cast<DrawArraysIndirectCommand*>(
            glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, indirect_buffer_byte_size, 
                             GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

        for (i = 0; i < NUM_DRAWS; i++) {
            object.get_sub_object_info(i % object.get_sub_object_count(), cmd[i].first, cmd[i].count);
            cmd[i].primCount = 1;
            cmd[i].baseInstance = i;
        }

        glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
    }

    glBindVertexArray(object.get_vao());

    glGenBuffers(1, &draw_index_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, draw_index_buffer);
    glBufferData(GL_ARRAY_BUFFER, NUM_DRAWS * sizeof(GLuint), NULL, GL_STATIC_DRAW);

    {
        auto draw_index = static_cast<GLuint*>(
            glMapBufferRange(GL_ARRAY_BUFFER, 0, NUM_DRAWS * sizeof(GLuint), 
                             GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

        for (i = 0; i < NUM_DRAWS; i++)
            draw_index[i] = i;

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    // Set location = 10 attribute (in) to 0 as initial value
    // And order it to increase by each instance by using glVertexAttribDivisor(location, inc)
    // draw_index_buffer VBO will be bound to location = 10 attribute.
    glVertexAttribIPointer(10, 1, GL_UNSIGNED_INT, 0, NULL);
    glVertexAttribDivisor(10, 1);
    glEnableVertexAttribArray(10);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
}

void multidrawindirect_app::render(double currentTime) {
    int j;
    static const float one = 1.0f;
    static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };

    static double last_time = 0.0;
    static double total_time = 0.0;

    if (!paused) total_time += (currentTime - last_time);
    last_time = currentTime;

    float t = float(total_time);
    int i = int(total_time * 3.0f);
    /*! Initialize */
    glViewport(0, 0, info.windowWidth, info.windowHeight);
    glClearBufferfv(GL_COLOR, 0, black);
    glClearBufferfv(GL_DEPTH, 0, &one);
    /*! Set view matrix and projection matrix */
    const vmath::mat4 view_matrix = vmath::lookat(
        vmath::vec3(100.0f * cosf(t * 0.023f), 100.0f * cosf(t * 0.023f), 300.0f * sinf(t * 0.037f) - 600.0f),
        vmath::vec3(0.0f, 0.0f, 260.0f),
        vmath::normalize(vmath::vec3(0.1f - cosf(t * 0.1f) * 0.3f, 1.0f, 0.0f)));
    const vmath::mat4 proj_matrix = vmath::perspective(50.0f,
        (float)info.windowWidth / (float)info.windowHeight,
        1.0f,
        2000.0f);
    /*! Use shader */
    glUseProgram(render_program);

    glUniform1f(uniforms.time, t);
    glUniformMatrix4fv(uniforms.view_matrix, 1, GL_FALSE, view_matrix);
    glUniformMatrix4fv(uniforms.proj_matrix, 1, GL_FALSE, proj_matrix);
    glUniformMatrix4fv(uniforms.viewproj_matrix, 1, GL_FALSE, proj_matrix * view_matrix);
    /*! Bind object group */
    glBindVertexArray(object.get_vao());
    /*! Render call */
    if (mode == MODE_MULTIDRAW) {
        glMultiDrawArraysIndirect(GL_TRIANGLES, NULL, NUM_DRAWS, 0);
    }
    else if (mode == MODE_SEPARATE_DRAWS) {
        for (j = 0; j < NUM_DRAWS; j++) {
            GLuint first, count;
            // In my opinion.., Object has a capability of storing sub mesh objects, not only one.
            // So, we need find first index (determined by glsl) and vertexs indices count.
            object.get_sub_object_info(j % object.get_sub_object_count(), first, count);
            // Draw each sub-mesh, [first, first + count) by one instance from gl_InstanceID = j;
            glDrawArraysInstancedBaseInstance(GL_TRIANGLES, first, count, 1, j);
        }
    }
}

void multidrawindirect_app::load_shaders() {
    GLuint shaders[2] = {
        sb6::shader::load("media/shaders/multidrawindirect/render.vs.glsl", GL_VERTEX_SHADER),
        sb6::shader::load("media/shaders/multidrawindirect/render.fs.glsl", GL_FRAGMENT_SHADER)
    };

    /*! If there is already render program, delete previous shader program. */
    if (render_program) glDeleteProgram(render_program);
    render_program = sb6::program::link_from_shaders(shaders, 2, true);

    /*! Set Uniform location from glGetUniformLocation */
    uniforms.time               = glGetUniformLocation(render_program, "time");
    uniforms.view_matrix        = glGetUniformLocation(render_program, "view_matrix");
    uniforms.proj_matrix        = glGetUniformLocation(render_program, "proj_matrix");
    uniforms.viewproj_matrix    = glGetUniformLocation(render_program, "viewproj_matrix");
}

void multidrawindirect_app::onKey(int key, int action) {
    if (action) {
        switch (key) {
        case 'P': paused = !paused; break;
        case 'V': vsync = !vsync; setVsync(vsync); break;
        case 'M':
            mode = MODE(mode + 1);
            if (mode > MODE_MAX) mode = MODE_FIRST;
            break;
        case 'R': load_shaders(); break;
        }
    }
}

DECLARE_MAIN(multidrawindirect_app)