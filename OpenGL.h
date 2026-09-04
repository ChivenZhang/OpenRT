#pragma once
#include <iostream>
#include <GL/glew.h>

// ====================================================================

inline void gl_hello_world()
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

struct gl_buffer_t
{
    GLuint handle = 0;
    size_t size = 0;
    GLenum usage = GL_STATIC_DRAW;
};

inline gl_buffer_t gl_create_buffer(
    size_t size,
    GLenum usage = GL_STATIC_DRAW,
    const void* data = nullptr
)
{
    gl_buffer_t result = {};
    glGenBuffers(1, &result.handle);
    glBindBuffer(GL_ARRAY_BUFFER, result.handle);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)size, data, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return result;
}

inline void gl_delete_buffer(gl_buffer_t buffer)
{
    glDeleteBuffers(1, &buffer.handle);
}

struct gl_buffer_bind_t
{
    uint32_t binding = 0;
    GLenum target = GL_UNIFORM_BUFFER;
};

inline void gl_bind_buffer(gl_buffer_t buffer, gl_buffer_bind_t desc = {})
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

inline void gl_read_buffer(
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

inline void gl_write_buffer(
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

struct gl_texture_t
{
    GLuint handle = 0;
    uint32_t width = 0, height = 0;
    GLenum target = GL_TEXTURE_2D;
    GLenum format = GL_RGBA;
    GLenum internal_format = GL_RGBA;
};

inline gl_texture_t gl_create_texture_color(
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

    result.width = width;
    result.height = height;
    result.target = GL_TEXTURE_2D;
    result.format = GL_RGBA;
    result.internal_format = GL_RGBA;
    return result;
}

inline gl_texture_t gl_create_texture_depth(
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

    result.width = width;
    result.height = height;
    result.target = GL_TEXTURE_2D;
    result.format = GL_DEPTH_COMPONENT;
    result.internal_format = GL_DEPTH_COMPONENT;
    return result;
}

inline gl_texture_t gl_create_texture_depth_stencil(
    int width,
    int height,
    const void* data)
{
    gl_texture_t result = {};
    glGenTextures(1, &result.handle);
    glBindTexture(GL_TEXTURE_2D, result.handle);

    // 使用 GL_DEPTH24_STENCIL8 格式（24位深度 + 8位模板）
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0,
                 GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, data);

    // 过滤方式（通常使用 NEAREST）
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // 边缘模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 设置深度比较模式（可选，用于阴影映射）
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    result.width = width;
    result.height = height;
    result.target = GL_TEXTURE_2D;
    result.format = GL_DEPTH_STENCIL;
    result.internal_format = GL_DEPTH24_STENCIL8;
    return result;
}

inline void gl_destroy_texture(gl_texture_t texture)
{
    glDeleteTextures(1, &texture.handle);
    texture.handle = 0;
}

struct gl_texture_bind_t
{
    uint32_t binding = 0;
    GLenum aspect_mode = GL_DEPTH_COMPONENT;
};

inline void gl_bind_texture(gl_texture_t texture, gl_texture_bind_t desc = {})
{
    glActiveTexture(GL_TEXTURE0 + desc.binding);
    glBindTexture(texture.target, texture.handle);

    if (texture.format == GL_DEPTH_COMPONENT || texture.format == GL_DEPTH_STENCIL)
    {
        glTexParameteri(texture.target, GL_DEPTH_STENCIL_TEXTURE_MODE, (GLint)desc.aspect_mode);
    }
}

struct gl_texture_storage_bind_t
{
    uint32_t binding = 0;
    uint32_t base_level = 0;
    uint32_t base_layer = 0;
    uint32_t level_count = 1;
    uint32_t layer_count = 1;
    GLenum access = GL_READ_WRITE;
};

inline void gl_bind_texture_storage(gl_texture_t texture, gl_texture_storage_bind_t desc = {})
{
    glBindImageTexture(desc.binding, texture.handle, (GLint)desc.base_level, 1 < desc.layer_count,
                       (GLint)desc.base_layer, desc.access, texture.internal_format);
}

// ====================================================================

struct gl_image_t
{
    void* pixels = nullptr;
    uint32_t width = 0, height = 0;
    GLenum format = GL_RGBA;
};

inline gl_texture_t gl_image_2_texture(gl_image_t image)
{
    return gl_create_texture_color(image.width, image.height, image.pixels);
}

inline gl_image_t gl_texture_2_image(gl_texture_t texture, void* buffer, size_t length)
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

struct gl_sampler_t
{
    GLuint handle = 0;
};

inline gl_sampler_t gl_create_sampler(
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

inline void gl_destroy_sampler(gl_sampler_t sampler)
{
    glDeleteSamplers(1, &sampler.handle);
    sampler.handle = 0;
}

inline void gl_bind_sampler(
    gl_sampler_t sampler,
    GLuint texture_unit)
{
    glBindSampler(texture_unit, sampler.handle);
}

// ====================================================================

struct gl_program_t
{
    GLuint handle = 0;
};

inline gl_program_t gl_create_program_graphics(
    const char* vert_src,
    const char* frag_src)
{
    gl_program_t result = {};

    // ---- Vertex Shader ----
    GLuint vs = 0;
    if (vert_src)
    {
        vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vert_src, NULL);
        glCompileShader(vs);
        GLint success = 0;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetShaderInfoLog(vs, sizeof(log), NULL, log);
            fprintf(stderr, "vs compile error:\n%s\n", log);
            abort();
        }
    }

    // ---- Fragment Shader ----
    GLuint fs = 0;
    if (frag_src)
    {
        fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &frag_src, NULL);
        glCompileShader(fs);
        GLint success = 0;
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetShaderInfoLog(fs, sizeof(log), NULL, log);
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
        char log[512];
        glGetProgramInfoLog(result.handle, sizeof(log), NULL, log);
        fprintf(stderr, "Program link error:\n%s\n", log);
        abort();
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return result;
}

inline gl_program_t gl_create_program_compute(const char* comp_src)
{
    gl_program_t result = {};

    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs, 1, &comp_src, NULL);
    glCompileShader(cs);

    GLint success = 0;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetShaderInfoLog(cs, sizeof(log), NULL, log);
        fprintf(stderr, "Compute shader compile error:\n%s\n", log);
        abort();
    }

    result.handle = glCreateProgram();
    glAttachShader(result.handle, cs);
    glLinkProgram(result.handle);

    glGetProgramiv(result.handle, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetProgramInfoLog(result.handle, sizeof(log), NULL, log);
        fprintf(stderr, "Compute program link error:\n%s\n", log);
        abort();
    }

    glDeleteShader(cs);

    return result;
}

inline gl_program_t gl_create_program_mesh(
    const char* task_src,
    const char* mesh_src,
    const char* frag_src)
{
    gl_program_t result = {};

    // ---- Task Shader（可选）----
    GLuint task = 0;
    if (task_src)
    {
        task = glCreateShader(GL_TASK_SHADER_NV);
        glShaderSource(task, 1, &task_src, NULL);
        glCompileShader(task);
        GLint success = 0;
        glGetShaderiv(task, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetShaderInfoLog(task, sizeof(log), NULL, log);
            fprintf(stderr, "Task shader compile error:\n%s\n", log);
            abort();
        }
    }

    // ---- Mesh Shader ----
    GLuint mesh = glCreateShader(GL_MESH_SHADER_NV);
    glShaderSource(mesh, 1, &mesh_src, NULL);
    glCompileShader(mesh);
    GLint success = 0;
    glGetShaderiv(mesh, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetShaderInfoLog(mesh, sizeof(log), NULL, log);
        fprintf(stderr, "Mesh shader compile error:\n%s\n", log);
        abort();
    }

    // ---- Fragment Shader ----
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &frag_src, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetShaderInfoLog(fs, sizeof(log), NULL, log);
        fprintf(stderr, "Fragment shader compile error:\n%s\n", log);
        abort();
    }

    // ---- Program ----
    result.handle = glCreateProgram();
    if (task)
        glAttachShader(result.handle, task);
    glAttachShader(result.handle, mesh);
    glAttachShader(result.handle, fs);
    glLinkProgram(result.handle);

    glGetProgramiv(result.handle, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetProgramInfoLog(result.handle, sizeof(log), NULL, log);
        fprintf(stderr, "Mesh program link error:\n%s\n", log);
        abort();
    }

    if (task)
        glDeleteShader(task);
    glDeleteShader(mesh);
    glDeleteShader(fs);

    return result;
}

inline void gl_destroy_program(gl_program_t program)
{
    glDeleteProgram(program.handle);
    program.handle = 0;
}

inline void gl_set_uniform_int(const char* name, int32_t value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform1i(loc, value);
}

inline void gl_set_uniform_uint(const char* name, uint32_t value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform1ui(loc, value);
}

inline void gl_set_uniform_float(const char* name, float value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform1f(loc, value);
}

inline void gl_set_uniform_vec2(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform2fv(loc, 1, value);
}

inline void gl_set_uniform_vec3(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform3fv(loc, 1, value);
}

inline void gl_set_uniform_vec4(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform4fv(loc, 1, value);
}

inline void gl_set_uniform_mat3(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniformMatrix3fv(loc, 1, GL_FALSE, value);
}

inline void gl_set_uniform_mat4(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniformMatrix4fv(loc, 1, GL_FALSE, value);
}

// ====================================================================

struct gl_pipeline_t
{
    GLuint handle = 0;
    gl_texture_t color;
    gl_texture_t depth;
};

inline gl_pipeline_t gl_create_pipeline(
    int width,
    int height,
    gl_texture_t color,
    gl_texture_t depth)
{
    gl_pipeline_t result = {};
    glGenFramebuffers(1, &result.handle);
    glBindFramebuffer(GL_FRAMEBUFFER, result.handle);

    if (color.handle && width == color.width && height == color.height)
    {
        glBindTexture(GL_TEXTURE_2D, color.handle);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color.handle, 0);
    }
    if (depth.handle && width == depth.width && height == depth.height)
    {
        glBindTexture(GL_TEXTURE_2D, depth.handle);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth.handle, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Framebuffer not complete\n");
        abort();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    result.color = color;
    result.depth = depth;
    return result;
}

inline void gl_destroy_pipeline(gl_pipeline_t pipeline)
{
    glDeleteFramebuffers(1, &pipeline.handle);
    pipeline.handle = 0;
}

struct gl_color_t
{
    float r = 0, g = 0, b = 0, a = 0;
};

struct gl_pipeline_bind_t
{
    gl_program_t program;
    bool clear_color = false;
    bool clear_depth = false;
    bool clear_stencil = false;
    bool depth_test = false;
    bool depth_write = true;
    bool scissor_test = false;
    gl_color_t color_value;
    float depth_value = 1.0f;
    GLenum depth_func = GL_LESS;
    GLenum cull_mode = GL_BACK;
    GLenum front_face = GL_CCW;
    GLenum fill_mode = GL_FILL;
};

inline void gl_begin_render(gl_pipeline_t pipeline, gl_pipeline_bind_t desc = {})
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, pipeline.handle);
    glUseProgram(desc.program.handle);

    glClearDepth(desc.depth_value);

    glClearColor(desc.color_value.r, desc.color_value.g, desc.color_value.b, desc.color_value.a);

    auto clearMask = GL_NONE;
    if (desc.clear_color) clearMask |= GL_COLOR_BUFFER_BIT;
    if (desc.clear_depth) clearMask |= GL_DEPTH_BUFFER_BIT;
    if (desc.clear_stencil) clearMask |= GL_STENCIL_BUFFER_BIT;
    if (clearMask != GL_NONE) glClear(clearMask);

    if (desc.depth_test) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);

    if (desc.scissor_test) glEnable(GL_SCISSOR_TEST);
    else glDisable(GL_SCISSOR_TEST);

    glDepthMask(desc.depth_write);

    glDepthFunc(desc.depth_func);

    glCullFace(desc.cull_mode);
    glFrontFace(desc.front_face);
    if (desc.cull_mode) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);

    if (desc.front_face) glEnable(GL_FRONT_FACE);
    else glDisable(GL_FRONT_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, desc.fill_mode);
}

inline void gl_end_render(gl_pipeline_t pipeline)
{
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER,0);
}

inline void gl_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
    glViewport(x, y, width, height);
}

inline void gl_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    glScissor(x, y, width, height);
}

inline void gl_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    glDispatchCompute(std::max(1U, groupX), std::max(1U, groupY), std::max(1U, groupZ));
}

inline void gl_draw_mesh_task(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    glDrawMeshTasksNV(0, std::max(1U, groupX) * std::max(1U, groupY) * std::max(1U, groupZ));
}

// ====================================================================

struct gl_mesh_t
{
    GLuint handle = 0;

    GLuint vertex_vbo = 0;
    GLuint normal_vbo = 0;
    GLuint uv_vbo = 0;
    GLsizei vertex_count = 0;

    GLuint index_vbo = 0;
    GLsizei index_count = 0;
    GLenum index_type = GL_UNSIGNED_INT;

    GLenum primitive_type = GL_TRIANGLES;
};

inline gl_mesh_t gl_create_mesh(
    const float* positions,
    const float* normals,
    const float* uvs,
    size_t vertex_count,
    const unsigned int* indices,
    size_t index_count)
{
    gl_mesh_t result = {};

    glGenVertexArrays(1, &result.handle);
    glGenBuffers(1, &result.vertex_vbo);
    glGenBuffers(1, &result.normal_vbo);
    glGenBuffers(1, &result.uv_vbo);
    glGenBuffers(1, &result.index_vbo);

    glBindVertexArray(result.handle);

    // position
    if (positions)
    {
        glBindBuffer(GL_ARRAY_BUFFER, result.vertex_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), positions, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    // normal
    if (normals)
    {
        glBindBuffer(GL_ARRAY_BUFFER, result.normal_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), normals, GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
    }

    // uv
    if (uvs)
    {
        glBindBuffer(GL_ARRAY_BUFFER, result.uv_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * 2 * sizeof(float), uvs, GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(2);
    }

    // index
    if (indices)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.index_vbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    }

    glBindVertexArray(0);

    result.index_count = (GLsizei)index_count;
    result.vertex_count = (GLsizei)vertex_count;
    result.index_type = GL_UNSIGNED_INT;
    return result;
}

inline void gl_destroy_mesh(gl_mesh_t mesh)
{
    glDeleteVertexArrays(1, &mesh.handle);
    glDeleteBuffers(1, &mesh.vertex_vbo);
    glDeleteBuffers(1, &mesh.normal_vbo);
    glDeleteBuffers(1, &mesh.uv_vbo);
    glDeleteBuffers(1, &mesh.index_vbo);
    mesh.handle = 0;
}

inline void gl_draw_mesh(gl_mesh_t mesh)
{
    glBindVertexArray(mesh.handle);
    if (mesh.index_count) glDrawElements(mesh.primitive_type, mesh.index_count, mesh.index_type, (void*)0);
    else glDrawArrays(mesh.primitive_type, 0, mesh.vertex_count);
    glBindVertexArray(0);
}

// ====================================================================

#include <vector>

inline gl_mesh_t gl_create_mesh_screen()
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

inline void gl_draw_screen(int width, int height, gl_texture_t texture)
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
    static auto program = gl_create_program_graphics(VS, FS);
    gl_begin_render({}, {.program = program,});
    gl_set_viewport(0, 0, width, height);
    gl_bind_texture(texture, {.binding = 0,});
    gl_draw_mesh(gl_create_mesh_screen());
    gl_end_render({});
}

inline gl_mesh_t gl_create_mesh_cube(float width, float height, float length)
{
    static const float v[24 * 3] = {
        // 前
        -1, -1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1,
        // 后
        1, -1, -1, -1, -1, -1, -1, 1, -1, 1, 1, -1,
        // 上
        -1, 1, 1, 1, 1, 1, 1, 1, -1, -1, 1, -1,
        // 下
        -1, -1, -1, 1, -1, -1, 1, -1, 1, -1, -1, 1,
        // 右
        1, -1, 1, 1, -1, -1, 1, 1, -1, 1, 1, 1,
        // 左
        -1, -1, -1, -1, -1, 1, -1, 1, 1, -1, 1, -1,
    };

    static const float n[24 * 3] = {
        // 前
        0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
        // 后
        0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1,
        // 上
        0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
        // 下
        0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0,
        // 右
        1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
        // 左
        -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0,
    };

    static const float uv[24 * 2] = {
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
    };

    static const unsigned int idx[6 * 6] = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23,
    };

    std::vector<float> positions(24 * 3);
    std::vector<float> normals(24 * 3);
    std::vector<float> uvs(24 * 2);

    for (int i = 0; i < 24; i++)
    {
        positions[i * 3 + 0] = v[i * 3 + 0] * width * 0.5f;
        positions[i * 3 + 1] = v[i * 3 + 1] * height * 0.5f;
        positions[i * 3 + 2] = v[i * 3 + 2] * length * 0.5f;

        normals[i * 3 + 0] = n[i * 3 + 0];
        normals[i * 3 + 1] = n[i * 3 + 1];
        normals[i * 3 + 2] = n[i * 3 + 2];

        uvs[i * 2 + 0] = uv[i * 2 + 0];
        uvs[i * 2 + 1] = uv[i * 2 + 1];
    }

    return gl_create_mesh(
        positions.data(), normals.data(), uvs.data(),
        24, idx, 36
    );
}

inline gl_mesh_t gl_create_mesh_sphere(float radius, int rings, int slices)
{
    int vertex_count = (rings + 1) * (slices + 1);
    int index_count = rings * slices * 6;

    std::vector<float> positions(vertex_count * 3);
    std::vector<float> normals(vertex_count * 3);
    std::vector<float> uvs(vertex_count * 2);
    std::vector<unsigned int> indices(index_count);

    int v = 0;
    for (int r = 0; r <= rings; r++)
    {
        float theta = r * 3.14159265f / rings;
        for (int s = 0; s <= slices; s++)
        {
            float phi = s * 2.0f * 3.14159265f / slices;

            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = sinf(theta) * sinf(phi);

            positions[v * 3 + 0] = x * radius;
            positions[v * 3 + 1] = y * radius;
            positions[v * 3 + 2] = z * radius;

            normals[v * 3 + 0] = x;
            normals[v * 3 + 1] = y;
            normals[v * 3 + 2] = z;

            uvs[v * 2 + 0] = (float)s / slices;
            uvs[v * 2 + 1] = (float)r / rings;

            v++;
        }
    }

    int idx = 0;
    for (int r = 0; r < rings; r++)
    {
        for (int s = 0; s < slices; s++)
        {
            int tl = r * (slices + 1) + s;
            int tr = tl + 1;
            int bl = (r + 1) * (slices + 1) + s;
            int br = bl + 1;

            indices[idx++] = tl;
            indices[idx++] = tr;
            indices[idx++] = bl;

            indices[idx++] = tr;
            indices[idx++] = br;
            indices[idx++] = bl;
        }
    }

    return gl_create_mesh(
        positions.data(), normals.data(), uvs.data(),
        vertex_count, indices.data(), index_count
    );
}

inline gl_mesh_t gl_create_mesh_plane(int N, float size = 1.0f)
{
    int vertsX = N + 1;
    int vertsY = N + 1;
    size_t vertex_count = vertsX * vertsY;

    std::vector<float> positions(vertex_count * 3);
    std::vector<float> normals(vertex_count * 3);
    std::vector<float> uvs(vertex_count * 2);

    // ---- 生成顶点 ----
    for (int z = 0; z <= N; z++)
    {
        for (int x = 0; x <= N; x++)
        {
            size_t i = z * vertsX + x;

            float fx = (float)x / N;
            float fz = (float)z / N;

            positions[i * 3 + 0] = fx * size;
            positions[i * 3 + 1] = 0.0f; // Y = 0
            positions[i * 3 + 2] = fz * size;

            normals[i * 3 + 0] = 0.0f;
            normals[i * 3 + 1] = 1.0f; // 法线朝上 (+Y)
            normals[i * 3 + 2] = 0.0f;

            uvs[i * 2 + 0] = fx;
            uvs[i * 2 + 1] = fz;
        }
    }

    // ---- 生成索引 ----
    size_t index_count = N * N * 6;
    std::vector<unsigned int> indices(index_count);

    size_t idx = 0;
    for (int z = 0; z < N; z++)
    {
        for (int x = 0; x < N; x++)
        {
            unsigned int tl = z * vertsX + x;
            unsigned int tr = tl + 1;
            unsigned int bl = (z + 1) * vertsX + x;
            unsigned int br = bl + 1;

            indices[idx++] = tl;
            indices[idx++] = bl;
            indices[idx++] = tr;

            indices[idx++] = tr;
            indices[idx++] = bl;
            indices[idx++] = br;
        }
    }

    return gl_create_mesh(
        positions.data(),
        normals.data(),
        uvs.data(),
        vertex_count,
        indices.data(),
        index_count
    );
}