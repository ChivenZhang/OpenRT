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
#ifdef DIRECTX_IMPLEMENTATION
#include "OpenRT.h"
#include <d3d12.h>

void dx_load_library(ID3D12Device* device, ID3D12CommandQueue* queue);
void dx_unload_library();

rt_buffer_t dx_create_buffer(rt_buffer_info_t const& info);
void dx_destroy_buffer(rt_buffer_t& buffer);
void dx_bind_buffer(rt_buffer_t& buffer, rt_buffer_bind_t bind = {});
void* dx_map_buffer(rt_buffer_t& buffer, rt_access_t mode, size_t offset, size_t size);
void dx_unmap_buffer(rt_buffer_t& buffer);

rt_texture_t dx_create_texture(rt_texture_info_t const& info);
void dx_destroy_texture(rt_texture_t& texture);
void dx_bind_texture(rt_texture_t& texture, rt_texture_bind_t bind = {});

rt_texture_view_t dx_create_texture_view(rt_texture_t& texture, rt_texture_view_info_t const& info);
void dx_destroy_texture_view(rt_texture_view_t& view);
void dx_bind_texture_view(rt_texture_view_t& view, rt_texture_view_bind_t bind = {});
void dx_bind_texture_storage(rt_texture_view_t& view, rt_texture_storage_bind_t bind = {});

rt_sampler_t dx_create_sampler(rt_sampler_info_t const& info);
void dx_destroy_sampler(rt_sampler_t& sampler);
void dx_bind_sampler(rt_sampler_t& sampler, rt_sampler_bind_t bind = {});

rt_module_compute_t dx_create_module_compute(rt_module_compute_info_t const& info);
rt_module_render_t dx_create_module_render(rt_module_render_info_t const& info);
void dx_destroy_module_render(rt_module_render_t& module);
void dx_destroy_module_compute(rt_module_compute_t& module);

void dx_begin_compute(rt_pass_compute_t& pass);
void dx_end_compute(rt_pass_compute_t& pass);
void dx_bind_module_compute(rt_module_compute_t& module);
void dx_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ);
void dx_dispatch_compute_indirect(rt_buffer_t& indirect, size_t offset);

void dx_begin_render(rt_pass_render_t& pass);
void dx_end_render(rt_pass_render_t& pass);
void dx_bind_module_render(rt_module_render_t& module);
void dx_set_viewport(float x, float y, float width, float height, float minDepth, float maxDepth);
void dx_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height);
void dx_set_blend_constant(float r, float g, float b, float a);
void dx_set_stencil_reference(int32_t refer);
void dx_draw_array(rt_buffer_t vbo[], uint32_t vbo_num, uint32_t vertex_num, uint32_t instance_num, uint32_t vertex_start, uint32_t instance_start);
void dx_draw_index(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& ebo, uint32_t vertex_num, uint32_t instance_num, uint32_t vertex_start, uint32_t instance_start);
void dx_draw_array_indirect(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& indirect, size_t offset);
void dx_draw_index_indirect(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& ebo, rt_buffer_t& indirect, size_t offset);
void dx_draw_mesh_task(uint32_t groupX, uint32_t groupY = 1, uint32_t groupZ = 1);
void dx_draw_mesh_task_indirect(rt_buffer_t& indirect, size_t offset, uint32_t draw_count, uint32_t draw_stride);

void dx_push_constant(uint8_t const* buffer, size_t length);
void dx_push_const_int(const char* name, int32_t value);
void dx_push_const_uint(const char* name, uint32_t value);
void dx_push_const_float(const char* name, float value);
void dx_push_const_vec2(const char* name, const float* value);
void dx_push_const_vec3(const char* name, const float* value);
void dx_push_const_vec4(const char* name, const float* value);
void dx_push_const_mat3(const char* name, const float* value);
void dx_push_const_mat4(const char* name, const float* value);

void dx_begin_transfer(rt_pass_transfer_t& pass);
void dx_end_transfer(rt_pass_transfer_t& pass);
void dx_copy_buffer(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize);
void dx_copy_buffer_data(rt_buffer_data_t source, rt_buffer_copy_t destination, size_t copySize);
void dx_copy_buffer_texture(rt_texture_copy_t source, rt_buffer_texel_t destination, rt_size_t copySize);
void dx_copy_texture(rt_texture_copy_t source, rt_texture_copy_t destination, rt_size_t copySize);
void dx_copy_texture_data(rt_texture_data_t source, rt_texture_copy_t destination, rt_size_t copySize);
void dx_copy_texture_buffer(rt_buffer_texel_t source, rt_texture_copy_t destination, rt_size_t copySize);

rt_mesh_t dx_create_mesh(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
void dx_destroy_mesh(rt_mesh_t& mesh);
void dx_draw_mesh(rt_mesh_t& mesh);
void dx_draw_mesh_multi(rt_mesh_t& mesh, uint32_t count);

rt_meshlet_t dx_create_meshlet(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
void dx_destroy_meshlet(rt_meshlet_t& meshlet);
void dx_draw_meshlet(rt_meshlet_t& meshlet);

void dx_submit();

#endif
