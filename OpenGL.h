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
#include "OpenRT.h"
#include <GL/glew.h>

void gl_load_library();
void gl_unload_library();

rt_buffer_t gl_create_buffer(rt_buffer_info_t const& info);
void gl_destroy_buffer(rt_buffer_t& buffer);
void gl_bind_buffer(rt_buffer_t buffer, rt_buffer_bind_t bind = {});
void* gl_map_buffer(rt_buffer_t& buffer, GLenum mode, size_t offset, size_t size);
void gl_unmap_buffer(rt_buffer_t& buffer);

rt_texture_t gl_create_texture(rt_texture_info_t const& info);
rt_texture_t gl_create_texture_color(uint32_t width, uint32_t height, const void* data);
rt_texture_t gl_create_texture_color_float(uint32_t width, uint32_t height, const void* data);
rt_texture_t gl_create_texture_depth(uint32_t width, uint32_t height, const void* data);
rt_texture_t gl_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data);
void gl_destroy_texture(rt_texture_t& texture);
void gl_bind_texture(rt_texture_t texture, rt_texture_bind_t bind = {});
void gl_bind_texture_storage(rt_texture_t texture, rt_texture_storage_bind_t bind = {});

rt_sampler_t gl_create_sampler(rt_sampler_info_t const& info);
void gl_destroy_sampler(rt_sampler_t& sampler);
void gl_bind_sampler(rt_sampler_t sampler, rt_sampler_bind_t bind = {});

rt_module_compute_t gl_create_module_compute(rt_module_compute_info_t const& info);
rt_module_render_t gl_create_module_render(rt_module_render_info_t const& info);
rt_module_render_t gl_create_module_meshlet(rt_module_render_info_t const& info);
void gl_destroy_module_render(rt_module_render_t& module);
void gl_destroy_module_compute(rt_module_compute_t& module);

void gl_push_constant(uint8_t const* buffer, size_t length);
void gl_push_const_int(const char* name, int32_t value);
void gl_push_const_uint(const char* name, uint32_t value);
void gl_push_const_float(const char* name, float value);
void gl_push_const_vec2(const char* name, const float* value);
void gl_push_const_vec3(const char* name, const float* value);
void gl_push_const_vec4(const char* name, const float* value);
void gl_push_const_mat3(const char* name, const float* value);
void gl_push_const_mat4(const char* name, const float* value);

void gl_begin_compute(rt_pass_compute_t& pass);
void gl_end_compute(rt_pass_compute_t& pass);
void gl_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ);

void gl_begin_render(rt_pass_render_t& pass);
void gl_end_render(rt_pass_render_t& pass);
void gl_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height);
void gl_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height);
void gl_draw_mesh_task(uint32_t groupX, uint32_t groupY = 1, uint32_t groupZ = 1);

void gl_begin_transfer(rt_pass_transfer_t& pass);
void gl_end_transfer(rt_pass_transfer_t& pass);
void gl_copy_buffer(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize);
void gl_copy_buffer_data(rt_buffer_data_t source, rt_buffer_copy_t destination, size_t copySize);
void gl_copy_buffer_texture(rt_texture_copy_t source, rt_buffer_texel_t destination, rt_size_t copySize);
void gl_copy_texture(rt_texture_copy_t source, rt_texture_copy_t destination, rt_size_t copySize);
void gl_copy_texture_data(rt_texture_data_t source, rt_texture_copy_t destination, rt_size_t copySize);
void gl_copy_texture_buffer(rt_buffer_texel_t source, rt_texture_copy_t destination, rt_size_t copySize);

rt_mesh_t gl_create_mesh(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
void gl_destroy_mesh(rt_mesh_t& mesh);
void gl_draw_mesh(rt_mesh_t const& mesh);
void gl_draw_mesh_multi(rt_mesh_t const& mesh, uint32_t count);

rt_meshlet_t gl_create_meshlet(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
void gl_destroy_meshlet(rt_meshlet_t& meshlet);
void gl_draw_meshlet(rt_meshlet_t const& meshlet);

rt_mesh_t gl_create_mesh_screen();
void gl_draw_screen(int width, int height, rt_texture_t texture, rt_color_t clear = {});

void gl_submit();

#endif