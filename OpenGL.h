#pragma once
#include <iostream>
#include <GL/glew.h>

#define GL_PI       3.14159265358979323846   // pi
#define GL_PI_2     1.57079632679489661923   // pi/2
#define GL_PI_4     0.785398163397448309616  // pi/4
#define GL_1_PI     0.318309886183790671538  // 1/pi
#define GL_2_PI     0.636619772367581343076  // 2/pi

/*
   Do this:
      #define OPENGL_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.

   // i.e. it should look like this:
   #include ...
   #define OPENGL_IMPLEMENTATION
   #include "OpenGL.h"
 */

// ====================================================================

struct gl_buffer_t
{
    GLuint handle = 0;
    size_t size = 0;
    GLenum usage = GL_STATIC_DRAW;
    GLenum target = GL_ARRAY_BUFFER;
};

struct gl_buffer_bind_t
{
    uint32_t binding = 0;
    GLenum target = GL_UNIFORM_BUFFER;
};

struct gl_texture_t
{
    GLuint handle = 0;
    uint32_t width = 0, height = 0;
    GLenum target = GL_TEXTURE_2D;
    GLenum format = GL_RGBA;
    GLenum internal_format = GL_RGBA;
};

struct gl_texture_bind_t
{
    uint32_t binding = 0;
    GLenum aspect_mode = GL_DEPTH_COMPONENT;
};

struct gl_texture_storage_bind_t
{
    uint32_t binding = 0;
    uint32_t base_level = 0;
    uint32_t base_layer = 0;
    uint32_t level_count = 1;
    uint32_t layer_count = 1;
    GLenum access = GL_WRITE_ONLY;
};

struct gl_image_t
{
    void* pixels = nullptr;
    uint32_t width = 0, height = 0;
    GLenum format = GL_RGBA;
};

struct gl_sampler_t
{
    GLuint handle = 0;
};

struct gl_module_t
{
    GLuint handle = 0;
    GLenum target = GL_NONE;
};

struct gl_pass_t
{
    GLuint handle = 0;
    gl_module_t module;

    struct
    {
        bool clear_color = false;
        bool clear_depth = false;
        bool clear_stencil = false;
        float color_value[4] = {};
        float depth_value = 1.0f;
        uint32_t stencil_value = (uint32_t)-1;
        struct
        {
            GLenum opt = GL_ADD, src = GL_ONE, dst = GL_ZERO;
        } blend;
    } screen;

    struct gl_color_attach_t
    {
        gl_texture_t texture;
        bool clear = false;
        float value[4] = {};
        struct
        {
            GLenum opt = GL_ADD, src = GL_ONE, dst = GL_ZERO;
        } blend_color, blend_alpha;
    };
    gl_color_attach_t color[2];

    struct
    {
        gl_texture_t texture;
        bool clear = false;
        bool write = false;
        GLenum func = GL_ALWAYS;
        float value = 1.0f;
    } depth;

    struct
    {
        bool clear = false;
        uint32_t read = (uint32_t)-1;
        uint32_t write = (uint32_t)-1;
        struct
        {
            GLenum func = GL_ALWAYS, sfail = GL_KEEP, dpfail = GL_KEEP, dppass = GL_KEEP;
        } back, front;
	    uint32_t value = (uint32_t)-1;
    } stencil;

    GLenum cull_mode = GL_BACK;
    GLenum front_face = GL_CCW;
    GLenum fill_mode = GL_FILL;
};

struct gl_mesh_t
{
    GLuint handle = 0;

    gl_buffer_t vertex_vbo;
    gl_buffer_t normal_vbo;
    gl_buffer_t uv_vbo;
    gl_buffer_t index_vbo;

    GLsizei vertex_count = 0;
    GLsizei index_count = 0;
    GLenum index_type = GL_UNSIGNED_INT;
    GLenum primitive_type = GL_TRIANGLES;
};

struct gl_meshlet_t
{
    gl_buffer_t vertex_vbo;
    gl_buffer_t normal_vbo;
    gl_buffer_t uv_vbo;
    gl_buffer_t index_vbo;

    GLsizei vertex_count = 0;
    GLsizei index_count = 0;
    GLenum index_type = GL_UNSIGNED_INT;
    GLenum primitive_type = GL_TRIANGLES;
};

// ====================================================================

void gl_hello_world();

gl_buffer_t gl_create_buffer(size_t size, GLenum usage = GL_STATIC_DRAW, const void* data = nullptr);
void gl_destroy_buffer(gl_buffer_t& buffer);
void gl_bind_buffer(gl_buffer_t buffer, gl_buffer_bind_t desc = {});
void gl_read_buffer(gl_buffer_t buffer, size_t offset, size_t size, void* data);
void gl_write_buffer(gl_buffer_t buffer, size_t offset, size_t size, const void* data);

gl_texture_t gl_create_texture_color(uint32_t width, uint32_t height, const void* data);
gl_texture_t gl_create_texture_depth(int width, int height, const void* data);
gl_texture_t gl_create_texture_depth_stencil(int width, int height, const void* data);
void gl_destroy_texture(gl_texture_t& texture);
void gl_bind_texture(gl_texture_t texture, gl_texture_bind_t desc = {});
void gl_bind_texture_storage(gl_texture_t texture, gl_texture_storage_bind_t desc = {});

gl_sampler_t gl_create_sampler(GLenum min_filter, GLenum mag_filter, GLenum wrap_s, GLenum wrap_t, GLenum wrap_r);
void gl_destroy_sampler(gl_sampler_t& sampler);
void gl_bind_sampler(gl_sampler_t sampler, GLuint texture_unit);

gl_texture_t gl_image_2_texture(gl_image_t image);
gl_image_t gl_texture_2_image(gl_texture_t texture, void* buffer, size_t length);

gl_module_t gl_create_module_compute(const char* comp_src);
gl_module_t gl_create_module_graphics(const char* vert_src, const char* frag_src);
gl_module_t gl_create_module_meshlet(const char* task_src, const char* mesh_src, const char* frag_src);
void gl_destroy_module(gl_module_t& module);

void gl_set_uniform_int(const char* name, int32_t value);
void gl_set_uniform_uint(const char* name, uint32_t value);
void gl_set_uniform_float(const char* name, float value);
void gl_set_uniform_vec2(const char* name, const float* value);
void gl_set_uniform_vec3(const char* name, const float* value);
void gl_set_uniform_vec4(const char* name, const float* value);
void gl_set_uniform_mat3(const char* name, const float* value);
void gl_set_uniform_mat4(const char* name, const float* value);

void gl_begin_compute(gl_pass_t& pass);
void gl_end_compute(gl_pass_t& pass);
void gl_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ);

void gl_begin_render(gl_pass_t& pass);
void gl_end_render(gl_pass_t& pass);
void gl_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height);
void gl_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height);

inline void (*gl_begin_meshlet)(gl_pass_t& pass) = gl_begin_render;
inline void (*gl_end_meshlet)(gl_pass_t& pass) = gl_end_render;
void gl_draw_mesh_task(uint32_t groupX, uint32_t groupY = 1, uint32_t groupZ = 1);

gl_mesh_t gl_create_mesh(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
void gl_destroy_mesh(gl_mesh_t& mesh);
void gl_draw_mesh(gl_mesh_t mesh);

gl_meshlet_t gl_create_meshlet(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
void gl_destroy_meshlet(gl_meshlet_t& meshlet);

gl_mesh_t gl_create_mesh_screen();
void gl_draw_screen(int width, int height, gl_texture_t texture);

// ====================================================================

#ifdef OPENGL_IMPLEMENTATION

static void gl_hello_world()
{
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW init failed: %s\n", (const char*)glewGetErrorString(err));
        abort();
    }
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
}

// ====================================================================

static gl_buffer_t gl_create_buffer(
    size_t size,
    GLenum usage,
    const void* data
)
{
    gl_buffer_t result = {};
    glGenBuffers(1, &result.handle);
    glBindBuffer(GL_ARRAY_BUFFER, result.handle);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)size, data, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    result.size = size;
    result.target = GL_ARRAY_BUFFER;
    return result;
}

static void gl_destroy_buffer(gl_buffer_t& buffer)
{
    glDeleteBuffers(1, &buffer.handle);
    buffer.handle = 0;
}

static void gl_bind_buffer(gl_buffer_t buffer, gl_buffer_bind_t desc)
{
    switch (desc.target)
    {
    default: break;
    case GL_UNIFORM_BUFFER:
        glBindBufferBase(GL_UNIFORM_BUFFER, desc.binding, buffer.handle);
        break;
    case GL_SHADER_STORAGE_BUFFER:
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, desc.binding, buffer.handle);
        break;
    }
}

static void gl_read_buffer(
    gl_buffer_t buffer,
    size_t offset,
    size_t size,
    void* data)
{
    if (!buffer.handle) return;
    if (!data || size == 0) return;
    if ((GLsizeiptr)(offset + size) > buffer.size) return; // 越界保护

    glBindBuffer(GL_ARRAY_BUFFER, buffer.handle);
    glGetBufferSubData(
        GL_ARRAY_BUFFER,
        (GLintptr)offset,
        (GLsizeiptr)size,
        data
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void gl_write_buffer(
    gl_buffer_t buffer,
    size_t offset,
    size_t size,
    const void* data)
{
    if (!buffer.handle) return;
    if (!data || size == 0) return;
    if (offset + size > buffer.size) return;

    glBindBuffer(GL_ARRAY_BUFFER, buffer.handle);
    glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)offset, (GLsizeiptr)size, data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ====================================================================

static gl_texture_t gl_create_texture_color(
    uint32_t width,
    uint32_t height,
    const void* data)
{
    gl_texture_t result = {};
    glGenTextures(1, &result.handle);
    glBindTexture(GL_TEXTURE_2D, result.handle);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLubyte clearColor[4] = { 0, 0, 0, 0 };
    if (data == nullptr) glClearTexImage(result.handle, 0, GL_RGBA, GL_UNSIGNED_BYTE, clearColor);

    result.width = width;
    result.height = height;
    result.target = GL_TEXTURE_2D;
    result.format = GL_RGBA;
    result.internal_format = GL_RGBA;
    return result;
}

static gl_texture_t gl_create_texture_depth(
    int width,
    int height,
    const void* data)
{
    gl_texture_t result = {};
    glGenTextures(1, &result.handle);
    glBindTexture(GL_TEXTURE_2D, result.handle);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, data);

    // 过滤方式（Shadow Map 推荐 NEAREST）
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // 边缘模式（阴影常用 CLAMP_TO_BORDER）
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // 超出 [0,1] 的深度视为 1.0（无阴影）
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    GLfloat clearDepth = 1.0f;
    if (data == nullptr) glClearTexImage(result.handle, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &clearDepth);

    result.width = width;
    result.height = height;
    result.target = GL_TEXTURE_2D;
    result.format = GL_DEPTH_COMPONENT;
    result.internal_format = GL_DEPTH_COMPONENT;
    return result;
}

static gl_texture_t gl_create_texture_depth_stencil(
    int width,
    int height,
    const void* data)
{
    gl_texture_t result = {};
    glGenTextures(1, &result.handle);
    glBindTexture(GL_TEXTURE_2D, result.handle);

    // 使用 GL_DEPTH24_STENCIL8 格式（24位深度 + 8位模板）
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, data);

    // 过滤方式（通常使用 NEAREST）
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // 边缘模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 设置深度比较模式（可选，用于阴影映射）
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    GLuint clearValue = 0xFFFFFF00u; // 深度 24 位全 1 (= 1.0)，模板 8 位 = 0
    if (data == nullptr) glClearTexSubImage(result.handle, 0, 0, 0, 0, width, height, 1, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, &clearValue );

    result.width = width;
    result.height = height;
    result.target = GL_TEXTURE_2D;
    result.format = GL_DEPTH_STENCIL;
    result.internal_format = GL_DEPTH24_STENCIL8;
    return result;
}

static void gl_destroy_texture(gl_texture_t& texture)
{
    glDeleteTextures(1, &texture.handle);
    texture.handle = 0;
}

static void gl_bind_texture(gl_texture_t texture, gl_texture_bind_t desc)
{
    glActiveTexture(GL_TEXTURE0 + desc.binding);
    glBindTexture(texture.target, texture.handle);

    if (texture.format == GL_DEPTH_COMPONENT || texture.format == GL_DEPTH_STENCIL)
    {
        glTexParameteri(texture.target, GL_DEPTH_STENCIL_TEXTURE_MODE, (GLint)desc.aspect_mode);
    }
}

static void gl_bind_texture_storage(gl_texture_t texture, gl_texture_storage_bind_t desc)
{
    glBindImageTexture(desc.binding, texture.handle, (GLint)desc.base_level, 1 < desc.layer_count,
                       (GLint)desc.base_layer, desc.access, texture.internal_format);
}

// ====================================================================

static gl_sampler_t gl_create_sampler(
    GLenum min_filter,      // 缩小过滤方式，如 GL_LINEAR_MIPMAP_LINEAR
    GLenum mag_filter,      // 放大过滤方式，如 GL_LINEAR
    GLenum wrap_s,          // S 轴环绕方式，如 GL_REPEAT
    GLenum wrap_t,          // T 轴环绕方式，如 GL_REPEAT
    GLenum wrap_r)          // R 轴环绕方式，如 GL_REPEAT
{
    gl_sampler_t result = {};

    glGenSamplers(1, &result.handle);

    // 设置过滤方式
    glSamplerParameteri(result.handle, GL_TEXTURE_MIN_FILTER, min_filter);
    glSamplerParameteri(result.handle, GL_TEXTURE_MAG_FILTER, mag_filter);

    // 设置环绕方式
    glSamplerParameteri(result.handle, GL_TEXTURE_WRAP_S, wrap_s);
    glSamplerParameteri(result.handle, GL_TEXTURE_WRAP_T, wrap_t);
    glSamplerParameteri(result.handle, GL_TEXTURE_WRAP_R, wrap_r);

    return result;
}

static void gl_destroy_sampler(gl_sampler_t& sampler)
{
    glDeleteSamplers(1, &sampler.handle);
    sampler.handle = 0;
}

static void gl_bind_sampler(
    gl_sampler_t sampler,
    GLuint texture_unit)
{
    glBindSampler(texture_unit, sampler.handle);
}

// ====================================================================

static gl_texture_t gl_image_2_texture(gl_image_t image)
{
    return gl_create_texture_color(image.width, image.height, image.pixels);
}

static gl_image_t gl_texture_2_image(gl_texture_t texture, void* buffer, size_t length)
{
    gl_image_t result = {};

    if (texture.target == GL_TEXTURE_2D)
    {
        if (length < texture.width * texture.height * sizeof(uint32_t)) return result;

        glBindTexture(texture.target, texture.handle);
        glGetTexImage(
            texture.target,
            0,
            texture.format,
            GL_UNSIGNED_BYTE,
            buffer
        );
        glBindTexture(texture.target, 0);
        result.pixels = buffer;
    }

    result.width = texture.width;
    result.height = texture.height;
    result.format = texture.format;
    return result;
}

// ====================================================================

static gl_module_t gl_create_module_compute(const char* comp_src)
{
    gl_module_t result = {};

    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs, 1, &comp_src, nullptr);
    glCompileShader(cs);

    GLint success = 0;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(cs, sizeof(log), nullptr, log);
        fprintf(stderr, "Compute shader compile error:\n%s\n", log);
        abort();
    }

    result.handle = glCreateProgram();
    glAttachShader(result.handle, cs);
    glLinkProgram(result.handle);

    glGetProgramiv(result.handle, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(result.handle, sizeof(log), nullptr, log);
        fprintf(stderr, "Compute program link error:\n%s\n", log);
        abort();
    }

    glDeleteShader(cs);

    result.target = GL_COMPUTE_SHADER;
    return result;
}

static gl_module_t gl_create_module_graphics(
    const char* vert_src,
    const char* frag_src)
{
    gl_module_t result = {};

    // ---- Vertex Shader ----
    GLuint vs = 0;
    if (vert_src)
    {
        vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vert_src, nullptr);
        glCompileShader(vs);
        GLint success = 0;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[1024];
            glGetShaderInfoLog(vs, sizeof(log), nullptr, log);
            fprintf(stderr, "vs compile error:\n%s\n", log);
            abort();
        }
    }

    // ---- Fragment Shader ----
    GLuint fs = 0;
    if (frag_src)
    {
        fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &frag_src, nullptr);
        glCompileShader(fs);
        GLint success = 0;
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[1024];
            glGetShaderInfoLog(fs, sizeof(log), nullptr, log);
            fprintf(stderr, "vs compile error:\n%s\n", log);
            abort();
        }
    }

    // ---- Program ----
    result.handle = glCreateProgram();
    if (vs)
        glAttachShader(result.handle, vs);
    if (fs)
        glAttachShader(result.handle, fs);
    glLinkProgram(result.handle);

    GLint success = 0;
    glGetProgramiv(result.handle, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(result.handle, sizeof(log), nullptr, log);
        fprintf(stderr, "Program link error:\n%s\n", log);
        abort();
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    result.target = GL_VERTEX_SHADER;
    return result;
}

static gl_module_t gl_create_module_meshlet(
    const char* task_src,
    const char* mesh_src,
    const char* frag_src)
{
    gl_module_t result = {};

    // ---- Task Shader（可选）----
    GLuint ts = 0;
    if (task_src)
    {
        ts = glCreateShader(GL_TASK_SHADER_NV);
        glShaderSource(ts, 1, &task_src, nullptr);
        glCompileShader(ts);
        GLint success = 0;
        glGetShaderiv(ts, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[1024];
            glGetShaderInfoLog(ts, sizeof(log), nullptr, log);
            fprintf(stderr, "Task shader compile error:\n%s\n", log);
            abort();
        }
    }

    // ---- Mesh Shader ----
    GLuint ms = glCreateShader(GL_MESH_SHADER_NV);
    glShaderSource(ms, 1, &mesh_src, nullptr);
    glCompileShader(ms);
    GLint success = 0;
    glGetShaderiv(ms, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(ms, sizeof(log), nullptr, log);
        fprintf(stderr, "Mesh shader compile error:\n%s\n", log);
        abort();
    }

    // ---- Fragment Shader ----
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &frag_src, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(fs, sizeof(log), nullptr, log);
        fprintf(stderr, "Fragment shader compile error:\n%s\n", log);
        abort();
    }

    // ---- Program ----
    result.handle = glCreateProgram();
    if (ts) glAttachShader(result.handle, ts);
    glAttachShader(result.handle, ms);
    glAttachShader(result.handle, fs);
    glLinkProgram(result.handle);

    glGetProgramiv(result.handle, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(result.handle, sizeof(log), nullptr, log);
        fprintf(stderr, "Mesh program link error:\n%s\n", log);
        abort();
    }

    if (ts) glDeleteShader(ts);
    glDeleteShader(ms);
    glDeleteShader(fs);

    result.target = GL_MESH_SHADER_NV;
    return result;
}

static void gl_destroy_module(gl_module_t& module)
{
    glDeleteProgram(module.handle);
    module.handle = 0;
}

static void gl_set_uniform_int(const char* name, int32_t value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform1i(loc, value);
}

static void gl_set_uniform_uint(const char* name, uint32_t value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform1ui(loc, value);
}

static void gl_set_uniform_float(const char* name, float value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform1f(loc, value);
}

static void gl_set_uniform_vec2(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform2fv(loc, 1, value);
}

static void gl_set_uniform_vec3(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform3fv(loc, 1, value);
}

static void gl_set_uniform_vec4(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform4fv(loc, 1, value);
}

static void gl_set_uniform_mat3(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniformMatrix3fv(loc, 1, GL_FALSE, value);
}

static void gl_set_uniform_mat4(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniformMatrix4fv(loc, 1, GL_FALSE, value);
}

// ====================================================================

inline void gl_begin_compute(gl_pass_t& pass)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }

    if (pass.module.handle == 0)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }

    if (pass.module.target != GL_COMPUTE_SHADER)
    {
        fprintf(stderr, "Pipeline module is not compute shader\n");
        abort();
    }

    glUseProgram(pass.module.handle);
}

inline void gl_end_compute(gl_pass_t& pass)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program && program != pass.module.handle)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }

    if (pass.module.handle == 0)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }

    if (pass.module.target != GL_COMPUTE_SHADER)
    {
        fprintf(stderr, "Pipeline module is not compute shader\n");
        abort();
    }

    glUseProgram(0);
}

static void gl_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    glDispatchCompute(std::max(1U, groupX), std::max(1U, groupY), std::max(1U, groupZ));
}

static void gl_begin_render(gl_pass_t& pass)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }
    if (pass.module.handle == 0)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (pass.module.target != GL_VERTEX_SHADER && pass.module.target != GL_MESH_SHADER_NV)
    {
        fprintf(stderr, "Pipeline module is not render shader\n");
        abort();
    }

    pass.handle = 0;
    glUseProgram(pass.module.handle);

    glDisable(GL_SCISSOR_TEST);

    // Depth State

    if (pass.depth.func == GL_ALWAYS && pass.depth.write == false)
    {
        glDisable(GL_DEPTH_TEST);
    }
    else
    {
        glEnable(GL_DEPTH_TEST);
    }
    glDepthMask(pass.depth.write);
    glDepthFunc(pass.depth.func);

    // Stencil State

    if (pass.stencil.back.func != GL_ALWAYS || pass.stencil.back.sfail != GL_KEEP || pass.stencil.back.dpfail != GL_KEEP || pass.stencil.back.dppass != GL_KEEP
        || pass.stencil.front.func != GL_ALWAYS || pass.stencil.front.sfail != GL_KEEP || pass.stencil.front.dpfail != GL_KEEP || pass.stencil.front.dppass != GL_KEEP)
    {
        glEnable(GL_STENCIL_TEST);
    }
    else
    {
        glDisable(GL_STENCIL_TEST);
    }
    glStencilMask(pass.stencil.write);
    glStencilFuncSeparate(GL_BACK, pass.stencil.back.func, 0, pass.stencil.read);
    glStencilFuncSeparate(GL_FRONT, pass.stencil.front.func, 0, pass.stencil.read);
    glStencilOpSeparate(GL_BACK, pass.stencil.back.sfail, pass.stencil.back.dpfail, pass.stencil.back.dppass);
    glStencilOpSeparate(GL_FRONT, pass.stencil.front.sfail, pass.stencil.front.dpfail, pass.stencil.front.dppass);

    // Render State

    bool offscreen = pass.depth.texture.handle;
    for (size_t i=0; i<std::size(pass.color) && !offscreen; ++i)
    {
        if (pass.color[i].texture.handle) offscreen = true;
    }
    if (offscreen)
    {
        glGenFramebuffers(1, &pass.handle);
        glBindFramebuffer(GL_FRAMEBUFFER, pass.handle);

        int32_t colorCount = 0;
        GLenum colorAttachments[16]{};
        uint32_t width = 0, height = 0;
        for (size_t i=0; i<std::size(pass.color); ++i)
        {
            if (pass.color[i].texture.handle)
            {
                glBindTexture(GL_TEXTURE_2D, pass.color[i].texture.handle);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, pass.color[i].texture.handle, 0);
                colorAttachments[colorCount++] = GL_COLOR_ATTACHMENT0 + i;
                width = std::max(width, pass.color[i].texture.width);
                height = std::max(height, pass.color[i].texture.height);
            }
        }
        if (colorCount) glDrawBuffers(colorCount, colorAttachments);

        if (pass.depth.texture.handle)
        {
            glBindTexture(GL_TEXTURE_2D, pass.depth.texture.handle);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass.depth.texture.handle, 0);
            width = std::max(width, pass.depth.texture.width);
            height = std::max(height, pass.depth.texture.height);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            fprintf(stderr, "Framebuffer not complete\n");
            abort();
        }
        gl_set_viewport(0, 0, (int32_t)width, (int32_t)height);

        glDisable(GL_BLEND);
        for (size_t i=0; i<std::size(pass.color); ++i)
        {
            if (pass.color[i].texture.handle)
            {
                if (pass.color[i].clear) glColorMask(true, true, true, true);
                if (pass.color[i].clear) glClearBufferfv(GL_COLOR, (int32_t)i, pass.color[i].value);

                if (pass.color[i].blend_color.opt != GL_ADD || pass.color[i].blend_color.src != GL_ONE || pass.color[i].blend_color.dst != GL_ZERO
                    || pass.color[i].blend_alpha.opt != GL_ADD || pass.color[i].blend_alpha.src != GL_ONE || pass.color[i].blend_alpha.dst != GL_ZERO) glEnable(GL_BLEND);
                glBlendEquationSeparatei(i, pass.color[i].blend_color.opt, pass.color[i].blend_alpha.opt);
                glBlendFuncSeparatei(i, pass.color[i].blend_color.src, pass.color[i].blend_color.dst, pass.color[i].blend_alpha.src, pass.color[i].blend_alpha.dst);
            }
        }

        if (pass.depth.texture.handle && (pass.depth.texture.format == GL_DEPTH_COMPONENT || pass.depth.texture.format == GL_DEPTH_STENCIL))
        {
            if (pass.depth.clear)
            {
                glClearBufferfv(GL_DEPTH, 0, &pass.depth.value);
            }
            if (pass.depth.clear && pass.depth.texture.format == GL_DEPTH_STENCIL)
            {
                auto stencilValue = (int32_t)pass.stencil.value;
                glClearBufferiv(GL_STENCIL, 0, &stencilValue);
            }
        }
    }
    else
    {
        glClearColor(pass.screen.color_value[0], pass.screen.color_value[1], pass.screen.color_value[2], pass.screen.color_value[3]);
        glClearDepth(pass.depth.value);
        glClearStencil((int32_t)pass.stencil.value);

        auto mask = GL_NONE;
        if (pass.screen.clear_color) mask |= GL_COLOR_BUFFER_BIT;
        if (pass.screen.clear_depth) mask |= GL_DEPTH_BUFFER_BIT;
        if (pass.screen.clear_stencil) mask |= GL_STENCIL_BUFFER_BIT;
        if (mask) glClear(mask);

        if (pass.screen.blend.opt != GL_ADD || pass.screen.blend.src != GL_ONE || pass.screen.blend.dst != GL_ZERO) glEnable(GL_BLEND);
        else glDisable(GL_BLEND);
        glBlendEquation(pass.screen.blend.opt);
        glBlendFunc(pass.screen.blend.src, pass.screen.blend.dst);
    }

    // Primitive State

    glFrontFace(pass.front_face);
    if (pass.cull_mode) glCullFace(pass.cull_mode);
    if (pass.cull_mode) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);

    if (pass.front_face) glEnable(GL_FRONT_FACE);
    else glDisable(GL_FRONT_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, pass.fill_mode);
}

static void gl_end_render(gl_pass_t& pass)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program && program != pass.module.handle)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }
    if (pass.module.handle == 0)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (pass.module.target != GL_VERTEX_SHADER && pass.module.target != GL_MESH_SHADER_NV)
    {
        fprintf(stderr, "Pipeline module is not render shader\n");
        abort();
    }

    bool offscreen = pass.depth.texture.handle;
    for (size_t i=0; i<std::size(pass.color) && !offscreen; ++i)
    {
        if (pass.color[i].texture.handle) offscreen = true;
    }
    if (offscreen)
    {
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glDeleteFramebuffers(1, &pass.handle);
        pass.handle = 0;
    }

    glUseProgram(0);
}

static void gl_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
    glViewport(x, y, width, height);
}

static void gl_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    glScissor(x, y, width, height);
}

static void gl_draw_mesh_task(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    glDrawMeshTasksNV(0, std::max(1U, groupX) * std::max(1U, groupY) * std::max(1U, groupZ));
}

// ====================================================================

static gl_mesh_t gl_create_mesh(
    const float* vertices,   // vec3
    const float* normals,   // vec3
    const float* uvs,       // vec2
    size_t vertex_count,
    const unsigned int* indices,
    size_t index_count)
{
    gl_mesh_t result = {};

    glGenVertexArrays(1, &result.handle);
    glBindVertexArray(result.handle);

    if (vertices)
    {
        result.vertex_vbo = gl_create_buffer(vertex_count * 3 * sizeof(float), GL_STATIC_DRAW, vertices);
        glBindBuffer(GL_ARRAY_BUFFER, result.vertex_vbo.handle);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    if (normals)
    {
        result.normal_vbo = gl_create_buffer(vertex_count * 3 * sizeof(float), GL_STATIC_DRAW, normals);
        glBindBuffer(GL_ARRAY_BUFFER, result.normal_vbo.handle);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
    }

    if (uvs)
    {
        result.uv_vbo = gl_create_buffer(vertex_count * 2 * sizeof(float), GL_STATIC_DRAW, uvs);
        glBindBuffer(GL_ARRAY_BUFFER, result.uv_vbo.handle);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(2);
    }

    if (indices)
    {
        result.index_vbo = gl_create_buffer(index_count * sizeof(uint32_t), GL_STATIC_DRAW, indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.index_vbo.handle);
    }

    glBindVertexArray(0);

    result.vertex_count = (GLsizei)vertex_count;
    result.index_count = (GLsizei)index_count;
    result.index_type = GL_UNSIGNED_INT;
    result.primitive_type = GL_TRIANGLES;
    return result;
}

static void gl_destroy_mesh(gl_mesh_t& mesh)
{
    gl_destroy_buffer(mesh.vertex_vbo);
    gl_destroy_buffer(mesh.normal_vbo);
    gl_destroy_buffer(mesh.uv_vbo);
    gl_destroy_buffer(mesh.index_vbo);

    glDeleteVertexArrays(1, &mesh.handle);
    mesh.handle = 0;
}

static void gl_draw_mesh(gl_mesh_t mesh)
{
    glBindVertexArray(mesh.handle);
    if (mesh.index_count) glDrawElements(mesh.primitive_type, mesh.index_count, mesh.index_type, (void*)0);
    else glDrawArrays(mesh.primitive_type, 0, mesh.vertex_count);
    glBindVertexArray(0);
}

// ====================================================================

static gl_meshlet_t gl_create_meshlet(
    const float* vertices,   // vec4
    const float* normals,   // vec4
    const float* uvs,
    size_t vertex_count,
    const unsigned int* indices,
    size_t index_count)
{
    gl_meshlet_t result = {};

    if (vertices)
    {
        result.vertex_vbo = gl_create_buffer(vertex_count * 4 * sizeof(float), GL_STATIC_DRAW, vertices);
    }

    if (normals)
    {
        result.normal_vbo = gl_create_buffer(vertex_count * 4 * sizeof(float), GL_STATIC_DRAW, normals);
    }

    if (uvs)
    {
        result.uv_vbo = gl_create_buffer(vertex_count * 2 * sizeof(float), GL_STATIC_DRAW, uvs);
    }

    if (indices)
    {
        result.index_vbo = gl_create_buffer(index_count * sizeof(uint32_t), GL_STATIC_DRAW, indices);
    }

    result.vertex_count = (GLsizei)vertex_count;
    result.index_count = (GLsizei)index_count;
    result.index_type = GL_UNSIGNED_INT;
    result.primitive_type = GL_TRIANGLES;
    return result;
}

static void gl_destroy_meshlet(gl_meshlet_t& meshlet)
{
    gl_destroy_buffer(meshlet.vertex_vbo);
    gl_destroy_buffer(meshlet.normal_vbo);
    gl_destroy_buffer(meshlet.uv_vbo);
    gl_destroy_buffer(meshlet.index_vbo);
}

// ====================================================================

static gl_mesh_t gl_create_mesh_screen()
{
    static const float points[] = {
        -1.0f, -1.0f, 0.0f,
        3.0f, -1.0f, 0.0f,
        -1.0f, 3.0f, 0.0f,
    };
    static const float uvs[] = {
        0.0f, 0.0f,
        2.0f, 0.0f,
        0.0f, 2.0f,
    };
    static auto quad = gl_create_mesh(points, nullptr, uvs, 3, nullptr, 0);
    return quad;
}

static void gl_draw_screen(int width, int height, gl_texture_t texture)
{
    constexpr auto VS = R"(
        #version 460
        layout(location = 0) in vec3 in_vertex;
        layout(location = 1) in vec3 in_normal;
        layout(location = 2) in vec2 in_uv;
        out vec3 vertex;
        out vec3 normal;
        out vec2 uv;

        void main()
        {
            vertex = in_vertex;
            normal = in_normal;
            uv = in_uv;
            gl_Position = vec4(in_vertex, 1.0);
        }
    )";
    constexpr auto FS = R"(
        #version 460
        in vec3 vertex;
        in vec3 normal;
        in vec2 uv;
        out vec4 final;
        layout(binding = 0) uniform sampler2D texture0;

        void main()
        {
            final = texture(texture0, uv);
        }
    )";
    static auto module = gl_create_module_graphics(VS, FS);
    gl_pass_t pass = {.module = module,};
    gl_begin_render(pass);
    gl_set_viewport(0, 0, width, height);
    gl_bind_texture(texture, {.binding = 0,});
    gl_draw_mesh(gl_create_mesh_screen());
    gl_end_render(pass);
}

#endif