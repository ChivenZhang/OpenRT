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
#include <GL/glew.h>
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

#define GL_MAX_COLOR_TEXTURE_NUM 2
#define GL_MAX_VERTEX_BUFFER_NUM 10
#define GL_MAX_BINDING_HANDLE_NUM 16
#define GL_PI 3.14159265358979323846    // pi
#define GL_PI_2 1.57079632679489661923  // pi/2
#define GL_PI_4 0.785398163397448309616 // pi/4
#define GL_1_PI 0.318309886183790671538 // 1/pi
#define GL_2_PI 0.636619772367581343076 // 2/pi

// ====================================================================

enum rt_buffer_usage_t : uint32_t
{
    GL_BUFFER_USAGE_MAP_READ      = 0x0001,
    GL_BUFFER_USAGE_MAP_WRITE     = 0x0002,
    GL_BUFFER_USAGE_COPY_SRC      = 0x0004,
    GL_BUFFER_USAGE_COPY_DST      = 0x0008,
    GL_BUFFER_USAGE_INDEX         = 0x0010,
    GL_BUFFER_USAGE_VERTEX        = 0x0020,
    GL_BUFFER_USAGE_UNIFORM       = 0x0040,
    GL_BUFFER_USAGE_STORAGE       = 0x0080,
    GL_BUFFER_USAGE_INDIRECT      = 0x0100,
    GL_BUFFER_USAGE_QUERY_RESOLVE = 0x0200,
};
using rt_buffer_usages_t = uint32_t;

struct rt_buffer_t
{
    GLuint handle = 0;
    size_t size = 0;
    rt_buffer_usages_t usage = 0;
    void* native = nullptr;
};

struct rt_buffer_info_t
{
    size_t size = 0;                    // 缓冲区大小（字节）
    rt_buffer_usages_t usage = GL_BUFFER_USAGE_MAP_READ | GL_BUFFER_USAGE_MAP_WRITE | GL_BUFFER_USAGE_COPY_SRC | GL_BUFFER_USAGE_COPY_DST; // rt_buffer_usage_t
    const void* data = nullptr;         // 初始数据指针，可为 nullptr
};

struct rt_buffer_bind_t
{
    uint32_t binding = 0;
    GLenum target = GL_UNIFORM_BUFFER; // GL_UNIFORM_BUFFER / GL_SHADER_STORAGE_BUFFER
};

// ====================================================================

struct rt_texture_t
{
    GLuint handle = 0;
    uint32_t width = 0, height = 0, depth = 1;
    GLenum target = GL_TEXTURE_2D;
    GLenum format = GL_RGBA;
    GLenum internal_format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;
    uint32_t mipmaps = 0;
    uint32_t samples = 1;
    void* native = nullptr;
};

struct rt_texture_info_t
{
    uint32_t width = 0, height = 0, depth = 1;
    GLenum target = GL_TEXTURE_2D;          // GL_TEXTURE_1D / GL_TEXTURE_2D / GL_TEXTURE_3D / GL_TEXTURE_2D_ARRAY / GL_TEXTURE_2D_MULTISAMPLE
    GLenum format = GL_RGBA;                // 数据格式
    GLenum internal_format = GL_RGBA8;      // 内部存储格式
    GLenum type = GL_UNSIGNED_BYTE;         // 数据类型
    GLenum min_filter = GL_LINEAR_MIPMAP_LINEAR;  // GL_NEAREST / GL_LINEAR / GL_NEAREST_MIPMAP_NEAREST / GL_LINEAR_MIPMAP_NEAREST / GL_NEAREST_MIPMAP_LINEAR / GL_LINEAR_MIPMAP_LINEAR
    GLenum mag_filter = GL_LINEAR;                // GL_NEAREST / GL_LINEAR
    GLenum wrap_s = GL_REPEAT;                    // GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER / GL_MIRROR_CLAMP_TO_EDGE
    GLenum wrap_t = GL_REPEAT;                    // GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER / GL_MIRROR_CLAMP_TO_EDGE
    GLenum wrap_r = GL_REPEAT;                    // GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER / GL_MIRROR_CLAMP_TO_EDGE
    uint32_t mipmaps = 0;   // 0:auto generate
    uint32_t samples = 1;   // 1x / 4x
    GLfloat border[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const void* data = nullptr;
};

struct rt_texture_bind_t
{
    uint32_t binding = 0;
    GLenum aspect_mode = GL_DEPTH_COMPONENT; // GL_DEPTH_COMPONENT / GL_STENCIL_INDEX
};

struct rt_texture_storage_bind_t
{
    uint32_t binding = 0;
    uint32_t base_level = 0;
    uint32_t base_layer = 0;
    uint32_t level_count = 1;
    uint32_t layer_count = 1;
    GLenum access = GL_WRITE_ONLY; // GL_WRITE_ONLY / GL_READ_ONLY / GL_READ_WRITE
};

// ====================================================================

struct rt_sampler_t
{
    GLuint handle = 0;
    void* native = nullptr;
};

struct rt_sampler_info_t
{
    GLenum min_filter = GL_LINEAR_MIPMAP_LINEAR;  // GL_NEAREST / GL_LINEAR / GL_NEAREST_MIPMAP_NEAREST / GL_LINEAR_MIPMAP_NEAREST / GL_NEAREST_MIPMAP_LINEAR / GL_LINEAR_MIPMAP_LINEAR
    GLenum mag_filter = GL_LINEAR;                // GL_NEAREST / GL_LINEAR
    GLenum wrap_s = GL_REPEAT;                    // GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER / GL_MIRROR_CLAMP_TO_EDGE
    GLenum wrap_t = GL_REPEAT;                    // GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER / GL_MIRROR_CLAMP_TO_EDGE
    GLenum wrap_r = GL_REPEAT;                    // GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER / GL_MIRROR_CLAMP_TO_EDGE
};

struct rt_sampler_bind_t
{
    uint32_t binding = 0;
};

// ====================================================================

struct rt_module_compute_info_t
{
    const char* cshader = nullptr;
    uint32_t clength = 0;
};

struct rt_module_compute_t
{
    GLuint handle = 0;
    void* native = nullptr;
};

struct rt_vertex_t
{
    uint32_t location = 0;
    GLenum type = 0;    // GL_BYTE / GL_UNSIGNED_BYTE / GL_SHORT / GL_UNSIGNED_SHORT / GL_INT / GL_UNSIGNED_INT / GL_FLOAT / GL_DOUBLE
    GLenum count = 0;
    bool instance = false;
};
inline rt_vertex_t rt_vertex_vertex{.location = 0, .type = GL_FLOAT, .count = 3, .instance = false};
inline rt_vertex_t rt_vertex_normal{.location = 1, .type = GL_FLOAT, .count = 3, .instance = false};
inline rt_vertex_t rt_vertex_uv{.location = 2, .type = GL_FLOAT, .count = 2, .instance = false};

enum rt_binding_type_t : uint32_t
{
    GL_BINDING_BUFFER = 1,
    GL_BINDING_TEXTURE = 2,
    GL_BINDING_STORAGE_TEXTURE = 3,
    GL_BINDING_SAMPLER = 4,
};

struct rt_binding_t
{
    uint32_t binding = 0;
    rt_binding_type_t type = {};  // GL_BINDING_BUFFER / GL_BINDING_TEXTURE / GL_BINDING_STORAGE_TEXTURE / GL_BINDING_SAMPLER
};

struct rt_module_render_info_t
{
    const char* vshader = nullptr;
    uint32_t vlength = 0;
    const char* tshader = nullptr;
    uint32_t tlength = 0;
    const char* mshader = nullptr;
    uint32_t mlength = 0;
    const char* fshader = nullptr;
    uint32_t flength = 0;

    struct
    {
        struct
        {
            GLenum func = GL_FUNC_ADD; // GL_FUNC_ADD / GL_FUNC_SUBTRACT / GL_FUNC_REVERSE_SUBTRACT / GL_MIN / GL_MAX
            GLenum src = GL_ONE; // GL_ZERO / GL_ONE / GL_SRC_COLOR / GL_ONE_MINUS_SRC_COLOR / GL_DST_COLOR / GL_ONE_MINUS_DST_COLOR / GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA / GL_DST_ALPHA / GL_ONE_MINUS_DST_ALPHA / GL_CONSTANT_COLOR / GL_ONE_MINUS_CONSTANT_COLOR / GL_CONSTANT_ALPHA / GL_ONE_MINUS_CONSTANT_ALPHA / GL_SRC_ALPHA_SATURATE
            GLenum dst = GL_ZERO; // GL_ZERO / GL_ONE / GL_SRC_COLOR / GL_ONE_MINUS_SRC_COLOR / GL_DST_COLOR / GL_ONE_MINUS_DST_COLOR / GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA / GL_DST_ALPHA / GL_ONE_MINUS_DST_ALPHA / GL_CONSTANT_COLOR / GL_ONE_MINUS_CONSTANT_COLOR / GL_CONSTANT_ALPHA / GL_ONE_MINUS_CONSTANT_ALPHA / GL_SRC_ALPHA_SATURATE
        } color, alpha;
    } colors[GL_MAX_COLOR_TEXTURE_NUM];
    struct
    {
        bool write = false;
        float bias = 0.0f;
        float biasSlope = 0.0f;
        float biasClamp = 0.0f;
        GLenum func = GL_ALWAYS; // GL_NEVER / GL_LESS / GL_EQUAL / GL_LEQUAL / GL_GREATER / GL_NOTEQUAL / GL_GEQUAL / GL_ALWAYS
    } depth;
    struct
    {
        uint32_t read = 0xFFFFFFFF;
        uint32_t write = 0xFFFFFFFF;
        struct
        {
            GLenum func = GL_ALWAYS; // GL_NEVER / GL_LESS / GL_EQUAL / GL_LEQUAL / GL_GREATER / GL_NOTEQUAL / GL_GEQUAL / GL_ALWAYS
            GLenum sfail = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
            GLenum zfail = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
            GLenum zpass = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
        } back, front;
    } stencil;

    GLenum index_type = GL_UNSIGNED_INT;
    rt_vertex_t vertex[GL_MAX_VERTEX_BUFFER_NUM];
    rt_binding_t binding[GL_MAX_BINDING_HANDLE_NUM];

    GLenum cull_mode = GL_BACK; // GL_NONE / GL_FRONT / GL_BACK / GL_FRONT_AND_BACK
    GLenum front_face = GL_CCW; // GL_CW / GL_CCW
    GLenum fill_mode = GL_FILL; // GL_POINT / GL_LINE / GL_FILL
    GLenum primitive = GL_TRIANGLES; // GL_POINTS / GL_LINES / GL_LINE_LOOP / GL_LINE_STRIP / GL_TRIANGLES / GL_TRIANGLE_STRIP / GL_TRIANGLE_FAN
};

struct rt_module_render_t
{
    GLuint handle = 0;
    GLuint vertex_vao = 0;   // VAO

    struct
    {
        struct
        {
            GLenum func = GL_FUNC_ADD;
            GLenum src = GL_ONE;
            GLenum dst = GL_ZERO;
        } color, alpha;
    } colors[GL_MAX_COLOR_TEXTURE_NUM];
    struct
    {
        bool write = false;
        float bias = 0.0f;
        float biasSlope = 0.0f;
        float biasClamp = 0.0f;
        GLenum func = GL_ALWAYS;
    } depth;
    struct
    {
        uint32_t read = 0xFFFFFFFF;
        uint32_t write = 0xFFFFFFFF;
        struct
        {
            GLenum func = GL_ALWAYS;
            GLenum sfail = GL_KEEP;
            GLenum zfail = GL_KEEP;
            GLenum zpass = GL_KEEP;
        } back, front;
    } stencil;

    GLenum index_type = GL_UNSIGNED_INT;
    rt_vertex_t vertex[GL_MAX_VERTEX_BUFFER_NUM];
    rt_binding_t binding[GL_MAX_BINDING_HANDLE_NUM];

    GLenum cull_mode = GL_BACK;
    GLenum front_face = GL_CCW;
    GLenum fill_mode = GL_FILL;
    GLenum primitive = GL_TRIANGLES;

    void* native = nullptr;
};

// ====================================================================

struct rt_pass_compute_t
{
    GLuint handle = 0;
    rt_module_compute_t& module;
    void* native = nullptr;
};

struct rt_color_t
{
    float r = 0, g = 0, b = 0, a = 0;
};

struct rt_pass_render_t
{
    GLuint handle = 0;
    rt_module_render_t& module;

    struct
    {
        rt_texture_t texture;
        bool clear = false;
        rt_color_t value;
    } colors[GL_MAX_COLOR_TEXTURE_NUM];
    struct
    {
        rt_texture_t texture;
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
                GLenum func = GL_FUNC_ADD; // GL_FUNC_ADD / GL_FUNC_SUBTRACT / GL_FUNC_REVERSE_SUBTRACT / GL_MIN / GL_MAX
                GLenum src = GL_ONE; // GL_ZERO / GL_ONE / GL_SRC_COLOR / GL_ONE_MINUS_SRC_COLOR / GL_DST_COLOR / GL_ONE_MINUS_DST_COLOR / GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA / GL_DST_ALPHA / GL_ONE_MINUS_DST_ALPHA / GL_CONSTANT_COLOR / GL_ONE_MINUS_CONSTANT_COLOR / GL_CONSTANT_ALPHA / GL_ONE_MINUS_CONSTANT_ALPHA / GL_SRC_ALPHA_SATURATE
                GLenum dst = GL_ZERO; // GL_ZERO / GL_ONE / GL_SRC_COLOR / GL_ONE_MINUS_SRC_COLOR / GL_DST_COLOR / GL_ONE_MINUS_DST_COLOR / GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA / GL_DST_ALPHA / GL_ONE_MINUS_DST_ALPHA / GL_CONSTANT_COLOR / GL_ONE_MINUS_CONSTANT_COLOR / GL_CONSTANT_ALPHA / GL_ONE_MINUS_CONSTANT_ALPHA / GL_SRC_ALPHA_SATURATE
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
            GLenum func = GL_ALWAYS; // GL_NEVER / GL_LESS / GL_EQUAL / GL_LEQUAL / GL_GREATER / GL_NOTEQUAL / GL_GEQUAL / GL_ALWAYS
        } depth;
        struct
        {
            bool clear = false;
            uint32_t read = 0xFFFFFFFF;
            uint32_t write = 0xFFFFFFFF;
            int32_t value = -1;
            int32_t refer = 0;
            GLenum func = GL_ALWAYS; // GL_NEVER / GL_LESS / GL_EQUAL / GL_LEQUAL / GL_GREATER / GL_NOTEQUAL / GL_GEQUAL / GL_ALWAYS
            GLenum sfail = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
            GLenum zfail = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
            GLenum zpass = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
        } stencil;
    } screen;

    void* native = nullptr;
};

struct rt_pass_transfer_t
{
    GLuint handle = 0;
    void* native = nullptr;
};

// ====================================================================

struct rt_mesh_t
{
    GLuint handle = 0;
    rt_buffer_t index;
    rt_buffer_t vertex[GL_MAX_VERTEX_BUFFER_NUM];
    uint32_t location[GL_MAX_VERTEX_BUFFER_NUM] = {};
    void* native = nullptr;
};

struct rt_meshlet_t
{
    GLuint handle = 0;
    rt_buffer_t index;
    rt_buffer_t vertex[GL_MAX_VERTEX_BUFFER_NUM];
    uint32_t location[GL_MAX_VERTEX_BUFFER_NUM + 1] = {};
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
    GLenum aspect = GL_DEPTH_COMPONENT; // GL_DEPTH_COMPONENT / GL_STENCIL_INDEX
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

OPENRT_API void rt_load_library(const char* backend = nullptr);
OPENRT_API void rt_unload_library();

OPENRT_API rt_buffer_t (*rt_create_buffer)(rt_buffer_info_t const& info);
OPENRT_API void (*rt_destroy_buffer)(rt_buffer_t& buffer);
OPENRT_API void (*rt_bind_buffer)(rt_buffer_t& buffer, rt_buffer_bind_t bind);
OPENRT_API void* (*rt_map_buffer)(rt_buffer_t& buffer, GLenum mode, size_t offset, size_t size); // mode: GL_READ_ONLY / GL_WRITE_ONLY / GL_READ_WRITE
OPENRT_API void (*rt_unmap_buffer)(rt_buffer_t& buffer);

OPENRT_API rt_texture_t (*rt_create_texture)(rt_texture_info_t const& info);
OPENRT_API rt_texture_t (*rt_create_texture_color)(uint32_t width, uint32_t height, const void* data);
OPENRT_API rt_texture_t (*rt_create_texture_color_float)(uint32_t width, uint32_t height, const void* data);
OPENRT_API rt_texture_t (*rt_create_texture_depth)(uint32_t width, uint32_t height, const void* data);
OPENRT_API rt_texture_t (*rt_create_texture_depth_stencil)(uint32_t width, uint32_t height, const void* data);
OPENRT_API void (*rt_destroy_texture)(rt_texture_t& texture);
OPENRT_API void (*rt_bind_texture)(rt_texture_t& texture, rt_texture_bind_t bind);
OPENRT_API void (*rt_bind_texture_storage)(rt_texture_t& texture, rt_texture_storage_bind_t bind);

OPENRT_API rt_sampler_t (*rt_create_sampler)(rt_sampler_info_t const& info);
OPENRT_API void (*rt_destroy_sampler)(rt_sampler_t& sampler);
OPENRT_API void (*rt_bind_sampler)(rt_sampler_t& sampler, rt_sampler_bind_t bind);

OPENRT_API rt_module_compute_t (*rt_create_module_compute)(rt_module_compute_info_t const& info);
OPENRT_API rt_module_render_t (*rt_create_module_render)(rt_module_render_info_t const& info);
OPENRT_API rt_module_render_t (*rt_create_module_meshlet)(rt_module_render_info_t const& info);
OPENRT_API void (*rt_destroy_module_render)(rt_module_render_t& module);
OPENRT_API void (*rt_destroy_module_compute)(rt_module_compute_t& module);

OPENRT_API void (*rt_begin_compute)(rt_pass_compute_t& pass);
OPENRT_API void (*rt_end_compute)(rt_pass_compute_t& pass);
OPENRT_API void (*rt_dispatch_compute)(uint32_t groupX, uint32_t groupY, uint32_t groupZ);

OPENRT_API void (*rt_begin_render)(rt_pass_render_t& pass);
OPENRT_API void (*rt_end_render)(rt_pass_render_t& pass);
OPENRT_API void (*rt_set_viewport)(int32_t x, int32_t y, int32_t width, int32_t height);
OPENRT_API void (*rt_set_scissor)(int32_t x, int32_t y, int32_t width, int32_t height);
OPENRT_API void (*rt_draw_mesh_task)(uint32_t groupX, uint32_t groupY, uint32_t groupZ);

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

OPENRT_API rt_mesh_t (*rt_create_mesh_screen)();
OPENRT_API void (*rt_draw_screen)(int width, int height, rt_color_t clear, rt_texture_t& texture);

OPENRT_API void (*rt_submit)();