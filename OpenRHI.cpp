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
#include "OpenRHI.h"

rhi_buffer_t (*rhi_create_buffer)(rhi_buffer_desc_t const& info) = nullptr;
void (*rhi_destroy_buffer)(rhi_buffer_t& buffer) = nullptr;
void (*rhi_bind_buffer)(rhi_buffer_t buffer, rhi_buffer_bind_t bind) = nullptr;
void (*rhi_read_buffer)(rhi_buffer_t buffer, size_t offset, size_t size, void* data) = nullptr;
void (*rhi_write_buffer)(rhi_buffer_t buffer, size_t offset, size_t size, const void* data) = nullptr;

// Texture 相关
rhi_texture_t (*rhi_create_texture)(rhi_texture_desc_t const& info) = nullptr;
rhi_texture_t (*rhi_create_texture_color)(uint32_t width, uint32_t height, const void* data) = nullptr;
rhi_texture_t (*rhi_create_texture_depth)(uint32_t width, uint32_t height, const void* data) = nullptr;
rhi_texture_t (*rhi_create_texture_depth_stencil)(uint32_t width, uint32_t height, const void* data) = nullptr;
void (*rhi_destroy_texture)(rhi_texture_t& texture) = nullptr;
void (*rhi_bind_texture)(rhi_texture_t texture, rhi_texture_bind_t bind) = nullptr;
void (*rhi_bind_texture_storage)(rhi_texture_t texture, rhi_texture_storage_bind_t bind) = nullptr;
rhi_texture_t (*rhi_load_texture)(rhi_image_t const& image) = nullptr;
rhi_image_t (*rhi_load_image)(rhi_texture_t const& texture, void* buffer, size_t length) = nullptr;

// Sampler 相关
rhi_sampler_t (*rhi_create_sampler)(rhi_sampler_desc_t const& info) = nullptr;
void (*rhi_destroy_sampler)(rhi_sampler_t& sampler) = nullptr;
void (*rhi_bind_sampler)(rhi_sampler_t sampler, rhi_sampler_bind_t bind) = nullptr;

// Module 相关
rhi_module_t (*rhi_create_module_compute)(const char* comp_src, rhi_compute_info_t const& info) = nullptr;
rhi_module_t (*rhi_create_module_render)(const char* vert_src, const char* frag_src, rhi_render_info_t const& info) = nullptr;
rhi_module_t (*rhi_create_module_meshlet)(const char* task_src, const char* mesh_src, const char* frag_src, rhi_render_info_t const& info) = nullptr;
void (*rhi_destroy_module)(rhi_module_t& module) = nullptr;

// Uniform 相关
void (*rhi_push_constant)(uint8_t const* buffer, size_t length) = nullptr;
void (*rhi_push_const_int)(const char* name, int32_t value) = nullptr;
void (*rhi_push_const_uint)(const char* name, uint32_t value) = nullptr;
void (*rhi_push_const_float)(const char* name, float value) = nullptr;
void (*rhi_push_const_vec2)(const char* name, const float* value) = nullptr;
void (*rhi_push_const_vec3)(const char* name, const float* value) = nullptr;
void (*rhi_push_const_vec4)(const char* name, const float* value) = nullptr;
void (*rhi_push_const_mat3)(const char* name, const float* value) = nullptr;
void (*rhi_push_const_mat4)(const char* name, const float* value) = nullptr;

// Compute Pass 相关
void (*rhi_begin_compute)(rhi_pass_t& pass) = nullptr;
void (*rhi_end_compute)(rhi_pass_t& pass) = nullptr;
void (*rhi_dispatch_compute)(uint32_t groupX, uint32_t groupY, uint32_t groupZ) = nullptr;

// Render Pass 相关
void (*rhi_begin_render)(rhi_pass_t& pass) = nullptr;
void (*rhi_end_render)(rhi_pass_t& pass) = nullptr;
void (*rhi_set_viewport)(int32_t x, int32_t y, int32_t width, int32_t height) = nullptr;
void (*rhi_set_scissor)(int32_t x, int32_t y, int32_t width, int32_t height) = nullptr;
void (*rhi_draw_mesh_task)(uint32_t groupX, uint32_t groupY, uint32_t groupZ) = nullptr;

// Mesh 相关
rhi_mesh_t (*rhi_create_mesh)(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count) = nullptr;
void (*rhi_destroy_mesh)(rhi_mesh_t& mesh) = nullptr;
void (*rhi_draw_mesh)(rhi_mesh_t const& mesh) = nullptr;

// Meshlet 相关
rhi_meshlet_t (*rhi_create_meshlet)(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count) = nullptr;
void (*rhi_destroy_meshlet)(rhi_meshlet_t& meshlet) = nullptr;
void (*rhi_draw_meshlet)(rhi_meshlet_t const& meshlet) = nullptr;

// Screen 相关
rhi_mesh_t (*rhi_create_mesh_screen)() = nullptr;
void (*rhi_draw_screen)(int width, int height, rhi_texture_t texture, rhi_color_t clear) = nullptr;