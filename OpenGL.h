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

void gl_load_library();
void gl_unload_library();

gl_buffer_t gl_create_buffer(gl_buffer_desc_t const& info);
void gl_destroy_buffer(gl_buffer_t& buffer);
void gl_bind_buffer(gl_buffer_t buffer, gl_buffer_bind_t bind = {});
void gl_read_buffer(gl_buffer_t buffer, size_t offset, size_t size, void* data);
void gl_write_buffer(gl_buffer_t buffer, size_t offset, size_t size, const void* data);

gl_texture_t gl_create_texture(gl_texture_desc_t const& info);
gl_texture_t gl_create_texture_color(uint32_t width, uint32_t height, const void* data);
gl_texture_t gl_create_texture_depth(uint32_t width, uint32_t height, const void* data);
gl_texture_t gl_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data);
void gl_destroy_texture(gl_texture_t& texture);
void gl_bind_texture(gl_texture_t texture, gl_texture_bind_t bind = {});
void gl_bind_texture_storage(gl_texture_t texture, gl_texture_storage_bind_t bind = {});
gl_texture_t gl_load_texture(gl_image_t const& image);
gl_image_t gl_load_image(gl_texture_t const& texture, void* buffer, size_t length);

gl_sampler_t gl_create_sampler(gl_sampler_desc_t const& info);
void gl_destroy_sampler(gl_sampler_t& sampler);
void gl_bind_sampler(gl_sampler_t sampler, gl_sampler_bind_t bind = {});

gl_module_t gl_create_module_compute(const char* comp_src, rhi_compute_info_t const& info);
gl_module_t gl_create_module_render(const char* vert_src, const char* frag_src, rhi_render_info_t const& info);
gl_module_t gl_create_module_meshlet(const char* task_src, const char* mesh_src, const char* frag_src, rhi_render_info_t const& info);
void gl_destroy_module(gl_module_t& module);

void gl_push_constant(uint8_t const* buffer, size_t length);
void gl_push_const_int(const char* name, int32_t value);
void gl_push_const_uint(const char* name, uint32_t value);
void gl_push_const_float(const char* name, float value);
void gl_push_const_vec2(const char* name, const float* value);
void gl_push_const_vec3(const char* name, const float* value);
void gl_push_const_vec4(const char* name, const float* value);
void gl_push_const_mat3(const char* name, const float* value);
void gl_push_const_mat4(const char* name, const float* value);

void gl_begin_compute(gl_pass_t& pass);
void gl_end_compute(gl_pass_t& pass);
void gl_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ);

void gl_begin_render(gl_pass_t& pass);
void gl_end_render(gl_pass_t& pass);
void gl_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height);
void gl_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height);
void gl_draw_mesh_task(uint32_t groupX, uint32_t groupY = 1, uint32_t groupZ = 1);

void gl_begin_transfer(rhi_pass_t& pass);
void gl_end_transfer(rhi_pass_t& pass);
void gl_copy_buffer(rhi_buffer_copy_t source, rhi_buffer_copy_t destination, size_t copySize);
void gl_copy_buffer_data(rhi_buffer_data_t source, rhi_buffer_copy_t destination, size_t copySize);
void gl_copy_buffer_texture(rhi_texture_copy_t source, rhi_buffer_texel_t destination, rhi_vec3_t copySize);
void gl_copy_texture(rhi_texture_copy_t source, rhi_texture_copy_t destination, rhi_vec3_t copySize);
void gl_copy_texture_data(rhi_texture_data_t source, rhi_texture_copy_t destination, rhi_vec3_t copySize);
void gl_copy_texture_buffer(rhi_buffer_texel_t source, rhi_texture_copy_t destination, rhi_vec3_t copySize);

gl_mesh_t gl_create_mesh(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
void gl_destroy_mesh(gl_mesh_t& mesh);
void gl_draw_mesh(gl_mesh_t const& mesh);

gl_meshlet_t gl_create_meshlet(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
void gl_destroy_meshlet(gl_meshlet_t& meshlet);
void gl_draw_meshlet(gl_meshlet_t const& meshlet);

gl_mesh_t gl_create_mesh_screen();
void gl_draw_screen(int width, int height, gl_texture_t texture, gl_color_t clear = {});

// ====================================================================
#endif