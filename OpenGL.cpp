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
#ifdef OPENGL_IMPLEMENTATION
#include "OpenGL.h"
#include <iostream>
#include <numeric>

enum rt_module_type_t : uint32_t
{
    GL_MODULE_RENDER = 1,
    GL_MODULE_COMPUTE = 2,
    GL_MODULE_MESHLET = 3,
    GL_MODULE_TRANSFER = 4,
};

struct OpenGL
{
    GLenum currentPassType = GL_NONE;
    union
    {
        void* currentPipeline = nullptr;
        rt_pass_render_t* currentRenderPass;
        rt_pass_compute_t* currentComputePass;
        rt_pass_transfer_t* currentTransferPass;
    };
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

    rt_create_buffer = gl_create_buffer;
    rt_destroy_buffer = gl_destroy_buffer;
    rt_bind_buffer = gl_bind_buffer;
    rt_map_buffer = gl_map_buffer;
    rt_unmap_buffer = gl_unmap_buffer;
    rt_create_texture = gl_create_texture;
    rt_create_texture_color = gl_create_texture_color;
    rt_create_texture_depth = gl_create_texture_depth;
    rt_create_texture_depth_stencil = gl_create_texture_depth_stencil;
    rt_destroy_texture = gl_destroy_texture;
    rt_bind_texture = gl_bind_texture;
    rt_bind_texture_storage = gl_bind_texture_storage;
    rt_create_sampler = gl_create_sampler;
    rt_destroy_sampler = gl_destroy_sampler;
    rt_bind_sampler = gl_bind_sampler;
    rt_create_module_compute = gl_create_module_compute;
    rt_create_module_render = gl_create_module_render;
    rt_create_module_meshlet = gl_create_module_meshlet;
    rt_destroy_module_render = gl_destroy_module_render;
    rt_destroy_module_compute = gl_destroy_module_compute;
    rt_begin_compute = gl_begin_compute;
    rt_end_compute = gl_end_compute;
    rt_dispatch_compute = gl_dispatch_compute;
    rt_begin_render = gl_begin_render;
    rt_end_render = gl_end_render;
    rt_set_viewport = gl_set_viewport;
    rt_set_scissor = gl_set_scissor;
    rt_draw_mesh_task = gl_draw_mesh_task;
    rt_push_constant = gl_push_constant;
    rt_push_const_int = gl_push_const_int;
    rt_push_const_uint = gl_push_const_uint;
    rt_push_const_float = gl_push_const_float;
    rt_push_const_vec2 = gl_push_const_vec2;
    rt_push_const_vec3 = gl_push_const_vec3;
    rt_push_const_vec4 = gl_push_const_vec4;
    rt_push_const_mat3 = gl_push_const_mat3;
    rt_push_const_mat4 = gl_push_const_mat4;
    rt_begin_transfer = gl_begin_transfer;
    rt_end_transfer = gl_end_transfer;
    rt_copy_buffer = gl_copy_buffer;
    rt_copy_buffer_data = gl_copy_buffer_data;
    rt_copy_buffer_texture = gl_copy_buffer_texture;
    rt_copy_texture = gl_copy_texture;
    rt_copy_texture_data = gl_copy_texture_data;
    rt_copy_texture_buffer = gl_copy_texture_buffer;
    rt_create_mesh = gl_create_mesh;
    rt_destroy_mesh = gl_destroy_mesh;
    rt_draw_mesh = gl_draw_mesh;
    rt_create_meshlet = gl_create_meshlet;
    rt_destroy_meshlet = gl_destroy_meshlet;
    rt_draw_meshlet = gl_draw_meshlet;
    rt_create_mesh_screen = gl_create_mesh_screen;
    rt_draw_screen = gl_draw_screen;
    rt_submit = gl_submit;
}

void gl_unload_library()
{
    opengl.currentPassType = GL_NONE;
    opengl.currentPipeline = nullptr;

    // ====================================================================

    if(rt_create_buffer == gl_create_buffer) rt_create_buffer = nullptr;
    if(rt_destroy_buffer == gl_destroy_buffer) rt_destroy_buffer = nullptr;
    if(rt_bind_buffer == gl_bind_buffer) rt_bind_buffer = nullptr;
    if(rt_map_buffer == gl_map_buffer) rt_map_buffer = nullptr;
    if(rt_unmap_buffer == gl_unmap_buffer) rt_unmap_buffer = nullptr;
    if(rt_create_texture == gl_create_texture) rt_create_texture = nullptr;
    if(rt_create_texture_color == gl_create_texture_color) rt_create_texture_color = nullptr;
    if(rt_create_texture_depth == gl_create_texture_depth) rt_create_texture_depth = nullptr;
    if(rt_create_texture_depth_stencil == gl_create_texture_depth_stencil) rt_create_texture_depth_stencil = nullptr;
    if(rt_destroy_texture == gl_destroy_texture) rt_destroy_texture = nullptr;
    if(rt_bind_texture == gl_bind_texture) rt_bind_texture = nullptr;
    if(rt_bind_texture_storage == gl_bind_texture_storage) rt_bind_texture_storage = nullptr;
    if(rt_create_sampler == gl_create_sampler) rt_create_sampler = nullptr;
    if(rt_destroy_sampler == gl_destroy_sampler) rt_destroy_sampler = nullptr;
    if(rt_bind_sampler == gl_bind_sampler) rt_bind_sampler = nullptr;
    if(rt_create_module_compute == gl_create_module_compute) rt_create_module_compute = nullptr;
    if(rt_create_module_render == gl_create_module_render) rt_create_module_render = nullptr;
    if(rt_create_module_meshlet == gl_create_module_meshlet) rt_create_module_meshlet = nullptr;
    if(rt_destroy_module_render == gl_destroy_module_render) rt_destroy_module_render = nullptr;
    if(rt_destroy_module_compute == gl_destroy_module_compute) rt_destroy_module_compute = nullptr;
    if(rt_begin_compute == gl_begin_compute) rt_begin_compute = nullptr;
    if(rt_end_compute == gl_end_compute) rt_end_compute = nullptr;
    if(rt_dispatch_compute == gl_dispatch_compute) rt_dispatch_compute = nullptr;
    if(rt_begin_render == gl_begin_render) rt_begin_render = nullptr;
    if(rt_end_render == gl_end_render) rt_end_render = nullptr;
    if(rt_set_viewport == gl_set_viewport) rt_set_viewport = nullptr;
    if(rt_set_scissor == gl_set_scissor) rt_set_scissor = nullptr;
    if(rt_draw_mesh_task == gl_draw_mesh_task) rt_draw_mesh_task = nullptr;
    if(rt_push_constant == gl_push_constant) rt_push_constant = nullptr;
    if(rt_push_const_int == gl_push_const_int) rt_push_const_int = nullptr;
    if(rt_push_const_uint == gl_push_const_uint) rt_push_const_uint = nullptr;
    if(rt_push_const_float == gl_push_const_float) rt_push_const_float = nullptr;
    if(rt_push_const_vec2 == gl_push_const_vec2) rt_push_const_vec2 = nullptr;
    if(rt_push_const_vec3 == gl_push_const_vec3) rt_push_const_vec3 = nullptr;
    if(rt_push_const_vec4 == gl_push_const_vec4) rt_push_const_vec4 = nullptr;
    if(rt_push_const_mat3 == gl_push_const_mat3) rt_push_const_mat3 = nullptr;
    if(rt_push_const_mat4 == gl_push_const_mat4) rt_push_const_mat4 = nullptr;
    if(rt_begin_transfer == gl_begin_transfer) rt_begin_transfer = nullptr;
    if(rt_end_transfer == gl_end_transfer) rt_end_transfer = nullptr;
    if(rt_copy_buffer == gl_copy_buffer) rt_copy_buffer = nullptr;
    if(rt_copy_buffer_data == gl_copy_buffer_data) rt_copy_buffer_data = nullptr;
    if(rt_copy_buffer_texture == gl_copy_buffer_texture) rt_copy_buffer_texture = nullptr;
    if(rt_copy_texture == gl_copy_texture) rt_copy_texture = nullptr;
    if(rt_copy_texture_data == gl_copy_texture_data) rt_copy_texture_data = nullptr;
    if(rt_copy_texture_buffer == gl_copy_texture_buffer) rt_copy_texture_buffer = nullptr;
    if(rt_create_mesh == gl_create_mesh) rt_create_mesh = nullptr;
    if(rt_destroy_mesh == gl_destroy_mesh) rt_destroy_mesh = nullptr;
    if(rt_draw_mesh == gl_draw_mesh) rt_draw_mesh = nullptr;
    if(rt_create_meshlet == gl_create_meshlet) rt_create_meshlet = nullptr;
    if(rt_destroy_meshlet == gl_destroy_meshlet) rt_destroy_meshlet = nullptr;
    if(rt_draw_meshlet == gl_draw_meshlet) rt_draw_meshlet = nullptr;
    if(rt_create_mesh_screen == gl_create_mesh_screen) rt_create_mesh_screen = nullptr;
    if(rt_draw_screen == gl_draw_screen) rt_draw_screen = nullptr;
    if(rt_submit == gl_submit) rt_submit = nullptr;
}

// ====================================================================

rt_buffer_t gl_create_buffer(rt_buffer_info_t const& info)
{
    if (info.usage == 0)
    {
        fprintf(stderr, "Buffer usage must not be 0");
        abort();
    }

    rt_buffer_t result = {};
    glGenBuffers(1, &result.handle);
    glBindBuffer(GL_ARRAY_BUFFER, result.handle);

    GLbitfield flags = 0;
    if (info.usage & GL_BUFFER_USAGE_MAP_READ)
        flags |= GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    if (info.usage & GL_BUFFER_USAGE_MAP_WRITE)
        flags |= GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    if (info.usage & GL_BUFFER_USAGE_COPY_DST)
        flags |= GL_DYNAMIC_STORAGE_BIT;
    glBufferStorage(GL_ARRAY_BUFFER, (GLsizeiptr)info.size, info.data, flags);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    result.size = info.size;
    result.usage = info.usage;
    return result;
}

void gl_destroy_buffer(rt_buffer_t& buffer)
{
    glDeleteBuffers(1, &buffer.handle);
    buffer.handle = 0;
}

void gl_bind_buffer(rt_buffer_t buffer, rt_buffer_bind_t bind)
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

void* gl_map_buffer(rt_buffer_t& buffer, GLenum mode, size_t offset, size_t size)
{
    if (!buffer.handle)
        return nullptr;
    if (offset > buffer.size)
        return nullptr;
    if (size == 0)
        size = buffer.size - offset;
    if (size == 0 || offset + size > buffer.size)
        return nullptr;

    GLbitfield access = 0;
    switch (mode)
    {
    case GL_READ_ONLY:
        access = GL_MAP_READ_BIT;
        break;
    case GL_WRITE_ONLY:
        access = GL_MAP_WRITE_BIT;
        break;
    case GL_READ_WRITE:
        access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        break;
    default:
        fprintf(stderr, "Unsupported buffer map mode");
        abort();
    }
    if (buffer.usage & (GL_BUFFER_USAGE_MAP_READ | GL_BUFFER_USAGE_MAP_WRITE))
        access |= GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

    glBindBuffer(GL_ARRAY_BUFFER, buffer.handle);
    void* ptr = glMapBufferRange(GL_ARRAY_BUFFER, (GLintptr)offset, (GLsizeiptr)size, access);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return ptr;
}

void gl_unmap_buffer(rt_buffer_t& buffer)
{
    if (!buffer.handle)
        return;

    glBindBuffer(GL_ARRAY_BUFFER, buffer.handle);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ====================================================================

rt_texture_t gl_create_texture(rt_texture_info_t const& info)
{
    rt_texture_t result = {};

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

rt_texture_t gl_create_texture_color(uint32_t width, uint32_t height, const void* data)
{
    rt_texture_info_t info
    {
        .width = width,
        .height = height,
        .target = GL_TEXTURE_2D,
        .format = GL_RGBA,
        .internal_format = GL_RGBA,
        .type = GL_UNSIGNED_BYTE,
        .min_filter = GL_LINEAR,
        .mag_filter = GL_LINEAR,
        .wrap_s = GL_REPEAT,
        .wrap_t = GL_REPEAT,
        .data = data,
    };
    return gl_create_texture(info);
}

rt_texture_t gl_create_texture_depth(uint32_t width, uint32_t height, const void* data)
{
    rt_texture_info_t info
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

rt_texture_t gl_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data)
{
    rt_texture_info_t info
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

void gl_destroy_texture(rt_texture_t& texture)
{
    glDeleteTextures(1, &texture.handle);
    texture.handle = 0;
}

void gl_bind_texture(rt_texture_t texture, rt_texture_bind_t bind)
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

void gl_bind_texture_storage(rt_texture_t texture, rt_texture_storage_bind_t bind)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    glBindImageTexture(bind.binding, texture.handle, (GLint)bind.base_level, 1 < bind.layer_count,
                       (GLint)bind.base_layer, bind.access, texture.internal_format);
}

// ====================================================================

rt_sampler_t gl_create_sampler(rt_sampler_info_t const& info)
{
    rt_sampler_t result = {};

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

void gl_destroy_sampler(rt_sampler_t& sampler)
{
    glDeleteSamplers(1, &sampler.handle);
    sampler.handle = 0;
}

void gl_bind_sampler(rt_sampler_t sampler, rt_sampler_bind_t bind)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    glBindSampler(bind.binding, sampler.handle);
}

// ====================================================================

rt_module_compute_t gl_create_module_compute(rt_module_compute_info_t const& info)
{
    rt_module_compute_t result = {};

    if (!info.cshader)
    {
        fprintf(stderr, "Compute shader source is empty\n");
        abort();
    }

    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    GLint clength = (GLint)info.clength;
    glShaderSource(cs, 1, &info.cshader, info.clength ? &clength : nullptr);
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

    return result;
}

rt_module_render_t gl_create_module_render(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};

    // ---- Vertex Shader ----
    GLuint vs = 0;
    if (info.vshader)
    {
        vs = glCreateShader(GL_VERTEX_SHADER);
        GLint vlength = (GLint)info.vlength;
        glShaderSource(vs, 1, &info.vshader, info.vlength ? &vlength : nullptr);
        glCompileShader(vs);
        GLint success = 0;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[1024];
            glGetShaderInfoLog(vs, sizeof(log), nullptr, log);
            fprintf(stderr, "Vertex shader compile error:\n%s\n", log);
            abort();
        }
    }

    // ---- Fragment Shader ----
    GLuint fs = 0;
    if (info.fshader)
    {
        fs = glCreateShader(GL_FRAGMENT_SHADER);
        GLint flength = (GLint)info.flength;
        glShaderSource(fs, 1, &info.fshader, info.flength ? &flength : nullptr);
        glCompileShader(fs);
        GLint success = 0;
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[1024];
            glGetShaderInfoLog(fs, sizeof(log), nullptr, log);
            fprintf(stderr, "Fragment shader compile error:\n%s\n", log);
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

    // ---- Vertex Array ----
    glGenVertexArrays(1, &result.vertex_vao);
    glBindVertexArray(result.vertex_vao);
    for (uint32_t i = 0; i < std::size(info.vertex); ++i)
    {
        auto& vertex = info.vertex[i];
        if (vertex.type == GL_NONE || vertex.count == 0)
            continue;

        glEnableVertexAttribArray(vertex.location);
        switch (vertex.type)
        {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        case GL_INT:
        case GL_UNSIGNED_INT:
            glVertexAttribIFormat(vertex.location, (GLint)vertex.count, vertex.type, 0);
            break;
        case GL_DOUBLE:
            glVertexAttribLFormat(vertex.location, (GLint)vertex.count, vertex.type, 0);
            break;
        default:
            glVertexAttribFormat(vertex.location, (GLint)vertex.count, vertex.type, GL_FALSE, 0);
            break;
        }
        glVertexAttribBinding(vertex.location, i);
        glVertexBindingDivisor(i, vertex.instance ? 1 : 0);
    }
    glBindVertexArray(0);

    for (size_t i = 0; i < std::size(info.colors); ++i)
    {
        result.colors[i].color.func = info.colors[i].color.func;
        result.colors[i].color.src = info.colors[i].color.src;
        result.colors[i].color.dst = info.colors[i].color.dst;
        result.colors[i].alpha.func = info.colors[i].alpha.func;
        result.colors[i].alpha.src = info.colors[i].alpha.src;
        result.colors[i].alpha.dst = info.colors[i].alpha.dst;
    }
    result.depth.write = info.depth.write;
    result.depth.bias = info.depth.bias;
    result.depth.biasSlope = info.depth.biasSlope;
    result.depth.biasClamp = info.depth.biasClamp;
    result.depth.func = info.depth.func;
    result.stencil.read = info.stencil.read;
    result.stencil.write = info.stencil.write;
    result.stencil.back.func = info.stencil.back.func;
    result.stencil.back.sfail = info.stencil.back.sfail;
    result.stencil.back.zfail = info.stencil.back.zfail;
    result.stencil.back.zpass = info.stencil.back.zpass;
    result.stencil.front.func = info.stencil.front.func;
    result.stencil.front.sfail = info.stencil.front.sfail;
    result.stencil.front.zfail = info.stencil.front.zfail;
    result.stencil.front.zpass = info.stencil.front.zpass;
    result.index_type = info.index_type;
    for (size_t i = 0; i < std::size(info.vertex); ++i)
    {
        result.vertex[i].location = info.vertex[i].location;
        result.vertex[i].type = info.vertex[i].type;
        result.vertex[i].count = info.vertex[i].count;
        result.vertex[i].instance = info.vertex[i].instance;
    }
    for (size_t i = 0; i < std::size(info.binding); ++i)
    {
        result.binding[i].binding = info.binding[i].binding;
        result.binding[i].type = info.binding[i].type;
    }
    result.cull_mode = info.cull_mode;
    result.front_face = info.front_face;
    result.fill_mode = info.fill_mode;
    result.primitive = info.primitive;
    return result;
}

rt_module_render_t gl_create_module_meshlet(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};

    // ---- Task Shader（可选）----
    GLuint ts = 0;
    if (info.tshader)
    {
        ts = glCreateShader(GL_TASK_SHADER_NV);
        GLint tlength = (GLint)info.tlength;
        glShaderSource(ts, 1, &info.tshader, info.tlength ? &tlength : nullptr);
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
    if (!info.mshader)
    {
        fprintf(stderr, "Mesh shader source is empty\n");
        abort();
    }
    GLuint ms = glCreateShader(GL_MESH_SHADER_NV);
    GLint mlength = (GLint)info.mlength;
    glShaderSource(ms, 1, &info.mshader, info.mlength ? &mlength : nullptr);
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
    GLuint fs = 0;
    if (info.fshader)
    {
        fs = glCreateShader(GL_FRAGMENT_SHADER);
        GLint flength = (GLint)info.flength;
        glShaderSource(fs, 1, &info.fshader, info.flength ? &flength : nullptr);
        glCompileShader(fs);
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[1024];
            glGetShaderInfoLog(fs, sizeof(log), nullptr, log);
            fprintf(stderr, "Fragment shader compile error:\n%s\n", log);
            abort();
        }
    }

    // ---- Program ----
    result.handle = glCreateProgram();
    if (ts)
        glAttachShader(result.handle, ts);
    glAttachShader(result.handle, ms);
    if (fs)
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

    for (size_t i = 0; i < std::size(info.colors); ++i)
    {
        result.colors[i].color.func = info.colors[i].color.func;
        result.colors[i].color.src = info.colors[i].color.src;
        result.colors[i].color.dst = info.colors[i].color.dst;
        result.colors[i].alpha.func = info.colors[i].alpha.func;
        result.colors[i].alpha.src = info.colors[i].alpha.src;
        result.colors[i].alpha.dst = info.colors[i].alpha.dst;
    }
    result.depth.write = info.depth.write;
    result.depth.bias = info.depth.bias;
    result.depth.biasSlope = info.depth.biasSlope;
    result.depth.biasClamp = info.depth.biasClamp;
    result.depth.func = info.depth.func;
    result.stencil.read = info.stencil.read;
    result.stencil.write = info.stencil.write;
    result.stencil.back.func = info.stencil.back.func;
    result.stencil.back.sfail = info.stencil.back.sfail;
    result.stencil.back.zfail = info.stencil.back.zfail;
    result.stencil.back.zpass = info.stencil.back.zpass;
    result.stencil.front.func = info.stencil.front.func;
    result.stencil.front.sfail = info.stencil.front.sfail;
    result.stencil.front.zfail = info.stencil.front.zfail;
    result.stencil.front.zpass = info.stencil.front.zpass;
    result.index_type = info.index_type;
    for (size_t i = 0; i < std::size(info.vertex); ++i)
    {
        result.vertex[i].location = info.vertex[i].location;
        result.vertex[i].type = info.vertex[i].type;
        result.vertex[i].count = info.vertex[i].count;
        result.vertex[i].instance = info.vertex[i].instance;
    }
    for (size_t i = 0; i < std::size(info.binding); ++i)
    {
        result.binding[i].binding = info.binding[i].binding;
        result.binding[i].type = info.binding[i].type;
    }
    result.cull_mode = info.cull_mode;
    result.front_face = info.front_face;
    result.fill_mode = info.fill_mode;
    result.primitive = info.primitive;
    return result;
}

void gl_destroy_module_render(rt_module_render_t& module)
{
    if (module.vertex_vao) glDeleteVertexArrays(1, &module.vertex_vao);
    module.vertex_vao = 0;
    glDeleteProgram(module.handle);
    module.handle = 0;
}

void gl_destroy_module_compute(rt_module_compute_t& module)
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

void gl_begin_compute(rt_pass_compute_t& pass)
{
    if (pass.module.handle == 0)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (opengl.currentPipeline != nullptr)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }
    opengl.currentComputePass = &pass;
    opengl.currentPassType = GL_MODULE_COMPUTE;

    glUseProgram(pass.module.handle);
}

void gl_end_compute(rt_pass_compute_t& pass)
{
    if (pass.module.handle == 0)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (opengl.currentComputePass != &pass)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }
    opengl.currentPassType = GL_NONE;
    opengl.currentComputePass = nullptr;

    glUseProgram(0);
}

void gl_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (opengl.currentPassType != GL_MODULE_COMPUTE)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }

    glDispatchCompute(std::max(1U, groupX), std::max(1U, groupY), std::max(1U, groupZ));
}

void gl_begin_render(rt_pass_render_t& pass)
{
    if (pass.module.handle == 0)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (opengl.currentPipeline != nullptr)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }
    opengl.currentPassType = GL_MODULE_RENDER;
    opengl.currentRenderPass = &pass;

    pass.handle = 0;
    glUseProgram(pass.module.handle);
    glBindVertexArray(pass.module.vertex_vao);

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

                if (pass.module.colors[i].color.func != GL_ADD || pass.module.colors[i].color.src != GL_ONE ||
                    pass.module.colors[i].color.dst != GL_ZERO || pass.module.colors[i].alpha.func != GL_ADD ||
                    pass.module.colors[i].alpha.src != GL_ONE || pass.module.colors[i].alpha.dst != GL_ZERO)
                    glEnable(GL_BLEND);
                glBlendEquationSeparatei(i, pass.module.colors[i].color.func, pass.module.colors[i].alpha.func);
                glBlendFuncSeparatei(i, pass.module.colors[i].color.src, pass.module.colors[i].color.dst, pass.module.colors[i].alpha.src, pass.module.colors[i].alpha.dst);
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

        if (pass.module.depth.func == GL_ALWAYS && pass.module.depth.write == false)
        {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }
        else
        {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(pass.module.depth.write);
        }
        glDepthFunc(pass.module.depth.func);

        if (pass.module.depth.bias == 0 && pass.module.depth.biasSlope == 0)
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
        else
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffsetClamp(pass.module.depth.biasSlope, pass.module.depth.bias, pass.module.depth.biasClamp);
        }

        // Stencil State

        if (pass.module.stencil.back.func != GL_ALWAYS || pass.module.stencil.back.sfail != GL_KEEP ||
            pass.module.stencil.back.zfail != GL_KEEP || pass.module.stencil.back.zpass != GL_KEEP ||
            pass.module.stencil.front.func != GL_ALWAYS || pass.module.stencil.front.sfail != GL_KEEP ||
            pass.module.stencil.front.zfail != GL_KEEP || pass.module.stencil.front.zpass != GL_KEEP)
        {
            glEnable(GL_STENCIL_TEST);
        }
        else
        {
            glDisable(GL_STENCIL_TEST);
        }
        glStencilMask(pass.module.stencil.write);
        glStencilFuncSeparate(GL_BACK, pass.module.stencil.back.func, pass.stencil.refer, pass.module.stencil.read);
        glStencilFuncSeparate(GL_FRONT, pass.module.stencil.front.func, pass.stencil.refer, pass.module.stencil.read);
        glStencilOpSeparate(GL_BACK, pass.module.stencil.back.sfail, pass.module.stencil.back.zfail, pass.module.stencil.back.zpass);
        glStencilOpSeparate(GL_FRONT, pass.module.stencil.front.sfail, pass.module.stencil.front.zfail, pass.module.stencil.front.zpass);
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

    glFrontFace(pass.module.front_face);
    if (pass.module.cull_mode)
        glCullFace(pass.module.cull_mode);
    if (pass.module.cull_mode)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, pass.module.fill_mode);
}

void gl_end_render(rt_pass_render_t& pass)
{
    if (pass.module.handle == 0)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (opengl.currentRenderPass != &pass)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }
    opengl.currentPassType = GL_NONE;
    opengl.currentRenderPass = nullptr;

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
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (opengl.currentPassType != GL_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }

    glViewport(x, y, width, height);
}

void gl_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (opengl.currentPassType != GL_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }

    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, width, height);
}

void gl_draw_mesh_task(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (opengl.currentPassType != GL_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }

    glDrawMeshTasksNV(0, std::max(1U, groupX) * std::max(1U, groupY) * std::max(1U, groupZ));
}

// ====================================================================

// 判断是否为打包像素类型（整个像素由一个数据单元表示）
static bool gl_is_packed_type(GLenum type)
{
    switch (type)
    {
    case GL_UNSIGNED_BYTE_3_3_2:
    case GL_UNSIGNED_BYTE_2_3_3_REV:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_5_6_5_REV:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
    case GL_UNSIGNED_INT_8_8_8_8:
    case GL_UNSIGNED_INT_8_8_8_8_REV:
    case GL_UNSIGNED_INT_10_10_10_2:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
    case GL_UNSIGNED_INT_5_9_9_9_REV:
    case GL_UNSIGNED_INT_24_8:
    case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
        return true;
    default:
        return false;
    }
}

// 单个数据单元（type）占用的字节数；PBO 偏移必须是该值的整数倍
static uint32_t gl_type_size(GLenum type)
{
    switch (type)
    {
    case GL_UNSIGNED_BYTE:
    case GL_BYTE:
    case GL_UNSIGNED_BYTE_3_3_2:
    case GL_UNSIGNED_BYTE_2_3_3_REV:
        return 1;
    case GL_UNSIGNED_SHORT:
    case GL_SHORT:
    case GL_HALF_FLOAT:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_5_6_5_REV:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
        return 2;
    case GL_UNSIGNED_INT:
    case GL_INT:
    case GL_FLOAT:
    case GL_UNSIGNED_INT_8_8_8_8:
    case GL_UNSIGNED_INT_8_8_8_8_REV:
    case GL_UNSIGNED_INT_10_10_10_2:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
    case GL_UNSIGNED_INT_5_9_9_9_REV:
    case GL_UNSIGNED_INT_24_8:
        return 4;
    case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
        return 8;
    default:
        fprintf(stderr, "Unsupported pixel type\n");
        abort();
    }
}

// 计算单个像素在客户端内存中占用的字节数
static uint32_t gl_bytes_per_pixel(GLenum format, GLenum type)
{
    // 打包类型：整像素占用固定字节数
    if (gl_is_packed_type(type))
        return gl_type_size(type);

    uint32_t components = 0;
    switch (format)
    {
    case GL_RED:
    case GL_RED_INTEGER:
    case GL_GREEN:
    case GL_BLUE:
    case GL_ALPHA:
    case GL_DEPTH_COMPONENT:
    case GL_STENCIL_INDEX:
        components = 1;
        break;
    case GL_RG:
    case GL_RG_INTEGER:
    case GL_DEPTH_STENCIL:
        components = 2;
        break;
    case GL_RGB:
    case GL_RGB_INTEGER:
    case GL_BGR:
    case GL_BGR_INTEGER:
        components = 3;
        break;
    case GL_RGBA:
    case GL_RGBA_INTEGER:
    case GL_BGRA:
    case GL_BGRA_INTEGER:
        components = 4;
        break;
    default:
        fprintf(stderr, "Unsupported pixel format\n");
        abort();
    }

    return components * gl_type_size(type);
}

// 根据 sized internal format 推导颜色纹理的传输格式与类型，未识别的格式返回 false
static bool rt_color_transfer_format(GLenum internal_format, GLenum& format, GLenum& type)
{
    switch (internal_format)
    {
    // ---- 归一化无符号 8 位 ----
    case GL_R8:            format = GL_RED;  type = GL_UNSIGNED_BYTE; return true;
    case GL_RG8:           format = GL_RG;   type = GL_UNSIGNED_BYTE; return true;
    case GL_RGB8:
    case GL_SRGB8:         format = GL_RGB;  type = GL_UNSIGNED_BYTE; return true;
    case GL_RGBA8:
    case GL_SRGB8_ALPHA8:  format = GL_RGBA; type = GL_UNSIGNED_BYTE; return true;
    // ---- 归一化有符号 8 位 ----
    case GL_R8_SNORM:      format = GL_RED;  type = GL_BYTE; return true;
    case GL_RG8_SNORM:     format = GL_RG;   type = GL_BYTE; return true;
    case GL_RGB8_SNORM:    format = GL_RGB;  type = GL_BYTE; return true;
    case GL_RGBA8_SNORM:   format = GL_RGBA; type = GL_BYTE; return true;
    // ---- 归一化无符号 16 位 ----
    case GL_R16:           format = GL_RED;  type = GL_UNSIGNED_SHORT; return true;
    case GL_RG16:          format = GL_RG;   type = GL_UNSIGNED_SHORT; return true;
    case GL_RGB16:         format = GL_RGB;  type = GL_UNSIGNED_SHORT; return true;
    case GL_RGBA16:        format = GL_RGBA; type = GL_UNSIGNED_SHORT; return true;
    // ---- 归一化有符号 16 位 ----
    case GL_R16_SNORM:     format = GL_RED;  type = GL_SHORT; return true;
    case GL_RG16_SNORM:    format = GL_RG;   type = GL_SHORT; return true;
    case GL_RGB16_SNORM:   format = GL_RGB;  type = GL_SHORT; return true;
    case GL_RGBA16_SNORM:  format = GL_RGBA; type = GL_SHORT; return true;
    // ---- 半精度浮点 ----
    case GL_R16F:          format = GL_RED;  type = GL_HALF_FLOAT; return true;
    case GL_RG16F:         format = GL_RG;   type = GL_HALF_FLOAT; return true;
    case GL_RGB16F:        format = GL_RGB;  type = GL_HALF_FLOAT; return true;
    case GL_RGBA16F:       format = GL_RGBA; type = GL_HALF_FLOAT; return true;
    // ---- 单精度浮点 ----
    case GL_R32F:          format = GL_RED;  type = GL_FLOAT; return true;
    case GL_RG32F:         format = GL_RG;   type = GL_FLOAT; return true;
    case GL_RGB32F:        format = GL_RGB;  type = GL_FLOAT; return true;
    case GL_RGBA32F:       format = GL_RGBA; type = GL_FLOAT; return true;
    // ---- 无符号整数 ----
    case GL_R8UI:          format = GL_RED_INTEGER;  type = GL_UNSIGNED_BYTE; return true;
    case GL_RG8UI:         format = GL_RG_INTEGER;   type = GL_UNSIGNED_BYTE; return true;
    case GL_RGB8UI:        format = GL_RGB_INTEGER;  type = GL_UNSIGNED_BYTE; return true;
    case GL_RGBA8UI:       format = GL_RGBA_INTEGER; type = GL_UNSIGNED_BYTE; return true;
    case GL_R16UI:         format = GL_RED_INTEGER;  type = GL_UNSIGNED_SHORT; return true;
    case GL_RG16UI:        format = GL_RG_INTEGER;   type = GL_UNSIGNED_SHORT; return true;
    case GL_RGB16UI:       format = GL_RGB_INTEGER;  type = GL_UNSIGNED_SHORT; return true;
    case GL_RGBA16UI:      format = GL_RGBA_INTEGER; type = GL_UNSIGNED_SHORT; return true;
    case GL_R32UI:         format = GL_RED_INTEGER;  type = GL_UNSIGNED_INT; return true;
    case GL_RG32UI:        format = GL_RG_INTEGER;   type = GL_UNSIGNED_INT; return true;
    case GL_RGB32UI:       format = GL_RGB_INTEGER;  type = GL_UNSIGNED_INT; return true;
    case GL_RGBA32UI:      format = GL_RGBA_INTEGER; type = GL_UNSIGNED_INT; return true;
    // ---- 有符号整数 ----
    case GL_R8I:           format = GL_RED_INTEGER;  type = GL_BYTE; return true;
    case GL_RG8I:          format = GL_RG_INTEGER;   type = GL_BYTE; return true;
    case GL_RGB8I:         format = GL_RGB_INTEGER;  type = GL_BYTE; return true;
    case GL_RGBA8I:        format = GL_RGBA_INTEGER; type = GL_BYTE; return true;
    case GL_R16I:          format = GL_RED_INTEGER;  type = GL_SHORT; return true;
    case GL_RG16I:         format = GL_RG_INTEGER;   type = GL_SHORT; return true;
    case GL_RGB16I:        format = GL_RGB_INTEGER;  type = GL_SHORT; return true;
    case GL_RGBA16I:       format = GL_RGBA_INTEGER; type = GL_SHORT; return true;
    case GL_R32I:          format = GL_RED_INTEGER;  type = GL_INT; return true;
    case GL_RG32I:         format = GL_RG_INTEGER;   type = GL_INT; return true;
    case GL_RGB32I:        format = GL_RGB_INTEGER;  type = GL_INT; return true;
    case GL_RGBA32I:       format = GL_RGBA_INTEGER; type = GL_INT; return true;
    // ---- 打包格式 ----
    case GL_RGB10_A2:      format = GL_RGBA;         type = GL_UNSIGNED_INT_2_10_10_10_REV; return true;
    case GL_RGB10_A2UI:    format = GL_RGBA_INTEGER; type = GL_UNSIGNED_INT_2_10_10_10_REV; return true;
    case GL_R11F_G11F_B10F: format = GL_RGB;         type = GL_UNSIGNED_INT_10F_11F_11F_REV; return true;
    case GL_RGB9_E5:       format = GL_RGB;          type = GL_UNSIGNED_INT_5_9_9_9_REV; return true;
    case GL_RGB565:        format = GL_RGB;          type = GL_UNSIGNED_SHORT_5_6_5; return true;
    case GL_RGB5_A1:       format = GL_RGBA;         type = GL_UNSIGNED_SHORT_5_5_5_1; return true;
    case GL_RGBA4:         format = GL_RGBA;         type = GL_UNSIGNED_SHORT_4_4_4_4; return true;
    default:
        return false;
    }
}

// 根据纹理与 aspect 决定传输使用的像素格式和类型
// download 为 true 时允许从深度模板纹理中单独读取深度或模板
static void gl_transfer_format(rt_texture_t const& texture, GLenum aspect, bool download, GLenum& format, GLenum& type)
{
    switch (texture.format)
    {
    // ---- 深度 + 模板 ----
    case GL_DEPTH_STENCIL:
        // 上传只能整体写入 depth+stencil；下载可按 aspect 单独读取
        if (download && aspect == GL_STENCIL_INDEX)
        {
            format = GL_STENCIL_INDEX;
            type = GL_UNSIGNED_BYTE;
        }
        else if (download && aspect == GL_DEPTH_COMPONENT)
        {
            format = GL_DEPTH_COMPONENT;
            type = GL_FLOAT;
        }
        else
        {
            format = GL_DEPTH_STENCIL;
            type = texture.type;
        }
        return;

    // ---- 仅深度 ----
    case GL_DEPTH_COMPONENT:
        if (aspect == GL_STENCIL_INDEX)
        {
            fprintf(stderr, "Texture has no stencil aspect\n");
            abort();
        }
        format = GL_DEPTH_COMPONENT;
        type = texture.type;
        return;

    // ---- 仅模板 ----
    case GL_STENCIL_INDEX:
        if (aspect == GL_DEPTH_COMPONENT && download)
        {
            fprintf(stderr, "Texture has no depth aspect\n");
            abort();
        }
        format = GL_STENCIL_INDEX;
        type = GL_UNSIGNED_BYTE;
        return;

    // ---- 颜色 ----
    default:
        // 优先根据 sized internal format 推导（可正确处理整数、半精度、打包等格式），
        // 未识别（如 unsized GL_RGBA）时回退到纹理创建时记录的 format/type
        if (!rt_color_transfer_format(texture.internal_format, format, type))
        {
            format = texture.format;
            type = texture.type;
        }
        return;
    }
}

// 校验纹理拷贝区域是否越界
static bool gl_check_texture_region(rt_texture_copy_t const& region, rt_size_t copySize)
{
    if (region.texture.handle == 0)
        return false;
    if (copySize.x == 0 || copySize.y == 0)
        return false;
    uint32_t width = 0, height = 0;
    width = std::max(1U, region.texture.width >> region.mipLevel);
    height = std::max(1U, region.texture.height >> region.mipLevel);

    if ((uint64_t)region.origin.x + copySize.x > width)
        return false;
    if ((uint64_t)region.origin.y + copySize.y > height)
        return false;
    // 2D 纹理没有深度维度，z 偏移必须为 0 且最多拷贝一层
    if (region.texture.target == GL_TEXTURE_2D && (region.origin.z != 0 || copySize.z > 1))
        return false;
    return true;
}

// 校验线性像素布局（bytesPerRow / rowsPerImage / offset）是否合法且不越界
// pbo 为 true 时额外要求 offset 是 type 大小的整数倍（GL 对 PBO 偏移的硬性要求）
static bool gl_check_texel_layout(uint32_t bytesPerRow, uint32_t rowsPerImage, size_t offset, size_t size,
                                  rt_size_t copySize, uint32_t bytesPerPixel, GLenum type, bool pbo)
{
    size_t tightRow = (size_t)copySize.x * bytesPerPixel;
    size_t rowStride = bytesPerRow ? bytesPerRow : tightRow;
    size_t imageRows = rowsPerImage ? rowsPerImage : copySize.y;
    size_t depth = std::max(1U, copySize.z);

    // GL_*_ROW_LENGTH 以像素为单位，bytesPerRow 必须能被整像素整除且不小于紧凑行
    if (bytesPerRow && (bytesPerRow % bytesPerPixel != 0 || rowStride < tightRow))
        return false;
    // GL_*_IMAGE_HEIGHT 不能小于实际拷贝高度，否则各层会重叠
    if (rowsPerImage && imageRows < copySize.y)
        return false;
    if (pbo && (offset % gl_type_size(type)) != 0)
        return false;

    // 最后一层的最后一行只需容纳紧凑宽度，之前的行/层按跨距计算
    size_t required = rowStride * imageRows * (depth - 1) + rowStride * (copySize.y - 1) + tightRow;
    if (offset > size || required > size - offset)
        return false;
    return true;
}

void gl_begin_transfer(rt_pass_transfer_t& pass)
{
    if (opengl.currentPipeline != nullptr)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }
    opengl.currentPassType = GL_MODULE_TRANSFER;
    opengl.currentTransferPass = &pass;
}

void gl_end_transfer(rt_pass_transfer_t& pass)
{
    if (opengl.currentTransferPass != &pass)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }
    opengl.currentPassType = GL_NONE;
    opengl.currentTransferPass = nullptr;

    // 保证传输结果对后续的着色器读取、顶点拉取和纹理采样可见
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_PIXEL_BUFFER_BARRIER_BIT |
                    GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT | GL_UNIFORM_BARRIER_BIT |
                    GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

void gl_copy_buffer(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (opengl.currentPassType != GL_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (source.buffer.handle == 0 || destination.buffer.handle == 0 || copySize == 0)
        return;
    if (source.offset + copySize > source.buffer.size)
        return;
    if (destination.offset + copySize > destination.buffer.size)
        return;

    glCopyNamedBufferSubData(source.buffer.handle, destination.buffer.handle,
                             (GLintptr)source.offset, (GLintptr)destination.offset, (GLsizeiptr)copySize);
}

void gl_copy_buffer_data(rt_buffer_data_t source, rt_buffer_copy_t destination, size_t copySize)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (opengl.currentPassType != GL_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (source.data == nullptr || destination.buffer.handle == 0 || copySize == 0)
        return;
    if (source.offset + copySize > source.size)
        return;
    if (destination.offset + copySize > destination.buffer.size)
        return;

    glNamedBufferSubData(destination.buffer.handle, (GLintptr)destination.offset, (GLsizeiptr)copySize,
                         source.data + source.offset);
}

void gl_copy_buffer_texture(rt_texture_copy_t source, rt_buffer_texel_t destination, rt_size_t copySize)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (opengl.currentPassType != GL_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (destination.buffer.handle == 0)
        return;
    if (!gl_check_texture_region(source, copySize))
        return;

    GLenum format = 0, type = 0;
    gl_transfer_format(source.texture, source.aspect, true, format, type);
    uint32_t bytesPerPixel = gl_bytes_per_pixel(format, type);
    if (!gl_check_texel_layout(destination.bytesPerRow, destination.rowsPerImage, destination.offset, destination.buffer.size,
                               copySize, bytesPerPixel, type, true))
        return;
    uint32_t depth = std::max(1U, copySize.z);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, destination.buffer.handle);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, (GLint)(destination.bytesPerRow / bytesPerPixel));
    glPixelStorei(GL_PACK_IMAGE_HEIGHT, (GLint)destination.rowsPerImage);
    glGetTextureSubImage(source.texture.handle, (GLint)source.mipLevel,
                         (GLint)source.origin.x, (GLint)source.origin.y, (GLint)source.origin.z,
                         (GLsizei)copySize.x, (GLsizei)copySize.y, (GLsizei)depth,
                         format, type, (GLsizei)(destination.buffer.size - destination.offset),
                         (void*)(uintptr_t)destination.offset);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_IMAGE_HEIGHT, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void gl_copy_texture(rt_texture_copy_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (opengl.currentPassType != GL_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (copySize.x == 0 || copySize.y == 0)
        return;
    if (!gl_check_texture_region(source, copySize) || !gl_check_texture_region(destination, copySize))
        return;

    GLsizei depth = (GLsizei)std::max(1U, copySize.z);
    glCopyImageSubData(source.texture.handle, source.texture.target, (GLint)source.mipLevel,
                       (GLint)source.origin.x, (GLint)source.origin.y, (GLint)source.origin.z,
                       destination.texture.handle, destination.texture.target, (GLint)destination.mipLevel,
                       (GLint)destination.origin.x, (GLint)destination.origin.y, (GLint)destination.origin.z,
                       (GLsizei)copySize.x, (GLsizei)copySize.y, depth);

    if (destination.texture.mipmaps) glGenerateTextureMipmap(destination.texture.handle);
}

void gl_copy_texture_data(rt_texture_data_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (opengl.currentPassType != GL_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (source.data == nullptr)
        return;
    if (!gl_check_texture_region(destination, copySize))
        return;

    GLenum format = 0, type = 0;
    gl_transfer_format(destination.texture, destination.aspect, false, format, type);
    uint32_t bytesPerPixel = gl_bytes_per_pixel(format, type);
    if (!gl_check_texel_layout(source.bytesPerRow, source.rowsPerImage, source.offset, source.size,
                               copySize, bytesPerPixel, type, false))
        return;

    const void* data = source.data + source.offset;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)(source.bytesPerRow / bytesPerPixel));
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, (GLint)source.rowsPerImage);
    if (destination.texture.target == GL_TEXTURE_2D)
    {
        glTextureSubImage2D(destination.texture.handle, (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLint)destination.origin.y,
                            (GLsizei)copySize.x, (GLsizei)copySize.y, format, type, data);

        if (destination.texture.mipmaps) glGenerateTextureMipmap(destination.texture.handle);
    }
    else if (destination.texture.target == GL_TEXTURE_3D)
    {
        glTextureSubImage3D(destination.texture.handle, (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLint)destination.origin.y, (GLint)destination.origin.z,
                            (GLsizei)copySize.x, (GLsizei)copySize.y, (GLsizei)std::max(1U, copySize.z), format, type, data);

        if (destination.texture.mipmaps) glGenerateTextureMipmap(destination.texture.handle);
    }
    else
    {
        fprintf(stderr, "Unsupported texture target\n");
        abort();
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
}

void gl_copy_texture_buffer(rt_buffer_texel_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (opengl.currentPassType != GL_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (source.buffer.handle == 0)
        return;
    if (!gl_check_texture_region(destination, copySize))
        return;

    GLenum format = 0, type = 0;
    gl_transfer_format(destination.texture, destination.aspect, false, format, type);
    uint32_t bytesPerPixel = gl_bytes_per_pixel(format, type);
    if (!gl_check_texel_layout(source.bytesPerRow, source.rowsPerImage, source.offset, source.buffer.size,
                               copySize, bytesPerPixel, type, true))
        return;

    // PBO 模式下 data 参数被解释为缓冲区内的字节偏移
    const void* data = (const void*)(uintptr_t)source.offset;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, source.buffer.handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)(source.bytesPerRow / bytesPerPixel));
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, (GLint)source.rowsPerImage);
    if (destination.texture.target == GL_TEXTURE_2D)
    {
        glTextureSubImage2D(destination.texture.handle, (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLint)destination.origin.y,
                            (GLsizei)copySize.x, (GLsizei)copySize.y, format, type, data);

        if (destination.texture.mipmaps) glGenerateTextureMipmap(destination.texture.handle);
    }
    else if (destination.texture.target == GL_TEXTURE_3D)
    {
        glTextureSubImage3D(destination.texture.handle, (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLint)destination.origin.y, (GLint)destination.origin.z,
                            (GLsizei)copySize.x, (GLsizei)copySize.y, (GLsizei)std::max(1U, copySize.z), format, type, data);

        if (destination.texture.mipmaps) glGenerateTextureMipmap(destination.texture.handle);
    }
    else
    {
        fprintf(stderr, "Unsupported texture target\n");
        abort();
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

// ====================================================================

static GLsizei gl_vertex_type_size(GLenum type)
{
    switch (type)
    {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_HALF_FLOAT:
        return 2;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
        return 4;
    case GL_DOUBLE:
        return 8;
    default:
        return 4;
    }
}

static GLsizei gl_index_type_size(GLenum type)
{
    switch (type)
    {
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_UNSIGNED_SHORT:
        return 2;
    default:
        return 4;
    }
}

rt_mesh_t gl_create_mesh(const float* vertices, // vec3
                                const float* normals, // vec3
                                const float* uvs, // vec2
                                size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    rt_mesh_t result = {};

    if (vertices)
        result.vertex[0] = gl_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = vertices,});

    if (normals)
        result.vertex[1] = gl_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = normals,});

    if (uvs)
        result.vertex[2] = gl_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = uvs,});

    if (indices)
        result.index = gl_create_buffer({.size = index_count * sizeof(uint32_t), .usage = GL_BUFFER_USAGE_INDEX | GL_BUFFER_USAGE_COPY_DST, .data = indices,});

    std::iota(result.location, result.location + std::size(result.location), 0);
    return result;
}

void gl_destroy_mesh(rt_mesh_t& mesh)
{
    for (auto& vertex : mesh.vertex)
        gl_destroy_buffer(vertex);
    gl_destroy_buffer(mesh.index);
}

void gl_draw_mesh(rt_mesh_t const& mesh)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (opengl.currentPassType != GL_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }

    rt_module_render_t const& module = opengl.currentRenderPass->module;

    GLsizei vertex_count = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.type == GL_NONE || layout.count == 0)
            continue;

        GLuint buffer = 0;
        GLsizei stride = gl_vertex_type_size(layout.type) * (GLsizei)layout.count;
        for (uint32_t k = 0; k < std::size(mesh.vertex); ++k)
        {
            if (mesh.vertex[k].handle == 0 || mesh.location[k] != layout.location)
                continue;
            buffer = mesh.vertex[k].handle;
            if (vertex_count == 0 && stride > 0)
                vertex_count = (GLsizei)(mesh.vertex[k].size / (size_t)stride);
            glBindVertexBuffer(layout.location, buffer, 0, buffer ? stride : 0);
            break;
        }
    }

    if (mesh.index.handle)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.index.handle);
        GLsizei index_stride = gl_index_type_size(module.index_type);
        auto index_count = (GLsizei)(mesh.index.size / (size_t)index_stride);
        glDrawElements(module.primitive, index_count, module.index_type, (void*)0);
    }
    else
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDrawArrays(module.primitive, 0, vertex_count);
    }
}

// ====================================================================

rt_meshlet_t gl_create_meshlet(const float* vertices, // vec4
                                      const float* normals, // vec4
                                      const float* uvs, size_t vertex_count, const unsigned int* indices,
                                      size_t index_count)
{
    rt_meshlet_t result = {};

    if (vertices)
        result.vertex[0] = gl_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = vertices,});

    if (normals)
        result.vertex[1] = gl_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = normals,});

    if (uvs)
        result.vertex[2] = gl_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = uvs,});

    if (indices)
        result.index = gl_create_buffer({.size = index_count * sizeof(uint32_t), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = indices,});

    std::iota(result.location, result.location + std::size(result.location), 0);
    return result;
}

void gl_destroy_meshlet(rt_meshlet_t& meshlet)
{
    for (auto& vertex : meshlet.vertex)
        gl_destroy_buffer(vertex);
    gl_destroy_buffer(meshlet.index);
}

void gl_draw_meshlet(rt_meshlet_t const& meshlet)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }
    if (opengl.currentPassType != GL_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin\n");
        abort();
    }

    rt_module_render_t const& module = opengl.currentRenderPass->module;

    uint32_t index_binding = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.type == GL_NONE || layout.count == 0)
            continue;

        for (uint32_t k = 0; k < std::size(meshlet.vertex); ++k)
        {
            if (meshlet.vertex[k].handle == 0 || meshlet.location[k] != layout.location)
                continue;
            gl_bind_buffer(meshlet.vertex[k], {.binding = layout.location, .target = GL_SHADER_STORAGE_BUFFER,});
            break;
        }
        if (layout.location + 1 > index_binding)
            index_binding = layout.location + 1;
    }

    if (meshlet.index.handle)
    {
        gl_bind_buffer(meshlet.index, {.binding = index_binding, .target = GL_SHADER_STORAGE_BUFFER,});
        GLsizei index_stride = gl_index_type_size(module.index_type);
        auto index_count = (GLsizei)(meshlet.index.size / (size_t)index_stride);
        glDrawMeshTasksNV(0, index_count / 3);
    }
}

// ====================================================================

rt_mesh_t gl_create_mesh_screen()
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
    static auto quad = gl_create_mesh(points, nullptr, uvs, 3, nullptr, 0);
    return quad;
}

void gl_draw_screen(int width, int height, rt_texture_t texture, rt_color_t clear)
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
    static auto module = gl_create_module_render({.vshader = VS, .fshader = FS, .vertex = {rt_vertex_vertex, {}, rt_vertex_uv,},});
    rt_pass_render_t pass = {.module = module, .screen = {.color = { .clear = true, .value = clear, }}};
    gl_begin_render(pass);
    gl_set_viewport(0, 0, width, height);
    gl_bind_texture(texture, { .binding = 0, });
    gl_draw_mesh(gl_create_mesh_screen());
    gl_end_render(pass);
}

void gl_submit()
{
    glFlush();
}

#endif
