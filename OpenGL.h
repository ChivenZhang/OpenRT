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
#ifdef OPENGL_IMPLEMENTATION
#include "OpenRHI.h"
#include <GL/glew.h>

// ====================================================================

using gl_buffer_t = rhi_buffer_t;
using gl_buffer_desc_t = rhi_buffer_desc_t;
using gl_buffer_bind_t = rhi_buffer_bind_t;

using gl_texture_t = rhi_texture_t;
using gl_texture_desc_t = rhi_texture_desc_t;
using gl_texture_bind_t = rhi_texture_bind_t;
using gl_texture_storage_bind_t = rhi_texture_storage_bind_t;

using gl_image_t = rhi_image_t;

using gl_sampler_t = rhi_sampler_t;
using gl_sampler_desc_t = rhi_sampler_desc_t;
using gl_sampler_bind_t = rhi_sampler_bind_t;

using gl_module_t = rhi_module_t;

using gl_color_t = rhi_color_t;
using gl_pass_t = rhi_pass_t;

using gl_mesh_t = rhi_mesh_t;
using gl_meshlet_t = rhi_meshlet_t;

// ====================================================================

/* @brief Load the OpenGL function pointers and initialize the extension library. */
void gl_load_library();

/*
 * @brief Create a buffer object with the given size, usage hint, and optional initial data.
 * @param info Buffer creation data.
 * @return The created buffer.
 */
gl_buffer_t gl_create_buffer(gl_buffer_desc_t const& info);

/*
 * @brief Delete a buffer object and reset its handle to zero.
 * @param buffer The buffer object to delete.
 */
void gl_destroy_buffer(gl_buffer_t& buffer);
/*
 * @brief Bind a buffer to an indexed binding point of the given target.
 * @param buffer The buffer object to bind.
 * @param bind   Binding descriptor specifying the target and binding index.
 */
void gl_bind_buffer(gl_buffer_t buffer, gl_buffer_bind_t bind = {});
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
 * @param info Texture creation data.
 * @return The created texture.
 */
gl_texture_t gl_create_texture(gl_texture_desc_t const& info);

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
 * @param bind    Binding descriptor specifying the texture unit and aspect mode.
 */
void gl_bind_texture(gl_texture_t texture, gl_texture_bind_t bind = {});
/*
 * @brief Bind a texture to an image unit for shader read/write access.
 * @param texture The texture to bind.
 * @param bind    Binding descriptor specifying the image unit, levels, layers, and access mode.
 */
void gl_bind_texture_storage(gl_texture_t texture, gl_texture_storage_bind_t bind = {});
/*
 * @brief Upload an image to the GPU and return the resulting texture.
 * @param image The image to upload.
 * @return The created texture.
 */
gl_texture_t gl_load_texture(gl_image_t const& image);
/*
 * @brief Download a texture into a host memory buffer and return it as an image.
 * @param texture The texture to download.
 * @param buffer  Destination host memory buffer.
 * @param length  Size of the destination buffer in bytes.
 * @return The image referencing the downloaded data.
 */
gl_image_t gl_load_image(gl_texture_t const& texture, void* buffer, size_t length);

/*
 * @brief Create a sampler object with the specified filters and wrap modes.
 * @param info Sampler creation data.
 * @return The created sampler.
 */
gl_sampler_t gl_create_sampler(gl_sampler_desc_t const& info);
/*
 * @brief Delete a sampler object and reset its handle to zero.
 * @param sampler The sampler to delete.
 */
void gl_destroy_sampler(gl_sampler_t& sampler);
/*
 * @brief Bind a sampler object to a texture unit.
 * @param sampler The sampler to bind.
 * @param bind    Binding descriptor specifying the texture unit.
 */
void gl_bind_sampler(gl_sampler_t sampler, gl_sampler_bind_t bind = {});

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
void gl_push_const_int(const char* name, int32_t value);
/*
 * @brief Set an unsigned integer uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Unsigned integer value to set.
 */
void gl_push_const_uint(const char* name, uint32_t value);
/*
 * @brief Set a float uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Float value to set.
 */
void gl_push_const_float(const char* name, float value);
/*
 * @brief Set a vec2 uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Pointer to the 2-component float vector.
 */
void gl_push_const_vec2(const char* name, const float* value);
/*
 * @brief Set a vec3 uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Pointer to the 3-component float vector.
 */
void gl_push_const_vec3(const char* name, const float* value);
/*
 * @brief Set a vec4 uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Pointer to the 4-component float vector.
 */
void gl_push_const_vec4(const char* name, const float* value);
/*
 * @brief Set a mat3 uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Pointer to the 3x3 matrix in column-major order.
 */
void gl_push_const_mat3(const char* name, const float* value);
/*
 * @brief Set a mat4 uniform of the currently bound program.
 * @param name  Uniform name.
 * @param value Pointer to the 4x4 matrix in column-major order.
 */
void gl_push_const_mat4(const char* name, const float* value);

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
 * @brief Draw meshlets with the given work-group counts.
 * @param groupX Number of work groups in the X dimension.
 * @param groupY Number of work groups in the Y dimension.
 * @param groupZ Number of work groups in the Z dimension.
 */
void gl_draw_mesh_task(uint32_t groupX, uint32_t groupY = 1, uint32_t groupZ = 1);

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
void gl_draw_mesh(gl_mesh_t const& mesh);

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
/*
 * @brief Draw a meshlet.
 * @param mesh The meshlet to draw.
 */
void gl_draw_meshlet(gl_meshlet_t const& meshlet);

/* @brief Create a fullscreen triangle mesh for screen-space drawing. */
gl_mesh_t gl_create_mesh_screen();
/*
 * @brief Draw a texture to the screen.
 * @param width   Screen width in pixels.
 * @param height  Screen height in pixels.
 * @param texture The texture to draw.
 * @param color   Optional clear color.
 */
void gl_draw_screen(int width, int height, gl_texture_t texture, gl_color_t clear = {});

// ====================================================================
#endif