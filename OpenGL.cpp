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

static GLenum rt_to_gl_buffer_target(rt_buffer_target_t target)
{
    switch (target)
    {
        case RT_UNIFORM_BUFFER: return GL_UNIFORM_BUFFER;
        case RT_SHADER_STORAGE_BUFFER: return GL_SHADER_STORAGE_BUFFER;
        default: return GL_UNIFORM_BUFFER;
    }
}

static GLenum rt_to_gl_texture_target(rt_texture_target_t target)
{
    switch (target)
    {
        case RT_TEXTURE_1D: return GL_TEXTURE_1D;
        case RT_TEXTURE_2D: return GL_TEXTURE_2D;
        case RT_TEXTURE_3D: return GL_TEXTURE_3D;
        case RT_TEXTURE_2D_ARRAY: return GL_TEXTURE_2D_ARRAY;
        case RT_TEXTURE_2D_MULTISAMPLE: return GL_TEXTURE_2D_MULTISAMPLE;
        default: return GL_TEXTURE_2D;
    }
}

static GLenum rt_to_gl_format(rt_format_t format)
{
    switch (format)
    {
        case RT_STENCIL_INDEX: return GL_STENCIL_INDEX;
        case RT_DEPTH_COMPONENT: return GL_DEPTH_COMPONENT;
        case RT_RED: return GL_RED;
        case RT_RGB: return GL_RGB;
        case RT_RGBA: return GL_RGBA;
        case RT_RG: return GL_RG;
        case RT_DEPTH_STENCIL: return GL_DEPTH_STENCIL;
        default: return GL_RGBA;
    }
}

static GLenum rt_to_gl_internal_format(rt_internal_format_t format)
{
    switch (format)
    {
        case RT_R8: return GL_R8;
        case RT_R16: return GL_R16;
        case RT_RG8: return GL_RG8;
        case RT_RG16: return GL_RG16;
        case RT_R16F: return GL_R16F;
        case RT_R32F: return GL_R32F;
        case RT_RG16F: return GL_RG16F;
        case RT_RG32F: return GL_RG32F;
        case RT_RGB8: return GL_RGB8;
        case RT_RGB16: return GL_RGB16;
        case RT_RGBA8: return GL_RGBA8;
        case RT_RGBA16: return GL_RGBA16;
        case RT_SRGB8_ALPHA8: return GL_SRGB8_ALPHA8;
        case RT_RGB16F: return GL_RGB16F;
        case RT_RGBA16F: return GL_RGBA16F;
        case RT_RGB32F: return GL_RGB32F;
        case RT_RGBA32F: return GL_RGBA32F;
        case RT_DEPTH_COMPONENT16: return GL_DEPTH_COMPONENT16;
        case RT_DEPTH_COMPONENT24: return GL_DEPTH_COMPONENT24;
        case RT_DEPTH_COMPONENT32F: return GL_DEPTH_COMPONENT32F;
        case RT_DEPTH24_STENCIL8: return GL_DEPTH24_STENCIL8;
        case RT_DEPTH32F_STENCIL8: return GL_DEPTH32F_STENCIL8;
        default: return GL_RGBA8;
    }
}

static GLenum rt_to_gl_type(rt_type_t type)
{
    switch (type)
    {
        case RT_TYPE_NONE: return GL_UNSIGNED_BYTE;
        case RT_BYTE: return GL_BYTE;
        case RT_UNSIGNED_BYTE: return GL_UNSIGNED_BYTE;
        case RT_SHORT: return GL_SHORT;
        case RT_UNSIGNED_SHORT: return GL_UNSIGNED_SHORT;
        case RT_INT: return GL_INT;
        case RT_UNSIGNED_INT: return GL_UNSIGNED_INT;
        case RT_FLOAT: return GL_FLOAT;
        case RT_DOUBLE: return GL_DOUBLE;
        case RT_HALF_FLOAT: return GL_HALF_FLOAT;
        case RT_UNSIGNED_INT_24_8: return GL_UNSIGNED_INT_24_8;
        default: return GL_UNSIGNED_BYTE;
    }
}

static GLenum rt_to_gl_filter(rt_filter_t filter)
{
    switch (filter)
    {
        case RT_NEAREST: return GL_NEAREST;
        case RT_LINEAR: return GL_LINEAR;
        case RT_NEAREST_MIPMAP_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
        case RT_LINEAR_MIPMAP_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
        case RT_NEAREST_MIPMAP_LINEAR: return GL_NEAREST_MIPMAP_LINEAR;
        case RT_LINEAR_MIPMAP_LINEAR: return GL_LINEAR_MIPMAP_LINEAR;
        default: return GL_LINEAR;
    }
}

static GLenum rt_to_gl_wrap(rt_wrap_t wrap)
{
    switch (wrap)
    {
        case RT_REPEAT: return GL_REPEAT;
        case RT_CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
        case RT_CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
        case RT_MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
        case RT_MIRROR_CLAMP_TO_EDGE: return GL_MIRROR_CLAMP_TO_EDGE;
        default: return GL_REPEAT;
    }
}

static GLenum rt_to_gl_access(rt_access_t access)
{
    switch (access)
    {
        case RT_READ_ONLY: return GL_READ_ONLY;
        case RT_WRITE_ONLY: return GL_WRITE_ONLY;
        case RT_READ_WRITE: return GL_READ_WRITE;
        default: return GL_WRITE_ONLY;
    }
}

static GLenum rt_to_gl_blend_op(rt_blend_op_t op)
{
    switch (op)
    {
        case RT_FUNC_ADD: return GL_FUNC_ADD;
        case RT_MIN: return GL_MIN;
        case RT_MAX: return GL_MAX;
        case RT_FUNC_SUBTRACT: return GL_FUNC_SUBTRACT;
        case RT_FUNC_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
        default: return GL_FUNC_ADD;
    }
}

static GLenum rt_to_gl_blend_factor(rt_blend_factor_t factor)
{
    switch (factor)
    {
        case RT_BLEND_ZERO: return GL_ZERO;
        case RT_BLEND_ONE: return GL_ONE;
        case RT_BLEND_SRC_COLOR: return GL_SRC_COLOR;
        case RT_BLEND_ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
        case RT_BLEND_SRC_ALPHA: return GL_SRC_ALPHA;
        case RT_BLEND_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
        case RT_BLEND_DST_ALPHA: return GL_DST_ALPHA;
        case RT_BLEND_ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
        case RT_BLEND_DST_COLOR: return GL_DST_COLOR;
        case RT_BLEND_ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
        case RT_BLEND_SRC_ALPHA_SATURATE: return GL_SRC_ALPHA_SATURATE;
        case RT_BLEND_CONSTANT_COLOR: return GL_CONSTANT_COLOR;
        case RT_BLEND_ONE_MINUS_CONSTANT_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
        case RT_BLEND_CONSTANT_ALPHA: return GL_CONSTANT_ALPHA;
        case RT_BLEND_ONE_MINUS_CONSTANT_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;
        default: return GL_ONE;
    }
}

static GLenum rt_to_gl_compare(rt_compare_op_t func)
{
    switch (func)
    {
        case RT_NEVER: return GL_NEVER;
        case RT_LESS: return GL_LESS;
        case RT_EQUAL: return GL_EQUAL;
        case RT_LEQUAL: return GL_LEQUAL;
        case RT_GREATER: return GL_GREATER;
        case RT_NOTEQUAL: return GL_NOTEQUAL;
        case RT_GEQUAL: return GL_GEQUAL;
        case RT_ALWAYS: return GL_ALWAYS;
        default: return GL_ALWAYS;
    }
}

static GLenum rt_to_gl_stencil_op(rt_stencil_op_t op)
{
    switch (op)
    {
        case RT_STENCIL_ZERO: return GL_ZERO;
        case RT_STENCIL_INVERT: return GL_INVERT;
        case RT_STENCIL_KEEP: return GL_KEEP;
        case RT_STENCIL_REPLACE: return GL_REPLACE;
        case RT_STENCIL_INCR: return GL_INCR;
        case RT_STENCIL_DECR: return GL_DECR;
        case RT_STENCIL_INCR_WRAP: return GL_INCR_WRAP;
        case RT_STENCIL_DECR_WRAP: return GL_DECR_WRAP;
        default: return GL_KEEP;
    }
}

static GLenum rt_to_gl_cull(rt_cull_mode_t mode)
{
    switch (mode)
    {
        case RT_CULL_NONE: return GL_NONE;
        case RT_CULL_FRONT: return GL_FRONT;
        case RT_CULL_BACK: return GL_BACK;
        case RT_CULL_FRONT_AND_BACK: return GL_FRONT_AND_BACK;
        default: return GL_NONE;
    }
}

static GLenum rt_to_gl_front_face(rt_front_face_t face)
{
    switch (face)
    {
        case RT_CW: return GL_CW;
        case RT_CCW: return GL_CCW;
        default: return GL_CCW;
    }
}

static GLenum rt_to_gl_fill(rt_fill_mode_t fill)
{
    switch (fill)
    {
        case RT_POINT: return GL_POINT;
        case RT_LINE: return GL_LINE;
        case RT_FILL: return GL_FILL;
        default: return GL_FILL;
    }
}

static GLenum rt_to_gl_primitive(rt_primitive_t primitive)
{
    switch (primitive)
    {
        case RT_POINTS: return GL_POINTS;
        case RT_LINES: return GL_LINES;
        case RT_LINE_LOOP: return GL_LINE_LOOP;
        case RT_LINE_STRIP: return GL_LINE_STRIP;
        case RT_TRIANGLES: return GL_TRIANGLES;
        case RT_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        case RT_TRIANGLE_FAN: return GL_TRIANGLE_FAN;
        default: return GL_TRIANGLES;
    }
}

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
    fprintf(stdout, "GLSL   Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    fprintf(stdout, "OpenGL Render : %s\n", glGetString(GL_RENDERER));
    fprintf(stdout, "OpenGL Vendor : %s\n", glGetString(GL_VENDOR));
    GLint maxMeshOutputPrimitives = 0;
    glGetIntegerv(GL_MAX_MESH_OUTPUT_PRIMITIVES_NV, &maxMeshOutputPrimitives);
    GLint maxMeshOutputVertices = 0;
    glGetIntegerv(GL_MAX_MESH_OUTPUT_VERTICES_NV, &maxMeshOutputVertices);
    fprintf(stdout, "Meshlet Primitives: %d\n", maxMeshOutputPrimitives);
    fprintf(stdout, "Meshlet Vertices  : %d\n", maxMeshOutputVertices);
    fflush(stdout);

    // ====================================================================

    rt_unload_library = gl_unload_library;
    rt_create_buffer = gl_create_buffer;
    rt_destroy_buffer = gl_destroy_buffer;
    rt_bind_buffer = gl_bind_buffer;
    rt_map_buffer = gl_map_buffer;
    rt_unmap_buffer = gl_unmap_buffer;
    rt_create_texture = gl_create_texture;
    rt_create_texture_color = gl_create_texture_color;
    rt_create_texture_color_float = gl_create_texture_color_float;
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
    rt_draw_mesh_multi = gl_draw_mesh_multi;
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

    if(rt_unload_library == gl_unload_library) rt_unload_library = nullptr;
    if(rt_create_buffer == gl_create_buffer) rt_create_buffer = nullptr;
    if(rt_destroy_buffer == gl_destroy_buffer) rt_destroy_buffer = nullptr;
    if(rt_bind_buffer == gl_bind_buffer) rt_bind_buffer = nullptr;
    if(rt_map_buffer == gl_map_buffer) rt_map_buffer = nullptr;
    if(rt_unmap_buffer == gl_unmap_buffer) rt_unmap_buffer = nullptr;
    if(rt_create_texture == gl_create_texture) rt_create_texture = nullptr;
    if(rt_create_texture_color == gl_create_texture_color) rt_create_texture_color = nullptr;
    if(rt_create_texture_color_float == gl_create_texture_color_float) rt_create_texture_color_float = nullptr;
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
    if (info.usage & RT_BUFFER_USAGE_MAP_READ)
        flags |= GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    if (info.usage & RT_BUFFER_USAGE_MAP_WRITE)
        flags |= GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    if (info.usage & RT_BUFFER_USAGE_COPY_DST)
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

void gl_bind_buffer(rt_buffer_t& buffer, rt_buffer_bind_t bind)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    switch (bind.target)
    {
    case RT_UNIFORM_BUFFER:
        glBindBufferBase(rt_to_gl_buffer_target(bind.target), bind.binding, buffer.handle);
        break;
    case RT_SHADER_STORAGE_BUFFER:
        glBindBufferBase(rt_to_gl_buffer_target(bind.target), bind.binding, buffer.handle);
        break;
    default:
        fprintf(stderr, "Unsupported buffer target");
        abort();
    }
}

void* gl_map_buffer(rt_buffer_t& buffer, rt_access_t mode, size_t offset, size_t size)
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
    case RT_READ_ONLY:
        access = GL_MAP_READ_BIT;
        break;
    case RT_WRITE_ONLY:
        access = GL_MAP_WRITE_BIT;
        break;
    case RT_READ_WRITE:
        access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        break;
    default:
        fprintf(stderr, "Unsupported buffer map mode");
        abort();
    }
    if (buffer.usage & (RT_BUFFER_USAGE_MAP_READ | RT_BUFFER_USAGE_MAP_WRITE))
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

    uint32_t mipmaps = 1;
    uint32_t samples = info.samples ? info.samples : 1;
    uint32_t depth = info.depth ? info.depth : 1;
    rt_texture_target_t target = info.target;
    if (info.width == 0 || (target != RT_TEXTURE_1D && info.height == 0))
    {
        fprintf(stderr, "Texture size must not be 0");
        abort();
    }
    if ((target == RT_TEXTURE_2D_MULTISAMPLE) != (samples > 1))
    {
        fprintf(stderr, "RT_TEXTURE_2D_MULTISAMPLE requires samples > 1");
        abort();
    }

    if (target != RT_TEXTURE_2D_MULTISAMPLE)
    {
        uint32_t maxDim = info.width;
        if (target != RT_TEXTURE_1D) maxDim = std::max(maxDim, info.height);
        if (target == RT_TEXTURE_3D) maxDim = std::max(maxDim, depth);
        uint32_t maxLevels = 1;
        while (maxDim > 1)
        {
            maxDim >>= 1;
            maxLevels++;
        }
        mipmaps = info.mipmaps ? std::min(info.mipmaps, maxLevels) : maxLevels;
    }

    GLenum glTarget = rt_to_gl_texture_target(target);
    GLenum glInternal = rt_to_gl_internal_format(info.internal_format);
    GLenum glFormat = rt_to_gl_format(info.format);
    GLenum glType = rt_to_gl_type(info.type);

    glGenTextures(1, &result.handle);
    glBindTexture(glTarget, result.handle);

    if (target == RT_TEXTURE_1D)
    {
        glTexStorage1D(glTarget, (GLsizei)mipmaps, glInternal, (GLsizei)info.width);
    }
    else if (target == RT_TEXTURE_2D)
    {
        glTexStorage2D(glTarget, (GLsizei)mipmaps, glInternal, (GLsizei)info.width, (GLsizei)info.height);
    }
    else if (target == RT_TEXTURE_2D_ARRAY || target == RT_TEXTURE_3D)
    {
        glTexStorage3D(glTarget, (GLsizei)mipmaps, glInternal, (GLsizei)info.width, (GLsizei)info.height, (GLsizei)depth);
    }
    else if (target == RT_TEXTURE_2D_MULTISAMPLE)
    {
        glTexStorage2DMultisample(glTarget, (GLsizei)samples, glInternal, (GLsizei)info.width, (GLsizei)info.height, GL_TRUE);
    }
    else
    {
        fprintf(stderr, "Unsupported texture target");
        abort();
    }

    if (target != RT_TEXTURE_2D_MULTISAMPLE && info.data)
    {
        if (target == RT_TEXTURE_1D)
        {
            glTexSubImage1D(glTarget, 0, 0, (GLsizei)info.width, glFormat, glType, info.data);
        }
        else if (target == RT_TEXTURE_2D)
        {
            glTexSubImage2D(glTarget, 0, 0, 0, (GLsizei)info.width, (GLsizei)info.height, glFormat, glType, info.data);
        }
        else if (info.target == RT_TEXTURE_3D || info.target == RT_TEXTURE_2D_ARRAY)
        {
            glTexSubImage3D(glTarget, 0, 0, 0, 0, (GLsizei)info.width, (GLsizei)info.height, (GLsizei)depth, glFormat, glType, info.data);
        }
    }

    if (info.data == nullptr)
    {
        if (info.format == RT_DEPTH_COMPONENT)
        {
            GLfloat clearValue = 1.0f;
            glClearTexImage(result.handle, 0, glFormat, glType, &clearValue);
        }
        else if (info.format == RT_DEPTH_STENCIL)
        {
            if (info.internal_format == RT_DEPTH24_STENCIL8)
            {
                GLuint clearValue = 0xFFFFFF00u; // 深度 24 位全 1 (= 1.0)，模板 8 位 = 0
                glClearTexImage(result.handle, 0, glFormat, glType, &clearValue);
            }
            else if (info.internal_format == RT_DEPTH32F_STENCIL8)
            {
                struct
                {
                    float depth;
                    uint32_t stencil;
                } clearValue = {1.0f, 0};
                glClearTexImage(result.handle, 0, glFormat, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, &clearValue);
            }
            else
            {
                fprintf(stderr, "Unsupported depth stencil format");
                abort();
            }
        }
        else
        {
            if (info.type == RT_FLOAT)
            {
                GLfloat clearValue[4] = {0, 0, 0, 0};
                glClearTexImage(result.handle, 0, glFormat, glType, clearValue);
            }
            else
            {
                GLubyte clearValue[4] = {0, 0, 0, 0};
                glClearTexImage(result.handle, 0, glFormat, glType, clearValue);
            }
        }
    }

    if (target != RT_TEXTURE_2D_MULTISAMPLE)
    {
        glTexParameteri(glTarget, GL_TEXTURE_WRAP_S, rt_to_gl_wrap(info.wrap_s));
        glTexParameteri(glTarget, GL_TEXTURE_WRAP_T, rt_to_gl_wrap(info.wrap_t));
        glTexParameteri(glTarget, GL_TEXTURE_WRAP_R, rt_to_gl_wrap(info.wrap_r));
        glTexParameteri(glTarget, GL_TEXTURE_MIN_FILTER, rt_to_gl_filter(info.min_filter));
        glTexParameteri(glTarget, GL_TEXTURE_MAG_FILTER, rt_to_gl_filter(info.mag_filter));
        glTexParameteri(glTarget, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(glTarget, GL_TEXTURE_MAX_LEVEL, (GLint)(mipmaps - 1));

        if (info.wrap_s == RT_CLAMP_TO_BORDER || info.wrap_t == RT_CLAMP_TO_BORDER || info.wrap_r == RT_CLAMP_TO_BORDER)
        {
            glTexParameterfv(glTarget, GL_TEXTURE_BORDER_COLOR, info.border);
        }

        if (info.min_filter == RT_NEAREST_MIPMAP_NEAREST || info.min_filter == RT_LINEAR_MIPMAP_NEAREST || info.min_filter == RT_NEAREST_MIPMAP_LINEAR || info.min_filter == RT_LINEAR_MIPMAP_LINEAR)
        {
            glGenerateMipmap(glTarget);
        }
    }

    glBindTexture(glTarget, 0);

    result.width = info.width;
    result.height = (target == RT_TEXTURE_1D) ? 1 : info.height;
    result.depth = depth;
    result.target = target;
    result.format = info.format;
    result.internal_format = info.internal_format;
    result.type = info.type;
    result.mipmaps = mipmaps;
    result.samples = samples;
    return result;
}

rt_texture_t gl_create_texture_color(uint32_t width, uint32_t height, const void* data)
{
    rt_texture_info_t info
    {
        .width = width,
        .height = height,
        .target = RT_TEXTURE_2D,
        .format = RT_RGBA,
        .internal_format = RT_RGBA8,
        .type = RT_UNSIGNED_BYTE,
        .min_filter = RT_LINEAR,
        .mag_filter = RT_LINEAR,
        .wrap_s = RT_CLAMP_TO_BORDER,
        .wrap_t = RT_CLAMP_TO_BORDER,
        .border = {0.0f, 0.0f, 0.0f, 0.0f},
        .data = data,
    };
    return gl_create_texture(info);
}

rt_texture_t gl_create_texture_color_float(uint32_t width, uint32_t height, const void* data)
{
    rt_texture_info_t info
    {
        .width = width,
        .height = height,
        .target = RT_TEXTURE_2D,
        .format = RT_RGBA,
        .internal_format = RT_RGBA32F,
        .type = RT_FLOAT,
        .min_filter = RT_LINEAR,
        .mag_filter = RT_LINEAR,
        .wrap_s = RT_CLAMP_TO_BORDER,
        .wrap_t = RT_CLAMP_TO_BORDER,
        .border = {0.0f, 0.0f, 0.0f, 0.0f},
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
        .target = RT_TEXTURE_2D,
        .format = RT_DEPTH_COMPONENT,
        .internal_format = RT_DEPTH_COMPONENT32F,
        .type = RT_FLOAT,
        .min_filter = RT_NEAREST,
        .mag_filter = RT_NEAREST,
        .wrap_s = RT_CLAMP_TO_BORDER,
        .wrap_t = RT_CLAMP_TO_BORDER,
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
        .target = RT_TEXTURE_2D,
        .format = RT_DEPTH_STENCIL,
        .internal_format = RT_DEPTH24_STENCIL8,
        .type = RT_UNSIGNED_INT_24_8,
        .min_filter = RT_NEAREST,
        .mag_filter = RT_NEAREST,
        .wrap_s = RT_CLAMP_TO_EDGE,
        .wrap_t = RT_CLAMP_TO_EDGE,
        .data = data,
    };
    return gl_create_texture(info);
}

void gl_destroy_texture(rt_texture_t& texture)
{
    glDeleteTextures(1, &texture.handle);
    texture.handle = 0;
}

void gl_bind_texture(rt_texture_t& texture, rt_texture_bind_t bind)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    glActiveTexture(GL_TEXTURE0 + bind.binding);
    glBindTexture(rt_to_gl_texture_target(texture.target), texture.handle);

    if (texture.format == RT_DEPTH_COMPONENT || texture.format == RT_DEPTH_STENCIL)
    {
        glTexParameteri(rt_to_gl_texture_target(texture.target), GL_DEPTH_STENCIL_TEXTURE_MODE, rt_to_gl_format(bind.aspect_mode));
    }
}

void gl_bind_texture_storage(rt_texture_t& texture, rt_texture_storage_bind_t bind)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    glBindImageTexture(bind.binding, texture.handle, (GLint)bind.base_level, 1 < bind.layer_count,
                       (GLint)bind.base_layer, rt_to_gl_access(bind.access), rt_to_gl_internal_format(texture.internal_format));
}

// ====================================================================

rt_sampler_t gl_create_sampler(rt_sampler_info_t const& info)
{
    rt_sampler_t result = {};

    glGenSamplers(1, &result.handle);

    // 设置过滤方式
    glSamplerParameteri(result.handle, GL_TEXTURE_MIN_FILTER, rt_to_gl_filter(info.min_filter));
    glSamplerParameteri(result.handle, GL_TEXTURE_MAG_FILTER, rt_to_gl_filter(info.mag_filter));

    // 设置环绕方式
    glSamplerParameteri(result.handle, GL_TEXTURE_WRAP_S, rt_to_gl_wrap(info.wrap_s));
    glSamplerParameteri(result.handle, GL_TEXTURE_WRAP_T, rt_to_gl_wrap(info.wrap_t));
    glSamplerParameteri(result.handle, GL_TEXTURE_WRAP_R, rt_to_gl_wrap(info.wrap_r));

    return result;
}

void gl_destroy_sampler(rt_sampler_t& sampler)
{
    glDeleteSamplers(1, &sampler.handle);
    sampler.handle = 0;
}

void gl_bind_sampler(rt_sampler_t& sampler, rt_sampler_bind_t bind)
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
        if (vertex.type == RT_TYPE_NONE || vertex.count == 0)
            continue;

        glEnableVertexAttribArray(vertex.location);
        GLenum glType = rt_to_gl_type(vertex.type);
        switch (vertex.type)
        {
        case RT_BYTE:
        case RT_UNSIGNED_BYTE:
        case RT_SHORT:
        case RT_UNSIGNED_SHORT:
        case RT_INT:
        case RT_UNSIGNED_INT:
            glVertexAttribIFormat(vertex.location, (GLint)vertex.count, glType, 0);
            break;
        case RT_FLOAT:
            glVertexAttribFormat(vertex.location, (GLint)vertex.count, glType, GL_FALSE, 0);
            break;
        case RT_DOUBLE:
            glVertexAttribLFormat(vertex.location, (GLint)vertex.count, glType, 0);
            break;
        case RT_HALF_FLOAT:
        case RT_UNSIGNED_INT_24_8:
        default:
            glVertexAttribFormat(vertex.location, (GLint)vertex.count, glType, GL_FALSE, 0);
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
    if (opengl.currentPipeline != nullptr)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }
    if (pass.module.handle == 0)
    {
        fprintf(stderr, "Pipeline module is not created\n");
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
    if (opengl.currentPipeline != nullptr)
    {
        fprintf(stderr, "Pipeline not end\n");
        abort();
    }
    if (pass.module.handle == 0)
    {
        fprintf(stderr, "Pipeline module is not created\n");
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
        uint32_t width = 0, height = 0;
        GLenum colorAttachments[GL_MAX_COLOR_TEXTURE_NUM] = {};
        for (size_t i = 0; i < std::size(pass.colors); ++i)
        {
            if (pass.colors[i].texture.handle)
            {
                glBindTexture(rt_to_gl_texture_target(pass.colors[i].texture.target), pass.colors[i].texture.handle);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, rt_to_gl_texture_target(pass.colors[i].texture.target), pass.colors[i].texture.handle, 0);
                colorAttachments[colorCount++] = GL_COLOR_ATTACHMENT0 + i;
                width = std::max(width, pass.colors[i].texture.width);
                height = std::max(height, pass.colors[i].texture.height);
            }
        }
        if (colorCount) glDrawBuffers(colorCount, colorAttachments);

        if (pass.depth.texture.handle)
        {
            glBindTexture(rt_to_gl_texture_target(pass.depth.texture.target), pass.depth.texture.handle);
            if (pass.depth.texture.format == RT_DEPTH_COMPONENT)
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, rt_to_gl_texture_target(pass.depth.texture.target), pass.depth.texture.handle, 0);
            }
            else if (pass.depth.texture.format == RT_DEPTH_STENCIL)
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, rt_to_gl_texture_target(pass.depth.texture.target), pass.depth.texture.handle, 0);
            }
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
                {
                    glColorMask(true, true, true, true);
                    glClearBufferfv(GL_COLOR, (int32_t)i, &pass.colors[i].value.r);
                }

                if (pass.module.colors[i].color.func != RT_FUNC_ADD || pass.module.colors[i].color.src != RT_BLEND_ONE ||
                    pass.module.colors[i].color.dst != RT_BLEND_ZERO || pass.module.colors[i].alpha.func != RT_FUNC_ADD ||
                    pass.module.colors[i].alpha.src != RT_BLEND_ONE || pass.module.colors[i].alpha.dst != RT_BLEND_ZERO)
                {
                    glEnable(GL_BLEND);
                }

                glBlendEquationSeparatei(i, rt_to_gl_blend_op(pass.module.colors[i].color.func), rt_to_gl_blend_op(pass.module.colors[i].alpha.func));
                glBlendFuncSeparatei(i, rt_to_gl_blend_factor(pass.module.colors[i].color.src), rt_to_gl_blend_factor(pass.module.colors[i].color.dst), rt_to_gl_blend_factor(pass.module.colors[i].alpha.src), rt_to_gl_blend_factor(pass.module.colors[i].alpha.dst));
            }
        }

        if (pass.depth.texture.handle &&
            (pass.depth.texture.format == RT_DEPTH_COMPONENT || pass.depth.texture.format == RT_DEPTH_STENCIL))
        {
            if (pass.depth.clear)
            {
                glClearBufferfv(GL_DEPTH, 0, &pass.depth.value);
            }
            if (pass.stencil.clear && pass.depth.texture.format == RT_DEPTH_STENCIL)
            {
                glClearBufferiv(GL_STENCIL, 0, &pass.stencil.value);
            }
        }

        // Depth State

        if (pass.module.depth.func == RT_ALWAYS && pass.module.depth.write == false)
        {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }
        else
        {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(pass.module.depth.write);
        }
        glDepthFunc(rt_to_gl_compare(pass.module.depth.func));

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

        if (pass.module.stencil.back.func != RT_ALWAYS || pass.module.stencil.back.sfail != RT_STENCIL_KEEP ||
            pass.module.stencil.back.zfail != RT_STENCIL_KEEP || pass.module.stencil.back.zpass != RT_STENCIL_KEEP ||
            pass.module.stencil.front.func != RT_ALWAYS || pass.module.stencil.front.sfail != RT_STENCIL_KEEP ||
            pass.module.stencil.front.zfail != RT_STENCIL_KEEP || pass.module.stencil.front.zpass != RT_STENCIL_KEEP)
        {
            glEnable(GL_STENCIL_TEST);
        }
        else
        {
            glDisable(GL_STENCIL_TEST);
        }
        glStencilMask(pass.module.stencil.write);
        glStencilFuncSeparate(GL_BACK, rt_to_gl_compare(pass.module.stencil.back.func), pass.stencil.refer, pass.module.stencil.read);
        glStencilFuncSeparate(GL_FRONT, rt_to_gl_compare(pass.module.stencil.front.func), pass.stencil.refer, pass.module.stencil.read);
        glStencilOpSeparate(GL_BACK, rt_to_gl_stencil_op(pass.module.stencil.back.sfail), rt_to_gl_stencil_op(pass.module.stencil.back.zfail), rt_to_gl_stencil_op(pass.module.stencil.back.zpass));
        glStencilOpSeparate(GL_FRONT, rt_to_gl_stencil_op(pass.module.stencil.front.sfail), rt_to_gl_stencil_op(pass.module.stencil.front.zfail), rt_to_gl_stencil_op(pass.module.stencil.front.zpass));
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

        if (pass.screen.color.blend.func != RT_FUNC_ADD || pass.screen.color.blend.src != RT_BLEND_ONE ||
            pass.screen.color.blend.dst != RT_BLEND_ZERO)
        {
            glEnable(GL_BLEND);
        }
        else
        {
            glDisable(GL_BLEND);
        }
        glBlendEquation(rt_to_gl_blend_op(pass.screen.color.blend.func));
        glBlendFunc(rt_to_gl_blend_factor(pass.screen.color.blend.src), rt_to_gl_blend_factor(pass.screen.color.blend.dst));

        // Depth State

        if (pass.screen.depth.func == RT_ALWAYS && pass.screen.depth.write == false)
        {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }
        else
        {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(pass.screen.depth.write);
        }
        glDepthFunc(rt_to_gl_compare(pass.screen.depth.func));

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

        if (pass.screen.stencil.func != RT_ALWAYS || pass.screen.stencil.sfail != RT_STENCIL_KEEP ||
            pass.screen.stencil.zfail != RT_STENCIL_KEEP || pass.screen.stencil.zpass != RT_STENCIL_KEEP)
        {
            glEnable(GL_STENCIL_TEST);
        }
        else
        {
            glDisable(GL_STENCIL_TEST);
        }
        glStencilMask(pass.screen.stencil.write);
        glStencilFunc(rt_to_gl_compare(pass.screen.stencil.func), pass.screen.stencil.refer, pass.screen.stencil.read);
        glStencilOp(rt_to_gl_stencil_op(pass.screen.stencil.sfail), rt_to_gl_stencil_op(pass.screen.stencil.zfail), rt_to_gl_stencil_op(pass.screen.stencil.zpass));
    }

    // Primitive State

    glFrontFace(rt_to_gl_front_face(pass.module.front_face));
    if (pass.module.cull_mode)
    {
        glCullFace(rt_to_gl_cull(pass.module.cull_mode));
    }
    if (pass.module.cull_mode)
    {
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }

    glPolygonMode(GL_FRONT_AND_BACK, rt_to_gl_fill(pass.module.fill_mode));
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
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_INT_8_8_8_8:
    case GL_UNSIGNED_INT_10_10_10_2:
    case GL_UNSIGNED_BYTE_2_3_3_REV:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_5_6_5_REV:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
    case GL_UNSIGNED_INT_8_8_8_8_REV:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_24_8:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
    case GL_UNSIGNED_INT_5_9_9_9_REV:
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
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        return 2;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
        return 4;
    case GL_HALF_FLOAT:
        return 2;
    case GL_UNSIGNED_BYTE_3_3_2:
        return 1;
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
        return 2;
    case GL_UNSIGNED_INT_8_8_8_8:
    case GL_UNSIGNED_INT_10_10_10_2:
        return 4;
    case GL_UNSIGNED_BYTE_2_3_3_REV:
        return 1;
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_5_6_5_REV:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
        return 2;
    case GL_UNSIGNED_INT_8_8_8_8_REV:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_24_8:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
    case GL_UNSIGNED_INT_5_9_9_9_REV:
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
    case GL_STENCIL_INDEX:
    case GL_DEPTH_COMPONENT:
    case GL_RED:
    case GL_GREEN:
    case GL_BLUE:
    case GL_ALPHA:
        components = 1;
        break;
    case GL_RGB:
        components = 3;
        break;
    case GL_RGBA:
        components = 4;
        break;
    case GL_BGR:
        components = 3;
        break;
    case GL_BGRA:
        components = 4;
        break;
    case GL_RG:
    case GL_RG_INTEGER:
        components = 2;
        break;
    case GL_DEPTH_STENCIL:
        components = 2;
        break;
    case GL_RED_INTEGER:
        components = 1;
        break;
    case GL_RGB_INTEGER:
        components = 3;
        break;
    case GL_RGBA_INTEGER:
        components = 4;
        break;
    case GL_BGR_INTEGER:
        components = 3;
        break;
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
static bool rt_color_transfer_format(rt_internal_format_t internal_format, GLenum& format, GLenum& type)
{
    switch (internal_format)
    {
    case RT_R8:            format = rt_to_gl_format(RT_RED);  type = rt_to_gl_type(RT_UNSIGNED_BYTE); return true;
    case RT_R16:           format = rt_to_gl_format(RT_RED);  type = rt_to_gl_type(RT_UNSIGNED_SHORT); return true;
    case RT_RG8:           format = rt_to_gl_format(RT_RG);   type = rt_to_gl_type(RT_UNSIGNED_BYTE); return true;
    case RT_RG16:          format = rt_to_gl_format(RT_RG);   type = rt_to_gl_type(RT_UNSIGNED_SHORT); return true;
    case RT_R16F:          format = rt_to_gl_format(RT_RED);  type = rt_to_gl_type(RT_HALF_FLOAT); return true;
    case RT_R32F:          format = rt_to_gl_format(RT_RED);  type = rt_to_gl_type(RT_FLOAT); return true;
    case RT_RG16F:         format = rt_to_gl_format(RT_RG);   type = rt_to_gl_type(RT_HALF_FLOAT); return true;
    case RT_RG32F:         format = rt_to_gl_format(RT_RG);   type = rt_to_gl_type(RT_FLOAT); return true;
    case RT_RGB8:          format = rt_to_gl_format(RT_RGB);  type = rt_to_gl_type(RT_UNSIGNED_BYTE); return true;
    case RT_RGB16:         format = rt_to_gl_format(RT_RGB);  type = rt_to_gl_type(RT_UNSIGNED_SHORT); return true;
    case RT_RGBA8:         format = rt_to_gl_format(RT_RGBA); type = rt_to_gl_type(RT_UNSIGNED_BYTE); return true;
    case RT_RGBA16:        format = rt_to_gl_format(RT_RGBA); type = rt_to_gl_type(RT_UNSIGNED_SHORT); return true;
    case RT_SRGB8_ALPHA8:  format = rt_to_gl_format(RT_RGBA); type = rt_to_gl_type(RT_UNSIGNED_BYTE); return true;
    case RT_RGB16F:        format = rt_to_gl_format(RT_RGB);  type = rt_to_gl_type(RT_HALF_FLOAT); return true;
    case RT_RGBA16F:       format = rt_to_gl_format(RT_RGBA); type = rt_to_gl_type(RT_HALF_FLOAT); return true;
    case RT_RGB32F:        format = rt_to_gl_format(RT_RGB);  type = rt_to_gl_type(RT_FLOAT); return true;
    case RT_RGBA32F:       format = rt_to_gl_format(RT_RGBA); type = rt_to_gl_type(RT_FLOAT); return true;
    case GL_RGBA4:         format = rt_to_gl_format(RT_RGBA); type = GL_UNSIGNED_SHORT_4_4_4_4; return true;
    case GL_RGB5_A1:       format = rt_to_gl_format(RT_RGBA); type = GL_UNSIGNED_SHORT_5_5_5_1; return true;
    case GL_RGB10_A2:      format = rt_to_gl_format(RT_RGBA); type = GL_UNSIGNED_INT_2_10_10_10_REV; return true;
    case GL_R8I:           format = GL_RED_INTEGER;  type = rt_to_gl_type(RT_BYTE); return true;
    case GL_R8UI:          format = GL_RED_INTEGER;  type = rt_to_gl_type(RT_UNSIGNED_BYTE); return true;
    case GL_R16I:          format = GL_RED_INTEGER;  type = rt_to_gl_type(RT_SHORT); return true;
    case GL_R16UI:         format = GL_RED_INTEGER;  type = rt_to_gl_type(RT_UNSIGNED_SHORT); return true;
    case GL_R32I:          format = GL_RED_INTEGER;  type = rt_to_gl_type(RT_INT); return true;
    case GL_R32UI:         format = GL_RED_INTEGER;  type = rt_to_gl_type(RT_UNSIGNED_INT); return true;
    case GL_RG8I:          format = GL_RG_INTEGER;   type = rt_to_gl_type(RT_BYTE); return true;
    case GL_RG8UI:         format = GL_RG_INTEGER;   type = rt_to_gl_type(RT_UNSIGNED_BYTE); return true;
    case GL_RG16I:         format = GL_RG_INTEGER;   type = rt_to_gl_type(RT_SHORT); return true;
    case GL_RG16UI:        format = GL_RG_INTEGER;   type = rt_to_gl_type(RT_UNSIGNED_SHORT); return true;
    case GL_RG32I:         format = GL_RG_INTEGER;   type = rt_to_gl_type(RT_INT); return true;
    case GL_RG32UI:        format = GL_RG_INTEGER;   type = rt_to_gl_type(RT_UNSIGNED_INT); return true;
    case GL_R11F_G11F_B10F: format = rt_to_gl_format(RT_RGB); type = GL_UNSIGNED_INT_10F_11F_11F_REV; return true;
    case GL_RGB9_E5:       format = rt_to_gl_format(RT_RGB);  type = GL_UNSIGNED_INT_5_9_9_9_REV; return true;
    case GL_SRGB8:         format = rt_to_gl_format(RT_RGB);  type = rt_to_gl_type(RT_UNSIGNED_BYTE); return true;
    case GL_RGB565:        format = rt_to_gl_format(RT_RGB);  type = GL_UNSIGNED_SHORT_5_6_5; return true;
    case GL_RGBA32UI:      format = GL_RGBA_INTEGER; type = rt_to_gl_type(RT_UNSIGNED_INT); return true;
    case GL_RGB32UI:       format = GL_RGB_INTEGER;  type = rt_to_gl_type(RT_UNSIGNED_INT); return true;
    case GL_RGBA16UI:      format = GL_RGBA_INTEGER; type = rt_to_gl_type(RT_UNSIGNED_SHORT); return true;
    case GL_RGB16UI:       format = GL_RGB_INTEGER;  type = rt_to_gl_type(RT_UNSIGNED_SHORT); return true;
    case GL_RGBA8UI:       format = GL_RGBA_INTEGER; type = rt_to_gl_type(RT_UNSIGNED_BYTE); return true;
    case GL_RGB8UI:        format = GL_RGB_INTEGER;  type = rt_to_gl_type(RT_UNSIGNED_BYTE); return true;
    case GL_RGBA32I:       format = GL_RGBA_INTEGER; type = rt_to_gl_type(RT_INT); return true;
    case GL_RGB32I:        format = GL_RGB_INTEGER;  type = rt_to_gl_type(RT_INT); return true;
    case GL_RGBA16I:       format = GL_RGBA_INTEGER; type = rt_to_gl_type(RT_SHORT); return true;
    case GL_RGB16I:        format = GL_RGB_INTEGER;  type = rt_to_gl_type(RT_SHORT); return true;
    case GL_RGBA8I:        format = GL_RGBA_INTEGER; type = rt_to_gl_type(RT_BYTE); return true;
    case GL_RGB8I:         format = GL_RGB_INTEGER;  type = rt_to_gl_type(RT_BYTE); return true;
    case GL_R8_SNORM:      format = rt_to_gl_format(RT_RED);  type = rt_to_gl_type(RT_BYTE); return true;
    case GL_RG8_SNORM:     format = rt_to_gl_format(RT_RG);   type = rt_to_gl_type(RT_BYTE); return true;
    case GL_RGB8_SNORM:    format = rt_to_gl_format(RT_RGB);  type = rt_to_gl_type(RT_BYTE); return true;
    case GL_RGBA8_SNORM:   format = rt_to_gl_format(RT_RGBA); type = rt_to_gl_type(RT_BYTE); return true;
    case GL_R16_SNORM:     format = rt_to_gl_format(RT_RED);  type = rt_to_gl_type(RT_SHORT); return true;
    case GL_RG16_SNORM:    format = rt_to_gl_format(RT_RG);   type = rt_to_gl_type(RT_SHORT); return true;
    case GL_RGB16_SNORM:   format = rt_to_gl_format(RT_RGB);  type = rt_to_gl_type(RT_SHORT); return true;
    case GL_RGBA16_SNORM:  format = rt_to_gl_format(RT_RGBA); type = rt_to_gl_type(RT_SHORT); return true;
    case GL_RGB10_A2UI:    format = GL_RGBA_INTEGER; type = GL_UNSIGNED_INT_2_10_10_10_REV; return true;
    default:
        return false;
    }
}

// 根据纹理与 aspect 决定传输使用的像素格式和类型
// download 为 true 时允许从深度模板纹理中单独读取深度或模板
static void gl_transfer_format(rt_texture_t const& texture, rt_format_t aspect, bool download, GLenum& format, GLenum& type)
{
    switch (texture.format)
    {
    case RT_STENCIL_INDEX:
        if (aspect == RT_DEPTH_COMPONENT && download)
        {
            fprintf(stderr, "Texture has no depth aspect\n");
            abort();
        }
        format = rt_to_gl_format(RT_STENCIL_INDEX);
        type = rt_to_gl_type(RT_UNSIGNED_BYTE);
        return;
    case RT_DEPTH_COMPONENT:
        if (aspect == RT_STENCIL_INDEX)
        {
            fprintf(stderr, "Texture has no stencil aspect\n");
            abort();
        }
        format = rt_to_gl_format(RT_DEPTH_COMPONENT);
        type = rt_to_gl_type(texture.type);
        return;
    case RT_RED:
    case RT_RGB:
    case RT_RGBA:
    case RT_RG:
        if (!rt_color_transfer_format(texture.internal_format, format, type))
        {
            format = rt_to_gl_format(texture.format);
            type = rt_to_gl_type(texture.type);
        }
        return;
    case RT_DEPTH_STENCIL:
        if (download && aspect == RT_STENCIL_INDEX)
        {
            format = rt_to_gl_format(RT_STENCIL_INDEX);
            type = rt_to_gl_type(RT_UNSIGNED_BYTE);
        }
        else if (download && aspect == RT_DEPTH_COMPONENT)
        {
            format = rt_to_gl_format(RT_DEPTH_COMPONENT);
            type = rt_to_gl_type(RT_FLOAT);
        }
        else
        {
            format = rt_to_gl_format(RT_DEPTH_STENCIL);
            type = rt_to_gl_type(texture.type);
        }
        return;
    default:
        if (!rt_color_transfer_format(texture.internal_format, format, type))
        {
            format = rt_to_gl_format(texture.format);
            type = rt_to_gl_type(texture.type);
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
    // 1D / 2D 没有深度维度，z 偏移必须为 0 且最多拷贝一层
    if ((region.texture.target == RT_TEXTURE_1D || region.texture.target == RT_TEXTURE_2D) &&
        (region.origin.z != 0 || copySize.z > 1))
        return false;
    if (region.texture.target == RT_TEXTURE_1D && (region.origin.y != 0 || copySize.y > 1))
        return false;
    if (region.texture.target == RT_TEXTURE_2D_ARRAY)
    {
        if ((uint64_t)region.origin.z + std::max(1U, copySize.z) > std::max(1U, region.texture.depth))
            return false;
    }
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
    glCopyImageSubData(source.texture.handle, rt_to_gl_texture_target(source.texture.target), (GLint)source.mipLevel,
                       (GLint)source.origin.x, (GLint)source.origin.y, (GLint)source.origin.z,
                       destination.texture.handle, rt_to_gl_texture_target(destination.texture.target), (GLint)destination.mipLevel,
                       (GLint)destination.origin.x, (GLint)destination.origin.y, (GLint)destination.origin.z,
                       (GLsizei)copySize.x, (GLsizei)copySize.y, depth);

    if (destination.texture.mipmaps > 1) glGenerateTextureMipmap(destination.texture.handle);
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
    if (destination.texture.target == RT_TEXTURE_1D)
    {
        glTextureSubImage1D(destination.texture.handle, (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLsizei)copySize.x, format, type, data);

        if (destination.texture.mipmaps > 1) glGenerateTextureMipmap(destination.texture.handle);
    }
    else if (destination.texture.target == RT_TEXTURE_2D)
    {
        glTextureSubImage2D(destination.texture.handle, (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLint)destination.origin.y,
                            (GLsizei)copySize.x, (GLsizei)copySize.y, format, type, data);

        if (destination.texture.mipmaps > 1) glGenerateTextureMipmap(destination.texture.handle);
    }
    else if (destination.texture.target == RT_TEXTURE_3D || destination.texture.target == RT_TEXTURE_2D_ARRAY)
    {
        glTextureSubImage3D(destination.texture.handle, (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLint)destination.origin.y, (GLint)destination.origin.z,
                            (GLsizei)copySize.x, (GLsizei)copySize.y, (GLsizei)std::max(1U, copySize.z), format, type, data);

        if (destination.texture.mipmaps > 1) glGenerateTextureMipmap(destination.texture.handle);
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
    if (destination.texture.target == RT_TEXTURE_1D)
    {
        glTextureSubImage1D(destination.texture.handle, (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLsizei)copySize.x, format, type, data);

        if (destination.texture.mipmaps > 1) glGenerateTextureMipmap(destination.texture.handle);
    }
    else if (destination.texture.target == RT_TEXTURE_2D)
    {
        glTextureSubImage2D(destination.texture.handle, (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLint)destination.origin.y,
                            (GLsizei)copySize.x, (GLsizei)copySize.y, format, type, data);

        if (destination.texture.mipmaps > 1) glGenerateTextureMipmap(destination.texture.handle);
    }
    else if (destination.texture.target == RT_TEXTURE_3D || destination.texture.target == RT_TEXTURE_2D_ARRAY)
    {
        glTextureSubImage3D(destination.texture.handle, (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLint)destination.origin.y, (GLint)destination.origin.z,
                            (GLsizei)copySize.x, (GLsizei)copySize.y, (GLsizei)std::max(1U, copySize.z), format, type, data);

        if (destination.texture.mipmaps > 1) glGenerateTextureMipmap(destination.texture.handle);
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

static GLsizei rt_type_size(rt_type_t type)
{
    switch (type)
    {
    case RT_TYPE_NONE:
        return 4;
    case RT_BYTE:
    case RT_UNSIGNED_BYTE:
        return 1;
    case RT_SHORT:
    case RT_UNSIGNED_SHORT:
        return 2;
    case RT_INT:
    case RT_UNSIGNED_INT:
    case RT_FLOAT:
        return 4;
    case RT_DOUBLE:
        return 8;
    case RT_HALF_FLOAT:
        return 2;
    case RT_UNSIGNED_INT_24_8:
    default:
        return 4;
    }
}

static GLsizei rt_index_size(rt_type_t type)
{
    switch (type)
    {
    case RT_UNSIGNED_SHORT:
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
        result.vertex[0] = gl_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = vertices,});

    if (normals)
        result.vertex[1] = gl_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = normals,});

    if (uvs)
        result.vertex[2] = gl_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = uvs,});

    if (indices)
        result.index = gl_create_buffer({.size = index_count * sizeof(uint32_t), .usage = RT_BUFFER_USAGE_INDEX | RT_BUFFER_USAGE_COPY_DST, .data = indices,});

    std::iota(result.location, result.location + std::size(result.location), 0);
    return result;
}

void gl_destroy_mesh(rt_mesh_t& mesh)
{
    for (auto& vertex : mesh.vertex)
        gl_destroy_buffer(vertex);
    gl_destroy_buffer(mesh.index);
}

void gl_draw_mesh(rt_mesh_t& mesh)
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
        if (layout.type == RT_TYPE_NONE || layout.count == 0)
            continue;

        GLuint buffer = 0;
        GLsizei stride = rt_type_size(layout.type) * (GLsizei)layout.count;
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
        GLsizei index_stride = rt_index_size(module.index_type);
        auto index_count = (GLsizei)(mesh.index.size / (size_t)index_stride);
        glDrawElements(rt_to_gl_primitive(module.primitive), index_count, rt_to_gl_type(module.index_type), (void*)0);
    }
    else
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDrawArrays(rt_to_gl_primitive(module.primitive), 0, vertex_count);
    }
}

void gl_draw_mesh_multi(rt_mesh_t& mesh, uint32_t count)
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
        if (layout.type == RT_TYPE_NONE || layout.count == 0)
            continue;

        GLuint buffer = 0;
        GLsizei stride = rt_type_size(layout.type) * (GLsizei)layout.count;
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
        GLsizei index_stride = rt_index_size(module.index_type);
        auto index_count = (GLsizei)(mesh.index.size / (size_t)index_stride);
        glDrawElementsInstanced(rt_to_gl_primitive(module.primitive), index_count, rt_to_gl_type(module.index_type), (void*)0, (int32_t)count);
    }
    else
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDrawArraysInstanced(rt_to_gl_primitive(module.primitive), 0, vertex_count, (int32_t)count);
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
        result.vertex[0] = gl_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = vertices,});

    if (normals)
        result.vertex[1] = gl_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = normals,});

    if (uvs)
        result.vertex[2] = gl_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = uvs,});

    if (indices)
        result.index = gl_create_buffer({.size = index_count * sizeof(uint32_t), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = indices,});

    std::iota(result.location, result.location + std::size(result.location), 0);
    return result;
}

void gl_destroy_meshlet(rt_meshlet_t& meshlet)
{
    for (auto& vertex : meshlet.vertex)
        gl_destroy_buffer(vertex);
    gl_destroy_buffer(meshlet.index);
}

void gl_draw_meshlet(rt_meshlet_t& meshlet)
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
        if (layout.type == RT_TYPE_NONE || layout.count == 0)
            continue;

        for (uint32_t k = 0; k < std::size(meshlet.vertex); ++k)
        {
            if (meshlet.vertex[k].handle == 0 || meshlet.location[k] != layout.location)
                continue;
            gl_bind_buffer(meshlet.vertex[k], {.binding = layout.location, .target = RT_SHADER_STORAGE_BUFFER,});
            break;
        }
        if (layout.location + 1 > index_binding)
            index_binding = layout.location + 1;
    }

    if (meshlet.index.handle)
    {
        gl_bind_buffer(meshlet.index, {.binding = index_binding, .target = RT_SHADER_STORAGE_BUFFER,});
        GLsizei index_stride = rt_index_size(module.index_type);
        auto index_count = (GLsizei)(meshlet.index.size / (size_t)index_stride);
        glDrawMeshTasksNV(0, index_count / 3);
    }
}

// ====================================================================

rt_mesh_t gl_create_mesh_screen()
{
    const float points[]
    {
        -1.0f, -1.0f, 0.0f,
        +3.0f, -1.0f, 0.0f,
        -1.0f, +3.0f, 0.0f,
    };
    const float uvs[]
    {
        0.0f, 0.0f,
        2.0f, 0.0f,
        0.0f, 2.0f,
    };
    return gl_create_mesh(points, nullptr, uvs, 3, nullptr, 0);
}

void gl_draw_screen(int width, int height, rt_color_t clear, rt_texture_t& texture)
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
    rt_pass_render_t pass = {.module = module, .screen = {.color = { .clear = true, .value = clear,}}};
    gl_begin_render(pass);
    gl_set_viewport(0, 0, width, height);
    gl_bind_texture(texture, { .binding = 0, });
    if (texture.handle)
    {
        static auto mesh = gl_create_mesh_screen();
        gl_draw_mesh(mesh);
    }
    gl_end_render(pass);
}

void gl_submit()
{
    glFlush();
}

#endif
