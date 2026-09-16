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
#include <iostream>

#define GL_PI 3.14159265358979323846    // pi
#define GL_PI_2 1.57079632679489661923  // pi/2
#define GL_PI_4 0.785398163397448309616 // pi/4
#define GL_1_PI 0.318309886183790671538 // 1/pi
#define GL_2_PI 0.636619772367581343076 // 2/pi

struct rhi_buffer_t
{
    GLuint handle = 0;
    size_t size = 0;
    GLenum usage = GL_STATIC_DRAW;
    GLenum target = GL_ARRAY_BUFFER;
    void* native = nullptr;
};

struct rhi_buffer_desc_t
{
    size_t size = 0;                    // 缓冲区大小（字节）
    GLenum usage = GL_STATIC_DRAW;      // GL_STREAM_DRAW / GL_STREAM_READ / GL_STREAM_COPY / GL_STATIC_DRAW / GL_STATIC_READ / GL_STATIC_COPY / GL_DYNAMIC_DRAW / GL_DYNAMIC_READ / GL_DYNAMIC_COPY
    const void* data = nullptr;         // 初始数据指针，可为 nullptr
};

struct rhi_buffer_bind_t
{
    uint32_t binding = 0;
    GLenum target = GL_UNIFORM_BUFFER; // GL_UNIFORM_BUFFER / GL_SHADER_STORAGE_BUFFER
};

struct rhi_texture_t
{
    GLuint handle = 0;
    uint32_t width = 0, height = 0;
    GLenum target = GL_TEXTURE_2D;
    GLenum format = GL_RGBA;
    GLenum internal_format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;
    bool mipmaps = false;
    void* native = nullptr;
};

struct rhi_texture_desc_t
{
    uint32_t width = 0;
    uint32_t height = 0;
    GLenum target = GL_TEXTURE_2D;
    GLenum format = GL_RGBA;                // 数据格式
    GLenum internal_format = GL_RGBA8;      // 内部存储格式
    GLenum type = GL_UNSIGNED_BYTE;         // 数据类型
    GLenum min_filter = GL_LINEAR_MIPMAP_LINEAR;  // GL_NEAREST / GL_LINEAR / GL_NEAREST_MIPMAP_NEAREST / GL_LINEAR_MIPMAP_NEAREST / GL_NEAREST_MIPMAP_LINEAR / GL_LINEAR_MIPMAP_LINEAR
    GLenum mag_filter = GL_LINEAR;                // GL_NEAREST / GL_LINEAR
    GLenum wrap_s = GL_REPEAT;                    // GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER / GL_MIRROR_CLAMP_TO_EDGE
    GLenum wrap_t = GL_REPEAT;                    // GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER / GL_MIRROR_CLAMP_TO_EDGE
    GLenum wrap_r = GL_REPEAT;                    // GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER / GL_MIRROR_CLAMP_TO_EDGE
    GLfloat border[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const void* data = nullptr;
};

struct rhi_texture_bind_t
{
    uint32_t binding = 0;
    GLenum aspect_mode = GL_DEPTH_COMPONENT; // GL_DEPTH_COMPONENT / GL_STENCIL_INDEX
};

struct rhi_texture_storage_bind_t
{
    uint32_t binding = 0;
    uint32_t base_level = 0;
    uint32_t base_layer = 0;
    uint32_t level_count = 1;
    uint32_t layer_count = 1;
    GLenum access = GL_WRITE_ONLY; // GL_WRITE_ONLY / GL_READ_ONLY / GL_READ_WRITE
};

struct rhi_image_t
{
    void* pixels = nullptr;
    uint32_t width = 0, height = 0;
    GLenum format = GL_RGBA;
    void* native = nullptr;
};

struct rhi_sampler_t
{
    GLuint handle = 0;
    void* native = nullptr;
};

struct rhi_sampler_desc_t
{
    GLenum min_filter = GL_LINEAR_MIPMAP_LINEAR;  // GL_NEAREST / GL_LINEAR / GL_NEAREST_MIPMAP_NEAREST / GL_LINEAR_MIPMAP_NEAREST / GL_NEAREST_MIPMAP_LINEAR / GL_LINEAR_MIPMAP_LINEAR
    GLenum mag_filter = GL_LINEAR;                // GL_NEAREST / GL_LINEAR
    GLenum wrap_s = GL_REPEAT;                    // GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER / GL_MIRROR_CLAMP_TO_EDGE
    GLenum wrap_t = GL_REPEAT;                    // GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER / GL_MIRROR_CLAMP_TO_EDGE
    GLenum wrap_r = GL_REPEAT;                    // GL_REPEAT / GL_MIRRORED_REPEAT / GL_CLAMP_TO_EDGE / GL_CLAMP_TO_BORDER / GL_MIRROR_CLAMP_TO_EDGE
};

struct rhi_sampler_bind_t
{
    uint32_t binding = 0;
};

struct rhi_module_t
{
    GLuint handle = 0;
    GLenum target = GL_NONE;
    void* native = nullptr;
};

struct rhi_color_t
{
    float r = 0, g = 0, b = 0, a = 0;
};

struct rhi_pass_t
{
    GLuint handle = 0;
    rhi_module_t module;

    // Offscreen Mode

    struct
    {
        rhi_texture_t texture;
        bool clear = false;
        rhi_color_t value;
        struct
        {
            GLenum func = GL_ADD; // GL_ADD / GL_SUBTRACT / GL_REVERSE_SUBTRACT / GL_MIN / GL_MAX
            GLenum src = GL_ONE; // GL_ZERO / GL_ONE / GL_SRC_COLOR / GL_ONE_MINUS_SRC_COLOR / GL_DST_COLOR / GL_ONE_MINUS_DST_COLOR / GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA / GL_DST_ALPHA / GL_ONE_MINUS_DST_ALPHA / GL_CONSTANT_COLOR / GL_ONE_MINUS_CONSTANT_COLOR / GL_CONSTANT_ALPHA / GL_ONE_MINUS_CONSTANT_ALPHA / GL_SRC_ALPHA_SATURATE
            GLenum dst = GL_ZERO; // GL_ZERO / GL_ONE / GL_SRC_COLOR / GL_ONE_MINUS_SRC_COLOR / GL_DST_COLOR / GL_ONE_MINUS_DST_COLOR / GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA / GL_DST_ALPHA / GL_ONE_MINUS_DST_ALPHA / GL_CONSTANT_COLOR / GL_ONE_MINUS_CONSTANT_COLOR / GL_CONSTANT_ALPHA / GL_ONE_MINUS_CONSTANT_ALPHA / GL_SRC_ALPHA_SATURATE
        } color, alpha;
    } colors[2];
    struct
    {
        rhi_texture_t texture;
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
        struct
        {
            GLenum func = GL_ALWAYS; // GL_NEVER / GL_LESS / GL_EQUAL / GL_LEQUAL / GL_GREATER / GL_NOTEQUAL / GL_GEQUAL / GL_ALWAYS
            GLenum sfail = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
            GLenum zfail = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
            GLenum zpass = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
        } back, front;
    } stencil;

    // Screen Mode

    struct
    {
        struct
        {
            bool clear = false;
            rhi_color_t value;
            struct
            {
                GLenum func = GL_ADD; // GL_ADD / GL_SUBTRACT / GL_REVERSE_SUBTRACT / GL_MIN / GL_MAX
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

    GLenum cull_mode = GL_BACK; // GL_NONE / GL_FRONT / GL_BACK / GL_FRONT_AND_BACK
    GLenum front_face = GL_CCW; // GL_CW / GL_CCW
    GLenum fill_mode = GL_FILL; // GL_POINT / GL_LINE / GL_FILL
    void* native = nullptr;
};

struct rhi_mesh_t
{
    GLuint handle = 0;

    rhi_buffer_t vertex_vbo;
    rhi_buffer_t normal_vbo;
    rhi_buffer_t uv_vbo;
    rhi_buffer_t index_vbo;

    GLsizei vertex_count = 0;
    GLsizei index_count = 0;
    GLenum index_type = GL_UNSIGNED_INT;
    GLenum primitive_type = GL_TRIANGLES;
    void* native = nullptr;
};

struct rhi_meshlet_t
{
    rhi_buffer_t vertex_vbo;
    rhi_buffer_t normal_vbo;
    rhi_buffer_t uv_vbo;
    rhi_buffer_t index_vbo;

    GLsizei vertex_count = 0;
    GLsizei index_count = 0;
    GLenum index_type = GL_UNSIGNED_INT;
    GLenum primitive_type = GL_TRIANGLES;
    void* native = nullptr;
};

// ====================================================================

extern rhi_buffer_t (*rhi_create_buffer)(rhi_buffer_desc_t const& info);
extern void (*rhi_destroy_buffer)(rhi_buffer_t& buffer);
extern void (*rhi_bind_buffer)(rhi_buffer_t buffer, rhi_buffer_bind_t bind);
extern void (*rhi_read_buffer)(rhi_buffer_t buffer, size_t offset, size_t size, void* data);
extern void (*rhi_write_buffer)(rhi_buffer_t buffer, size_t offset, size_t size, const void* data);

extern rhi_texture_t (*rhi_create_texture)(rhi_texture_desc_t const& info);
extern rhi_texture_t (*rhi_create_texture_color)(uint32_t width, uint32_t height, const void* data);
extern rhi_texture_t (*rhi_create_texture_depth)(uint32_t width, uint32_t height, const void* data);
extern rhi_texture_t (*rhi_create_texture_depth_stencil)(uint32_t width, uint32_t height, const void* data);
extern void (*rhi_destroy_texture)(rhi_texture_t& texture);
extern void (*rhi_bind_texture)(rhi_texture_t texture, rhi_texture_bind_t bind);
extern void (*rhi_bind_texture_storage)(rhi_texture_t texture, rhi_texture_storage_bind_t bind);
extern rhi_texture_t (*rhi_load_texture)(rhi_image_t const& image);
extern rhi_image_t (*rhi_load_image)(rhi_texture_t const& texture, void* buffer, size_t length);

extern rhi_sampler_t (*rhi_create_sampler)(rhi_sampler_desc_t const& info);
extern void (*rhi_destroy_sampler)(rhi_sampler_t& sampler);
extern void (*rhi_bind_sampler)(rhi_sampler_t sampler, rhi_sampler_bind_t bind);

extern rhi_module_t (*rhi_create_module_compute)(const char* comp_src);
extern rhi_module_t (*rhi_create_module_render)(const char* vert_src, const char* frag_src);
extern rhi_module_t (*rhi_create_module_meshlet)(const char* task_src, const char* mesh_src, const char* frag_src);
extern void (*rhi_destroy_module)(rhi_module_t& module);

extern void (*rhi_push_const_int)(const char* name, int32_t value);
extern void (*rhi_push_const_uint)(const char* name, uint32_t value);
extern void (*rhi_push_const_float)(const char* name, float value);
extern void (*rhi_push_const_vec2)(const char* name, const float* value);
extern void (*rhi_push_const_vec3)(const char* name, const float* value);
extern void (*rhi_push_const_vec4)(const char* name, const float* value);
extern void (*rhi_push_const_mat3)(const char* name, const float* value);
extern void (*rhi_push_const_mat4)(const char* name, const float* value);

extern void (*rhi_begin_compute)(rhi_pass_t& pass);
extern void (*rhi_end_compute)(rhi_pass_t& pass);
extern void (*rhi_dispatch_compute)(uint32_t groupX, uint32_t groupY, uint32_t groupZ);

extern void (*rhi_begin_render)(rhi_pass_t& pass);
extern void (*rhi_end_render)(rhi_pass_t& pass);
extern void (*rhi_set_viewport)(int32_t x, int32_t y, int32_t width, int32_t height);
extern void (*rhi_set_scissor)(int32_t x, int32_t y, int32_t width, int32_t height);
extern void (*rhi_draw_mesh_task)(uint32_t groupX, uint32_t groupY, uint32_t groupZ);

extern rhi_mesh_t (*rhi_create_mesh)(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
extern void (*rhi_destroy_mesh)(rhi_mesh_t& mesh);
extern void (*rhi_draw_mesh)(rhi_mesh_t const& mesh);

extern rhi_meshlet_t (*rhi_create_meshlet)(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
extern void (*rhi_destroy_meshlet)(rhi_meshlet_t& meshlet);
extern void (*rhi_draw_meshlet)(rhi_meshlet_t const& meshlet);

extern void (*rhi_begin_transfer)(rhi_pass_t& pass);

extern rhi_mesh_t (*rhi_create_mesh_screen)();
extern void (*rhi_draw_screen)(int width, int height, rhi_texture_t texture, rhi_color_t clear);
