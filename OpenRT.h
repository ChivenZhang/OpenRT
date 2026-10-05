#pragma once
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
#include <cstddef>
#include <cstdint>

#ifndef OPENRT_API
#  ifdef OPENRT_STATIC
#    define OPENRT_API extern "C"
#  else
#    ifdef _WIN32
#      ifdef OPENRT_EXPORTS
#        define OPENRT_API extern "C" __declspec(dllexport)
#      else
#        define OPENRT_API extern "C" __declspec(dllimport)
#      endif
#    else
#      define OPENRT_API extern "C" __attribute__((visibility("default")))
#    endif
#  endif
#endif

#define RT_MAX_COLOR_TEXTURE_NUM 8
#define RT_MAX_VERTEX_BUFFER_NUM 8
#define RT_MAX_VERTEX_ATTRIB_NUM 16
#define RT_MAX_BINDING_HANDLE_NUM 16
#define RT_PI 3.14159265358979323846    // pi
#define RT_PI_2 1.57079632679489661923  // pi/2
#define RT_PI_4 0.785398163397448309616 // pi/4
#define RT_1_PI 0.318309886183790671538 // 1/pi
#define RT_2_PI 0.636619772367581343076 // 2/pi

// ====================================================================

enum rt_buffer_usage_t : uint32_t
{
    RT_BUFFER_USAGE_MAP_READ      = 0x0001,
    RT_BUFFER_USAGE_MAP_WRITE     = 0x0002,
    RT_BUFFER_USAGE_COPY_SRC      = 0x0004,
    RT_BUFFER_USAGE_COPY_DST      = 0x0008,
    RT_BUFFER_USAGE_INDEX         = 0x0010,
    RT_BUFFER_USAGE_VERTEX        = 0x0020,
    RT_BUFFER_USAGE_UNIFORM       = 0x0040,
    RT_BUFFER_USAGE_STORAGE       = 0x0080,
    RT_BUFFER_USAGE_INDIRECT      = 0x0100,
    RT_BUFFER_USAGE_QUERY_RESOLVE = 0x0200,
};
using rt_buffer_usages_t = uint32_t;

enum rt_texture_usage_t : uint32_t
{
    RT_TEXTURE_USAGE_COPY_SRC          = 0x0001,
    RT_TEXTURE_USAGE_COPY_DST          = 0x0002,
    RT_TEXTURE_USAGE_TEXTURE_BINDING   = 0x0004,
    RT_TEXTURE_USAGE_STORAGE_BINDING   = 0x0008,
    RT_TEXTURE_USAGE_RENDER_ATTACHMENT = 0x0010,
};
using rt_texture_usages_t = uint32_t;

enum rt_buffer_target_t : uint32_t
{
    RT_UNIFORM_BUFFER = 0,
    RT_SHADER_STORAGE_BUFFER,
};

enum rt_texture_target_t : uint32_t
{
    RT_TEXTURE_1D = 0,
    RT_TEXTURE_2D,
    RT_TEXTURE_3D,
    RT_TEXTURE_2D_ARRAY,
    RT_TEXTURE_2D_MULTISAMPLE,
};

enum rt_texture_sample_t : uint32_t
{
    RT_TEXTURE_SAMPLE_1X = 1,
    RT_TEXTURE_SAMPLE_4X = 4,
};

enum rt_texture_format_t : uint32_t
{
    RT_TEXTURE_NONE = 0,
    RT_TEXTURE_R8UNORM,
    RT_TEXTURE_R8SNORM,
    RT_TEXTURE_R8UINT,
    RT_TEXTURE_R8SINT,
    RT_TEXTURE_R16UNORM,
    RT_TEXTURE_R16SNORM,
    RT_TEXTURE_R16UINT,
    RT_TEXTURE_R16SINT,
    RT_TEXTURE_R16FLOAT,
    RT_TEXTURE_RG8UNORM,
    RT_TEXTURE_RG8SNORM,
    RT_TEXTURE_RG8UINT,
    RT_TEXTURE_RG8SINT,
    RT_TEXTURE_R32UINT,
    RT_TEXTURE_R32SINT,
    RT_TEXTURE_R32FLOAT,
    RT_TEXTURE_RG16UNORM,
    RT_TEXTURE_RG16SNORM,
    RT_TEXTURE_RG16UINT,
    RT_TEXTURE_RG16SINT,
    RT_TEXTURE_RG16FLOAT,
    RT_TEXTURE_RGBA8UNORM,
    RT_TEXTURE_RGBA8UNORM_SRGB,
    RT_TEXTURE_RGBA8SNORM,
    RT_TEXTURE_RGBA8UINT,
    RT_TEXTURE_RGBA8SINT,
    RT_TEXTURE_BGRA8UNORM,
    RT_TEXTURE_BGRA8UNORM_SRGB,
    RT_TEXTURE_RGB10A2UINT,
    RT_TEXTURE_RGB10A2UNORM,
    RT_TEXTURE_RG11B10UFLOAT,
    RT_TEXTURE_RGB9E5UFLOAT,
    RT_TEXTURE_RG32UINT,
    RT_TEXTURE_RG32SINT,
    RT_TEXTURE_RG32FLOAT,
    RT_TEXTURE_RGBA16UNORM,
    RT_TEXTURE_RGBA16SNORM,
    RT_TEXTURE_RGBA16UINT,
    RT_TEXTURE_RGBA16SINT,
    RT_TEXTURE_RGBA16FLOAT,
    RT_TEXTURE_RGBA32UINT,
    RT_TEXTURE_RGBA32SINT,
    RT_TEXTURE_RGBA32FLOAT,
    RT_TEXTURE_STENCIL8,
    RT_TEXTURE_DEPTH16UNORM,
    RT_TEXTURE_DEPTH24PLUS,
    RT_TEXTURE_DEPTH24PLUS_STENCIL8,
    RT_TEXTURE_DEPTH32FLOAT,
    RT_TEXTURE_DEPTH32FLOAT_STENCIL8,
};

enum rt_texture_aspect_t : uint32_t
{
    RT_TEXTURE_ASPECT_ALL = 0,
    RT_TEXTURE_ASPECT_STENCIL,
    RT_TEXTURE_ASPECT_DEPTH,
};

inline bool rt_texture_has_depth(rt_texture_format_t format)
{
    switch (format)
    {
        case RT_TEXTURE_DEPTH16UNORM:
        case RT_TEXTURE_DEPTH24PLUS:
        case RT_TEXTURE_DEPTH24PLUS_STENCIL8:
        case RT_TEXTURE_DEPTH32FLOAT:
        case RT_TEXTURE_DEPTH32FLOAT_STENCIL8:
            return true;
        default:
            return false;
    }
}

inline bool rt_texture_has_stencil(rt_texture_format_t format)
{
    switch (format)
    {
        case RT_TEXTURE_STENCIL8:
        case RT_TEXTURE_DEPTH24PLUS_STENCIL8:
        case RT_TEXTURE_DEPTH32FLOAT_STENCIL8:
            return true;
        default:
            return false;
    }
}

enum rt_index_type_t : uint32_t
{
    RT_INDEX_UINT16 = 0,
    RT_INDEX_UINT32,
};

enum rt_vertex_format_t : uint32_t
{
    RT_VERTEX_NONE = 0,
    RT_VERTEX_UINT8,
    RT_VERTEX_UINT8X2,
    RT_VERTEX_UINT8X4,
    RT_VERTEX_SINT8,
    RT_VERTEX_SINT8X2,
    RT_VERTEX_SINT8X4,
    RT_VERTEX_UNORM8,
    RT_VERTEX_UNORM8X2,
    RT_VERTEX_UNORM8X4,
    RT_VERTEX_SNORM8,
    RT_VERTEX_SNORM8X2,
    RT_VERTEX_SNORM8X4,
    RT_VERTEX_UINT16,
    RT_VERTEX_UINT16X2,
    RT_VERTEX_UINT16X4,
    RT_VERTEX_SINT16,
    RT_VERTEX_SINT16X2,
    RT_VERTEX_SINT16X4,
    RT_VERTEX_UNORM16,
    RT_VERTEX_UNORM16X2,
    RT_VERTEX_UNORM16X4,
    RT_VERTEX_SNORM16,
    RT_VERTEX_SNORM16X2,
    RT_VERTEX_SNORM16X4,
    RT_VERTEX_FLOAT16,
    RT_VERTEX_FLOAT16X2,
    RT_VERTEX_FLOAT16X4,
    RT_VERTEX_FLOAT32,
    RT_VERTEX_FLOAT32X2,
    RT_VERTEX_FLOAT32X3,
    RT_VERTEX_FLOAT32X4,
    RT_VERTEX_UINT32,
    RT_VERTEX_UINT32X2,
    RT_VERTEX_UINT32X3,
    RT_VERTEX_UINT32X4,
    RT_VERTEX_SINT32,
    RT_VERTEX_SINT32X2,
    RT_VERTEX_SINT32X3,
    RT_VERTEX_SINT32X4,
};

enum rt_filter_t : uint32_t
{
    RT_NEAREST = 0,
    RT_LINEAR,
    RT_NEAREST_MIPMAP_NEAREST,
    RT_LINEAR_MIPMAP_NEAREST,
    RT_NEAREST_MIPMAP_LINEAR,
    RT_LINEAR_MIPMAP_LINEAR,
};

enum rt_address_t : uint32_t
{
    RT_REPEAT = 0,
    RT_CLAMP_TO_EDGE,
    RT_CLAMP_TO_BORDER,
    RT_MIRRORED_REPEAT,
    RT_MIRROR_CLAMP_TO_EDGE,
};

enum rt_access_t : uint32_t
{
    RT_READ_ONLY = 0,
    RT_WRITE_ONLY,
    RT_READ_WRITE,
};

enum rt_binding_type_t : uint32_t
{
    RT_BINDING_NONE = 0,
    RT_BINDING_UNIFORM_BUFFER,
    RT_BINDING_STORAGE_BUFFER,
    RT_BINDING_SAMPLER,
    RT_BINDING_TEXTURE,
    RT_BINDING_STORAGE_TEXTURE,
};

enum rt_blend_op_t : uint32_t
{
    RT_FUNC_ADD = 0,
    RT_MIN,
    RT_MAX,
    RT_FUNC_SUBTRACT,
    RT_FUNC_REVERSE_SUBTRACT,
};

enum rt_blend_factor_t : uint32_t
{
    RT_BLEND_ZERO = 0,
    RT_BLEND_ONE,
    RT_BLEND_SRC_COLOR,
    RT_BLEND_ONE_MINUS_SRC_COLOR,
    RT_BLEND_SRC_ALPHA,
    RT_BLEND_ONE_MINUS_SRC_ALPHA,
    RT_BLEND_DST_ALPHA,
    RT_BLEND_ONE_MINUS_DST_ALPHA,
    RT_BLEND_DST_COLOR,
    RT_BLEND_ONE_MINUS_DST_COLOR,
    RT_BLEND_SRC_ALPHA_SATURATE,
    RT_BLEND_CONSTANT_COLOR,
    RT_BLEND_ONE_MINUS_CONSTANT_COLOR,
    RT_BLEND_CONSTANT_ALPHA,
    RT_BLEND_ONE_MINUS_CONSTANT_ALPHA,
};

enum rt_compare_op_t : uint32_t
{
    RT_NEVER = 0,
    RT_LESS,
    RT_EQUAL,
    RT_LEQUAL,
    RT_GREATER,
    RT_NOTEQUAL,
    RT_GEQUAL,
    RT_ALWAYS,
};

enum rt_stencil_op_t : uint32_t
{
    RT_STENCIL_ZERO = 0,
    RT_STENCIL_INVERT,
    RT_STENCIL_KEEP,
    RT_STENCIL_REPLACE,
    RT_STENCIL_INCR,
    RT_STENCIL_DECR,
    RT_STENCIL_INCR_WRAP,
    RT_STENCIL_DECR_WRAP,
};

enum rt_cull_mode_t : uint32_t
{
    RT_CULL_NONE = 0,
    RT_CULL_FRONT,
    RT_CULL_BACK,
    RT_CULL_FRONT_AND_BACK,
};

enum rt_wind_mode_t : uint32_t
{
    RT_CW = 0,
    RT_CCW,
};

enum rt_fill_mode_t : uint32_t
{
    RT_POINT = 0,
    RT_LINE,
    RT_FILL,
};

enum rt_primitive_t : uint32_t
{
    RT_POINTS = 0,
    RT_LINES,
    RT_LINE_STRIP,
    RT_TRIANGLES,
    RT_TRIANGLE_STRIP,
};

enum rt_module_type_t : uint32_t
{
    RT_MODULE_NONE = 0,
    RT_MODULE_RENDER,
    RT_MODULE_COMPUTE,
    RT_MODULE_MESHLET,
    RT_MODULE_TRANSFER,
};

// ====================================================================

enum rt_backend_t : uint32_t
{
    RT_OPENGL = 0,
    RT_VULKAN,
    RT_DIRECTX,
    RT_METAL,
    RT_WEBGPU,
};

struct rt_load_info_t
{
    rt_backend_t backend = RT_OPENGL;   // RT_OPENGL / RT_VULKAN / RT_DIRECTX / RT_METAL / RT_WEBGPU
    union
    {
        struct
        {
        } opengl;
        struct
        {
            void* instance = nullptr;
            void* physical = nullptr;
            void* device = nullptr;
            void* queue = nullptr;
            void* cmdbuf = nullptr;
            uint32_t family = 0;
        } vulkan;
        struct
        {
            void* device = nullptr;
            void* queue = nullptr;
        } directx;
        struct
        {
            void* device = nullptr;
            void* queue = nullptr;
        } metal;
        struct
        {
            void* device = nullptr;
            void* queue = nullptr;
        } webgpu;
    };
};

// ====================================================================

struct rt_buffer_t
{
    uint32_t handle = 0;
    size_t size = 0;
    rt_buffer_usages_t usage = 0;
    void* native = nullptr;
};

struct rt_buffer_info_t
{
    size_t size = 0;                    // 缓冲区大小（字节）
    rt_buffer_usages_t usage = RT_BUFFER_USAGE_MAP_READ | RT_BUFFER_USAGE_MAP_WRITE | RT_BUFFER_USAGE_COPY_SRC | RT_BUFFER_USAGE_COPY_DST; // rt_buffer_usage_t
    const void* data = nullptr;         // 初始数据指针，可为 nullptr
};

struct rt_buffer_bind_t
{
    uint32_t binding = 0;
};

// ====================================================================

struct rt_texture_view_t
{
    uint32_t handle = 0;
    rt_texture_target_t target = RT_TEXTURE_2D;
    rt_texture_format_t format = RT_TEXTURE_NONE;
    rt_texture_aspect_t aspect = RT_TEXTURE_ASPECT_ALL;
    rt_texture_usages_t usage = 0;
    uint32_t base_layer = 0;
    uint32_t layer_count = 1;
    uint32_t base_level = 0;
    uint32_t level_count = 1;
    void* native = nullptr;
};

struct rt_texture_view_info_t
{
    rt_texture_target_t target = RT_TEXTURE_2D; // RT_TEXTURE_1D / RT_TEXTURE_2D / RT_TEXTURE_3D / RT_TEXTURE_2D_ARRAY / RT_TEXTURE_2D_MULTISAMPLE
    rt_texture_format_t format = RT_TEXTURE_RGBA8UNORM;
    rt_texture_aspect_t aspect = RT_TEXTURE_ASPECT_ALL; // RT_TEXTURE_ASPECT_ALL / RT_TEXTURE_ASPECT_STENCIL / RT_TEXTURE_ASPECT_DEPTH
    rt_texture_usages_t usage = RT_TEXTURE_USAGE_COPY_SRC | RT_TEXTURE_USAGE_COPY_DST | RT_TEXTURE_USAGE_TEXTURE_BINDING | RT_TEXTURE_USAGE_STORAGE_BINDING | RT_TEXTURE_USAGE_RENDER_ATTACHMENT; // rt_texture_usage_t
    uint32_t base_layer = 0;
    uint32_t layer_count = 1;
    uint32_t base_level = 0;
    uint32_t level_count = 1;
};

struct rt_texture_view_bind_t
{
    uint32_t binding = 0;
};

// ====================================================================

struct rt_texture_t
{
    uint32_t handle = 0;
    uint32_t width = 0, height = 0, depth = 1;
    rt_texture_target_t target = RT_TEXTURE_2D;
    rt_texture_format_t format = RT_TEXTURE_NONE;
    rt_texture_sample_t samples = RT_TEXTURE_SAMPLE_1X;
    rt_texture_usages_t usage = 0;
    uint32_t mipmaps = 0;
    rt_texture_view_t default_view = {};
    void* native = nullptr;
};

struct rt_texture_info_t
{
    uint32_t width = 0, height = 0, depth = 1;
    rt_texture_target_t target = RT_TEXTURE_2D;          // RT_TEXTURE_1D / RT_TEXTURE_2D / RT_TEXTURE_3D / RT_TEXTURE_2D_ARRAY / RT_TEXTURE_2D_MULTISAMPLE
    rt_texture_format_t format = RT_TEXTURE_RGBA8UNORM;
    rt_texture_usages_t usage = RT_TEXTURE_USAGE_COPY_SRC | RT_TEXTURE_USAGE_COPY_DST | RT_TEXTURE_USAGE_TEXTURE_BINDING | RT_TEXTURE_USAGE_STORAGE_BINDING | RT_TEXTURE_USAGE_RENDER_ATTACHMENT; // rt_texture_usage_t
    rt_filter_t min_filter = RT_LINEAR_MIPMAP_LINEAR;  // RT_NEAREST / RT_LINEAR / RT_NEAREST_MIPMAP_NEAREST / RT_LINEAR_MIPMAP_NEAREST / RT_NEAREST_MIPMAP_LINEAR / RT_LINEAR_MIPMAP_LINEAR
    rt_filter_t mag_filter = RT_LINEAR;                // RT_NEAREST / RT_LINEAR
    rt_address_t address_u = RT_REPEAT;                    // RT_REPEAT / RT_MIRRORED_REPEAT / RT_CLAMP_TO_EDGE / RT_CLAMP_TO_BORDER / RT_MIRROR_CLAMP_TO_EDGE
    rt_address_t address_v = RT_REPEAT;                    // RT_REPEAT / RT_MIRRORED_REPEAT / RT_CLAMP_TO_EDGE / RT_CLAMP_TO_BORDER / RT_MIRROR_CLAMP_TO_EDGE
    rt_address_t address_w = RT_REPEAT;                    // RT_REPEAT / RT_MIRRORED_REPEAT / RT_CLAMP_TO_EDGE / RT_CLAMP_TO_BORDER / RT_MIRROR_CLAMP_TO_EDGE
    uint32_t mipmaps = 0;   // 0:auto generate
    rt_texture_sample_t samples = RT_TEXTURE_SAMPLE_1X; // RT_TEXTURE_SAMPLE_1X / RT_TEXTURE_SAMPLE_4X
    float border[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const void* data = nullptr;
};

struct rt_texture_bind_t
{
    uint32_t binding = 0;
};

struct rt_texture_storage_bind_t
{
    uint32_t binding = 0;
    uint32_t base_layer = 0;
    uint32_t layer_count = 1;
    uint32_t base_level = 0;
    uint32_t level_count = 1;
    rt_access_t access = RT_WRITE_ONLY; // RT_WRITE_ONLY / RT_READ_ONLY / RT_READ_WRITE
};

// ====================================================================

struct rt_sampler_t
{
    uint32_t handle = 0;
    void* native = nullptr;
};

struct rt_sampler_info_t
{
    rt_filter_t min_filter = RT_LINEAR_MIPMAP_LINEAR;  // RT_NEAREST / RT_LINEAR / RT_NEAREST_MIPMAP_NEAREST / RT_LINEAR_MIPMAP_NEAREST / RT_NEAREST_MIPMAP_LINEAR / RT_LINEAR_MIPMAP_LINEAR
    rt_filter_t mag_filter = RT_LINEAR;                // RT_NEAREST / RT_LINEAR
    rt_address_t address_u = RT_REPEAT;                    // RT_REPEAT / RT_MIRRORED_REPEAT / RT_CLAMP_TO_EDGE / RT_CLAMP_TO_BORDER / RT_MIRROR_CLAMP_TO_EDGE
    rt_address_t address_v = RT_REPEAT;                    // RT_REPEAT / RT_MIRRORED_REPEAT / RT_CLAMP_TO_EDGE / RT_CLAMP_TO_BORDER / RT_MIRROR_CLAMP_TO_EDGE
    rt_address_t address_w = RT_REPEAT;                    // RT_REPEAT / RT_MIRRORED_REPEAT / RT_CLAMP_TO_EDGE / RT_CLAMP_TO_BORDER / RT_MIRROR_CLAMP_TO_EDGE
};

struct rt_sampler_bind_t
{
    uint32_t binding = 0;
};

// ====================================================================

struct rt_binding_t
{
    uint32_t binding = 0;
    rt_binding_type_t type = {};  // RT_BINDING_UNIFORM_BUFFER / RT_BINDING_STORAGE_BUFFER / RT_BINDING_TEXTURE / RT_BINDING_STORAGE_TEXTURE / RT_BINDING_SAMPLER
};

struct rt_module_compute_info_t
{
    struct
    {
        const char* code = nullptr;
        uint32_t size = 0;
        const char* entry = "main";
    } cshader;

    rt_binding_t binding[RT_MAX_BINDING_HANDLE_NUM];
};

struct rt_module_compute_t
{
    uint32_t handle = 0;

    rt_binding_t binding[RT_MAX_BINDING_HANDLE_NUM];
    
    void* native = nullptr;
};

struct rt_vertex_attrib_t
{
    uint32_t location = 0;
    uint32_t offset = 0;
    rt_vertex_format_t format = RT_VERTEX_NONE;
};

struct rt_vertex_t
{
    uint32_t stride = 0;
    bool instance = false;
    rt_vertex_attrib_t attrib[RT_MAX_VERTEX_ATTRIB_NUM];
};
inline rt_vertex_t rt_vertex_vertex{.stride = 12, .instance = false, .attrib = {{.location = 0, .offset = 0, .format = RT_VERTEX_FLOAT32X3}}};
inline rt_vertex_t rt_vertex_normal{.stride = 12, .instance = false, .attrib = {{.location = 1, .offset = 0, .format = RT_VERTEX_FLOAT32X3}}};
inline rt_vertex_t rt_vertex_uv{.stride = 8, .instance = false, .attrib = {{.location = 2, .offset = 0, .format = RT_VERTEX_FLOAT32X2}}};

struct rt_module_render_info_t
{
    struct
    {
        const char* code = nullptr;
        uint32_t size = 0;
        const char* entry = "main";
    } vshader, tshader, mshader, fshader;

    struct
    {
        rt_texture_format_t format = RT_TEXTURE_NONE;
        struct
        {
            bool r = true, g = true, b = true, a = true;
        } write;
        struct
        {
            rt_blend_op_t func = RT_FUNC_ADD; // RT_FUNC_ADD / RT_FUNC_SUBTRACT / RT_FUNC_REVERSE_SUBTRACT / RT_MIN / RT_MAX
            rt_blend_factor_t src = RT_BLEND_ONE; // RT_BLEND_ZERO / RT_BLEND_ONE / RT_BLEND_SRC_COLOR / RT_BLEND_ONE_MINUS_SRC_COLOR / RT_BLEND_DST_COLOR / RT_BLEND_ONE_MINUS_DST_COLOR / RT_BLEND_SRC_ALPHA / RT_BLEND_ONE_MINUS_SRC_ALPHA / RT_BLEND_DST_ALPHA / RT_BLEND_ONE_MINUS_DST_ALPHA / RT_BLEND_CONSTANT_COLOR / RT_BLEND_ONE_MINUS_CONSTANT_COLOR / RT_BLEND_CONSTANT_ALPHA / RT_BLEND_ONE_MINUS_CONSTANT_ALPHA / RT_BLEND_SRC_ALPHA_SATURATE
            rt_blend_factor_t dst = RT_BLEND_ZERO; // RT_BLEND_ZERO / RT_BLEND_ONE / RT_BLEND_SRC_COLOR / RT_BLEND_ONE_MINUS_SRC_COLOR / RT_BLEND_DST_COLOR / RT_BLEND_ONE_MINUS_DST_COLOR / RT_BLEND_SRC_ALPHA / RT_BLEND_ONE_MINUS_SRC_ALPHA / RT_BLEND_DST_ALPHA / RT_BLEND_ONE_MINUS_DST_ALPHA / RT_BLEND_CONSTANT_COLOR / RT_BLEND_ONE_MINUS_CONSTANT_COLOR / RT_BLEND_CONSTANT_ALPHA / RT_BLEND_ONE_MINUS_CONSTANT_ALPHA / RT_BLEND_SRC_ALPHA_SATURATE
        } color, alpha;
    } colors[RT_MAX_COLOR_TEXTURE_NUM];
    struct
    {
        bool write = false;
        float bias = 0.0f;
        float biasSlope = 0.0f;
        float biasClamp = 0.0f;
        rt_compare_op_t func = RT_ALWAYS; // RT_NEVER / RT_LESS / RT_EQUAL / RT_LEQUAL / RT_GREATER / RT_NOTEQUAL / RT_GEQUAL / RT_ALWAYS
    } depth;
    struct
    {
        uint32_t read = 0xFFFFFFFF;
        uint32_t write = 0xFFFFFFFF;
        struct
        {
            rt_compare_op_t func = RT_ALWAYS; // RT_NEVER / RT_LESS / RT_EQUAL / RT_LEQUAL / RT_GREATER / RT_NOTEQUAL / RT_GEQUAL / RT_ALWAYS
            rt_stencil_op_t sfail = RT_STENCIL_KEEP; // RT_STENCIL_KEEP / RT_STENCIL_ZERO / RT_STENCIL_REPLACE / RT_STENCIL_INCR / RT_STENCIL_INCR_WRAP / RT_STENCIL_DECR / RT_STENCIL_DECR_WRAP / RT_STENCIL_INVERT
            rt_stencil_op_t zfail = RT_STENCIL_KEEP; // RT_STENCIL_KEEP / RT_STENCIL_ZERO / RT_STENCIL_REPLACE / RT_STENCIL_INCR / RT_STENCIL_INCR_WRAP / RT_STENCIL_DECR / RT_STENCIL_DECR_WRAP / RT_STENCIL_INVERT
            rt_stencil_op_t zpass = RT_STENCIL_KEEP; // RT_STENCIL_KEEP / RT_STENCIL_ZERO / RT_STENCIL_REPLACE / RT_STENCIL_INCR / RT_STENCIL_INCR_WRAP / RT_STENCIL_DECR / RT_STENCIL_DECR_WRAP / RT_STENCIL_INVERT
        } back, front;
    } stencil;

    rt_index_type_t index_type = RT_INDEX_UINT32; // RT_INDEX_UINT16 / RT_INDEX_UINT32
    rt_vertex_t vertex[RT_MAX_VERTEX_BUFFER_NUM];
    rt_binding_t binding[RT_MAX_BINDING_HANDLE_NUM];

    rt_cull_mode_t cull_mode = RT_CULL_BACK; // RT_CULL_NONE / RT_CULL_FRONT / RT_CULL_BACK / RT_CULL_FRONT_AND_BACK
    rt_wind_mode_t wind_mode = RT_CCW; // RT_CW / RT_CCW
    rt_fill_mode_t fill_mode = RT_FILL; // RT_POINT / RT_LINE / RT_FILL
    rt_primitive_t primitive = RT_TRIANGLES; // RT_POINTS / RT_LINES / RT_LINE_STRIP / RT_TRIANGLES / RT_TRIANGLE_STRIP
};

struct rt_module_render_t
{
    uint32_t handle = 0;

    struct
    {
        rt_texture_format_t format = RT_TEXTURE_NONE;
        struct
        {
            bool r = true, g = true, b = true, a = true;
        } write;
        struct
        {
            rt_blend_op_t func = RT_FUNC_ADD;
            rt_blend_factor_t src = RT_BLEND_ONE;
            rt_blend_factor_t dst = RT_BLEND_ZERO;
        } color, alpha;
    } colors[RT_MAX_COLOR_TEXTURE_NUM];
    struct
    {
        bool write = false;
        float bias = 0.0f;
        float biasSlope = 0.0f;
        float biasClamp = 0.0f;
        rt_compare_op_t func = RT_ALWAYS;
    } depth;
    struct
    {
        uint32_t read = 0xFFFFFFFF;
        uint32_t write = 0xFFFFFFFF;
        struct
        {
            rt_compare_op_t func = RT_ALWAYS;
            rt_stencil_op_t sfail = RT_STENCIL_KEEP;
            rt_stencil_op_t zfail = RT_STENCIL_KEEP;
            rt_stencil_op_t zpass = RT_STENCIL_KEEP;
        } back, front;
    } stencil;

    rt_index_type_t index_type = RT_INDEX_UINT32;
    rt_vertex_t vertex[RT_MAX_VERTEX_BUFFER_NUM];
    rt_binding_t binding[RT_MAX_BINDING_HANDLE_NUM];

    rt_cull_mode_t cull_mode = RT_CULL_BACK;
    rt_wind_mode_t wind_mode = RT_CCW;
    rt_fill_mode_t fill_mode = RT_FILL;
    rt_primitive_t primitive = RT_TRIANGLES;

    void* native = nullptr;
};

// ====================================================================

struct rt_pass_compute_t
{
    uint32_t handle = 0;
    void* native = nullptr;
};

struct rt_color_t
{
    float r = 0, g = 0, b = 0, a = 0;
};

struct rt_pass_render_t
{
    uint32_t handle = 0;

    struct
    {
        rt_texture_view_t texture_view = {};
        bool clear = false;
        rt_color_t value;
    } colors[RT_MAX_COLOR_TEXTURE_NUM];
    struct
    {
        rt_texture_view_t texture_view = {};
        bool clear = false;
        float value = 1.0f;
    } depth;
    struct
    {
        bool clear = false;
        int32_t value = -1;
        int32_t refer = 0;
    } stencil;

    // Screen Mode

    struct
    {
        struct
        {
            bool clear = false;
            rt_color_t value;
            struct
            {
                rt_blend_op_t func = RT_FUNC_ADD; // RT_FUNC_ADD / RT_FUNC_SUBTRACT / RT_FUNC_REVERSE_SUBTRACT / RT_MIN / RT_MAX
                rt_blend_factor_t src = RT_BLEND_ONE; // RT_BLEND_ZERO / RT_BLEND_ONE / RT_BLEND_SRC_COLOR / RT_BLEND_ONE_MINUS_SRC_COLOR / RT_BLEND_DST_COLOR / RT_BLEND_ONE_MINUS_DST_COLOR / RT_BLEND_SRC_ALPHA / RT_BLEND_ONE_MINUS_SRC_ALPHA / RT_BLEND_DST_ALPHA / RT_BLEND_ONE_MINUS_DST_ALPHA / RT_BLEND_CONSTANT_COLOR / RT_BLEND_ONE_MINUS_CONSTANT_COLOR / RT_BLEND_CONSTANT_ALPHA / RT_BLEND_ONE_MINUS_CONSTANT_ALPHA / RT_BLEND_SRC_ALPHA_SATURATE
                rt_blend_factor_t dst = RT_BLEND_ZERO; // RT_BLEND_ZERO / RT_BLEND_ONE / RT_BLEND_SRC_COLOR / RT_BLEND_ONE_MINUS_SRC_COLOR / RT_BLEND_DST_COLOR / RT_BLEND_ONE_MINUS_DST_COLOR / RT_BLEND_SRC_ALPHA / RT_BLEND_ONE_MINUS_SRC_ALPHA / RT_BLEND_DST_ALPHA / RT_BLEND_ONE_MINUS_DST_ALPHA / RT_BLEND_CONSTANT_COLOR / RT_BLEND_ONE_MINUS_CONSTANT_COLOR / RT_BLEND_CONSTANT_ALPHA / RT_BLEND_ONE_MINUS_CONSTANT_ALPHA / RT_BLEND_SRC_ALPHA_SATURATE
            } blend;
        } color;
        struct
        {
            bool clear = false;
            bool write = false;
            float value = 1.0f;
            float bias = 0.0f;
            float biasSlope = 0.0f;
            float biasClamp = 0.0f;
            rt_compare_op_t func = RT_ALWAYS; // RT_NEVER / RT_LESS / RT_EQUAL / RT_LEQUAL / RT_GREATER / RT_NOTEQUAL / RT_GEQUAL / RT_ALWAYS
        } depth;
        struct
        {
            bool clear = false;
            uint32_t read = 0xFFFFFFFF;
            uint32_t write = 0xFFFFFFFF;
            int32_t value = -1;
            int32_t refer = 0;
            rt_compare_op_t func = RT_ALWAYS; // RT_NEVER / RT_LESS / RT_EQUAL / RT_LEQUAL / RT_GREATER / RT_NOTEQUAL / RT_GEQUAL / RT_ALWAYS
            rt_stencil_op_t sfail = RT_STENCIL_KEEP; // RT_STENCIL_KEEP / RT_STENCIL_ZERO / RT_STENCIL_REPLACE / RT_STENCIL_INCR / RT_STENCIL_INCR_WRAP / RT_STENCIL_DECR / RT_STENCIL_DECR_WRAP / RT_STENCIL_INVERT
            rt_stencil_op_t zfail = RT_STENCIL_KEEP; // RT_STENCIL_KEEP / RT_STENCIL_ZERO / RT_STENCIL_REPLACE / RT_STENCIL_INCR / RT_STENCIL_INCR_WRAP / RT_STENCIL_DECR / RT_STENCIL_DECR_WRAP / RT_STENCIL_INVERT
            rt_stencil_op_t zpass = RT_STENCIL_KEEP; // RT_STENCIL_KEEP / RT_STENCIL_ZERO / RT_STENCIL_REPLACE / RT_STENCIL_INCR / RT_STENCIL_INCR_WRAP / RT_STENCIL_DECR / RT_STENCIL_DECR_WRAP / RT_STENCIL_INVERT
        } stencil;
    } screen;

    void* native = nullptr;
};

struct rt_pass_transfer_t
{
    uint32_t handle = 0;
    void* native = nullptr;
};

// ====================================================================

struct rt_mesh_t
{
    uint32_t handle = 0;
    rt_buffer_t index;
    rt_buffer_t vertex[RT_MAX_VERTEX_BUFFER_NUM];
    uint32_t location[RT_MAX_VERTEX_BUFFER_NUM] = {};
    void* native = nullptr;
};

struct rt_meshlet_t
{
    uint32_t handle = 0;
    rt_buffer_t index;
    rt_buffer_t vertex[RT_MAX_VERTEX_BUFFER_NUM];
    uint32_t location[RT_MAX_VERTEX_BUFFER_NUM + 1] = {};
    void* native = nullptr;
};

// ====================================================================

struct rt_size_t
{
    uint32_t x = 0, y = 0, z = 0;
};

struct rt_buffer_copy_t
{
    rt_buffer_t& buffer;
    size_t offset = 0;
};

struct rt_buffer_data_t
{
    const uint8_t* data = nullptr;
    size_t size = 0;
    size_t offset = 0;
};

struct rt_buffer_texel_t
{
    rt_buffer_t& buffer;
    size_t offset = 0;
    uint32_t bytesPerRow = 0;
    uint32_t rowsPerImage = 0;
};

struct rt_texture_copy_t
{
    rt_texture_t& texture;
    rt_texture_aspect_t aspect = RT_TEXTURE_ASPECT_ALL;
    uint32_t mipLevel = 0;
    rt_size_t origin;
};

struct rt_texture_data_t
{
    const uint8_t* data = nullptr;
    size_t size = 0;
    size_t offset = 0;
    uint32_t bytesPerRow = 0;
    uint32_t rowsPerImage = 0;
};

// ====================================================================

OPENRT_API void rt_load_library(rt_load_info_t const& info = {});
OPENRT_API void (*rt_unload_library)();

OPENRT_API rt_buffer_t (*rt_create_buffer)(rt_buffer_info_t const& info);
OPENRT_API void (*rt_destroy_buffer)(rt_buffer_t& buffer);
OPENRT_API void (*rt_bind_buffer)(rt_buffer_t& buffer, rt_buffer_bind_t bind);
OPENRT_API void* (*rt_map_buffer)(rt_buffer_t& buffer, rt_access_t mode, size_t offset, size_t size); // mode: RT_READ_ONLY / RT_WRITE_ONLY / RT_READ_WRITE
OPENRT_API void (*rt_unmap_buffer)(rt_buffer_t& buffer);

OPENRT_API rt_texture_t (*rt_create_texture)(rt_texture_info_t const& info);
OPENRT_API void (*rt_destroy_texture)(rt_texture_t& texture);
OPENRT_API void (*rt_bind_texture)(rt_texture_t& texture, rt_texture_bind_t bind);

OPENRT_API rt_texture_view_t (*rt_create_texture_view)(rt_texture_t& texture, rt_texture_view_info_t const& info);
OPENRT_API void (*rt_destroy_texture_view)(rt_texture_view_t& view);
OPENRT_API void (*rt_bind_texture_view)(rt_texture_view_t& view, rt_texture_view_bind_t bind);
OPENRT_API void (*rt_bind_texture_storage)(rt_texture_view_t& view, rt_texture_storage_bind_t bind);

OPENRT_API rt_sampler_t (*rt_create_sampler)(rt_sampler_info_t const& info);
OPENRT_API void (*rt_destroy_sampler)(rt_sampler_t& sampler);
OPENRT_API void (*rt_bind_sampler)(rt_sampler_t& sampler, rt_sampler_bind_t bind);

OPENRT_API rt_module_compute_t (*rt_create_module_compute)(rt_module_compute_info_t const& info);
OPENRT_API rt_module_render_t (*rt_create_module_render)(rt_module_render_info_t const& info);
OPENRT_API void (*rt_destroy_module_render)(rt_module_render_t& module);
OPENRT_API void (*rt_destroy_module_compute)(rt_module_compute_t& module);

OPENRT_API void (*rt_begin_compute)(rt_pass_compute_t& pass);
OPENRT_API void (*rt_end_compute)(rt_pass_compute_t& pass);
OPENRT_API void (*rt_bind_module_compute)(rt_module_compute_t& module);
OPENRT_API void (*rt_dispatch_compute)(uint32_t groupX, uint32_t groupY, uint32_t groupZ);
OPENRT_API void (*rt_dispatch_compute_indirect)(rt_buffer_t& indirect, size_t offset);

OPENRT_API void (*rt_begin_render)(rt_pass_render_t& pass);
OPENRT_API void (*rt_end_render)(rt_pass_render_t& pass);
OPENRT_API void (*rt_bind_module_render)(rt_module_render_t& module);
OPENRT_API void (*rt_set_viewport)(float x, float y, float width, float height, float minDepth, float maxDepth);
OPENRT_API void (*rt_set_scissor)(int32_t x, int32_t y, int32_t width, int32_t height);
OPENRT_API void (*rt_set_blend_constant)(float r, float g, float b, float a);
OPENRT_API void (*rt_set_stencil_reference)(int32_t value);
OPENRT_API void (*rt_draw_array)(rt_buffer_t vbo[], uint32_t vbo_num, uint32_t vertex_num, uint32_t instance_num, uint32_t vertex_start, uint32_t instance_start);
OPENRT_API void (*rt_draw_index)(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& ebo, uint32_t vertex_num, uint32_t instance_num, uint32_t vertex_start, uint32_t instance_start);
OPENRT_API void (*rt_draw_array_indirect)(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& indirect, size_t offset);
OPENRT_API void (*rt_draw_index_indirect)(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& ebo, rt_buffer_t& indirect, size_t offset);
OPENRT_API void (*rt_draw_mesh_task)(uint32_t groupX, uint32_t groupY, uint32_t groupZ);
OPENRT_API void (*rt_draw_mesh_task_indirect)(rt_buffer_t& indirect, size_t offset, uint32_t draw_count, uint32_t draw_stride);

OPENRT_API void (*rt_push_constant)(uint8_t const* buffer, size_t length);
OPENRT_API void (*rt_push_const_int)(const char* name, int32_t value);
OPENRT_API void (*rt_push_const_uint)(const char* name, uint32_t value);
OPENRT_API void (*rt_push_const_float)(const char* name, float value);
OPENRT_API void (*rt_push_const_vec2)(const char* name, const float* value);
OPENRT_API void (*rt_push_const_vec3)(const char* name, const float* value);
OPENRT_API void (*rt_push_const_vec4)(const char* name, const float* value);
OPENRT_API void (*rt_push_const_mat3)(const char* name, const float* value);
OPENRT_API void (*rt_push_const_mat4)(const char* name, const float* value);

OPENRT_API void (*rt_begin_transfer)(rt_pass_transfer_t& pass);
OPENRT_API void (*rt_end_transfer)(rt_pass_transfer_t& pass);
OPENRT_API void (*rt_copy_buffer)(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize);
OPENRT_API void (*rt_copy_buffer_data)(rt_buffer_data_t source, rt_buffer_copy_t destination, size_t copySize);
OPENRT_API void (*rt_copy_buffer_texture)(rt_texture_copy_t source, rt_buffer_texel_t destination, rt_size_t copySize);
OPENRT_API void (*rt_copy_texture)(rt_texture_copy_t source, rt_texture_copy_t destination, rt_size_t copySize);
OPENRT_API void (*rt_copy_texture_data)(rt_texture_data_t source, rt_texture_copy_t destination, rt_size_t copySize);
OPENRT_API void (*rt_copy_texture_buffer)(rt_buffer_texel_t source, rt_texture_copy_t destination, rt_size_t copySize);

OPENRT_API rt_mesh_t (*rt_create_mesh)(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
OPENRT_API void (*rt_destroy_mesh)(rt_mesh_t& mesh);
OPENRT_API void (*rt_draw_mesh)(rt_mesh_t& mesh);
OPENRT_API void (*rt_draw_mesh_multi)(rt_mesh_t& mesh, uint32_t count);

OPENRT_API rt_meshlet_t (*rt_create_meshlet)(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
OPENRT_API void (*rt_destroy_meshlet)(rt_meshlet_t& meshlet);
OPENRT_API void (*rt_draw_meshlet)(rt_meshlet_t& meshlet);

OPENRT_API void (*rt_submit)();