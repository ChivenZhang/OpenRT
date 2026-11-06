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
#include <map>
#include <numeric>
#include <vector>

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

static GLenum rt_to_gl_texture_format(rt_texture_format_t format)
{
    switch (format)
    {
        case RT_TEXTURE_NONE: return GL_RGBA8;
        case RT_TEXTURE_R8UNORM: return GL_R8;
        case RT_TEXTURE_R8SNORM: return GL_R8_SNORM;
        case RT_TEXTURE_R8UINT: return GL_R8UI;
        case RT_TEXTURE_R8SINT: return GL_R8I;
        case RT_TEXTURE_R16UNORM: return GL_R16;
        case RT_TEXTURE_R16SNORM: return GL_R16_SNORM;
        case RT_TEXTURE_R16UINT: return GL_R16UI;
        case RT_TEXTURE_R16SINT: return GL_R16I;
        case RT_TEXTURE_R16FLOAT: return GL_R16F;
        case RT_TEXTURE_RG8UNORM: return GL_RG8;
        case RT_TEXTURE_RG8SNORM: return GL_RG8_SNORM;
        case RT_TEXTURE_RG8UINT: return GL_RG8UI;
        case RT_TEXTURE_RG8SINT: return GL_RG8I;
        case RT_TEXTURE_R32UINT: return GL_R32UI;
        case RT_TEXTURE_R32SINT: return GL_R32I;
        case RT_TEXTURE_R32FLOAT: return GL_R32F;
        case RT_TEXTURE_RG16UNORM: return GL_RG16;
        case RT_TEXTURE_RG16SNORM: return GL_RG16_SNORM;
        case RT_TEXTURE_RG16UINT: return GL_RG16UI;
        case RT_TEXTURE_RG16SINT: return GL_RG16I;
        case RT_TEXTURE_RG16FLOAT: return GL_RG16F;
        case RT_TEXTURE_RGBA8UNORM: return GL_RGBA8;
        case RT_TEXTURE_RGBA8UNORM_SRGB: return GL_SRGB8_ALPHA8;
        case RT_TEXTURE_RGBA8SNORM: return GL_RGBA8_SNORM;
        case RT_TEXTURE_RGBA8UINT: return GL_RGBA8UI;
        case RT_TEXTURE_RGBA8SINT: return GL_RGBA8I;
        case RT_TEXTURE_BGRA8UNORM: return GL_RGBA8;
        case RT_TEXTURE_BGRA8UNORM_SRGB: return GL_SRGB8_ALPHA8;
        case RT_TEXTURE_RGB10A2UINT: return GL_RGB10_A2UI;
        case RT_TEXTURE_RGB10A2UNORM: return GL_RGB10_A2;
        case RT_TEXTURE_RG11B10UFLOAT: return GL_R11F_G11F_B10F;
        case RT_TEXTURE_RGB9E5UFLOAT: return GL_RGB9_E5;
        case RT_TEXTURE_RG32UINT: return GL_RG32UI;
        case RT_TEXTURE_RG32SINT: return GL_RG32I;
        case RT_TEXTURE_RG32FLOAT: return GL_RG32F;
        case RT_TEXTURE_RGBA16UNORM: return GL_RGBA16;
        case RT_TEXTURE_RGBA16SNORM: return GL_RGBA16_SNORM;
        case RT_TEXTURE_RGBA16UINT: return GL_RGBA16UI;
        case RT_TEXTURE_RGBA16SINT: return GL_RGBA16I;
        case RT_TEXTURE_RGBA16FLOAT: return GL_RGBA16F;
        case RT_TEXTURE_RGBA32UINT: return GL_RGBA32UI;
        case RT_TEXTURE_RGBA32SINT: return GL_RGBA32I;
        case RT_TEXTURE_RGBA32FLOAT: return GL_RGBA32F;
        case RT_TEXTURE_STENCIL8: return GL_STENCIL_INDEX8;
        case RT_TEXTURE_DEPTH16UNORM: return GL_DEPTH_COMPONENT16;
        case RT_TEXTURE_DEPTH24PLUS: return GL_DEPTH_COMPONENT24;
        case RT_TEXTURE_DEPTH24PLUS_STENCIL8: return GL_DEPTH24_STENCIL8;
        case RT_TEXTURE_DEPTH32FLOAT: return GL_DEPTH_COMPONENT32F;
        case RT_TEXTURE_DEPTH32FLOAT_STENCIL8: return GL_DEPTH32F_STENCIL8;
        default: return GL_RGBA8;
    }
}

static GLsizei rt_to_gl_sample_count(rt_texture_sample_t samples)
{
    switch (samples)
    {
        case RT_TEXTURE_SAMPLE_1X: return 1;
        case RT_TEXTURE_SAMPLE_4X: return 4;
        default: return 1;
    }
}

static void rt_to_gl_transfer(rt_texture_format_t texFormat, rt_texture_aspect_t aspect, bool download, GLenum& format, GLenum& type)
{
    switch (texFormat)
    {
        case RT_TEXTURE_NONE: format = GL_RGBA; type = GL_UNSIGNED_BYTE; return;
        case RT_TEXTURE_R8UNORM: format = GL_RED; type = GL_UNSIGNED_BYTE; return;
        case RT_TEXTURE_R8SNORM: format = GL_RED; type = GL_BYTE; return;
        case RT_TEXTURE_R8UINT: format = GL_RED_INTEGER; type = GL_UNSIGNED_BYTE; return;
        case RT_TEXTURE_R8SINT: format = GL_RED_INTEGER; type = GL_BYTE; return;
        case RT_TEXTURE_R16UNORM: format = GL_RED; type = GL_UNSIGNED_SHORT; return;
        case RT_TEXTURE_R16SNORM: format = GL_RED; type = GL_SHORT; return;
        case RT_TEXTURE_R16UINT: format = GL_RED_INTEGER; type = GL_UNSIGNED_SHORT; return;
        case RT_TEXTURE_R16SINT: format = GL_RED_INTEGER; type = GL_SHORT; return;
        case RT_TEXTURE_R16FLOAT: format = GL_RED; type = GL_HALF_FLOAT; return;
        case RT_TEXTURE_RG8UNORM: format = GL_RG; type = GL_UNSIGNED_BYTE; return;
        case RT_TEXTURE_RG8SNORM: format = GL_RG; type = GL_BYTE; return;
        case RT_TEXTURE_RG8UINT: format = GL_RG_INTEGER; type = GL_UNSIGNED_BYTE; return;
        case RT_TEXTURE_RG8SINT: format = GL_RG_INTEGER; type = GL_BYTE; return;
        case RT_TEXTURE_R32UINT: format = GL_RED_INTEGER; type = GL_UNSIGNED_INT; return;
        case RT_TEXTURE_R32SINT: format = GL_RED_INTEGER; type = GL_INT; return;
        case RT_TEXTURE_R32FLOAT: format = GL_RED; type = GL_FLOAT; return;
        case RT_TEXTURE_RG16UNORM: format = GL_RG; type = GL_UNSIGNED_SHORT; return;
        case RT_TEXTURE_RG16SNORM: format = GL_RG; type = GL_SHORT; return;
        case RT_TEXTURE_RG16UINT: format = GL_RG_INTEGER; type = GL_UNSIGNED_SHORT; return;
        case RT_TEXTURE_RG16SINT: format = GL_RG_INTEGER; type = GL_SHORT; return;
        case RT_TEXTURE_RG16FLOAT: format = GL_RG; type = GL_HALF_FLOAT; return;
        case RT_TEXTURE_RGBA8UNORM:
        case RT_TEXTURE_RGBA8UNORM_SRGB: format = GL_RGBA; type = GL_UNSIGNED_BYTE; return;
        case RT_TEXTURE_RGBA8SNORM: format = GL_RGBA; type = GL_BYTE; return;
        case RT_TEXTURE_RGBA8UINT: format = GL_RGBA_INTEGER; type = GL_UNSIGNED_BYTE; return;
        case RT_TEXTURE_RGBA8SINT: format = GL_RGBA_INTEGER; type = GL_BYTE; return;
        case RT_TEXTURE_BGRA8UNORM:
        case RT_TEXTURE_BGRA8UNORM_SRGB: format = GL_BGRA; type = GL_UNSIGNED_BYTE; return;
        case RT_TEXTURE_RGB10A2UINT: format = GL_RGBA_INTEGER; type = GL_UNSIGNED_INT_2_10_10_10_REV; return;
        case RT_TEXTURE_RGB10A2UNORM: format = GL_RGBA; type = GL_UNSIGNED_INT_2_10_10_10_REV; return;
        case RT_TEXTURE_RG11B10UFLOAT: format = GL_RGB; type = GL_UNSIGNED_INT_10F_11F_11F_REV; return;
        case RT_TEXTURE_RGB9E5UFLOAT: format = GL_RGB; type = GL_UNSIGNED_INT_5_9_9_9_REV; return;
        case RT_TEXTURE_RG32UINT: format = GL_RG_INTEGER; type = GL_UNSIGNED_INT; return;
        case RT_TEXTURE_RG32SINT: format = GL_RG_INTEGER; type = GL_INT; return;
        case RT_TEXTURE_RG32FLOAT: format = GL_RG; type = GL_FLOAT; return;
        case RT_TEXTURE_RGBA16UNORM: format = GL_RGBA; type = GL_UNSIGNED_SHORT; return;
        case RT_TEXTURE_RGBA16SNORM: format = GL_RGBA; type = GL_SHORT; return;
        case RT_TEXTURE_RGBA16UINT: format = GL_RGBA_INTEGER; type = GL_UNSIGNED_SHORT; return;
        case RT_TEXTURE_RGBA16SINT: format = GL_RGBA_INTEGER; type = GL_SHORT; return;
        case RT_TEXTURE_RGBA16FLOAT: format = GL_RGBA; type = GL_HALF_FLOAT; return;
        case RT_TEXTURE_RGBA32UINT: format = GL_RGBA_INTEGER; type = GL_UNSIGNED_INT; return;
        case RT_TEXTURE_RGBA32SINT: format = GL_RGBA_INTEGER; type = GL_INT; return;
        case RT_TEXTURE_RGBA32FLOAT: format = GL_RGBA; type = GL_FLOAT; return;
        case RT_TEXTURE_STENCIL8: format = GL_STENCIL_INDEX; type = GL_UNSIGNED_BYTE; return;
        case RT_TEXTURE_DEPTH16UNORM: format = GL_DEPTH_COMPONENT; type = GL_UNSIGNED_SHORT; return;
        case RT_TEXTURE_DEPTH24PLUS: format = GL_DEPTH_COMPONENT; type = GL_UNSIGNED_INT; return;
        case RT_TEXTURE_DEPTH24PLUS_STENCIL8:
            switch (aspect)
            {
                case RT_TEXTURE_ASPECT_ALL:
                    format = GL_DEPTH_STENCIL; type = GL_UNSIGNED_INT_24_8; return;
                case RT_TEXTURE_ASPECT_STENCIL:
                    format = GL_STENCIL_INDEX; type = GL_UNSIGNED_BYTE; return;
                case RT_TEXTURE_ASPECT_DEPTH:
                    format = download ? GL_DEPTH_COMPONENT : GL_DEPTH_STENCIL;
                    type = download ? GL_UNSIGNED_INT : GL_UNSIGNED_INT_24_8;
                    return;
                default:
                    format = GL_DEPTH_STENCIL; type = GL_UNSIGNED_INT_24_8; return;
            }
        case RT_TEXTURE_DEPTH32FLOAT: format = GL_DEPTH_COMPONENT; type = GL_FLOAT; return;
        case RT_TEXTURE_DEPTH32FLOAT_STENCIL8:
            switch (aspect)
            {
                case RT_TEXTURE_ASPECT_ALL:
                    format = GL_DEPTH_STENCIL; type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV; return;
                case RT_TEXTURE_ASPECT_STENCIL:
                    format = GL_STENCIL_INDEX; type = GL_UNSIGNED_BYTE; return;
                case RT_TEXTURE_ASPECT_DEPTH:
                    format = download ? GL_DEPTH_COMPONENT : GL_DEPTH_STENCIL;
                    type = download ? GL_FLOAT : GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
                    return;
                default:
                    format = GL_DEPTH_STENCIL; type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV; return;
            }
        default:
            format = GL_RGBA; type = GL_UNSIGNED_BYTE; return;
    }
}

static void rt_to_gl_vertex_format(GLuint index, rt_vertex_format_t format, GLuint offset)
{
    switch (format)
    {
        case RT_VERTEX_NONE:
            break;
        case RT_VERTEX_UINT8:
            glVertexAttribIFormat(index, 1, GL_UNSIGNED_BYTE, offset);
            break;
        case RT_VERTEX_UINT8X2:
            glVertexAttribIFormat(index, 2, GL_UNSIGNED_BYTE, offset);
            break;
        case RT_VERTEX_UINT8X4:
            glVertexAttribIFormat(index, 4, GL_UNSIGNED_BYTE, offset);
            break;
        case RT_VERTEX_SINT8:
            glVertexAttribIFormat(index, 1, GL_BYTE, offset);
            break;
        case RT_VERTEX_SINT8X2:
            glVertexAttribIFormat(index, 2, GL_BYTE, offset);
            break;
        case RT_VERTEX_SINT8X4:
            glVertexAttribIFormat(index, 4, GL_BYTE, offset);
            break;
        case RT_VERTEX_UNORM8:
            glVertexAttribFormat(index, 1, GL_UNSIGNED_BYTE, GL_TRUE, offset);
            break;
        case RT_VERTEX_UNORM8X2:
            glVertexAttribFormat(index, 2, GL_UNSIGNED_BYTE, GL_TRUE, offset);
            break;
        case RT_VERTEX_UNORM8X4:
            glVertexAttribFormat(index, 4, GL_UNSIGNED_BYTE, GL_TRUE, offset);
            break;
        case RT_VERTEX_SNORM8:
            glVertexAttribFormat(index, 1, GL_BYTE, GL_TRUE, offset);
            break;
        case RT_VERTEX_SNORM8X2:
            glVertexAttribFormat(index, 2, GL_BYTE, GL_TRUE, offset);
            break;
        case RT_VERTEX_SNORM8X4:
            glVertexAttribFormat(index, 4, GL_BYTE, GL_TRUE, offset);
            break;
        case RT_VERTEX_UINT16:
            glVertexAttribIFormat(index, 1, GL_UNSIGNED_SHORT, offset);
            break;
        case RT_VERTEX_UINT16X2:
            glVertexAttribIFormat(index, 2, GL_UNSIGNED_SHORT, offset);
            break;
        case RT_VERTEX_UINT16X4:
            glVertexAttribIFormat(index, 4, GL_UNSIGNED_SHORT, offset);
            break;
        case RT_VERTEX_SINT16:
            glVertexAttribIFormat(index, 1, GL_SHORT, offset);
            break;
        case RT_VERTEX_SINT16X2:
            glVertexAttribIFormat(index, 2, GL_SHORT, offset);
            break;
        case RT_VERTEX_SINT16X4:
            glVertexAttribIFormat(index, 4, GL_SHORT, offset);
            break;
        case RT_VERTEX_UNORM16:
            glVertexAttribFormat(index, 1, GL_UNSIGNED_SHORT, GL_TRUE, offset);
            break;
        case RT_VERTEX_UNORM16X2:
            glVertexAttribFormat(index, 2, GL_UNSIGNED_SHORT, GL_TRUE, offset);
            break;
        case RT_VERTEX_UNORM16X4:
            glVertexAttribFormat(index, 4, GL_UNSIGNED_SHORT, GL_TRUE, offset);
            break;
        case RT_VERTEX_SNORM16:
            glVertexAttribFormat(index, 1, GL_SHORT, GL_TRUE, offset);
            break;
        case RT_VERTEX_SNORM16X2:
            glVertexAttribFormat(index, 2, GL_SHORT, GL_TRUE, offset);
            break;
        case RT_VERTEX_SNORM16X4:
            glVertexAttribFormat(index, 4, GL_SHORT, GL_TRUE, offset);
            break;
        case RT_VERTEX_FLOAT16:
            glVertexAttribFormat(index, 1, GL_HALF_FLOAT, GL_FALSE, offset);
            break;
        case RT_VERTEX_FLOAT16X2:
            glVertexAttribFormat(index, 2, GL_HALF_FLOAT, GL_FALSE, offset);
            break;
        case RT_VERTEX_FLOAT16X4:
            glVertexAttribFormat(index, 4, GL_HALF_FLOAT, GL_FALSE, offset);
            break;
        case RT_VERTEX_FLOAT32:
            glVertexAttribFormat(index, 1, GL_FLOAT, GL_FALSE, offset);
            break;
        case RT_VERTEX_FLOAT32X2:
            glVertexAttribFormat(index, 2, GL_FLOAT, GL_FALSE, offset);
            break;
        case RT_VERTEX_FLOAT32X3:
            glVertexAttribFormat(index, 3, GL_FLOAT, GL_FALSE, offset);
            break;
        case RT_VERTEX_FLOAT32X4:
            glVertexAttribFormat(index, 4, GL_FLOAT, GL_FALSE, offset);
            break;
        case RT_VERTEX_UINT32:
            glVertexAttribIFormat(index, 1, GL_UNSIGNED_INT, offset);
            break;
        case RT_VERTEX_UINT32X2:
            glVertexAttribIFormat(index, 2, GL_UNSIGNED_INT, offset);
            break;
        case RT_VERTEX_UINT32X3:
            glVertexAttribIFormat(index, 3, GL_UNSIGNED_INT, offset);
            break;
        case RT_VERTEX_UINT32X4:
            glVertexAttribIFormat(index, 4, GL_UNSIGNED_INT, offset);
            break;
        case RT_VERTEX_SINT32:
            glVertexAttribIFormat(index, 1, GL_INT, offset);
            break;
        case RT_VERTEX_SINT32X2:
            glVertexAttribIFormat(index, 2, GL_INT, offset);
            break;
        case RT_VERTEX_SINT32X3:
            glVertexAttribIFormat(index, 3, GL_INT, offset);
            break;
        case RT_VERTEX_SINT32X4:
            glVertexAttribIFormat(index, 4, GL_INT, offset);
            break;
        default:
            break;
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

static GLenum rt_to_gl_address(rt_address_t address)
{
    switch (address)
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

static GLenum rt_to_gl_wind_mode(rt_wind_mode_t face)
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
        case RT_LINE_STRIP: return GL_LINE_STRIP;
        case RT_TRIANGLES: return GL_TRIANGLES;
        case RT_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        default: return GL_TRIANGLES;
    }
}

struct gl_buffer_native_t
{
    GLuint handle = 0;
};

struct gl_texture_native_t
{
    GLuint handle = 0;
    uint32_t width = 1, height = 1, depth = 1;
    rt_texture_target_t target = RT_TEXTURE_2D;
    rt_texture_format_t format = RT_TEXTURE_NONE;
    rt_texture_usages_t usages = 0;
    uint32_t levels = 1;
    rt_texture_sample_t samples = RT_TEXTURE_SAMPLE_1X;
};

struct gl_texture_view_native_t
{
    GLuint handle = 0;
    GLuint64 bindlessHandle = 0;
    gl_texture_native_t* texture = nullptr;
};

struct gl_sampler_native_t
{
    GLuint handle = 0;
};

struct gl_layout_native_t
{
    uint32_t count = 0;
};

struct gl_binding_native_t
{
    rt_layout_entry_t entry[RT_MAX_BINDING_HANDLE_NUM] = {};
};

struct gl_module_native_t
{
    GLuint program = 0;
    GLuint vao = 0;
    rt_module_layout_t layouts[RT_MAX_BINDING_HANDLE_NUM] = {};
};

struct gl_mesh_native_t
{
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

struct gl_meshlet_native_t
{
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

struct gl_pass_compute_native_t { uint32_t dummy = 0; };
struct gl_pass_transfer_native_t { uint32_t dummy = 0; };

struct OpenGL
{
    uint32_t bufferID = 0;
    uint32_t textureID = 0;
    uint32_t textureViewID = 0;
    uint32_t samplerID = 0;
    uint32_t layoutID = 0;
    uint32_t bindingID = 0;
    uint32_t moduleID = 0;
    uint32_t meshID = 0;
    uint32_t meshletID = 0;
    uint32_t passID = 0;
    rt_limits_t limits;

    std::map<uint32_t, gl_buffer_native_t> buffers;
    std::map<uint32_t, gl_texture_native_t> textures;
    std::map<uint32_t, gl_texture_view_native_t> textureViews;
    std::map<uint32_t, gl_sampler_native_t> samplers;
    std::map<uint32_t, gl_layout_native_t> layouts;
    std::map<uint32_t, gl_binding_native_t> bindings;
    std::map<uint32_t, gl_module_native_t> modules;
    std::map<uint32_t, gl_mesh_native_t> meshes;
    std::map<uint32_t, gl_meshlet_native_t> meshlets;
    std::map<uint32_t, gl_pass_compute_native_t> computePasses;
    std::map<uint32_t, gl_pass_transfer_native_t> transferPasses;
    GLuint fbo = 0;

    rt_module_type_t currentPassType = RT_MODULE_NONE;
    union
    {
        void* currentPipeline = nullptr;
        rt_pass_render_t* currentRenderPass;
        rt_pass_compute_t* currentComputePass;
        rt_pass_transfer_t* currentTransferPass;
    };
    rt_module_render_t* currentRenderModule = nullptr;
    rt_module_compute_t* currentComputeModule = nullptr;
    int32_t stencilRefer = 0;
} static thread_local opengl;

static gl_buffer_native_t* gl_buffer_native(rt_buffer_t const& buffer)
{
    if (!buffer.native || buffer.handle == 0) return nullptr;
    return (gl_buffer_native_t*)buffer.native;
}

static gl_sampler_native_t* gl_sampler_native(rt_sampler_t const& sampler)
{
    if (!sampler.native || sampler.handle == 0) return nullptr;
    return (gl_sampler_native_t*)sampler.native;
}

static gl_texture_native_t* gl_texture_native(rt_texture_t const& texture)
{
    if (!texture.native || texture.handle == 0) return nullptr;
    return (gl_texture_native_t*)texture.native;
}

static gl_texture_native_t* gl_texture_native(rt_texture_view_t const& view)
{
    if (!view.native || view.handle == 0) return nullptr;
    return ((gl_texture_view_native_t*)view.native)->texture;
}

static gl_texture_view_native_t* gl_texture_view_native(rt_texture_view_t const& view)
{
    if (!view.native || view.handle == 0) return nullptr;
    return (gl_texture_view_native_t*)view.native;
}

static gl_module_native_t* gl_module_native(uint32_t handle, void* native)
{
    if (!native || handle == 0) return nullptr;
    return (gl_module_native_t*)native;
}

static rt_module_render_t const& gl_current_render_module()
{
    if (opengl.currentRenderModule == nullptr)
    {
        fprintf(stderr, "Pipeline module not bound");
        abort();
    }
    return *opengl.currentRenderModule;
}



static GLuint gl_buffer_name(rt_buffer_t const& buffer)
{
    auto* native = gl_buffer_native(buffer);
    return native ? native->handle : 0;
}

static GLuint gl_texture_name(rt_texture_t const& texture)
{
    auto* native = gl_texture_native(texture);
    return native ? native->handle : 0;
}

static GLuint gl_texture_view_name(rt_texture_view_t const& view)
{
    auto* native = gl_texture_view_native(view);
    return native ? native->handle : 0;
}

static GLuint gl_sampler_name(rt_sampler_t const& sampler)
{
    auto* native = gl_sampler_native(sampler);
    return native ? native->handle : 0;
}

static void gl_release_bindless(gl_texture_view_native_t& native);

static uint32_t gl_limit(GLenum name);
static uint32_t gl_limit3(GLenum a, GLenum b, GLenum c);

void gl_load_library()
{
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        fprintf(stderr, "GLEW init failed: %s", (const char*)glewGetErrorString(err));
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
    rt_get_limits = gl_get_limits;
    rt_get_backend = gl_get_backend;
    rt_create_buffer = gl_create_buffer;
    rt_destroy_buffer = gl_destroy_buffer;
    rt_map_buffer = gl_map_buffer;
    rt_unmap_buffer = gl_unmap_buffer;
    rt_create_texture = gl_create_texture;
    rt_destroy_texture = gl_destroy_texture;
    rt_create_texture_view = gl_create_texture_view;
    rt_destroy_texture_view = gl_destroy_texture_view;
    rt_bind_texture_view = gl_bind_texture_view;
    rt_create_sampler = gl_create_sampler;
    rt_destroy_sampler = gl_destroy_sampler;
    rt_create_layout = gl_create_layout;
    rt_destroy_layout = gl_destroy_layout;
    rt_create_binding = gl_create_binding;
    rt_destroy_binding = gl_destroy_binding;
    rt_bind_binding = gl_bind_binding;
    rt_create_module_compute = gl_create_module_compute;
    rt_create_module_render = gl_create_module_render;
    rt_destroy_module_render = gl_destroy_module_render;
    rt_destroy_module_compute = gl_destroy_module_compute;
    rt_begin_compute = gl_begin_compute;
    rt_end_compute = gl_end_compute;
    rt_bind_module_compute = gl_bind_module_compute;
    rt_dispatch_compute = gl_dispatch_compute;
    rt_dispatch_compute_indirect = gl_dispatch_compute_indirect;
    rt_begin_render = gl_begin_render;
    rt_end_render = gl_end_render;
    rt_bind_module_render = gl_bind_module_render;
    rt_set_viewport = gl_set_viewport;
    rt_set_scissor = gl_set_scissor;
    rt_set_blend_constant = gl_set_blend_constant;
    rt_set_stencil_reference = gl_set_stencil_reference;
    rt_draw_array = gl_draw_array;
    rt_draw_index = gl_draw_index;
    rt_draw_array_indirect = gl_draw_array_indirect;
    rt_draw_index_indirect = gl_draw_index_indirect;
    rt_draw_mesh_task = gl_draw_mesh_task;
    rt_draw_mesh_task_indirect = gl_draw_mesh_task_indirect;
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
    rt_submit = gl_submit;

    rt_limits_t limits = {};
    limits.max_texture_dimension_1d = gl_limit(GL_MAX_TEXTURE_SIZE);
    limits.max_texture_dimension_2d = limits.max_texture_dimension_1d;
    limits.max_texture_dimension_3d = gl_limit(GL_MAX_3D_TEXTURE_SIZE);
    limits.max_texture_array_layers = gl_limit(GL_MAX_ARRAY_TEXTURE_LAYERS);
    limits.max_texture_samples = gl_limit(GL_MAX_SAMPLES);
    limits.max_uniform_buffer_binding_size = gl_limit(GL_MAX_UNIFORM_BLOCK_SIZE);
    limits.max_storage_buffer_binding_size = gl_limit(GL_MAX_SHADER_STORAGE_BLOCK_SIZE);
    limits.max_buffer_size = limits.max_storage_buffer_binding_size;
    limits.min_uniform_buffer_offset_alignment = gl_limit(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    limits.min_storage_buffer_offset_alignment = gl_limit(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT);
    limits.max_uniform_buffers_per_shader_stage = gl_limit3(GL_MAX_VERTEX_UNIFORM_BLOCKS, GL_MAX_FRAGMENT_UNIFORM_BLOCKS, GL_MAX_COMPUTE_UNIFORM_BLOCKS);
    limits.max_storage_buffers_per_shader_stage = gl_limit3(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS);
    limits.max_sampled_textures_per_shader_stage = gl_limit3(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, GL_MAX_TEXTURE_IMAGE_UNITS, GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS);
    limits.max_storage_textures_per_shader_stage = gl_limit3(GL_MAX_VERTEX_IMAGE_UNIFORMS, GL_MAX_FRAGMENT_IMAGE_UNIFORMS, GL_MAX_COMPUTE_IMAGE_UNIFORMS);
    limits.max_samplers_per_shader_stage = limits.max_sampled_textures_per_shader_stage;
    limits.max_vertex_buffers = gl_limit(GL_MAX_VERTEX_ATTRIB_BINDINGS);
    limits.max_vertex_attributes = gl_limit(GL_MAX_VERTEX_ATTRIBS);
    limits.max_vertex_buffer_array_stride = gl_limit(GL_MAX_VERTEX_ATTRIB_STRIDE);
    limits.max_color_attachments = gl_limit(GL_MAX_COLOR_ATTACHMENTS);
    limits.max_color_attachment_bytes_per_sample = limits.max_color_attachments * 16;
    limits.max_compute_workgroup_storage_size = gl_limit(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE);
    limits.max_compute_invocations_per_workgroup = gl_limit(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS);
    GLint workgroup[3] = {};
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workgroup[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workgroup[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workgroup[2]);
    limits.max_compute_workgroup_size_x = workgroup[0] > 0 ? (uint32_t)workgroup[0] : 0;
    limits.max_compute_workgroup_size_y = workgroup[1] > 0 ? (uint32_t)workgroup[1] : 0;
    limits.max_compute_workgroup_size_z = workgroup[2] > 0 ? (uint32_t)workgroup[2] : 0;
    GLint groups[3] = {};
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &groups[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &groups[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &groups[2]);
    uint32_t perDimension = groups[0] > 0 ? (uint32_t)groups[0] : 0;
    if (groups[1] > 0 && (uint32_t)groups[1] < perDimension) perDimension = (uint32_t)groups[1];
    if (groups[2] > 0 && (uint32_t)groups[2] < perDimension) perDimension = (uint32_t)groups[2];
    limits.max_compute_workgroups_per_dimension = perDimension;
    limits.max_bindless_handle_size = (uint32_t)sizeof(GLuint64);
    limits.max_bindless_textures = GLEW_ARB_bindless_texture ? 1048576u : 0u;
    limits.max_push_constant_size = gl_limit3(GL_MAX_VERTEX_UNIFORM_COMPONENTS, GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, GL_MAX_COMPUTE_UNIFORM_COMPONENTS) * 4;
    opengl.limits = limits;
}

void gl_unload_library()
{
    opengl.limits = {};
    glUseProgram(0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (opengl.fbo)
        glDeleteFramebuffers(1, &opengl.fbo);
    opengl.fbo = 0;
    for (auto& item : opengl.modules)
    {
        if (item.second.vao)
            glDeleteVertexArrays(1, &item.second.vao);
        if (item.second.program)
            glDeleteProgram(item.second.program);
    }
    opengl.modules.clear();
    for (auto& item : opengl.samplers)
    {
        if (item.second.handle)
            glDeleteSamplers(1, &item.second.handle);
    }
    opengl.samplers.clear();
    opengl.layouts.clear();
    opengl.bindings.clear();
    for (auto& item : opengl.textureViews)
    {
        gl_release_bindless(item.second);
        if (item.second.handle)
            glDeleteTextures(1, &item.second.handle);
    }
    opengl.textureViews.clear();
    for (auto& item : opengl.textures)
    {
        if (item.second.handle)
            glDeleteTextures(1, &item.second.handle);
    }
    opengl.textures.clear();
    for (auto& item : opengl.buffers)
    {
        if (item.second.handle)
            glDeleteBuffers(1, &item.second.handle);
    }
    opengl.buffers.clear();
    opengl.meshes.clear();
    opengl.meshlets.clear();
    opengl.computePasses.clear();
    opengl.transferPasses.clear();
    opengl.bufferID = opengl.textureID = opengl.textureViewID = opengl.samplerID = 0;
    opengl.layoutID = opengl.bindingID = opengl.moduleID = 0;
    opengl.meshID = opengl.meshletID = opengl.passID = 0;

    opengl.currentPassType = RT_MODULE_NONE;
    opengl.currentPipeline = nullptr;

    // ====================================================================

    if(rt_unload_library == gl_unload_library) rt_unload_library = nullptr;
    if(rt_get_limits == gl_get_limits) rt_get_limits = nullptr;
    if(rt_get_backend == gl_get_backend) rt_get_backend = nullptr;
    if(rt_create_buffer == gl_create_buffer) rt_create_buffer = nullptr;
    if(rt_destroy_buffer == gl_destroy_buffer) rt_destroy_buffer = nullptr;
    if(rt_map_buffer == gl_map_buffer) rt_map_buffer = nullptr;
    if(rt_unmap_buffer == gl_unmap_buffer) rt_unmap_buffer = nullptr;
    if(rt_create_texture == gl_create_texture) rt_create_texture = nullptr;
    if(rt_destroy_texture == gl_destroy_texture) rt_destroy_texture = nullptr;
    if(rt_create_texture_view == gl_create_texture_view) rt_create_texture_view = nullptr;
    if(rt_destroy_texture_view == gl_destroy_texture_view) rt_destroy_texture_view = nullptr;
    if(rt_bind_texture_view == gl_bind_texture_view) rt_bind_texture_view = nullptr;
    if(rt_create_sampler == gl_create_sampler) rt_create_sampler = nullptr;
    if(rt_destroy_sampler == gl_destroy_sampler) rt_destroy_sampler = nullptr;
    if(rt_create_layout == gl_create_layout) rt_create_layout = nullptr;
    if(rt_destroy_layout == gl_destroy_layout) rt_destroy_layout = nullptr;
    if(rt_create_binding == gl_create_binding) rt_create_binding = nullptr;
    if(rt_destroy_binding == gl_destroy_binding) rt_destroy_binding = nullptr;
    if(rt_bind_binding == gl_bind_binding) rt_bind_binding = nullptr;
    if(rt_create_module_compute == gl_create_module_compute) rt_create_module_compute = nullptr;
    if(rt_create_module_render == gl_create_module_render) rt_create_module_render = nullptr;
    if(rt_destroy_module_render == gl_destroy_module_render) rt_destroy_module_render = nullptr;
    if(rt_destroy_module_compute == gl_destroy_module_compute) rt_destroy_module_compute = nullptr;
    if(rt_begin_compute == gl_begin_compute) rt_begin_compute = nullptr;
    if(rt_end_compute == gl_end_compute) rt_end_compute = nullptr;
    if (rt_bind_module_compute == gl_bind_module_compute) rt_bind_module_compute = nullptr;
    if(rt_dispatch_compute == gl_dispatch_compute) rt_dispatch_compute = nullptr;
    if(rt_dispatch_compute_indirect == gl_dispatch_compute_indirect) rt_dispatch_compute_indirect = nullptr;
    if(rt_begin_render == gl_begin_render) rt_begin_render = nullptr;
    if(rt_end_render == gl_end_render) rt_end_render = nullptr;
    if (rt_bind_module_render == gl_bind_module_render) rt_bind_module_render = nullptr;
    if(rt_set_viewport == gl_set_viewport) rt_set_viewport = nullptr;
    if(rt_set_scissor == gl_set_scissor) rt_set_scissor = nullptr;
    if (rt_set_blend_constant == gl_set_blend_constant) rt_set_blend_constant = nullptr;
    if (rt_set_stencil_reference == gl_set_stencil_reference) rt_set_stencil_reference = nullptr;
    if(rt_draw_array == gl_draw_array) rt_draw_array = nullptr;
    if(rt_draw_index == gl_draw_index) rt_draw_index = nullptr;
    if(rt_draw_array_indirect == gl_draw_array_indirect) rt_draw_array_indirect = nullptr;
    if(rt_draw_index_indirect == gl_draw_index_indirect) rt_draw_index_indirect = nullptr;
    if(rt_draw_mesh_task == gl_draw_mesh_task) rt_draw_mesh_task = nullptr;
    if(rt_draw_mesh_task_indirect == gl_draw_mesh_task_indirect) rt_draw_mesh_task_indirect = nullptr;
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
    if(rt_submit == gl_submit) rt_submit = nullptr;
}

static uint32_t gl_limit(GLenum name)
{
    GLint value = 0;
    glGetIntegerv(name, &value);
    return value > 0 ? (uint32_t)value : 0;
}

static uint32_t gl_limit3(GLenum a, GLenum b, GLenum c)
{
    uint32_t value = gl_limit(a);
    uint32_t other = gl_limit(b);
    if (other < value) value = other;
    other = gl_limit(c);
    if (other < value) value = other;
    return value;
}

rt_limits_t gl_get_limits()
{
    return opengl.limits;
}

rt_backend_t gl_get_backend()
{
    return RT_OPENGL;
}

// ====================================================================

rt_buffer_t gl_create_buffer(rt_buffer_info_t const& info)
{
    if (info.usages == 0)
    {
        fprintf(stderr, "Buffer usage must not be 0");
        abort();
    }

    rt_buffer_t result = {};
    auto handle = opengl.bufferID + 1;
    auto& native = opengl.buffers[handle];
    glGenBuffers(1, &native.handle);
    glBindBuffer(GL_ARRAY_BUFFER, native.handle);

    GLbitfield flags = GL_DYNAMIC_STORAGE_BIT;
    if (info.usages & RT_BUFFER_USAGE_MAP_READ)
        flags |= GL_MAP_READ_BIT;
    if (info.usages & RT_BUFFER_USAGE_MAP_WRITE)
        flags |= GL_MAP_WRITE_BIT;

    glBufferStorage(GL_ARRAY_BUFFER, (GLsizeiptr)info.size, info.data, flags);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    opengl.bufferID = handle;
    result.handle = handle;
    result.size = info.size;
    result.usages = info.usages;
    result.native = &native;
    return result;
}

void gl_destroy_buffer(rt_buffer_t& buffer)
{
    if (auto* native = gl_buffer_native(buffer))
    {
        if (native->handle)
            glDeleteBuffers(1, &native->handle);
        opengl.buffers.erase(buffer.handle);
    }
    buffer = {};
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

    glBindBuffer(GL_ARRAY_BUFFER, gl_buffer_name(buffer));
    void* memory = glMapBufferRange(GL_ARRAY_BUFFER, (GLintptr)offset, (GLsizeiptr)size, access);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return memory;
}

void gl_unmap_buffer(rt_buffer_t& buffer)
{
    if (!gl_buffer_name(buffer))
        return;

    glBindBuffer(GL_ARRAY_BUFFER, gl_buffer_name(buffer));
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ====================================================================

static void gl_release_bindless(gl_texture_view_native_t& native)
{
    if (native.bindlessHandle)
    {
        glMakeTextureHandleNonResidentARB(native.bindlessHandle);
        native.bindlessHandle = 0;
    }
}

rt_texture_t gl_create_texture(rt_texture_info_t const& info)
{
    if (info.usages == 0)
    {
        fprintf(stderr, "Texture usage must not be 0");
        abort();
    }

    rt_texture_t result = {};

    uint32_t mipmaps = 1;
    rt_texture_sample_t samples = info.samples == RT_TEXTURE_SAMPLE_4X ? RT_TEXTURE_SAMPLE_4X : RT_TEXTURE_SAMPLE_1X;
    uint32_t depth = info.depth ? info.depth : 1;
    rt_texture_target_t target = info.target;
    if (info.width == 0 || (target != RT_TEXTURE_1D && info.height == 0))
    {
        fprintf(stderr, "Texture size must not be 0");
        abort();
    }
    if (samples == RT_TEXTURE_SAMPLE_4X && (target == RT_TEXTURE_2D || target == RT_TEXTURE_2D_MULTISAMPLE))
        target = RT_TEXTURE_2D_MULTISAMPLE;
    else if (target == RT_TEXTURE_2D_MULTISAMPLE)
        target = RT_TEXTURE_2D;
    if (target == RT_TEXTURE_1D || target == RT_TEXTURE_3D || target == RT_TEXTURE_2D_ARRAY)
        samples = RT_TEXTURE_SAMPLE_1X;

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
    GLenum glInternal = rt_to_gl_texture_format(info.format);
    GLenum glFormat = GL_RGBA;
    GLenum glType = GL_UNSIGNED_BYTE;
    rt_to_gl_transfer(info.format, RT_TEXTURE_ASPECT_ALL, false, glFormat, glType);

    auto handle = opengl.textureID + 1;
    auto& native = opengl.textures[handle];
    glGenTextures(1, &native.handle);
    glBindTexture(glTarget, native.handle);

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
        glTexStorage2DMultisample(glTarget, rt_to_gl_sample_count(samples), glInternal, (GLsizei)info.width, (GLsizei)info.height, GL_TRUE);
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

    if (info.data == nullptr && target != RT_TEXTURE_2D_MULTISAMPLE)
    {
        if (rt_texture_has_depth(info.format) && !rt_texture_has_stencil(info.format))
        {
            if (glType == GL_FLOAT)
            {
                GLfloat clearValue = 1.0f;
                glClearTexImage(native.handle, 0, glFormat, glType, &clearValue);
            }
            else if (glType == GL_UNSIGNED_SHORT)
            {
                GLushort clearValue = 0xFFFF;
                glClearTexImage(native.handle, 0, glFormat, glType, &clearValue);
            }
            else
            {
                GLuint clearValue = 0xFFFFFFFFu;
                glClearTexImage(native.handle, 0, glFormat, glType, &clearValue);
            }
        }
        else if (info.format == RT_TEXTURE_DEPTH24PLUS_STENCIL8)
        {
            GLuint clearValue = 0xFFFFFF00u;
            glClearTexImage(native.handle, 0, glFormat, glType, &clearValue);
        }
        else if (info.format == RT_TEXTURE_DEPTH32FLOAT_STENCIL8)
        {
            struct
            {
                float depth;
                uint32_t stencil;
            } clearValue = {1.0f, 0};
            glClearTexImage(native.handle, 0, glFormat, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, &clearValue);
        }
        else
        {
            glClearTexImage(native.handle, 0, glFormat, glType, nullptr);
        }
    }

    if (target != RT_TEXTURE_2D_MULTISAMPLE)
    {
        glTexParameteri(glTarget, GL_TEXTURE_WRAP_S, rt_to_gl_address(info.address_u));
        glTexParameteri(glTarget, GL_TEXTURE_WRAP_T, rt_to_gl_address(info.address_v));
        glTexParameteri(glTarget, GL_TEXTURE_WRAP_R, rt_to_gl_address(info.address_w));
        glTexParameteri(glTarget, GL_TEXTURE_MIN_FILTER, rt_to_gl_filter(info.min_filter));
        glTexParameteri(glTarget, GL_TEXTURE_MAG_FILTER, rt_to_gl_filter(info.mag_filter));
        glTexParameteri(glTarget, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(glTarget, GL_TEXTURE_MAX_LEVEL, (GLint)(mipmaps - 1));

        if (info.address_u == RT_CLAMP_TO_BORDER || info.address_v == RT_CLAMP_TO_BORDER || info.address_w == RT_CLAMP_TO_BORDER)
        {
            glTexParameterfv(glTarget, GL_TEXTURE_BORDER_COLOR, info.border);
        }

        if (info.min_filter == RT_NEAREST_MIPMAP_NEAREST || info.min_filter == RT_LINEAR_MIPMAP_NEAREST || info.min_filter == RT_NEAREST_MIPMAP_LINEAR || info.min_filter == RT_LINEAR_MIPMAP_LINEAR)
        {
            glGenerateMipmap(glTarget);
        }
    }

    glBindTexture(glTarget, 0);

    native.width = info.width;
    native.height = (target == RT_TEXTURE_1D) ? 1 : info.height;
    native.depth = depth;
    native.target = target;
    native.format = info.format;
    native.usages = info.usages;
    native.levels = mipmaps ? mipmaps : 1;
    native.samples = samples;
    opengl.textureID = handle;
    result.handle = handle;
    result.width = native.width;
    result.height = native.height;
    result.depth = native.depth;
    result.target = native.target;
    result.format = native.format;
    result.samples = native.samples;
    result.usages = native.usages;
    result.mipmaps = native.levels;
    result.min_filter = info.min_filter;
    result.mag_filter = info.mag_filter;
    result.address_u = info.address_u;
    result.address_v = info.address_v;
    result.address_w = info.address_w;
    for (int i = 0; i < 4; ++i)
        result.border[i] = info.border[i];
    result.native = &native;
    uint32_t layers = (target == RT_TEXTURE_2D_ARRAY && depth) ? depth : 1;
    result.default_view = gl_create_texture_view(result, {
        .target = native.target,
        .format = native.format,
        .aspect = RT_TEXTURE_ASPECT_ALL,
        .usages = info.usages,
        .layer_count = layers,
        .level_count = native.levels,
    });
    return result;
}

void gl_destroy_texture(rt_texture_t& texture)
{
    if (auto* native = gl_texture_native(texture))
    {
        for (auto it = opengl.textureViews.begin(); it != opengl.textureViews.end(); )
        {
            if (it->second.texture == native)
            {
                gl_release_bindless(it->second);
                if (it->second.handle)
                    glDeleteTextures(1, &it->second.handle);
                it = opengl.textureViews.erase(it);
            }
            else ++it;
        }

        if (native->handle)
            glDeleteTextures(1, &native->handle);
        opengl.textures.erase(texture.handle);
    }
    texture = {};
}


rt_texture_view_t gl_create_texture_view(rt_texture_t& texture, rt_texture_view_info_t const& info)
{
    auto* parentTex = gl_texture_native(texture);
    rt_texture_format_t parentFormat = parentTex ? parentTex->format : RT_TEXTURE_NONE;
    rt_texture_format_t format = info.format != RT_TEXTURE_NONE ? info.format : parentFormat;
    rt_texture_usages_t usages = info.usages ? info.usages : (parentTex ? parentTex->usages : 0);
    rt_texture_view_t result{
        .target = info.target,
        .format = format,
        .aspect = info.aspect,
        .usages = usages,
        .base_layer = info.base_layer,
        .layer_count = info.layer_count,
        .base_level = info.base_level,
        .level_count = info.level_count,
    };
    if (!parentTex || !parentTex->handle) return result;
    uint32_t levels = parentTex->levels ? parentTex->levels : 1;
    uint32_t layers = parentTex->target == RT_TEXTURE_2D_ARRAY && parentTex->depth ? parentTex->depth : 1;
    uint32_t levelCount = info.level_count ? info.level_count : levels - info.base_level;
    if (info.base_level >= levels || levelCount == 0) return result;
    if (info.base_level + levelCount > levels)
        levelCount = levels - info.base_level;
    uint32_t baseLayer = 0;
    uint32_t layerCount = 1;
    if (info.target != RT_TEXTURE_3D)
    {
        baseLayer = info.base_layer;
        layerCount = info.layer_count ? info.layer_count : (layers > baseLayer ? layers - baseLayer : 0);
        if (baseLayer >= layers || layerCount == 0) return result;
        if (baseLayer + layerCount > layers)
            layerCount = layers - baseLayer;
    }
    result.base_layer = baseLayer;
    result.layer_count = layerCount;
    result.base_level = info.base_level;
    result.level_count = levelCount;

    uint32_t handle = opengl.textureViewID + 1;
    auto& native = opengl.textureViews[handle];
    glGenTextures(1, &native.handle);
    GLenum internalFormat = rt_to_gl_texture_format(result.format);
    glTextureView(native.handle, rt_to_gl_texture_target(info.target), parentTex->handle, internalFormat,
                  (GLuint)info.base_level, (GLuint)levelCount, (GLuint)baseLayer, (GLuint)layerCount);
    if (!native.handle)
    {
        opengl.textureViews.erase(handle);
        return result;
    }
    if (rt_texture_has_depth(result.format) || rt_texture_has_stencil(result.format))
    {
        auto glTarget = rt_to_gl_texture_target(info.target);
        GLenum mode = result.aspect == RT_TEXTURE_ASPECT_STENCIL ? GL_STENCIL_INDEX : GL_DEPTH_COMPONENT;
        glBindTexture(glTarget, native.handle);
        glTexParameteri(glTarget, GL_DEPTH_STENCIL_TEXTURE_MODE, mode);
        glBindTexture(glTarget, 0);
    }
    native.texture = parentTex;
    opengl.textureViewID = handle;
    result.handle = handle;
    result.native = &native;
    if ((usages & RT_TEXTURE_USAGE_TEXTURE_BINDLESS) && GLEW_ARB_bindless_texture && native.handle && result.target != RT_TEXTURE_2D_MULTISAMPLE)
    {
        auto glTarget = rt_to_gl_texture_target(info.target);
        glBindTexture(glTarget, native.handle);
        glTexParameteri(glTarget, GL_TEXTURE_MIN_FILTER, rt_to_gl_filter(texture.min_filter));
        glTexParameteri(glTarget, GL_TEXTURE_MAG_FILTER, rt_to_gl_filter(texture.mag_filter));
        glTexParameteri(glTarget, GL_TEXTURE_WRAP_S, rt_to_gl_address(texture.address_u));
        glTexParameteri(glTarget, GL_TEXTURE_WRAP_T, rt_to_gl_address(texture.address_v));
        glTexParameteri(glTarget, GL_TEXTURE_WRAP_R, rt_to_gl_address(texture.address_w));
        glTexParameteri(glTarget, GL_TEXTURE_MAX_LEVEL, (GLint)(levelCount - 1));
        if (texture.address_u == RT_CLAMP_TO_BORDER || texture.address_v == RT_CLAMP_TO_BORDER || texture.address_w == RT_CLAMP_TO_BORDER)
            glTexParameterfv(glTarget, GL_TEXTURE_BORDER_COLOR, texture.border);
        glBindTexture(glTarget, 0);

        if (GLuint64 bindless = glGetTextureHandleARB(native.handle))
        {
            glMakeTextureHandleResidentARB(bindless);
            native.bindlessHandle = bindless;
            result.bindless = bindless;
        }
    }
    return result;
}

void gl_destroy_texture_view(rt_texture_view_t& view)
{
    if (auto* native = gl_texture_view_native(view))
    {
        gl_release_bindless(*native);
        if (native->handle)
            glDeleteTextures(1, &native->handle);
        opengl.textureViews.erase(view.handle);
    }
    view = {};
}

void gl_bind_texture_view(rt_texture_view_t& view, rt_texture_view_bind_t bind)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    GLenum target = rt_to_gl_texture_target(view.target);
    glActiveTexture(GL_TEXTURE0 + bind.binding);
    glBindTexture(target, gl_texture_view_name(view));
}


// ====================================================================

rt_sampler_t gl_create_sampler(rt_sampler_info_t const& info)
{
    rt_sampler_t result = {};
    auto handle = opengl.samplerID + 1;
    auto& native = opengl.samplers[handle];
    glGenSamplers(1, &native.handle);

    glSamplerParameteri(native.handle, GL_TEXTURE_MIN_FILTER, rt_to_gl_filter(info.min_filter));
    glSamplerParameteri(native.handle, GL_TEXTURE_MAG_FILTER, rt_to_gl_filter(info.mag_filter));
    glSamplerParameteri(native.handle, GL_TEXTURE_WRAP_S, rt_to_gl_address(info.address_u));
    glSamplerParameteri(native.handle, GL_TEXTURE_WRAP_T, rt_to_gl_address(info.address_v));
    glSamplerParameteri(native.handle, GL_TEXTURE_WRAP_R, rt_to_gl_address(info.address_w));

    opengl.samplerID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void gl_destroy_sampler(rt_sampler_t& sampler)
{
    if (auto* native = gl_sampler_native(sampler))
    {
        if (native->handle)
            glDeleteSamplers(1, &native->handle);
        opengl.samplers.erase(sampler.handle);
    }
    sampler = {};
}


// ====================================================================

static gl_layout_native_t* gl_layout_native(rt_layout_t const& layout)
{
    if (!layout.native || layout.handle == 0) return nullptr;
    return (gl_layout_native_t*)layout.native;
}

static gl_binding_native_t* gl_binding_native(rt_binding_t const& binding)
{
    if (!binding.native || binding.handle == 0) return nullptr;
    return (gl_binding_native_t*)binding.native;
}

static void gl_copy_module_layout(rt_layout_t const& layout, rt_module_layout_t* dst, rt_module_layout_t* native)
{
    for (size_t i = 0; i < RT_MAX_BINDING_HANDLE_NUM; ++i)
    {
        dst[i].binding = layout.entry[i].binding;
        dst[i].stages = layout.entry[i].stages;
        dst[i].type = layout.entry[i].type;
        native[i] = dst[i];
    }
}

rt_layout_t gl_create_layout(rt_layout_info_t const& info)
{
    rt_layout_t result = {};
    auto handle = opengl.layoutID + 1;
    auto& native = opengl.layouts[handle];
    uint32_t count = 0;
    for (size_t i = 0; i < std::size(info.entry); ++i)
    {
        result.entry[i] = info.entry[i];
        if (info.entry[i].type != RT_BINDING_NONE)
            count++;
    }
    native.count = count;
    opengl.layoutID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void gl_destroy_layout(rt_layout_t& layout)
{
    if (gl_layout_native(layout))
        opengl.layouts.erase(layout.handle);
    layout = {};
}

rt_binding_t gl_create_binding(rt_binding_info_t const& info)
{
    if (!info.layout.handle)
    {
        fprintf(stderr, "Binding requires a layout");
        abort();
    }

    rt_binding_t result = {};
    auto handle = opengl.bindingID + 1;
    auto& native = opengl.bindings[handle];
    for (size_t i = 0; i < std::size(info.entry); ++i)
    {
        result.entry[i] = info.entry[i];
        native.entry[i] = info.layout.entry[i];
    }
    opengl.bindingID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void gl_destroy_binding(rt_binding_t& binding)
{
    if (gl_binding_native(binding))
        opengl.bindings.erase(binding.handle);
    binding = {};
}

void gl_bind_binding(rt_binding_t& binding)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* native = gl_binding_native(binding);
    if (!native)
        return;

    for (size_t i = 0; i < RT_MAX_BINDING_HANDLE_NUM; ++i)
    {
        auto const& slot = native->entry[i];
        if (slot.type == RT_BINDING_NONE)
            continue;
        auto const& item = binding.entry[i];
        uint32_t index = slot.binding;
        if (slot.type == RT_BINDING_UNIFORM_BUFFER || slot.type == RT_BINDING_STORAGE_BUFFER)
        {
            if (!item.buffer.handle)
                continue;
            GLenum target = slot.type == RT_BINDING_STORAGE_BUFFER ? GL_SHADER_STORAGE_BUFFER : GL_UNIFORM_BUFFER;
            size_t size = item.buffer_bind.size;
            if (size == (size_t)-1U)
                size = item.buffer.size > item.buffer_bind.offset ? item.buffer.size - item.buffer_bind.offset : 0;
            glBindBufferRange(target, index, gl_buffer_name(item.buffer), (GLintptr)item.buffer_bind.offset, (GLsizeiptr)size);
        }
        else if (slot.type == RT_BINDING_TEXTURE)
        {
            if (!item.texture.handle)
                continue;
            glActiveTexture(GL_TEXTURE0 + index);
            glBindTexture(rt_to_gl_texture_target(item.texture.target), gl_texture_view_name(item.texture));
        }
        else if (slot.type == RT_BINDING_STORAGE_TEXTURE)
        {
            if (!item.texture_storage.handle)
                continue;
            auto const& image = item.texture_storage_bind;
            glBindImageTexture(index, gl_texture_view_name(item.texture_storage), (GLint)image.base_level, 1 < image.layer_count,
                               (GLint)image.base_layer, rt_to_gl_access(image.access), rt_to_gl_texture_format(item.texture_storage.format));
        }
        else if (slot.type == RT_BINDING_SAMPLER)
        {
            if (!item.sampler.handle)
                continue;
            glBindSampler(index, gl_sampler_name(item.sampler));
        }
    }
}

// ====================================================================

rt_module_compute_t gl_create_module_compute(rt_module_compute_info_t const& info)
{
    rt_module_compute_t result = {};

    if (!info.cshader.code)
    {
        fprintf(stderr, "Compute shader source is empty");
        abort();
    }

    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    GLint clength = (GLint)info.cshader.size;
    glShaderSource(cs, 1, &info.cshader.code, info.cshader.size ? &clength : nullptr);
    glCompileShader(cs);

    GLint success = 0;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(cs, sizeof(log), nullptr, log);
        fprintf(stderr, "Compute shader compile error:\n%s", log);
        abort();
    }

    auto handle = opengl.moduleID + 1;
    auto& native = opengl.modules[handle];
    native.program = glCreateProgram();
    glAttachShader(native.program, cs);
    glLinkProgram(native.program);

    glGetProgramiv(native.program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(native.program, sizeof(log), nullptr, log);
        abort();
    }

    glDeleteShader(cs);

    opengl.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    gl_copy_module_layout(info.layout, result.layout, native.layouts);
    return result;
}

rt_module_render_t gl_create_module_render(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};

    GLuint vs = 0, ts = 0, ms = 0, fs = 0;
    if (info.vshader.code)
    {
        // ---- Vertex Shader ----
        vs = glCreateShader(GL_VERTEX_SHADER);
        GLint vlength = (GLint)info.vshader.size;
        glShaderSource(vs, 1, &info.vshader.code, info.vshader.size ? &vlength : nullptr);
        glCompileShader(vs);
        GLint success = 0;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[1024];
            glGetShaderInfoLog(vs, sizeof(log), nullptr, log);
            fprintf(stderr, "Vertex shader compile error:\n%s", log);
            abort();
        }
    }
    else if (info.mshader.code)
    {
        // ---- Task Shader（可选）----
        if (info.tshader.code)
        {
            ts = glCreateShader(GL_TASK_SHADER_NV);
            GLint tlength = (GLint)info.tshader.size;
            glShaderSource(ts, 1, &info.tshader.code, info.tshader.size ? &tlength : nullptr);
            glCompileShader(ts);
            GLint success = 0;
            glGetShaderiv(ts, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                char log[1024];
                glGetShaderInfoLog(ts, sizeof(log), nullptr, log);
                fprintf(stderr, "Task shader compile error:\n%s", log);
                abort();
            }
        }

        // ---- Mesh Shader ----
        ms = glCreateShader(GL_MESH_SHADER_NV);
        GLint mlength = (GLint)info.mshader.size;
        glShaderSource(ms, 1, &info.mshader.code, info.mshader.size ? &mlength : nullptr);
        glCompileShader(ms);
        GLint success = 0;
        glGetShaderiv(ms, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[1024];
            glGetShaderInfoLog(ms, sizeof(log), nullptr, log);
            fprintf(stderr, "Mesh shader compile error:\n%s", log);
            abort();
        }
    }
    else
    {
        fprintf(stderr, "Render module requires a vertex shader or a mesh shader");
        abort();
    }

    // ---- Fragment Shader ----
    if (info.fshader.code)
    {
        fs = glCreateShader(GL_FRAGMENT_SHADER);
        GLint flength = (GLint)info.fshader.size;
        glShaderSource(fs, 1, &info.fshader.code, info.fshader.size ? &flength : nullptr);
        glCompileShader(fs);
        GLint success = 0;
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[1024];
            glGetShaderInfoLog(fs, sizeof(log), nullptr, log);
            fprintf(stderr, "Fragment shader compile error:\n%s", log);
            abort();
        }
    }

    // ---- Program ----
    auto handle = opengl.moduleID + 1;
    auto& native = opengl.modules[handle];
    native.program = glCreateProgram();
    if (vs)
        glAttachShader(native.program, vs);
    if (ts)
        glAttachShader(native.program, ts);
    if (ms)
        glAttachShader(native.program, ms);
    if (fs)
        glAttachShader(native.program, fs);
    glLinkProgram(native.program);

    GLint success = 0;
    glGetProgramiv(native.program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(native.program, sizeof(log), nullptr, log);
        fprintf(stderr, "Program link error:\n%s", log);
        abort();
    }

    if (vs) glDeleteShader(vs);
    if (ts) glDeleteShader(ts);
    if (ms) glDeleteShader(ms);
    if (fs) glDeleteShader(fs);

    glGenVertexArrays(1, &native.vao);
    glBindVertexArray(native.vao);
    for (uint32_t i = 0; i < std::size(info.vertex); ++i)
    {
        auto& vertex = info.vertex[i];
        for (auto& attrib : vertex.attrib)
        {
            if (attrib.format == RT_VERTEX_NONE)
                continue;
            glEnableVertexAttribArray(attrib.location);
            rt_to_gl_vertex_format(attrib.location, attrib.format, attrib.offset);
            glVertexAttribBinding(attrib.location, i);
            glVertexBindingDivisor(i, vertex.instance ? 1 : 0);
        }
    }
    glBindVertexArray(0);

    for (size_t i = 0; i < std::size(info.colors); ++i)
    {
        result.colors[i].format = info.colors[i].format;
        result.colors[i].write.r = info.colors[i].write.r;
        result.colors[i].write.g = info.colors[i].write.g;
        result.colors[i].write.b = info.colors[i].write.b;
        result.colors[i].write.a = info.colors[i].write.a;
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
        result.vertex[i] = info.vertex[i];
    }
    gl_copy_module_layout(info.layout, result.layout, native.layouts);
    result.cull_mode = info.cull_mode;
    result.wind_mode = info.wind_mode;
    result.fill_mode = info.fill_mode;
    result.primitive = info.primitive;
    opengl.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void gl_destroy_module_render(rt_module_render_t& module)
{
    if (auto* native = gl_module_native(module.handle, module.native))
    {
        if (native->vao)
            glDeleteVertexArrays(1, &native->vao);
        if (native->program)
            glDeleteProgram(native->program);
        opengl.modules.erase(module.handle);
    }
    module.handle = 0;
    module.native = nullptr;
}

void gl_destroy_module_compute(rt_module_compute_t& module)
{
    if (auto* native = gl_module_native(module.handle, module.native))
    {
        if (native->program)
            glDeleteProgram(native->program);
        opengl.modules.erase(module.handle);
    }
    module.handle = 0;
    module.native = nullptr;
}

// ====================================================================

void gl_begin_compute(rt_pass_compute_t& pass)
{
    if (opengl.currentPipeline != nullptr)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    auto handle = opengl.passID + 1;
    auto& native = opengl.computePasses[handle];
    pass.handle = handle;
    pass.native = &native;
    opengl.currentComputePass = &pass;
    opengl.currentPassType = RT_MODULE_COMPUTE;
    opengl.currentComputeModule = nullptr;
    opengl.passID = handle;
}

void gl_end_compute(rt_pass_compute_t& pass)
{
    if (opengl.currentComputePass != &pass)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }

    opengl.computePasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    opengl.currentPassType = RT_MODULE_NONE;
    opengl.currentComputePass = nullptr;
    opengl.currentComputeModule = nullptr;

    glUseProgram(0);
}

void gl_bind_module_compute(rt_module_compute_t& module)
{
    if (opengl.currentPipeline == nullptr || opengl.currentPassType != RT_MODULE_COMPUTE)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* native = gl_module_native(module.handle, module.native);
    if (module.handle == 0 || !native)
    {
        fprintf(stderr, "Pipeline module is not created");
        abort();
    }
    opengl.currentComputeModule = &module;
    glUseProgram(native->program);
}

void gl_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_COMPUTE)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentComputeModule == nullptr)
    {
        fprintf(stderr, "Pipeline module not bound");
        abort();
    }

    glDispatchCompute(std::max(1U, groupX), std::max(1U, groupY), std::max(1U, groupZ));

    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
}

void gl_dispatch_compute_indirect(rt_buffer_t& indirect, size_t offset)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_COMPUTE)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentComputeModule == nullptr)
    {
        fprintf(stderr, "Pipeline module not bound");
        abort();
    }

    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, gl_buffer_name(indirect));
    glDispatchComputeIndirect((GLintptr)offset);

    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
}

// ====================================================================

static GLsizei rt_to_gl_vertex_size(rt_vertex_format_t format)
{
    switch (format)
    {
        case RT_VERTEX_NONE: return 0;
        case RT_VERTEX_UINT8: return 1;
        case RT_VERTEX_UINT8X2: return 2;
        case RT_VERTEX_UINT8X4: return 4;
        case RT_VERTEX_SINT8: return 1;
        case RT_VERTEX_SINT8X2: return 2;
        case RT_VERTEX_SINT8X4: return 4;
        case RT_VERTEX_UNORM8: return 1;
        case RT_VERTEX_UNORM8X2: return 2;
        case RT_VERTEX_UNORM8X4: return 4;
        case RT_VERTEX_SNORM8: return 1;
        case RT_VERTEX_SNORM8X2: return 2;
        case RT_VERTEX_SNORM8X4: return 4;
        case RT_VERTEX_UINT16: return 2;
        case RT_VERTEX_UINT16X2: return 4;
        case RT_VERTEX_UINT16X4: return 8;
        case RT_VERTEX_SINT16: return 2;
        case RT_VERTEX_SINT16X2: return 4;
        case RT_VERTEX_SINT16X4: return 8;
        case RT_VERTEX_UNORM16: return 2;
        case RT_VERTEX_UNORM16X2: return 4;
        case RT_VERTEX_UNORM16X4: return 8;
        case RT_VERTEX_SNORM16: return 2;
        case RT_VERTEX_SNORM16X2: return 4;
        case RT_VERTEX_SNORM16X4: return 8;
        case RT_VERTEX_FLOAT16: return 2;
        case RT_VERTEX_FLOAT16X2: return 4;
        case RT_VERTEX_FLOAT16X4: return 8;
        case RT_VERTEX_FLOAT32: return 4;
        case RT_VERTEX_FLOAT32X2: return 8;
        case RT_VERTEX_FLOAT32X3: return 12;
        case RT_VERTEX_FLOAT32X4: return 16;
        case RT_VERTEX_UINT32: return 4;
        case RT_VERTEX_UINT32X2: return 8;
        case RT_VERTEX_UINT32X3: return 12;
        case RT_VERTEX_UINT32X4: return 16;
        case RT_VERTEX_SINT32: return 4;
        case RT_VERTEX_SINT32X2: return 8;
        case RT_VERTEX_SINT32X3: return 12;
        case RT_VERTEX_SINT32X4: return 16;
        default: return 0;
    }
}

static GLsizei rt_to_gl_vertex_stride(rt_vertex_t const& layout)
{
    // stride != 0: use it as-is; stride == 0: tight-pack from the attributes
    if (layout.stride)
        return (GLsizei)layout.stride;
    GLsizei stride = 0;
    for (auto& attrib : layout.attrib)
    {
        if (attrib.format == RT_VERTEX_NONE)
            continue;
        GLsizei end = (GLsizei)attrib.offset + rt_to_gl_vertex_size(attrib.format);
        if (end > stride)
            stride = end;
    }
    return stride;
}

static GLenum rt_to_gl_index_type(rt_index_type_t type)
{
    switch (type)
    {
    case RT_INDEX_UINT16: return GL_UNSIGNED_SHORT;
    case RT_INDEX_UINT32: return GL_UNSIGNED_INT;
    default: return GL_UNSIGNED_INT;
    }
}

void gl_begin_render(rt_pass_render_t& pass)
{
    if (opengl.currentPipeline != nullptr)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    opengl.currentPassType = RT_MODULE_RENDER;
    opengl.currentRenderPass = &pass;
    opengl.currentRenderModule = nullptr;

    glDisable(GL_SCISSOR_TEST);

    bool offscreen = pass.depth.texture.handle;
    for (size_t i = 0; i < std::size(pass.colors) && !offscreen; ++i)
    {
        if (pass.colors[i].texture.handle)
            offscreen = true;
    }
    opengl.stencilRefer = offscreen ? pass.stencil.refer : pass.screen.stencil.refer;
    glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);

    if (offscreen)
    {
        // Render State

        int32_t colorCount = 0;
        uint32_t width = 0, height = 0;
        GLuint colorNames[RT_MAX_COLOR_TEXTURE_NUM] = {};
        GLuint depthName = 0;
        GLenum colorAttachments[RT_MAX_COLOR_TEXTURE_NUM] = {};
        for (size_t i = 0; i < std::size(pass.colors); ++i)
        {
            if (pass.colors[i].texture.handle == 0 || pass.colors[i].texture.format == RT_TEXTURE_NONE)
                continue;
            colorNames[i] = gl_texture_view_name(pass.colors[i].texture);
            colorAttachments[i] = GL_COLOR_ATTACHMENT0 + (GLenum)i;
            width = std::max(width, gl_texture_native(pass.colors[i].texture)->width);
            height = std::max(height, gl_texture_native(pass.colors[i].texture)->height);
            colorCount = (int32_t)i + 1;
        }
        if (pass.depth.texture.handle)
        {
            depthName = gl_texture_view_name(pass.depth.texture);
            width = std::max(width, gl_texture_native(pass.depth.texture)->width);
            height = std::max(height, gl_texture_native(pass.depth.texture)->height);
        }

        if (opengl.fbo == 0)
            glGenFramebuffers(1, &opengl.fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, opengl.fbo);

        for (size_t i = 0; i < std::size(pass.colors); ++i)
        {
            GLenum target = colorNames[i] ? rt_to_gl_texture_target(pass.colors[i].texture.target) : GL_TEXTURE_2D;
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLenum)i, target, colorNames[i], 0);
        }
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 0, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, 0, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0);
        if (depthName)
        {
            GLenum target = rt_to_gl_texture_target(pass.depth.texture.target);
            if (rt_texture_has_depth(pass.depth.texture.format) && rt_texture_has_stencil(pass.depth.texture.format))
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, target, depthName, 0);
            }
            else if (rt_texture_has_depth(pass.depth.texture.format))
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, depthName, 0);
            }
            else if (rt_texture_has_stencil(pass.depth.texture.format))
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, target, depthName, 0);
            }
            else
            {
                fprintf(stderr, "Invalid depth attachment");
                abort();
            }
        }
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            fprintf(stderr, "Framebuffer not complete");
            abort();
        }
        if (colorCount) glDrawBuffers(colorCount, colorAttachments);
        gl_set_viewport(0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f);

        glDisable(GL_BLEND);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glStencilMask(0xFFFFFFFF);
        for (size_t i = 0; i < std::size(pass.colors); ++i)
        {
            if (pass.colors[i].texture.handle == 0 || pass.colors[i].texture.format == RT_TEXTURE_NONE)
                continue;

            if (pass.colors[i].clear)
            {
                glClearBufferfv(GL_COLOR, (int32_t)i, &pass.colors[i].value.r);
            }
        }

        if (pass.depth.texture.handle &&
            (rt_texture_has_depth(pass.depth.texture.format) || rt_texture_has_stencil(pass.depth.texture.format)))
        {
            if (pass.depth.clear && rt_texture_has_depth(pass.depth.texture.format))
            {
                glClearBufferfv(GL_DEPTH, 0, &pass.depth.value);
            }
            if (pass.stencil.clear && rt_texture_has_stencil(pass.depth.texture.format))
            {
                glClearBufferiv(GL_STENCIL, 0, &pass.stencil.value);
            }
        }
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glStencilMask(0xFFFFFFFF);
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
        glStencilFunc(rt_to_gl_compare(pass.screen.stencil.func), opengl.stencilRefer, pass.screen.stencil.read);
        glStencilOp(rt_to_gl_stencil_op(pass.screen.stencil.sfail), rt_to_gl_stencil_op(pass.screen.stencil.zfail), rt_to_gl_stencil_op(pass.screen.stencil.zpass));
    }
}

void gl_end_render(rt_pass_render_t& pass)
{
    if (opengl.currentRenderPass != &pass)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    opengl.currentPassType = RT_MODULE_NONE;
    opengl.currentRenderPass = nullptr;
    opengl.currentRenderModule = nullptr;

    bool offscreen = pass.depth.texture.handle;
    for (size_t i = 0; i < std::size(pass.colors) && !offscreen; ++i)
    {
        if (pass.colors[i].texture.handle)
            offscreen = true;
    }
    if (offscreen)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    pass.handle = 0;
    pass.native = nullptr;

    glUseProgram(0);
}

void gl_bind_module_render(rt_module_render_t& module)
{
    if (opengl.currentPipeline == nullptr || opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* native = gl_module_native(module.handle, module.native);
    if (module.handle == 0 || !native)
    {
        fprintf(stderr, "Pipeline module is not created");
        abort();
    }
    rt_pass_render_t const& pass = *opengl.currentRenderPass;
    opengl.currentRenderModule = &module;

    glUseProgram(native->program);
    glBindVertexArray(native->vao);

    bool offscreen = pass.depth.texture.handle;
    for (size_t i = 0; i < std::size(pass.colors) && !offscreen; ++i)
    {
        if (pass.colors[i].texture.handle)
            offscreen = true;
    }
    if (offscreen)
    {
        // Blend State

        glDisable(GL_BLEND);
        for (size_t i = 0; i < std::size(pass.colors); ++i)
        {
            if (pass.colors[i].texture.handle == 0 || pass.colors[i].texture.format == RT_TEXTURE_NONE)
                continue;

            if (module.colors[i].color.func != RT_FUNC_ADD || module.colors[i].color.src != RT_BLEND_ONE ||
                module.colors[i].color.dst != RT_BLEND_ZERO || module.colors[i].alpha.func != RT_FUNC_ADD ||
                module.colors[i].alpha.src != RT_BLEND_ONE || module.colors[i].alpha.dst != RT_BLEND_ZERO)
            {
                glEnable(GL_BLEND);
            }

            glColorMaski((GLuint)i, module.colors[i].write.r, module.colors[i].write.g, module.colors[i].write.b, module.colors[i].write.a);
            glBlendEquationSeparatei(i, rt_to_gl_blend_op(module.colors[i].color.func), rt_to_gl_blend_op(module.colors[i].alpha.func));
            glBlendFuncSeparatei(i, rt_to_gl_blend_factor(module.colors[i].color.src), rt_to_gl_blend_factor(module.colors[i].color.dst), rt_to_gl_blend_factor(module.colors[i].alpha.src), rt_to_gl_blend_factor(module.colors[i].alpha.dst));
        }

        // Depth State

        if (module.depth.func == RT_ALWAYS && module.depth.write == false)
        {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }
        else
        {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(module.depth.write);
        }
        glDepthFunc(rt_to_gl_compare(module.depth.func));

        if (module.depth.bias == 0 && module.depth.biasSlope == 0)
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
        else
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffsetClamp(module.depth.biasSlope, module.depth.bias, module.depth.biasClamp);
        }

        // Stencil State

        if (module.stencil.back.func != RT_ALWAYS || module.stencil.back.sfail != RT_STENCIL_KEEP ||
            module.stencil.back.zfail != RT_STENCIL_KEEP || module.stencil.back.zpass != RT_STENCIL_KEEP ||
            module.stencil.front.func != RT_ALWAYS || module.stencil.front.sfail != RT_STENCIL_KEEP ||
            module.stencil.front.zfail != RT_STENCIL_KEEP || module.stencil.front.zpass != RT_STENCIL_KEEP)
        {
            glEnable(GL_STENCIL_TEST);
        }
        else
        {
            glDisable(GL_STENCIL_TEST);
        }
        glStencilMask(module.stencil.write);
        glStencilFuncSeparate(GL_BACK, rt_to_gl_compare(module.stencil.back.func), opengl.stencilRefer, module.stencil.read);
        glStencilFuncSeparate(GL_FRONT, rt_to_gl_compare(module.stencil.front.func), opengl.stencilRefer, module.stencil.read);
        glStencilOpSeparate(GL_BACK, rt_to_gl_stencil_op(module.stencil.back.sfail), rt_to_gl_stencil_op(module.stencil.back.zfail), rt_to_gl_stencil_op(module.stencil.back.zpass));
        glStencilOpSeparate(GL_FRONT, rt_to_gl_stencil_op(module.stencil.front.sfail), rt_to_gl_stencil_op(module.stencil.front.zfail), rt_to_gl_stencil_op(module.stencil.front.zpass));
    }

    // Primitive State

    glFrontFace(rt_to_gl_wind_mode(module.wind_mode));
    if (module.cull_mode)
    {
        glCullFace(rt_to_gl_cull(module.cull_mode));
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }

    glPolygonMode(GL_FRONT_AND_BACK, rt_to_gl_fill(module.fill_mode));
}

void gl_set_viewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    glViewportIndexedf(0, x, y, width, height);
    glDepthRangef(minDepth, maxDepth);
}

void gl_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, width, height);
}

void gl_set_blend_constant(float r, float g, float b, float a)
{
    if (opengl.currentPipeline == nullptr || opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    glBlendColor(r, g, b, a);
}

void gl_set_stencil_reference(int32_t refer)
{
    if (opengl.currentPipeline == nullptr || opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    opengl.stencilRefer = refer;

    rt_pass_render_t const& pass = *opengl.currentRenderPass;
    bool offscreen = pass.depth.texture.handle;
    for (size_t i = 0; i < std::size(pass.colors) && !offscreen; ++i)
    {
        if (pass.colors[i].texture.handle)
            offscreen = true;
    }
    if (offscreen)
    {
        if (opengl.currentRenderModule == nullptr)
            return;
        rt_module_render_t const& module = *opengl.currentRenderModule;
        glStencilFuncSeparate(GL_BACK, rt_to_gl_compare(module.stencil.back.func), refer, module.stencil.read);
        glStencilFuncSeparate(GL_FRONT, rt_to_gl_compare(module.stencil.front.func), refer, module.stencil.read);
    }
    else
    {
        glStencilFunc(rt_to_gl_compare(pass.screen.stencil.func), refer, pass.screen.stencil.read);
    }
}

void gl_draw_array(rt_buffer_view_t vbo[], uint32_t vbo_num, uint32_t vertex_num, uint32_t instance_num, uint32_t vertex_start, uint32_t instance_start)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    rt_module_render_t const& module = gl_current_render_module();
    uint32_t count = vbo_num;
    if (count > std::size(module.vertex))
        count = (uint32_t)std::size(module.vertex);
    for (uint32_t i = 0; i < count; ++i)
    {
        GLsizei stride = rt_to_gl_vertex_stride(module.vertex[i]);
        if (stride == 0 || !vbo || vbo[i].buffer.handle == 0)
            continue;
        glBindVertexBuffer(i, gl_buffer_name(vbo[i].buffer), (GLintptr)vbo[i].offset, stride);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDrawArraysInstancedBaseInstance(rt_to_gl_primitive(module.primitive), (GLint)vertex_start, (GLsizei)vertex_num, (GLsizei)instance_num, instance_start);
}

void gl_draw_index(rt_buffer_view_t vbo[], uint32_t vbo_num, rt_buffer_view_t& ebo, uint32_t index_num, uint32_t instance_num, uint32_t index_start, uint32_t vertex_start, uint32_t instance_start)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    rt_module_render_t const& module = gl_current_render_module();
    uint32_t count = vbo_num;
    if (count > std::size(module.vertex))
        count = (uint32_t)std::size(module.vertex);
    for (uint32_t i = 0; i < count; ++i)
    {
        GLsizei stride = rt_to_gl_vertex_stride(module.vertex[i]);
        if (stride == 0 || !vbo || vbo[i].buffer.handle == 0)
            continue;
        glBindVertexBuffer(i, gl_buffer_name(vbo[i].buffer), (GLintptr)vbo[i].offset, stride);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_buffer_name(ebo.buffer));
    size_t indexOffset = ebo.offset + (size_t)index_start * (module.index_type == RT_INDEX_UINT16 ? 2u : 4u);
    glDrawElementsInstancedBaseVertexBaseInstance(rt_to_gl_primitive(module.primitive), (GLsizei)index_num, rt_to_gl_index_type(module.index_type), (void*)indexOffset, (GLsizei)instance_num, (GLint)vertex_start, instance_start);
}

void gl_draw_array_indirect(rt_buffer_view_t vbo[], uint32_t vbo_num, rt_buffer_t& indirect, size_t offset)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    rt_module_render_t const& module = gl_current_render_module();
    uint32_t count = vbo_num;
    if (count > std::size(module.vertex))
        count = (uint32_t)std::size(module.vertex);
    for (uint32_t i = 0; i < count; ++i)
    {
        GLsizei stride = rt_to_gl_vertex_stride(module.vertex[i]);
        if (stride == 0 || !vbo || vbo[i].buffer.handle == 0)
            continue;
        glBindVertexBuffer(i, gl_buffer_name(vbo[i].buffer), (GLintptr)vbo[i].offset, stride);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, gl_buffer_name(indirect));
    glDrawArraysIndirect(rt_to_gl_primitive(module.primitive), (void*)offset);
}

void gl_draw_index_indirect(rt_buffer_view_t vbo[], uint32_t vbo_num, rt_buffer_view_t& ebo, rt_buffer_t& indirect, size_t offset)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    rt_module_render_t const& module = gl_current_render_module();
    uint32_t count = vbo_num;
    if (count > std::size(module.vertex))
        count = (uint32_t)std::size(module.vertex);
    for (uint32_t i = 0; i < count; ++i)
    {
        GLsizei stride = rt_to_gl_vertex_stride(module.vertex[i]);
        if (stride == 0 || !vbo || vbo[i].buffer.handle == 0)
            continue;
        glBindVertexBuffer(i, gl_buffer_name(vbo[i].buffer), (GLintptr)vbo[i].offset, stride);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_buffer_name(ebo.buffer));
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, gl_buffer_name(indirect));
    glDrawElementsIndirect(rt_to_gl_primitive(module.primitive), rt_to_gl_index_type(module.index_type), (void*)offset);
}

void gl_draw_mesh_task(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    glDrawMeshTasksNV(0, std::max(1U, groupX) * std::max(1U, groupY) * std::max(1U, groupZ));
}

void gl_draw_mesh_task_indirect(rt_buffer_t& indirect, size_t offset, uint32_t draw_count, uint32_t draw_stride)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, gl_buffer_name(indirect));
    glMultiDrawMeshTasksIndirectNV((GLintptr)offset, (GLsizei)draw_count, (GLsizei)draw_stride);
}

void gl_push_constant(uint8_t const* buffer, size_t length)
{
    fprintf(stderr, "OpenGL: does not support rt_push_constant");
    abort();
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
        fprintf(stderr, "Unsupported pixel type");
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
        fprintf(stderr, "Unsupported pixel format");
        abort();
    }

    return components * gl_type_size(type);
}

static void gl_transfer_format(rt_texture_t const& texture, rt_texture_aspect_t aspect, bool download, GLenum& format, GLenum& type)
{
    auto* tex = gl_texture_native(texture);
    rt_texture_format_t texFormat = tex ? tex->format : RT_TEXTURE_NONE;
    if (aspect == RT_TEXTURE_ASPECT_DEPTH && !rt_texture_has_depth(texFormat))
    {
        fprintf(stderr, "Texture has no depth aspect");
        abort();
    }
    if (aspect == RT_TEXTURE_ASPECT_STENCIL && !rt_texture_has_stencil(texFormat))
    {
        fprintf(stderr, "Texture has no stencil aspect");
        abort();
    }
    rt_to_gl_transfer(texFormat, aspect, download, format, type);
}

// 校验纹理拷贝区域是否越界
static bool gl_check_texture_region(rt_texture_copy_t const& region, rt_size_t copySize)
{
    auto* tex = gl_texture_native(region.texture);
    if (!tex || tex->handle == 0)
        return false;
    if (copySize.x == 0 || copySize.y == 0)
        return false;
    uint32_t width = std::max(1U, tex->width >> region.mipLevel);
    uint32_t height = std::max(1U, tex->height >> region.mipLevel);

    if ((uint64_t)region.origin.x + copySize.x > width)
        return false;
    if ((uint64_t)region.origin.y + copySize.y > height)
        return false;
    // 1D / 2D 没有深度维度，z 偏移必须为 0 且最多拷贝一层
    if ((tex->target == RT_TEXTURE_1D || tex->target == RT_TEXTURE_2D) &&
        (region.origin.z != 0 || copySize.z > 1))
        return false;
    if (tex->target == RT_TEXTURE_1D && (region.origin.y != 0 || copySize.y > 1))
        return false;
    if (tex->target == RT_TEXTURE_2D_ARRAY)
    {
        if ((uint64_t)region.origin.z + std::max(1U, copySize.z) > std::max(1U, tex->depth))
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
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    auto handle = opengl.passID + 1;
    auto& native = opengl.transferPasses[handle];
    pass.handle = handle;
    pass.native = &native;
    opengl.currentPassType = RT_MODULE_TRANSFER;
    opengl.currentTransferPass = &pass;
    opengl.passID = handle;
}

void gl_end_transfer(rt_pass_transfer_t& pass)
{
    if (opengl.currentTransferPass != &pass)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    opengl.transferPasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    opengl.currentPassType = RT_MODULE_NONE;
    opengl.currentTransferPass = nullptr;
}

void gl_copy_buffer(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (source.buffer.handle == 0 || destination.buffer.handle == 0 || copySize == 0)
        return;
    if (source.offset + copySize > source.buffer.size)
        return;
    if (destination.offset + copySize > destination.buffer.size)
        return;

    glCopyNamedBufferSubData(gl_buffer_name(source.buffer), gl_buffer_name(destination.buffer),
                             (GLintptr)source.offset, (GLintptr)destination.offset, (GLsizeiptr)copySize);
}

void gl_copy_buffer_data(rt_buffer_data_t source, rt_buffer_copy_t destination, size_t copySize)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (source.data == nullptr || destination.buffer.handle == 0 || copySize == 0)
        return;
    if (source.offset + copySize > source.size)
        return;
    if (destination.offset + copySize > destination.buffer.size)
        return;

    glNamedBufferSubData(gl_buffer_name(destination.buffer), (GLintptr)destination.offset, (GLsizeiptr)copySize,
                         source.data + source.offset);
}

void gl_copy_buffer_texture(rt_texture_copy_t source, rt_buffer_texel_t destination, rt_size_t copySize)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
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

    glBindBuffer(GL_PIXEL_PACK_BUFFER, gl_buffer_name(destination.buffer));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, (GLint)(destination.bytesPerRow / bytesPerPixel));
    glPixelStorei(GL_PACK_IMAGE_HEIGHT, (GLint)destination.rowsPerImage);
    glGetTextureSubImage(gl_texture_name(source.texture), (GLint)source.mipLevel,
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
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (copySize.x == 0 || copySize.y == 0)
        return;
    if (!gl_check_texture_region(source, copySize) || !gl_check_texture_region(destination, copySize))
        return;

    auto* srcTex = gl_texture_native(source.texture);
    auto* dstTex = gl_texture_native(destination.texture);
    if (!srcTex || !dstTex) return;
    GLsizei depth = (GLsizei)std::max(1U, copySize.z);
    glCopyImageSubData(gl_texture_name(source.texture), rt_to_gl_texture_target(srcTex->target), (GLint)source.mipLevel,
                       (GLint)source.origin.x, (GLint)source.origin.y, (GLint)source.origin.z,
                       gl_texture_name(destination.texture), rt_to_gl_texture_target(dstTex->target), (GLint)destination.mipLevel,
                       (GLint)destination.origin.x, (GLint)destination.origin.y, (GLint)destination.origin.z,
                       (GLsizei)copySize.x, (GLsizei)copySize.y, depth);

    if (dstTex->levels > 1) glGenerateTextureMipmap(gl_texture_name(destination.texture));
}

void gl_copy_texture_data(rt_texture_data_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
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
    auto* dstTex = gl_texture_native(destination.texture);
    rt_texture_target_t target = dstTex ? dstTex->target : RT_TEXTURE_2D;
    uint32_t levels = dstTex ? dstTex->levels : 1;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)(source.bytesPerRow / bytesPerPixel));
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, (GLint)source.rowsPerImage);
    if (target == RT_TEXTURE_1D)
    {
        glTextureSubImage1D(gl_texture_name(destination.texture), (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLsizei)copySize.x, format, type, data);

        if (levels > 1) glGenerateTextureMipmap(gl_texture_name(destination.texture));
    }
    else if (target == RT_TEXTURE_2D)
    {
        glTextureSubImage2D(gl_texture_name(destination.texture), (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLint)destination.origin.y,
                            (GLsizei)copySize.x, (GLsizei)copySize.y, format, type, data);

        if (levels > 1) glGenerateTextureMipmap(gl_texture_name(destination.texture));
    }
    else if (target == RT_TEXTURE_3D || target == RT_TEXTURE_2D_ARRAY)
    {
        glTextureSubImage3D(gl_texture_name(destination.texture), (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLint)destination.origin.y, (GLint)destination.origin.z,
                            (GLsizei)copySize.x, (GLsizei)copySize.y, (GLsizei)std::max(1U, copySize.z), format, type, data);

        if (levels > 1) glGenerateTextureMipmap(gl_texture_name(destination.texture));
    }
    else
    {
        fprintf(stderr, "Unsupported texture target");
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
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
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
    auto* dstTex = gl_texture_native(destination.texture);
    rt_texture_target_t target = dstTex ? dstTex->target : RT_TEXTURE_2D;
    uint32_t levels = dstTex ? dstTex->levels : 1;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_buffer_name(source.buffer));
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)(source.bytesPerRow / bytesPerPixel));
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, (GLint)source.rowsPerImage);
    if (target == RT_TEXTURE_1D)
    {
        glTextureSubImage1D(gl_texture_name(destination.texture), (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLsizei)copySize.x, format, type, data);

        if (levels > 1) glGenerateTextureMipmap(gl_texture_name(destination.texture));
    }
    else if (target == RT_TEXTURE_2D)
    {
        glTextureSubImage2D(gl_texture_name(destination.texture), (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLint)destination.origin.y,
                            (GLsizei)copySize.x, (GLsizei)copySize.y, format, type, data);

        if (levels > 1) glGenerateTextureMipmap(gl_texture_name(destination.texture));
    }
    else if (target == RT_TEXTURE_3D || target == RT_TEXTURE_2D_ARRAY)
    {
        glTextureSubImage3D(gl_texture_name(destination.texture), (GLint)destination.mipLevel,
                            (GLint)destination.origin.x, (GLint)destination.origin.y, (GLint)destination.origin.z,
                            (GLsizei)copySize.x, (GLsizei)copySize.y, (GLsizei)std::max(1U, copySize.z), format, type, data);

        if (levels > 1) glGenerateTextureMipmap(gl_texture_name(destination.texture));
    }
    else
    {
        fprintf(stderr, "Unsupported texture target");
        abort();
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

static GLsizei rt_index_size(rt_index_type_t type)
{
    switch (type)
    {
    case RT_INDEX_UINT16: return 2;
    case RT_INDEX_UINT32: return 4;
    default: return 4;
    }
}

rt_mesh_t gl_create_mesh(const float* vertices, // vec3
                                const float* normals, // vec3
                                const float* uvs, // vec2
                                size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    rt_mesh_t result = {};

    if (vertices)
        result.vertex[0] = gl_create_buffer({.size = vertex_count * 3 * sizeof(float), .usages = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = vertices,});

    if (normals)
        result.vertex[1] = gl_create_buffer({.size = vertex_count * 3 * sizeof(float), .usages = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = normals,});

    if (uvs)
        result.vertex[2] = gl_create_buffer({.size = vertex_count * 2 * sizeof(float), .usages = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = uvs,});

    if (indices)
        result.index = gl_create_buffer({.size = index_count * sizeof(uint32_t), .usages = RT_BUFFER_USAGE_INDEX | RT_BUFFER_USAGE_COPY_DST, .data = indices,});

    std::iota(result.location, result.location + std::size(result.location), 0);
    auto handle = opengl.meshID + 1;
    auto& native = opengl.meshes[handle];
    native.vertexCount = (uint32_t)vertex_count;
    native.indexCount = (uint32_t)index_count;
    opengl.meshID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void gl_destroy_mesh(rt_mesh_t& mesh)
{
    for (auto& vertex : mesh.vertex)
        gl_destroy_buffer(vertex);
    gl_destroy_buffer(mesh.index);
    opengl.meshes.erase(mesh.handle);
    mesh.handle = 0;
    mesh.native = nullptr;
}

void gl_draw_mesh(rt_mesh_t& mesh)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    rt_module_render_t const& module = gl_current_render_module();

    GLsizei vertex_count = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        GLsizei stride = rt_to_gl_vertex_stride(layout);
        if (stride == 0)
            continue;

        GLuint buffer = 0;
        for (uint32_t k = 0; k < std::size(mesh.vertex) && buffer == 0; ++k)
        {
            if (mesh.vertex[k].handle == 0)
                continue;
            for (auto& attrib : layout.attrib)
            {
                if (attrib.format == RT_VERTEX_NONE || mesh.location[k] != attrib.location)
                    continue;
                buffer = gl_buffer_name(mesh.vertex[k]);
                if (vertex_count == 0)
                    vertex_count = (GLsizei)(mesh.vertex[k].size / (size_t)stride);
                break;
            }
        }
        glBindVertexBuffer(i, buffer, 0, buffer ? stride : 0);
    }

    if (mesh.index.handle)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_buffer_name(mesh.index));
        GLsizei index_stride = rt_index_size(module.index_type);
        auto index_count = (GLsizei)(mesh.index.size / (size_t)index_stride);
        glDrawElements(rt_to_gl_primitive(module.primitive), index_count, rt_to_gl_index_type(module.index_type), (void*)0);
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
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    rt_module_render_t const& module = gl_current_render_module();

    GLsizei vertex_count = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        GLsizei stride = rt_to_gl_vertex_stride(layout);
        if (stride == 0)
            continue;

        GLuint buffer = 0;
        for (uint32_t k = 0; k < std::size(mesh.vertex) && buffer == 0; ++k)
        {
            if (mesh.vertex[k].handle == 0)
                continue;
            for (auto& attrib : layout.attrib)
            {
                if (attrib.format == RT_VERTEX_NONE || mesh.location[k] != attrib.location)
                    continue;
                buffer = gl_buffer_name(mesh.vertex[k]);
                if (vertex_count == 0)
                    vertex_count = (GLsizei)(mesh.vertex[k].size / (size_t)stride);
                break;
            }
        }
        glBindVertexBuffer(i, buffer, 0, buffer ? stride : 0);
    }

    if (mesh.index.handle)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_buffer_name(mesh.index));
        GLsizei index_stride = rt_index_size(module.index_type);
        auto index_count = (GLsizei)(mesh.index.size / (size_t)index_stride);
        glDrawElementsInstanced(rt_to_gl_primitive(module.primitive), index_count, rt_to_gl_index_type(module.index_type), (void*)0, (int32_t)count);
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
        result.vertex[0] = gl_create_buffer({.size = vertex_count * 4 * sizeof(float), .usages = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = vertices,});

    if (normals)
        result.vertex[1] = gl_create_buffer({.size = vertex_count * 4 * sizeof(float), .usages = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = normals,});

    if (uvs)
        result.vertex[2] = gl_create_buffer({.size = vertex_count * 2 * sizeof(float), .usages = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = uvs,});

    if (indices)
        result.index = gl_create_buffer({.size = index_count * sizeof(uint32_t), .usages = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = indices,});

    std::iota(result.location, result.location + std::size(result.location), 0);
    auto handle = opengl.meshletID + 1;
    auto& native = opengl.meshlets[handle];
    native.vertexCount = (uint32_t)vertex_count;
    native.indexCount = (uint32_t)index_count;
    opengl.meshletID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void gl_destroy_meshlet(rt_meshlet_t& meshlet)
{
    for (auto& vertex : meshlet.vertex)
        gl_destroy_buffer(vertex);
    gl_destroy_buffer(meshlet.index);
    opengl.meshlets.erase(meshlet.handle);
    meshlet.handle = 0;
    meshlet.native = nullptr;
}

void gl_draw_meshlet(rt_meshlet_t& meshlet)
{
    if (opengl.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (opengl.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    rt_module_render_t const& module = gl_current_render_module();

    uint32_t index_binding = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        for (auto& attrib : module.vertex[i].attrib)
        {
            if (attrib.format == RT_VERTEX_NONE)
                continue;

            for (uint32_t k = 0; k < std::size(meshlet.vertex); ++k)
            {
                if (meshlet.vertex[k].handle == 0 || meshlet.location[k] != attrib.location)
                    continue;
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, attrib.location, gl_buffer_name(meshlet.vertex[k]));
                break;
            }
            if (attrib.location + 1 > index_binding)
                index_binding = attrib.location + 1;
        }
    }

    if (meshlet.index.handle)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index_binding, gl_buffer_name(meshlet.index));
        GLsizei index_stride = rt_index_size(module.index_type);
        auto index_count = (GLsizei)(meshlet.index.size / (size_t)index_stride);
        glDrawMeshTasksNV(0, index_count / 3);
    }
}

void gl_submit()
{
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
    static auto layout = gl_create_layout({.entry = {{.binding = 0, .stages = RT_STAGE_FRAGMENT, .type = RT_BINDING_TEXTURE}}});
    static auto module = gl_create_module_render({.layout = layout, .vshader = {.code = VS}, .fshader = {.code = FS}, .vertex = {rt_vertex_vertex, {}, rt_vertex_uv,},});
    rt_pass_render_t pass = {.screen = {.color = { .clear = true, .value = clear,}, .depth = {.clear = true,}, .stencil = {.clear = true,}}};
    gl_begin_render(pass);
    static rt_binding_t binding = {};
    static uint32_t boundView = 0;
    if (texture.handle && texture.default_view.handle != boundView)
    {
        if (binding.handle)
            gl_destroy_binding(binding);
        binding = gl_create_binding({
            .layout = layout,
            .entry = {
                {.binding = 0, .texture = texture.default_view},
            },
        });
        boundView = texture.default_view.handle;
    }
    gl_bind_module_render(module);
    gl_set_viewport(0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f);
    if (texture.handle && binding.handle)
    {
        gl_bind_binding(binding);
        static auto mesh = gl_create_mesh_screen();
        gl_draw_mesh(mesh);
    }
    gl_end_render(pass);
}

#endif
