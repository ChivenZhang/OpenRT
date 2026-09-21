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
#include "OpenRT.h"
#include "OpenGL.h"

void rt_load_library(const char* backend)
{
    gl_load_library();
}

void rt_unload_library()
{
    gl_unload_library();
}

rt_buffer_t (*rt_create_buffer)(rt_buffer_info_t const& info) = nullptr;
void (*rt_destroy_buffer)(rt_buffer_t& buffer) = nullptr;
void (*rt_bind_buffer)(rt_buffer_t buffer, rt_buffer_bind_t bind) = nullptr;
void (*rt_read_buffer)(rt_buffer_t buffer, size_t offset, size_t size, void* data) = nullptr;
void (*rt_write_buffer)(rt_buffer_t buffer, size_t offset, size_t size, const void* data) = nullptr;
void* (*rt_map_buffer)(rt_buffer_t& buffer, GLenum mode, size_t offset, size_t size) = nullptr; // mode: GL_READ_ONLY / GL_WRITE_ONLY / GL_READ_WRITE
void (*rt_unmap_buffer)(rt_buffer_t& buffer) = nullptr;

rt_texture_t (*rt_create_texture)(rt_texture_info_t const& info) = nullptr;
rt_texture_t (*rt_create_texture_color)(uint32_t width, uint32_t height, const void* data) = nullptr;
rt_texture_t (*rt_create_texture_color_float)(uint32_t width, uint32_t height, const void* data) = nullptr;
rt_texture_t (*rt_create_texture_depth)(uint32_t width, uint32_t height, const void* data) = nullptr;
rt_texture_t (*rt_create_texture_depth_stencil)(uint32_t width, uint32_t height, const void* data) = nullptr;
void (*rt_destroy_texture)(rt_texture_t& texture) = nullptr;
void (*rt_bind_texture)(rt_texture_t texture, rt_texture_bind_t bind) = nullptr;
void (*rt_bind_texture_storage)(rt_texture_t texture, rt_texture_storage_bind_t bind) = nullptr;

rt_sampler_t (*rt_create_sampler)(rt_sampler_info_t const& info) = nullptr;
void (*rt_destroy_sampler)(rt_sampler_t& sampler) = nullptr;
void (*rt_bind_sampler)(rt_sampler_t sampler, rt_sampler_bind_t bind) = nullptr;

rt_module_compute_t (*rt_create_module_compute)(rt_module_compute_info_t const& info) = nullptr;
rt_module_render_t (*rt_create_module_render)(rt_module_render_info_t const& info) = nullptr;
rt_module_render_t (*rt_create_module_meshlet)(rt_module_render_info_t const& info) = nullptr;
void (*rt_destroy_module_render)(rt_module_render_t& module) = nullptr;
void (*rt_destroy_module_compute)(rt_module_compute_t& module) = nullptr;

void (*rt_begin_compute)(rt_pass_compute_t& pass) = nullptr;
void (*rt_end_compute)(rt_pass_compute_t& pass) = nullptr;
void (*rt_dispatch_compute)(uint32_t groupX, uint32_t groupY, uint32_t groupZ) = nullptr;

void (*rt_begin_render)(rt_pass_render_t& pass) = nullptr;
void (*rt_end_render)(rt_pass_render_t& pass) = nullptr;
void (*rt_set_viewport)(int32_t x, int32_t y, int32_t width, int32_t height) = nullptr;
void (*rt_set_scissor)(int32_t x, int32_t y, int32_t width, int32_t height) = nullptr;
void (*rt_draw_mesh_task)(uint32_t groupX, uint32_t groupY, uint32_t groupZ) = nullptr;

void (*rt_push_constant)(uint8_t const* buffer, size_t length) = nullptr;
void (*rt_push_const_int)(const char* name, int32_t value) = nullptr;
void (*rt_push_const_uint)(const char* name, uint32_t value) = nullptr;
void (*rt_push_const_float)(const char* name, float value) = nullptr;
void (*rt_push_const_vec2)(const char* name, const float* value) = nullptr;
void (*rt_push_const_vec3)(const char* name, const float* value) = nullptr;
void (*rt_push_const_vec4)(const char* name, const float* value) = nullptr;
void (*rt_push_const_mat3)(const char* name, const float* value) = nullptr;
void (*rt_push_const_mat4)(const char* name, const float* value) = nullptr;

void (*rt_begin_transfer)(rt_pass_transfer_t& pass) = nullptr;
void (*rt_end_transfer)(rt_pass_transfer_t& pass) = nullptr;
void (*rt_copy_buffer)(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize) = nullptr;
void (*rt_copy_buffer_data)(rt_buffer_data_t source, rt_buffer_copy_t destination, size_t copySize) = nullptr;
void (*rt_copy_buffer_texture)(rt_texture_copy_t source, rt_buffer_texel_t destination, rt_size_t copySize) = nullptr;
void (*rt_copy_texture)(rt_texture_copy_t source, rt_texture_copy_t destination, rt_size_t copySize) = nullptr;
void (*rt_copy_texture_data)(rt_texture_data_t source, rt_texture_copy_t destination, rt_size_t copySize) = nullptr;
void (*rt_copy_texture_buffer)(rt_buffer_texel_t source, rt_texture_copy_t destination, rt_size_t copySize) = nullptr;

rt_mesh_t (*rt_create_mesh)(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count) = nullptr;
void (*rt_destroy_mesh)(rt_mesh_t& mesh) = nullptr;
void (*rt_draw_mesh)(rt_mesh_t const& mesh) = nullptr;
void (*rt_draw_mesh_multi)(rt_mesh_t const& mesh, uint32_t count) = nullptr;

rt_meshlet_t (*rt_create_meshlet)(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count) = nullptr;
void (*rt_destroy_meshlet)(rt_meshlet_t& meshlet) = nullptr;
void (*rt_draw_meshlet)(rt_meshlet_t const& meshlet) = nullptr;

rt_mesh_t (*rt_create_mesh_screen)() = nullptr;
void (*rt_draw_screen)(int width, int height, rt_texture_t texture, rt_color_t clear) = nullptr;

void (*rt_submit)() = nullptr;