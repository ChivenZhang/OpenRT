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
#ifdef VULKAN_IMPLEMENTATION
#include "OpenRHI.h"
#include <vulkan/vulkan.h>

void vk_load_library(VkInstance instance, VkDevice device, uint32_t family);
void vk_unload_library();

rhi_buffer_t vk_create_buffer(rhi_buffer_desc_t const& info);
void vk_destroy_buffer(rhi_buffer_t& buffer);
void vk_bind_buffer(rhi_buffer_t buffer, rhi_buffer_bind_t bind = {});
void* vk_map_buffer(rhi_buffer_t& buffer, GLenum mode, size_t offset, size_t size);
void vk_unmap_buffer(rhi_buffer_t& buffer);

rhi_texture_t vk_create_texture(rhi_texture_desc_t const& info);
rhi_texture_t vk_create_texture_color(uint32_t width, uint32_t height, const void* data);
rhi_texture_t vk_create_texture_depth(uint32_t width, uint32_t height, const void* data);
rhi_texture_t vk_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data);
void vk_destroy_texture(rhi_texture_t& texture);
void vk_bind_texture(rhi_texture_t texture, rhi_texture_bind_t bind = {});
void vk_bind_texture_storage(rhi_texture_t texture, rhi_texture_storage_bind_t bind = {});

rhi_sampler_t vk_create_sampler(rhi_sampler_desc_t const& info);
void vk_destroy_sampler(rhi_sampler_t& sampler);
void vk_bind_sampler(rhi_sampler_t sampler, rhi_sampler_bind_t bind = {});

rhi_module_t vk_create_module_compute(const char* comp_src, rhi_compute_info_t const& info);
rhi_module_t vk_create_module_render(const char* vert_src, const char* frag_src, rhi_render_info_t const& info);
rhi_module_t vk_create_module_meshlet(const char* task_src, const char* mesh_src, const char* frag_src, rhi_render_info_t const& info);
void vk_destroy_module(rhi_module_t& module);

void vk_push_constant(uint8_t const* buffer, size_t length);
void vk_push_const_int(const char* name, int32_t value);
void vk_push_const_uint(const char* name, uint32_t value);
void vk_push_const_float(const char* name, float value);
void vk_push_const_vec2(const char* name, const float* value);
void vk_push_const_vec3(const char* name, const float* value);
void vk_push_const_vec4(const char* name, const float* value);
void vk_push_const_mat3(const char* name, const float* value);
void vk_push_const_mat4(const char* name, const float* value);

void vk_begin_compute(rhi_pass_t& pass);
void vk_end_compute(rhi_pass_t& pass);
void vk_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ);

void vk_begin_render(rhi_pass_t& pass);
void vk_end_render(rhi_pass_t& pass);
void vk_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height);
void vk_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height);
void vk_draw_mesh_task(uint32_t groupX, uint32_t groupY = 1, uint32_t groupZ = 1);

rhi_mesh_t vk_create_mesh(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
void vk_destroy_mesh(rhi_mesh_t& mesh);
void vk_draw_mesh(rhi_mesh_t const& mesh);

rhi_meshlet_t vk_create_meshlet(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count);
void vk_destroy_meshlet(rhi_meshlet_t& meshlet);
void vk_draw_meshlet(rhi_meshlet_t const& meshlet);

rhi_mesh_t vk_create_mesh_screen();
void vk_draw_screen(int width, int height, rhi_texture_t texture, rhi_color_t clear = {});

#endif