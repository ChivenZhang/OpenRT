/*=================================================
* Copyright © 2020-2026 ChivenZhang.
* All Rights Reserved.
* =====================Note=========================
*
*
* ====================History=======================
* Created by chivenzhang@gmail.com.
*
* =================================================*/
#include "OpenGL.h"
#ifdef OPENGL_IMPLEMENTATION

struct OpenGL
{
    gl_pass_t* currentPipeline = nullptr;
} static thread_local opengl;

void gl_load_library()
{
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        fprintf(stderr, "GLEW init failed: %s\n", (const char*)glewGetErrorString(err));
        abort();
    }
    fprintf(stdout, "OpenGL Version: %s\n", glGetString(GL_VERSION));
    fprintf(stdout, "GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    fprintf(stdout, "OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
    fprintf(stdout, "OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
    GLint maxMeshOutputPrimitives = 0;
    glGetIntegerv(GL_MAX_MESH_OUTPUT_PRIMITIVES_NV, &maxMeshOutputPrimitives);
    GLint maxMeshOutputVertices = 0;
    glGetIntegerv(GL_MAX_MESH_OUTPUT_VERTICES_NV, &maxMeshOutputVertices);
    fprintf(stdout, "Mesh Output Primitives %d\n", maxMeshOutputPrimitives);
    fprintf(stdout, "Mesh Output Vertices %d\n", maxMeshOutputVertices);

    // ====================================================================

    // Buffer 相关
    rhi_create_buffer = gl_create_buffer;
    rhi_destroy_buffer = gl_destroy_buffer;
    rhi_bind_buffer = gl_bind_buffer;
    rhi_read_buffer = gl_read_buffer;
    rhi_write_buffer = gl_write_buffer;

    // Texture 相关
    rhi_create_texture = gl_create_texture;
    rhi_create_texture_color = gl_create_texture_color;
    rhi_create_texture_depth = gl_create_texture_depth;
    rhi_create_texture_depth_stencil = gl_create_texture_depth_stencil;
    rhi_destroy_texture = gl_destroy_texture;
    rhi_bind_texture = gl_bind_texture;
    rhi_bind_texture_storage = gl_bind_texture_storage;
    rhi_load_texture = gl_load_texture;
    rhi_load_image = gl_load_image;

    // Sampler 相关
    rhi_create_sampler = gl_create_sampler;
    rhi_destroy_sampler = gl_destroy_sampler;
    rhi_bind_sampler = gl_bind_sampler;

    // Module 相关
    rhi_create_module_compute = gl_create_module_compute;
    rhi_create_module_render = gl_create_module_render;
    rhi_create_module_meshlet = gl_create_module_meshlet;
    rhi_destroy_module = gl_destroy_module;

    // Uniform 相关
    rhi_push_constant = gl_push_constant;
    rhi_push_const_int = gl_push_const_int;
    rhi_push_const_uint = gl_push_const_uint;
    rhi_push_const_float = gl_push_const_float;
    rhi_push_const_vec2 = gl_push_const_vec2;
    rhi_push_const_vec3 = gl_push_const_vec3;
    rhi_push_const_vec4 = gl_push_const_vec4;
    rhi_push_const_mat3 = gl_push_const_mat3;
    rhi_push_const_mat4 = gl_push_const_mat4;

    // Compute Pass 相关
    rhi_begin_compute = gl_begin_compute;
    rhi_end_compute = gl_end_compute;
    rhi_dispatch_compute = gl_dispatch_compute;

    // Render Pass 相关
    rhi_begin_render = gl_begin_render;
    rhi_end_render = gl_end_render;
    rhi_set_viewport = gl_set_viewport;
    rhi_set_scissor = gl_set_scissor;
    rhi_draw_mesh_task = gl_draw_mesh_task;

    // Mesh 相关
    rhi_create_mesh = gl_create_mesh;
    rhi_destroy_mesh = gl_destroy_mesh;
    rhi_draw_mesh = gl_draw_mesh;

    // Meshlet 相关
    rhi_create_meshlet = gl_create_meshlet;
    rhi_destroy_meshlet = gl_destroy_meshlet;
    rhi_draw_meshlet = gl_draw_meshlet;

    // Screen 相关
    rhi_create_mesh_screen = gl_create_mesh_screen;
    rhi_draw_screen = gl_draw_screen;
}

void gl_unload_library()
{
    opengl.currentPipeline = nullptr;
}

// ====================================================================

gl_buffer_t gl_create_buffer(gl_buffer_desc_t const& info)
{
    gl_buffer_t result = {};
    glGenBuffers(1, &result.handle);
    glBindBuffer(GL_ARRAY_BUFFER, result.handle);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)info.size, info.data, info.usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    result.size = info.size;
    result.target = GL_ARRAY_BUFFER;
    return result;
}

void gl_destroy_buffer(gl_buffer_t& buffer)
{
    glDeleteBuffers(1, &buffer.handle);
    buffer.handle = 0;
}

void gl_bind_buffer(gl_buffer_t buffer, gl_buffer_bind_t bind)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    switch (bind.target)
    {
    case GL_UNIFORM_BUFFER:
        glBindBufferBase(GL_UNIFORM_BUFFER, bind.binding, buffer.handle);
        break;
    case GL_SHADER_STORAGE_BUFFER:
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bind.binding, buffer.handle);
        break;
    default:
        fprintf(stderr, "Unsupported buffer target");
        abort();
    }
}

void gl_read_buffer(gl_buffer_t buffer, size_t offset, size_t size, void* data)
{
    if (!buffer.handle)
        return;
    if (!data || size == 0)
        return;
    if ((GLsizeiptr)(offset + size) > buffer.size)
        return; // 越界保护

    glBindBuffer(GL_ARRAY_BUFFER, buffer.handle);
    glGetBufferSubData(GL_ARRAY_BUFFER, (GLintptr)offset, (GLsizeiptr)size, data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void gl_write_buffer(gl_buffer_t buffer, size_t offset, size_t size, const void* data)
{
    if (!buffer.handle)
        return;
    if (!data || size == 0)
        return;
    if (offset + size > buffer.size)
        return;

    glBindBuffer(GL_ARRAY_BUFFER, buffer.handle);
    glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)offset, (GLsizeiptr)size, data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ====================================================================

gl_texture_t gl_create_texture(gl_texture_desc_t const& info)
{
    gl_texture_t result = {};

    // 生成并绑定纹理
    glGenTextures(1, &result.handle);
    glBindTexture(info.target, result.handle);

    // 上传纹理数据
    if (info.target == GL_TEXTURE_2D)
    {
        glTexImage2D(info.target, 0, info.internal_format, info.width, info.height, 0, info.format, info.type, info.data);
    }
    else if (info.target == GL_TEXTURE_3D)
    {
        glTexImage3D(info.target, 0, info.internal_format, info.width, info.height, 1, 0, info.format, info.type, info.data);
    }
    else
    {
        fprintf(stderr, "Unsupported texture target");
        abort();
    }

    // 处理空数据情况
    if (info.data == nullptr)
    {
        if (info.format == GL_DEPTH_COMPONENT)
        {
            GLfloat clearValue = 1.0f;
            glClearTexImage(result.handle, 0, info.format, info.type, &clearValue);
        }
        else if (info.format == GL_DEPTH_STENCIL)
        {
            if (info.internal_format == GL_DEPTH24_STENCIL8)
            {
                GLuint clearValue = 0xFFFFFF00u; // 深度 24 位全 1 (= 1.0)，模板 8 位 = 0
                glClearTexImage(result.handle, 0, info.format, info.type, &clearValue);
            }
            else if (info.internal_format == GL_DEPTH32F_STENCIL8)
            {
                struct Float32Uint24_8 {
                    float depth;
                    uint32_t stencil;
                } clearValue = {1.0f, 0};
                glClearTexImage(result.handle, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, &clearValue);
            }
            else
            {
                fprintf(stderr, "Unsupported depth stencil format");
                abort();
            }
        }
        else
        {
            GLubyte clearValue[4] = {0, 0, 0, 0};
            glClearTexImage(result.handle, 0, info.format, info.type, clearValue);
        }
    }

    // 生成 mipmap
    if (info.min_filter == GL_NEAREST_MIPMAP_NEAREST || info.min_filter == GL_LINEAR_MIPMAP_NEAREST || info.min_filter == GL_NEAREST_MIPMAP_LINEAR || info.min_filter == GL_LINEAR_MIPMAP_LINEAR)
    {
        glGenerateMipmap(info.target);
        result.mipmaps = true;
    }

    // 设置纹理参数
    glTexParameteri(info.target, GL_TEXTURE_WRAP_S, info.wrap_s);
    glTexParameteri(info.target, GL_TEXTURE_WRAP_T, info.wrap_t);
    glTexParameteri(info.target, GL_TEXTURE_MIN_FILTER, info.min_filter);
    glTexParameteri(info.target, GL_TEXTURE_MAG_FILTER, info.mag_filter);
    // 设置边框颜色（当 wrap 模式为 CLAMP_TO_BORDER 时使用）
    if (info.wrap_s == GL_CLAMP_TO_BORDER || info.wrap_t == GL_CLAMP_TO_BORDER || info.wrap_r == GL_CLAMP_TO_BORDER)
    {
        glTexParameterfv(info.target, GL_TEXTURE_BORDER_COLOR, info.border);
    }

    glBindTexture(info.target, 0);

    result.width = info.width;
    result.height = info.height;
    result.target = info.target;
    result.format = info.format;
    result.internal_format = info.internal_format;
    result.type = info.type;
    return result;
}

gl_texture_t gl_create_texture_color(uint32_t width, uint32_t height, const void* data)
{
    gl_texture_desc_t info
    {
        .width = width,
        .height = height,
        .target = GL_TEXTURE_2D,
        .format = GL_RGBA,
        .internal_format = GL_RGBA,
        .type = GL_UNSIGNED_BYTE,
        .min_filter = GL_LINEAR_MIPMAP_LINEAR,
        .mag_filter = GL_LINEAR,
        .wrap_s = GL_REPEAT,
        .wrap_t = GL_REPEAT,
        .data = data,
    };
    return gl_create_texture(info);
}

gl_texture_t gl_create_texture_depth(uint32_t width, uint32_t height, const void* data)
{
    gl_texture_desc_t info
    {
        .width = width,
        .height = height,
        .target = GL_TEXTURE_2D,
        .format = GL_DEPTH_COMPONENT,
        .internal_format = GL_DEPTH_COMPONENT32F,
        .type = GL_FLOAT,
        .min_filter = GL_NEAREST,
        .mag_filter = GL_NEAREST,
        .wrap_s = GL_CLAMP_TO_BORDER,
        .wrap_t = GL_CLAMP_TO_BORDER,
        .border = {1.0f, 1.0f, 1.0f, 1.0f},
        .data = data,
    };
    return gl_create_texture(info);
}

gl_texture_t gl_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data)
{
    gl_texture_desc_t info
    {
        .width = width,
        .height = height,
        .target = GL_TEXTURE_2D,
        .format = GL_DEPTH_STENCIL,
        .internal_format = GL_DEPTH24_STENCIL8,
        .type = GL_UNSIGNED_INT_24_8,
        .min_filter = GL_NEAREST,
        .mag_filter = GL_NEAREST,
        .wrap_s = GL_CLAMP_TO_EDGE,
        .wrap_t = GL_CLAMP_TO_EDGE,
        .data = data,
    };
    return gl_create_texture(info);
}

void gl_destroy_texture(gl_texture_t& texture)
{
    glDeleteTextures(1, &texture.handle);
    texture.handle = 0;
}

void gl_bind_texture(gl_texture_t texture, gl_texture_bind_t bind)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    glActiveTexture(GL_TEXTURE0 + bind.binding);
    glBindTexture(texture.target, texture.handle);

    if (texture.format == GL_DEPTH_COMPONENT || texture.format == GL_DEPTH_STENCIL)
    {
        glTexParameteri(texture.target, GL_DEPTH_STENCIL_TEXTURE_MODE, (GLint)bind.aspect_mode);
    }
}

void gl_bind_texture_storage(gl_texture_t texture, gl_texture_storage_bind_t bind)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    glBindImageTexture(bind.binding, texture.handle, (GLint)bind.base_level, 1 < bind.layer_count,
                       (GLint)bind.base_layer, bind.access, texture.internal_format);
}

gl_texture_t gl_load_texture(gl_image_t const& image)
{
    return gl_create_texture_color(image.width, image.height, image.pixels);
}

gl_image_t gl_load_image(gl_texture_t const& texture, void* buffer, size_t length)
{
    gl_image_t result = {};

    if (texture.target == GL_TEXTURE_2D)
    {
        if (length < texture.width * texture.height * sizeof(uint32_t))
            return result;

        glBindTexture(texture.target, texture.handle);
        glGetTexImage(texture.target, 0, texture.format, texture.type, buffer);
        glBindTexture(texture.target, 0);
        result.pixels = buffer;
        result.width = texture.width;
        result.height = texture.height;
        result.format = texture.format;
    }

    return result;
}

// ====================================================================

gl_sampler_t gl_create_sampler(gl_sampler_desc_t const& info)
{
    gl_sampler_t result = {};

    glGenSamplers(1, &result.handle);

    // 设置过滤方式
    glSamplerParameteri(result.handle, GL_TEXTURE_MIN_FILTER, info.min_filter);
    glSamplerParameteri(result.handle, GL_TEXTURE_MAG_FILTER, info.mag_filter);

    // 设置环绕方式
    glSamplerParameteri(result.handle, GL_TEXTURE_WRAP_S, info.wrap_s);
    glSamplerParameteri(result.handle, GL_TEXTURE_WRAP_T, info.wrap_t);
    glSamplerParameteri(result.handle, GL_TEXTURE_WRAP_R, info.wrap_r);

    return result;
}

void gl_destroy_sampler(gl_sampler_t& sampler)
{
    glDeleteSamplers(1, &sampler.handle);
    sampler.handle = 0;
}

void gl_bind_sampler(gl_sampler_t sampler, gl_sampler_bind_t bind)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    glBindSampler(bind.binding, sampler.handle);
}

// ====================================================================

gl_module_t gl_create_module_compute(const char* comp_src, rhi_compute_info_t const& info)
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
    result.compute = info;
    return result;
}

gl_module_t gl_create_module_render(const char* vert_src, const char* frag_src, rhi_render_info_t const& info)
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
    result.render = info;
    return result;
}

gl_module_t gl_create_module_meshlet(const char* task_src, const char* mesh_src, const char* frag_src, rhi_render_info_t const& info)
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
    if (ts)
        glAttachShader(result.handle, ts);
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

    if (ts)
        glDeleteShader(ts);
    glDeleteShader(ms);
    glDeleteShader(fs);

    result.target = GL_MESH_SHADER_NV;
    result.render = info;
    return result;
}

void gl_destroy_module(gl_module_t& module)
{
    glDeleteProgram(module.handle);
    module.handle = 0;
}

void gl_push_constant(uint8_t const* buffer, size_t length)
{
}

void gl_push_const_int(const char* name, int32_t value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint location = glGetUniformLocation(program, name);
    if (location >= 0)
        glUniform1i(location, value);
}

void gl_push_const_uint(const char* name, uint32_t value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint location = glGetUniformLocation(program, name);
    if (location >= 0)
        glUniform1ui(location, value);
}

void gl_push_const_float(const char* name, float value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint location = glGetUniformLocation(program, name);
    if (location >= 0)
        glUniform1f(location, value);
}

void gl_push_const_vec2(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint location = glGetUniformLocation(program, name);
    if (location >= 0)
        glUniform2fv(location, 1, value);
}

void gl_push_const_vec3(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint location = glGetUniformLocation(program, name);
    if (location >= 0)
        glUniform3fv(location, 1, value);
}

void gl_push_const_vec4(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint location = glGetUniformLocation(program, name);
    if (location >= 0)
        glUniform4fv(location, 1, value);
}

void gl_push_const_mat3(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint location = glGetUniformLocation(program, name);
    if (location >= 0)
        glUniformMatrix3fv(location, 1, GL_FALSE, value);
}

void gl_push_const_mat4(const char* name, const float* value)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint location = glGetUniformLocation(program, name);
    if (location >= 0)
        glUniformMatrix4fv(location, 1, GL_FALSE, value);
}

// ====================================================================

void gl_begin_compute(gl_pass_t& pass)
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
    opengl.currentPipeline = &pass;

    glUseProgram(pass.module.handle);
}

void gl_end_compute(gl_pass_t& pass)
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
    if (opengl.currentPipeline != &pass)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }
    opengl.currentPipeline = nullptr;

    glUseProgram(0);
}

void gl_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    glDispatchCompute(std::max(1U, groupX), std::max(1U, groupY), std::max(1U, groupZ));
}

void gl_begin_render(gl_pass_t& pass)
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
    opengl.currentPipeline = &pass;

    pass.handle = 0;
    glUseProgram(pass.module.handle);

    glDisable(GL_SCISSOR_TEST);

    bool offscreen = pass.depth.texture.handle;
    for (size_t i = 0; i < std::size(pass.colors) && !offscreen; ++i)
    {
        if (pass.colors[i].texture.handle)
            offscreen = true;
    }
    if (offscreen)
    {
        glGenFramebuffers(1, &pass.handle);
        glBindFramebuffer(GL_FRAMEBUFFER, pass.handle);

        // Render State

        int32_t colorCount = 0;
        GLenum colorAttachments[16]{};
        uint32_t width = 0, height = 0;
        for (size_t i = 0; i < std::size(pass.colors); ++i)
        {
            if (pass.colors[i].texture.handle)
            {
                glBindTexture(GL_TEXTURE_2D, pass.colors[i].texture.handle);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
                                       pass.colors[i].texture.handle, 0);
                colorAttachments[colorCount++] = GL_COLOR_ATTACHMENT0 + i;
                width = std::max(width, pass.colors[i].texture.width);
                height = std::max(height, pass.colors[i].texture.height);
            }
        }
        if (colorCount) glDrawBuffers(colorCount, colorAttachments);

        if (pass.depth.texture.handle)
        {
            glBindTexture(GL_TEXTURE_2D, pass.depth.texture.handle);
            if (pass.depth.texture.format == GL_DEPTH_COMPONENT)
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass.depth.texture.handle,
                                       0);
            else if (pass.depth.texture.format == GL_DEPTH_STENCIL)
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                                       pass.depth.texture.handle, 0);
            else
            {
                fprintf(stderr, "Invalid depth attachment\n");
                abort();
            }
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
        for (size_t i = 0; i < std::size(pass.colors); ++i)
        {
            if (pass.colors[i].texture.handle)
            {
                if (pass.colors[i].clear)
                    glColorMask(true, true, true, true);
                if (pass.colors[i].clear)
                    glClearBufferfv(GL_COLOR, (int32_t)i, &pass.colors[i].value.r);

                if (pass.module.render.colors[i].color.func != GL_ADD || pass.module.render.colors[i].color.src != GL_ONE ||
                    pass.module.render.colors[i].color.dst != GL_ZERO || pass.module.render.colors[i].alpha.func != GL_ADD ||
                    pass.module.render.colors[i].alpha.src != GL_ONE || pass.module.render.colors[i].alpha.dst != GL_ZERO)
                    glEnable(GL_BLEND);
                glBlendEquationSeparatei(i, pass.module.render.colors[i].color.func, pass.module.render.colors[i].alpha.func);
                glBlendFuncSeparatei(i, pass.module.render.colors[i].color.src, pass.module.render.colors[i].color.dst, pass.module.render.colors[i].alpha.src, pass.module.render.colors[i].alpha.dst);
            }
        }

        if (pass.depth.texture.handle &&
            (pass.depth.texture.format == GL_DEPTH_COMPONENT || pass.depth.texture.format == GL_DEPTH_STENCIL))
        {
            if (pass.depth.clear)
            {
                glClearBufferfv(GL_DEPTH, 0, &pass.depth.value);
            }
            if (pass.stencil.clear && pass.depth.texture.format == GL_DEPTH_STENCIL)
            {
                glClearBufferiv(GL_STENCIL, 0, &pass.stencil.value);
            }
        }

        // Depth State

        if (pass.module.render.depth.func == GL_ALWAYS && pass.module.render.depth.write == false)
        {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }
        else
        {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(pass.module.render.depth.write);
        }
        glDepthFunc(pass.module.render.depth.func);

        if (pass.module.render.depth.bias == 0 && pass.module.render.depth.biasSlope == 0)
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
        else
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffsetClamp(pass.module.render.depth.biasSlope, pass.module.render.depth.bias, pass.module.render.depth.biasClamp);
        }

        // Stencil State

        if (pass.module.render.stencil.back.func != GL_ALWAYS || pass.module.render.stencil.back.sfail != GL_KEEP ||
            pass.module.render.stencil.back.zfail != GL_KEEP || pass.module.render.stencil.back.zpass != GL_KEEP ||
            pass.module.render.stencil.front.func != GL_ALWAYS || pass.module.render.stencil.front.sfail != GL_KEEP ||
            pass.module.render.stencil.front.zfail != GL_KEEP || pass.module.render.stencil.front.zpass != GL_KEEP)
        {
            glEnable(GL_STENCIL_TEST);
        }
        else
        {
            glDisable(GL_STENCIL_TEST);
        }
        glStencilMask(pass.module.render.stencil.write);
        glStencilFuncSeparate(GL_BACK, pass.module.render.stencil.back.func, pass.stencil.refer, pass.module.render.stencil.read);
        glStencilFuncSeparate(GL_FRONT, pass.module.render.stencil.front.func, pass.stencil.refer, pass.module.render.stencil.read);
        glStencilOpSeparate(GL_BACK, pass.module.render.stencil.back.sfail, pass.module.render.stencil.back.zfail, pass.module.render.stencil.back.zpass);
        glStencilOpSeparate(GL_FRONT, pass.module.render.stencil.front.sfail, pass.module.render.stencil.front.zfail, pass.module.render.stencil.front.zpass);
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (pass.screen.color.clear)
        {
            glClearBufferfv(GL_COLOR, 0, &pass.screen.color.value.r);
        }
        if (pass.screen.depth.clear)
        {
            glClearBufferfv(GL_DEPTH, 0, &pass.screen.depth.value);
        }
        if (pass.screen.stencil.clear)
        {
            glClearBufferiv(GL_STENCIL, 0, &pass.screen.stencil.value);
        }

        // Render State

        if (pass.screen.color.blend.func != GL_ADD || pass.screen.color.blend.src != GL_ONE ||
            pass.screen.color.blend.dst != GL_ZERO)
        {
            glEnable(GL_BLEND);
        }
        else
        {
            glDisable(GL_BLEND);
        }
        glBlendEquation(pass.screen.color.blend.func);
        glBlendFunc(pass.screen.color.blend.src, pass.screen.color.blend.dst);

        // Depth State

        if (pass.screen.depth.func == GL_ALWAYS && pass.screen.depth.write == false)
        {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }
        else
        {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(pass.screen.depth.write);
        }
        glDepthFunc(pass.screen.depth.func);

        if (pass.screen.depth.bias == 0 && pass.screen.depth.biasSlope == 0)
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
        else
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffsetClamp(pass.screen.depth.biasSlope, pass.screen.depth.bias, pass.screen.depth.biasClamp);
        }

        // Stencil State

        if (pass.screen.stencil.func != GL_ALWAYS || pass.screen.stencil.sfail != GL_KEEP ||
            pass.screen.stencil.zfail != GL_KEEP || pass.screen.stencil.zpass != GL_KEEP)
        {
            glEnable(GL_STENCIL_TEST);
        }
        else
        {
            glDisable(GL_STENCIL_TEST);
        }
        glStencilMask(pass.screen.stencil.write);
        glStencilFunc(pass.screen.stencil.func, pass.screen.stencil.refer, pass.screen.stencil.read);
        glStencilOp(pass.screen.stencil.sfail, pass.screen.stencil.zfail, pass.screen.stencil.zpass);
    }

    // Primitive State

    glFrontFace(pass.module.render.front_face);
    if (pass.module.render.cull_mode)
        glCullFace(pass.module.render.cull_mode);
    if (pass.module.render.cull_mode)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    if (pass.module.render.front_face)
        glEnable(GL_FRONT_FACE);
    else
        glDisable(GL_FRONT_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, pass.module.render.fill_mode);
}

void gl_end_render(gl_pass_t& pass)
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
    if (opengl.currentPipeline != &pass)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }
    opengl.currentPipeline = nullptr;

    bool offscreen = pass.depth.texture.handle;
    for (size_t i = 0; i < std::size(pass.colors) && !offscreen; ++i)
    {
        if (pass.colors[i].texture.handle)
            offscreen = true;
    }
    if (offscreen)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &pass.handle);
        pass.handle = 0;
    }

    glUseProgram(0);
}

void gl_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
    glViewport(x, y, width, height);
}

void gl_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, width, height);
}

void gl_draw_mesh_task(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    glDrawMeshTasksNV(0, std::max(1U, groupX) * std::max(1U, groupY) * std::max(1U, groupZ));
}

// ====================================================================

gl_mesh_t gl_create_mesh(const float* vertices, // vec3
                                const float* normals, // vec3
                                const float* uvs, // vec2
                                size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    gl_mesh_t result = {};

    glGenVertexArrays(1, &result.handle);
    glBindVertexArray(result.handle);

    if (vertices)
    {
        result.vertex_vbo = gl_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = GL_STATIC_DRAW, .data = vertices,});
        glBindBuffer(GL_ARRAY_BUFFER, result.vertex_vbo.handle);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    if (normals)
    {
        result.normal_vbo = gl_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = GL_STATIC_DRAW, .data = normals,});
        glBindBuffer(GL_ARRAY_BUFFER, result.normal_vbo.handle);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
    }

    if (uvs)
    {
        result.uv_vbo = gl_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = GL_STATIC_DRAW, .data = uvs,});
        glBindBuffer(GL_ARRAY_BUFFER, result.uv_vbo.handle);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(2);
    }

    if (indices)
    {
        result.index_vbo = gl_create_buffer({.size = index_count * sizeof(uint32_t), .usage = GL_STATIC_DRAW, .data = indices,});
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.index_vbo.handle);
    }

    glBindVertexArray(0);

    result.vertex_count = (GLsizei)vertex_count;
    result.index_count = (GLsizei)index_count;
    result.index_type = GL_UNSIGNED_INT;
    result.primitive_type = GL_TRIANGLES;
    return result;
}

void gl_destroy_mesh(gl_mesh_t& mesh)
{
    gl_destroy_buffer(mesh.vertex_vbo);
    gl_destroy_buffer(mesh.normal_vbo);
    gl_destroy_buffer(mesh.uv_vbo);
    gl_destroy_buffer(mesh.index_vbo);

    glDeleteVertexArrays(1, &mesh.handle);
    mesh.handle = 0;
}

void gl_draw_mesh(gl_mesh_t const& mesh)
{
    glBindVertexArray(mesh.handle);
    if (mesh.index_count)
        glDrawElements(mesh.primitive_type, mesh.index_count, mesh.index_type, (void*)0);
    else
        glDrawArrays(mesh.primitive_type, 0, mesh.vertex_count);
    glBindVertexArray(0);
}

// ====================================================================

gl_meshlet_t gl_create_meshlet(const float* vertices, // vec4
                                      const float* normals, // vec4
                                      const float* uvs, size_t vertex_count, const unsigned int* indices,
                                      size_t index_count)
{
    gl_meshlet_t result = {};

    if (vertices)
    {
        result.vertex_vbo = gl_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = GL_STATIC_DRAW, .data = vertices,});
    }

    if (normals)
    {
        result.normal_vbo = gl_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = GL_STATIC_DRAW, .data = normals,});
    }

    if (uvs)
    {
        result.uv_vbo = gl_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = GL_STATIC_DRAW, .data = uvs,});
    }

    if (indices)
    {
        result.index_vbo = gl_create_buffer({.size = index_count * sizeof(uint32_t), .usage = GL_STATIC_DRAW, .data = indices,});
    }

    result.vertex_count = (GLsizei)vertex_count;
    result.index_count = (GLsizei)index_count;
    result.index_type = GL_UNSIGNED_INT;
    result.primitive_type = GL_TRIANGLES;
    return result;
}

void gl_destroy_meshlet(gl_meshlet_t& meshlet)
{
    gl_destroy_buffer(meshlet.vertex_vbo);
    gl_destroy_buffer(meshlet.normal_vbo);
    gl_destroy_buffer(meshlet.uv_vbo);
    gl_destroy_buffer(meshlet.index_vbo);
}

void gl_draw_meshlet(gl_meshlet_t const& meshlet)
{
    gl_bind_buffer(meshlet.vertex_vbo, {.binding = 0, .target = GL_SHADER_STORAGE_BUFFER,});
    gl_bind_buffer(meshlet.normal_vbo, {.binding = 1, .target = GL_SHADER_STORAGE_BUFFER,});
    gl_bind_buffer(meshlet.uv_vbo, {.binding = 2, .target = GL_SHADER_STORAGE_BUFFER,});
    gl_bind_buffer(meshlet.index_vbo, {.binding = 3, .target = GL_SHADER_STORAGE_BUFFER,});
    glDrawMeshTasksNV(0, meshlet.index_count / 3);
}

// ====================================================================

gl_mesh_t gl_create_mesh_screen()
{
    const float points[] = {
        -1.0f, -1.0f, 0.0f,
        3.0f, -1.0f, 0.0f,
        -1.0f, 3.0f, 0.0f,
    };
    const float uvs[] = {
        0.0f, 0.0f,
        2.0f, 0.0f,
        0.0f, 2.0f,
    };
    auto quad = gl_create_mesh(points, nullptr, uvs, 3, nullptr, 0);
    return quad;
}

void gl_draw_screen(int width, int height, gl_texture_t texture, gl_color_t clear)
{
    constexpr auto VS = R"(
        #version 460
        layout(location = 0) in vec3 in_vertex;
        layout(location = 1) in vec3 in_normal;
        layout(location = 2) in vec2 in_uv;
        layout(location = 0) out vec3 vertex;
        layout(location = 1) out vec3 normal;
        layout(location = 2) out vec2 uv;

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
        layout(location = 0) in vec3 vertex;
        layout(location = 1) in vec3 normal;
        layout(location = 2) in vec2 uv;
        layout(location = 0) out vec4 final;
        layout(binding = 0) uniform sampler2D texture0;

        void main()
        {
            final = texture(texture0, uv);
        }
    )";
    auto module = gl_create_module_render(VS, FS, {.vertex = {rhi_vertex_layout, rhi_normal_layout, rhi_uv_layout,},});
    gl_pass_t pass = {.module = module, .screen = {.color = { .clear = true, .value = clear, }}};
    gl_begin_render(pass);
    gl_set_viewport(0, 0, width, height);
    gl_bind_texture(texture, { .binding = 0, });
    gl_draw_mesh(gl_create_mesh_screen());
    gl_end_render(pass);
}

#endif