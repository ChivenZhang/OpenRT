#pragma once
#include <iostream>
#include <GL/glew.h>

#define GL_PI 3.14159265358979323846 // pi
#define GL_PI_2 1.57079632679489661923 // pi/2
#define GL_PI_4 0.785398163397448309616 // pi/4
#define GL_1_PI 0.318309886183790671538 // 1/pi
#define GL_2_PI 0.636619772367581343076 // 2/pi

/*
   Do this:
      #define OPENGL_IMPLEMENTATION
   before you include this file in *one* C / C++ file to create the implementation.

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
    GLenum target = GL_UNIFORM_BUFFER; // GL_UNIFORM_BUFFER / GL_SHADER_STORAGE_BUFFER
};

struct gl_texture_t
{
    GLuint handle = 0;
    uint32_t width = 0, height = 0;
    GLenum target = GL_TEXTURE_2D;
    GLenum format = GL_RGBA;
    GLenum internal_format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;
    bool mipmaps = false;
};

struct gl_texture_create_t
{
    uint32_t width = 0;
    uint32_t height = 0;
    GLenum target = GL_TEXTURE_2D;
    GLenum format = GL_RGBA;                // 数据格式
    GLenum internal_format = GL_RGBA8;      // 内部存储格式
    GLenum type = GL_UNSIGNED_BYTE;         // 数据类型
    GLenum wrap_s = GL_REPEAT;
    GLenum wrap_t = GL_REPEAT;
    GLenum wrap_r = GL_REPEAT;              // 3D 纹理使用
    GLenum min_filter = GL_NEAREST_MIPMAP_LINEAR;
    GLenum mag_filter = GL_LINEAR;
    GLfloat border[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const void* data = nullptr;
};

struct gl_texture_bind_t
{
    uint32_t binding = 0;
    GLenum aspect_mode = GL_DEPTH_COMPONENT; // GL_DEPTH_COMPONENT / GL_STENCIL_INDEX
};

struct gl_texture_storage_bind_t
{
    uint32_t binding = 0;
    uint32_t base_level = 0;
    uint32_t base_layer = 0;
    uint32_t level_count = 1;
    uint32_t layer_count = 1;
    GLenum access = GL_WRITE_ONLY; // GL_WRITE_ONLY / GL_READ_ONLY / GL_READ_WRITE
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

struct gl_sampler_bind_t
{
    uint32_t binding = 0;
};

struct gl_module_t
{
    GLuint handle = 0;
    GLenum target = GL_NONE;
};

struct gl_color_t
{
    float r = 0, g = 0, b = 0, a = 0;
};

struct gl_pass_t
{
    GLuint handle = 0;
    gl_module_t module;

    // Offscreen Mode

    struct
    {
        gl_texture_t texture;
        bool clear = false;
        gl_color_t value;
        struct
        {
            GLenum func = GL_ADD; // GL_ADD / GL_SUBTRACT / GL_REVERSE_SUBTRACT / GL_MIN / GL_MAX
            GLenum src = GL_ONE; // GL_ZERO / GL_ONE / GL_SRC_COLOR / GL_ONE_MINUS_SRC_COLOR / GL_DST_COLOR / GL_ONE_MINUS_DST_COLOR / GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA / GL_DST_ALPHA / GL_ONE_MINUS_DST_ALPHA / GL_CONSTANT_COLOR / GL_ONE_MINUS_CONSTANT_COLOR / GL_CONSTANT_ALPHA / GL_ONE_MINUS_CONSTANT_ALPHA / GL_SRC_ALPHA_SATURATE
            GLenum dst = GL_ZERO; // GL_ZERO / GL_ONE / GL_SRC_COLOR / GL_ONE_MINUS_SRC_COLOR / GL_DST_COLOR / GL_ONE_MINUS_DST_COLOR / GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA / GL_DST_ALPHA / GL_ONE_MINUS_DST_ALPHA / GL_CONSTANT_COLOR / GL_ONE_MINUS_CONSTANT_COLOR / GL_CONSTANT_ALPHA / GL_ONE_MINUS_CONSTANT_ALPHA / GL_SRC_ALPHA_SATURATE
        } color, alpha;
    } colors[2];
    struct
    {
        gl_texture_t texture;
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
        uint32_t read = (uint32_t)-1;
        uint32_t write = (uint32_t)-1;
        int32_t value = -1;
        int32_t refer = 0;
        struct
        {
            GLenum func = GL_ALWAYS; // GL_NEVER / GL_LESS / GL_EQUAL / GL_LEQUAL / GL_GREATER / GL_NOTEQUAL / GL_GEQUAL / GL_ALWAYS
            GLenum sfail = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
            GLenum dpfail = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
            GLenum dppass = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
        } back, front;
    } stencil;

    // Screen Mode

    struct
    {
        struct
        {
            bool clear = false;
            gl_color_t value;
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
            uint32_t read = (uint32_t)-1;
            uint32_t write = (uint32_t)-1;
            int32_t value = -1;
            int32_t refer = 0;
            GLenum func = GL_ALWAYS; // GL_NEVER / GL_LESS / GL_EQUAL / GL_LEQUAL / GL_GREATER / GL_NOTEQUAL / GL_GEQUAL / GL_ALWAYS
            GLenum sfail = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
            GLenum dpfail = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
            GLenum dppass = GL_KEEP; // GL_KEEP / GL_ZERO / GL_REPLACE / GL_INCR / GL_INCR_WRAP / GL_DECR / GL_DECR_WRAP / GL_INVERT
        } stencil;
    } screen;

    GLenum cull_mode = GL_BACK; // GL_NONE / GL_FRONT / GL_BACK / GL_FRONT_AND_BACK
    GLenum front_face = GL_CCW; // GL_CW / GL_CCW
    GLenum fill_mode = GL_FILL; // GL_POINT / GL_LINE / GL_FILL
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

/* @brief Load the OpenGL function pointers and initialize the extension library. */
void gl_load_library();

/*
 * @brief Create a buffer object with the given size, usage hint, and optional initial data.
 * @param size  Size of the buffer in bytes.
 * @param usage Usage hint for the buffer (e.g. GL_STATIC_DRAW).
 * @param data  Optional pointer to initial data, or nullptr.
 */
gl_buffer_t gl_create_buffer(size_t size, GLenum usage = GL_STATIC_DRAW, const void* data = nullptr);
/*
 * @brief Delete a buffer object and reset its handle to zero.
 * @param buffer The buffer object to delete.
 */
void gl_destroy_buffer(gl_buffer_t& buffer);
/*
 * @brief Bind a buffer to an indexed binding point of the given target.
 * @param buffer The buffer object to bind.
 * @param desc   Binding descriptor specifying the target and binding index.
 */
void gl_bind_buffer(gl_buffer_t buffer, gl_buffer_bind_t desc = {});
/*
 * @brief Read a range of bytes from a buffer into host memory.
 * @param buffer The buffer object to read from.
 * @param offset Byte offset from the start of the buffer.
 * @param size   Number of bytes to read.
 * @param data   Pointer to the destination host memory.
 */
void gl_read_buffer(gl_buffer_t buffer, size_t offset, size_t size, void* data);
/*
 * @brief Write a range of bytes from host memory into a buffer.
 * @param buffer The buffer object to write into.
 * @param offset Byte offset from the start of the buffer.
 * @param size   Number of bytes to write.
 * @param data   Pointer to the source host memory.
 */
void gl_write_buffer(gl_buffer_t buffer, size_t offset, size_t size, const void* data);

/*
 * @brief Create a texture with optional initial info data.
 * @param info   Texture creation data.
 */
gl_texture_t gl_create_texture(gl_texture_create_t const& info);

/*
 * @brief Create an RGBA color texture with optional initial pixel data.
 * @param width  Texture width in pixels.
 * @param height Texture height in pixels.
 * @param data   Optional pointer to initial pixel data, or nullptr.
 */
gl_texture_t gl_create_texture_color(uint32_t width, uint32_t height, const void* data);
/*
 * @brief Create a depth texture with optional initial depth data.
 * @param width  Texture width in pixels.
 * @param height Texture height in pixels.
 * @param data   Optional pointer to initial depth data, or nullptr.
 */
gl_texture_t gl_create_texture_depth(uint32_t width, uint32_t height, const void* data);
/*
 * @brief Create a combined depth-stencil texture with optional initial data.
 * @param width  Texture width in pixels.
 * @param height Texture height in pixels.
 * @param data   Optional pointer to initial depth-stencil data, or nullptr.
 */
gl_texture_t gl_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data);
/*
 * @brief Delete a texture and reset its handle to zero.
 * @param texture The texture to delete.
 */
void gl_destroy_texture(gl_texture_t& texture);
/*
 * @brief Bind a texture to a texture unit.
 * @param texture The texture to bind.
 * @param desc    Binding descriptor specifying the texture unit and aspect mode.
 */
void gl_bind_texture(gl_texture_t texture, gl_texture_bind_t desc = {});
/*
 * @brief Bind a texture to an image unit for shader read/write access.
 * @param texture The texture to bind.
 * @param desc    Binding descriptor specifying the image unit, levels, layers, and access mode.
 */
void gl_bind_texture_storage(gl_texture_t texture, gl_texture_storage_bind_t desc = {});
/*
 * @brief Upload an image to the GPU and return the resulting texture.
 * @param image The image to upload.
 * @return The created texture.
 */
gl_texture_t gl_load_texture(gl_image_t image);
/*
 * @brief Download a texture into a host memory buffer and return it as an image.
 * @param texture The texture to download.
 * @param buffer  Destination host memory buffer.
 * @param length  Size of the destination buffer in bytes.
 * @return The image referencing the downloaded data.
 */
gl_image_t gl_load_image(gl_texture_t texture, void* buffer, size_t length);

/*
 * @brief Create a sampler object with the specified filters and wrap modes.
 * @param min_filter Minification filter (e.g. GL_LINEAR_MIPMAP_LINEAR).
 * @param mag_filter Magnification filter (e.g. GL_LINEAR).
 * @param wrap_s     Wrap mode for the S axis (e.g. GL_REPEAT).
 * @param wrap_t     Wrap mode for the T axis (e.g. GL_REPEAT).
 * @param wrap_r     Wrap mode for the R axis (e.g. GL_REPEAT).
 * @return The created sampler.
 */
gl_sampler_t gl_create_sampler(GLenum min_filter, GLenum mag_filter, GLenum wrap_s, GLenum wrap_t, GLenum wrap_r);
/*
 * @brief Delete a sampler object and reset its handle to zero.
 * @param sampler The sampler to delete.
 */
void gl_destroy_sampler(gl_sampler_t& sampler);
/*
 * @brief Bind a sampler object to a texture unit.
 * @param sampler The sampler to bind.
 * @param desc    Binding descriptor specifying the texture unit.
 */
void gl_bind_sampler(gl_sampler_t sampler, gl_sampler_bind_t desc = {});

/*
 * @brief Create a compute module from a compute shader source string.
 * @param comp_src Compute shader source code.
 * @return The created module.
 */
gl_module_t gl_create_module_compute(const char* comp_src);
/*
 * @brief Create a render module from vertex and fragment shader source strings.
 * @param vert_src Vertex shader source code, or nullptr.
 * @param frag_src Fragment shader source code, or nullptr.
 * @return The created module.
 */
gl_module_t gl_create_module_render(const char* vert_src, const char* frag_src);
/*
 * @brief Create a meshlet module from task, mesh, and fragment shader source strings.
 * @param task_src Task shader source code, or nullptr.
 * @param mesh_src Mesh shader source code.
 * @param frag_src Fragment shader source code.
 * @return The created module.
 */
gl_module_t gl_create_module_meshlet(const char* task_src, const char* mesh_src, const char* frag_src);
/*
 * @brief Delete a shader module (program) and reset its handle to zero.
 * @param module The module to delete.
 */
void gl_destroy_module(gl_module_t& module);

/*
 * @brief Set an integer uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Integer value to set.
 */
void gl_set_uniform_int(const char* name, int32_t value);
/*
 * @brief Set an unsigned integer uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Unsigned integer value to set.
 */
void gl_set_uniform_uint(const char* name, uint32_t value);
/*
 * @brief Set a float uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Float value to set.
 */
void gl_set_uniform_float(const char* name, float value);
/*
 * @brief Set a vec2 uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Pointer to the 2-component float vector.
 */
void gl_set_uniform_vec2(const char* name, const float* value);
/*
 * @brief Set a vec3 uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Pointer to the 3-component float vector.
 */
void gl_set_uniform_vec3(const char* name, const float* value);
/*
 * @brief Set a vec4 uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Pointer to the 4-component float vector.
 */
void gl_set_uniform_vec4(const char* name, const float* value);
/*
 * @brief Set a mat3 uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Pointer to the 3x3 matrix in column-major order.
 */
void gl_set_uniform_mat3(const char* name, const float* value);
/*
 * @brief Set a mat4 uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Pointer to the 4x4 matrix in column-major order.
 */
void gl_set_uniform_mat4(const char* name, const float* value);

/*
 * @brief Begin a compute pass.
 * @param pass The compute pass descriptor.
 */
void gl_begin_compute(gl_pass_t& pass);
/*
 * @brief End a compute pass.
 * @param pass The compute pass descriptor.
 */
void gl_end_compute(gl_pass_t& pass);
/*
 * @brief Dispatch a compute workload with the given work-group counts.
 * @param groupX Number of work groups in the X dimension.
 * @param groupY Number of work groups in the Y dimension.
 * @param groupZ Number of work groups in the Z dimension.
 */
void gl_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ);

/*
 * @brief Begin a render pass.
 * @param pass The render pass descriptor.
 */
void gl_begin_render(gl_pass_t& pass);
/*
 * @brief End a render pass.
 * @param pass The render pass descriptor.
 */
void gl_end_render(gl_pass_t& pass);
/*
 * @brief Set the viewport rectangle.
 * @param x      Lower-left corner X coordinate.
 * @param y      Lower-left corner Y coordinate.
 * @param width  Viewport width in pixels.
 * @param height Viewport height in pixels.
 */
void gl_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height);
/*
 * @brief Enable the scissor test and set the scissor rectangle.
 * @param x      Lower-left corner X coordinate.
 * @param y      Lower-left corner Y coordinate.
 * @param width  Scissor rectangle width in pixels.
 * @param height Scissor rectangle height in pixels.
 */
void gl_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height);

/*
 * @brief Begin a meshlet (mesh shader) pass.
 * @param pass The meshlet pass descriptor.
 */
inline void (*gl_begin_meshlet)(gl_pass_t& pass) = gl_begin_render;
/*
 * @brief End a meshlet (mesh shader) pass.
 * @param pass The meshlet pass descriptor.
 */
inline void (*gl_end_meshlet)(gl_pass_t& pass) = gl_end_render;
/*
 * @brief Draw meshlets with the given work-group counts.
 * @param groupX Number of work groups in the X dimension.
 * @param groupY Number of work groups in the Y dimension.
 * @param groupZ Number of work groups in the Z dimension.
 */
void gl_draw_meshlet(uint32_t groupX, uint32_t groupY = 1, uint32_t groupZ = 1);

/*
 * @brief Create a mesh from vertex, normal, UV, and index data.
 * @param vertices     Pointer to vertex positions (vec3), or nullptr.
 * @param normals      Pointer to vertex normals (vec3), or nullptr.
 * @param uvs          Pointer to texture coordinates (vec2), or nullptr.
 * @param vertex_count Number of vertices.
 * @param indices      Pointer to index data, or nullptr.
 * @param index_count  Number of indices.
 * @return The created mesh.
 */
gl_mesh_t gl_create_mesh(const float* vertices, const float* normals, const float* uvs, size_t vertex_count,
                         const unsigned int* indices, size_t index_count);
/*
 * @brief Delete a mesh and its associated buffers.
 * @param mesh The mesh to delete.
 */
void gl_destroy_mesh(gl_mesh_t& mesh);
/*
 * @brief Draw a mesh.
 * @param mesh The mesh to draw.
 */
void gl_draw_mesh(gl_mesh_t mesh);

/*
 * @brief Create a meshlet from vertex, normal, UV, and index data.
 * @param vertices     Pointer to vertex positions (vec4), or nullptr.
 * @param normals      Pointer to vertex normals (vec4), or nullptr.
 * @param uvs          Pointer to texture coordinates, or nullptr.
 * @param vertex_count Number of vertices.
 * @param indices      Pointer to index data, or nullptr.
 * @param index_count  Number of indices.
 * @return The created meshlet.
 */
gl_meshlet_t gl_create_meshlet(const float* vertices, const float* normals, const float* uvs, size_t vertex_count,
                               const unsigned int* indices, size_t index_count);
/*
 * @brief Delete a meshlet and its associated buffers.
 * @param meshlet The meshlet to delete.
 */
void gl_destroy_meshlet(gl_meshlet_t& meshlet);

/* @brief Create a fullscreen triangle mesh for screen-space drawing. */
gl_mesh_t gl_create_mesh_screen();
/*
 * @brief Draw a texture to the screen.
 * @param width   Screen width in pixels.
 * @param height  Screen height in pixels.
 * @param texture The texture to draw.
 * @param color   Optional clear color.
 */
void gl_draw_screen(int width, int height, gl_texture_t texture, gl_color_t color = {});

// ====================================================================

#ifdef OPENGL_IMPLEMENTATION

static void gl_load_library()
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
}

// ====================================================================

static gl_buffer_t gl_create_buffer(size_t size, GLenum usage, const void* data)
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
    default:
        break;
    case GL_UNIFORM_BUFFER:
        glBindBufferBase(GL_UNIFORM_BUFFER, desc.binding, buffer.handle);
        break;
    case GL_SHADER_STORAGE_BUFFER:
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, desc.binding, buffer.handle);
        break;
    }
}

static void gl_read_buffer(gl_buffer_t buffer, size_t offset, size_t size, void* data)
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

static void gl_write_buffer(gl_buffer_t buffer, size_t offset, size_t size, const void* data)
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

static gl_texture_t gl_create_texture(gl_texture_create_t const& info)
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

static gl_texture_t gl_create_texture_color(uint32_t width, uint32_t height, const void* data)
{
    gl_texture_create_t info
    {
        .width = width,
        .height = height,
        .target = GL_TEXTURE_2D,
        .format = GL_RGBA,
        .internal_format = GL_RGBA,
        .type = GL_UNSIGNED_BYTE,
        .wrap_s = GL_REPEAT,
        .wrap_t = GL_REPEAT,
        .min_filter = GL_LINEAR_MIPMAP_LINEAR,
        .mag_filter = GL_LINEAR,
        .data = data,
    };
    return gl_create_texture(info);
}

static gl_texture_t gl_create_texture_depth(uint32_t width, uint32_t height, const void* data)
{
    gl_texture_create_t info
    {
        .width = width,
        .height = height,
        .target = GL_TEXTURE_2D,
        .format = GL_DEPTH_COMPONENT,
        .internal_format = GL_DEPTH_COMPONENT32F,
        .type = GL_FLOAT,
        .wrap_s = GL_CLAMP_TO_BORDER,
        .wrap_t = GL_CLAMP_TO_BORDER,
        .min_filter = GL_NEAREST,
        .mag_filter = GL_NEAREST,
        .border = {1.0f, 1.0f, 1.0f, 1.0f},
        .data = data,
    };
    return gl_create_texture(info);
}

static gl_texture_t gl_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data)
{
    gl_texture_create_t info
    {
        .width = width,
        .height = height,
        .target = GL_TEXTURE_2D,
        .format = GL_DEPTH_STENCIL,
        .internal_format = GL_DEPTH24_STENCIL8,
        .type = GL_UNSIGNED_INT_24_8,
        .wrap_s = GL_CLAMP_TO_EDGE,
        .wrap_t = GL_CLAMP_TO_EDGE,
        .min_filter = GL_NEAREST,
        .mag_filter = GL_NEAREST,
        .data = data,
    };
    return gl_create_texture(info);
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

static gl_texture_t gl_load_texture(gl_image_t image)
{
    return gl_create_texture_color(image.width, image.height, image.pixels);
}

static gl_image_t gl_load_image(gl_texture_t texture, void* buffer, size_t length)
{
    gl_image_t result = {};

    if (texture.target == GL_TEXTURE_2D)
    {
        if (length < texture.width * texture.height * sizeof(uint32_t))
            return result;

        glBindTexture(texture.target, texture.handle);
        glGetTexImage(texture.target, 0, texture.format, GL_UNSIGNED_BYTE, buffer);
        glBindTexture(texture.target, 0);
        result.pixels = buffer;
        result.width = texture.width;
        result.height = texture.height;
        result.format = texture.format;
    }

    return result;
}

// ====================================================================

static gl_sampler_t gl_create_sampler(GLenum min_filter, // 缩小过滤方式，如 GL_LINEAR_MIPMAP_LINEAR
                                      GLenum mag_filter, // 放大过滤方式，如 GL_LINEAR
                                      GLenum wrap_s, // S 轴环绕方式，如 GL_REPEAT
                                      GLenum wrap_t, // T 轴环绕方式，如 GL_REPEAT
                                      GLenum wrap_r) // R 轴环绕方式，如 GL_REPEAT
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

static void gl_bind_sampler(gl_sampler_t sampler, gl_sampler_bind_t desc)
{
    glBindSampler(desc.binding, sampler.handle);
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

static gl_module_t gl_create_module_render(const char* vert_src, const char* frag_src)
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

static gl_module_t gl_create_module_meshlet(const char* task_src, const char* mesh_src, const char* frag_src)
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
        if (colorCount)
            glDrawBuffers(colorCount, colorAttachments);

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

                if (pass.colors[i].color.func != GL_ADD || pass.colors[i].color.src != GL_ONE ||
                    pass.colors[i].color.dst != GL_ZERO || pass.colors[i].alpha.func != GL_ADD ||
                    pass.colors[i].alpha.src != GL_ONE || pass.colors[i].alpha.dst != GL_ZERO)
                    glEnable(GL_BLEND);
                glBlendEquationSeparatei(i, pass.colors[i].color.func, pass.colors[i].alpha.func);
                glBlendFuncSeparatei(i, pass.colors[i].color.src, pass.colors[i].color.dst, pass.colors[i].alpha.src,
                                     pass.colors[i].alpha.dst);
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

        if (pass.depth.func == GL_ALWAYS && pass.depth.write == false)
        {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }
        else
        {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(pass.depth.write);
        }
        glDepthFunc(pass.depth.func);

        if (pass.depth.bias == 0 && pass.depth.biasSlope == 0)
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
        else
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffsetClamp(pass.depth.biasSlope, pass.depth.bias, pass.depth.biasClamp);
        }

        // Stencil State

        if (pass.stencil.back.func != GL_ALWAYS || pass.stencil.back.sfail != GL_KEEP ||
            pass.stencil.back.dpfail != GL_KEEP || pass.stencil.back.dppass != GL_KEEP ||
            pass.stencil.front.func != GL_ALWAYS || pass.stencil.front.sfail != GL_KEEP ||
            pass.stencil.front.dpfail != GL_KEEP || pass.stencil.front.dppass != GL_KEEP)
        {
            glEnable(GL_STENCIL_TEST);
        }
        else
        {
            glDisable(GL_STENCIL_TEST);
        }
        glStencilMask(pass.stencil.write);
        glStencilFuncSeparate(GL_BACK, pass.stencil.back.func, pass.stencil.refer, pass.stencil.read);
        glStencilFuncSeparate(GL_FRONT, pass.stencil.front.func, pass.stencil.refer, pass.stencil.read);
        glStencilOpSeparate(GL_BACK, pass.stencil.back.sfail, pass.stencil.back.dpfail, pass.stencil.back.dppass);
        glStencilOpSeparate(GL_FRONT, pass.stencil.front.sfail, pass.stencil.front.dpfail, pass.stencil.front.dppass);
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
            pass.screen.stencil.dpfail != GL_KEEP || pass.screen.stencil.dppass != GL_KEEP)
        {
            glEnable(GL_STENCIL_TEST);
        }
        else
        {
            glDisable(GL_STENCIL_TEST);
        }
        glStencilMask(pass.screen.stencil.write);
        glStencilFunc(pass.screen.stencil.func, pass.screen.stencil.refer, pass.screen.stencil.read);
        glStencilOp(pass.screen.stencil.sfail, pass.screen.stencil.dpfail, pass.screen.stencil.dppass);
    }

    // Primitive State

    glFrontFace(pass.front_face);
    if (pass.cull_mode)
        glCullFace(pass.cull_mode);
    if (pass.cull_mode)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    if (pass.front_face)
        glEnable(GL_FRONT_FACE);
    else
        glDisable(GL_FRONT_FACE);

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

static void gl_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
    glViewport(x, y, width, height);
}

static void gl_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, width, height);
}

static void gl_draw_meshlet(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    glDrawMeshTasksNV(0, std::max(1U, groupX) * std::max(1U, groupY) * std::max(1U, groupZ));
}

// ====================================================================

static gl_mesh_t gl_create_mesh(const float* vertices, // vec3
                                const float* normals, // vec3
                                const float* uvs, // vec2
                                size_t vertex_count, const unsigned int* indices, size_t index_count)
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
    if (mesh.index_count)
        glDrawElements(mesh.primitive_type, mesh.index_count, mesh.index_type, (void*)0);
    else
        glDrawArrays(mesh.primitive_type, 0, mesh.vertex_count);
    glBindVertexArray(0);
}

// ====================================================================

static gl_meshlet_t gl_create_meshlet(const float* vertices, // vec4
                                      const float* normals, // vec4
                                      const float* uvs, size_t vertex_count, const unsigned int* indices,
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
        -1.0f,
        -1.0f,
        0.0f,
        3.0f,
        -1.0f,
        0.0f,
        -1.0f,
        3.0f,
        0.0f,
    };
    static const float uvs[] = {
        0.0f,
        0.0f,
        2.0f,
        0.0f,
        0.0f,
        2.0f,
    };
    static auto quad = gl_create_mesh(points, nullptr, uvs, 3, nullptr, 0);
    return quad;
}

static void gl_draw_screen(int width, int height, gl_texture_t texture, gl_color_t color)
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
    static auto module = gl_create_module_render(VS, FS);
    gl_pass_t pass = {.module = module,
                      .screen = {.color = {
                                     .clear = true,
                                     .value = color,
                                 }}};
    gl_begin_render(pass);
    gl_set_viewport(0, 0, width, height);
    gl_bind_texture(texture, {
                                 .binding = 0,
                             });
    gl_draw_mesh(gl_create_mesh_screen());
    gl_end_render(pass);
}

#endif