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
#include "Vulkan.h"

void vk_load_library()
{
}

vk_buffer_t vk_create_buffer(vk_buffer_desc_t const& info)
{
    return {};
}

void vk_destroy_buffer(vk_buffer_t& buffer)
{
}

void vk_bind_buffer(vk_buffer_t buffer, vk_buffer_bind_t bind)
{
}

void vk_read_buffer(vk_buffer_t buffer, size_t offset, size_t size, void* data)
{
}

void vk_write_buffer(vk_buffer_t buffer, size_t offset, size_t size, const void* data)
{
}

vk_texture_t vk_create_texture(vk_texture_desc_t const& info)
{
    return {};
}

vk_texture_t vk_create_texture_color(uint32_t width, uint32_t height, const void* data)
{
    return {};
}

vk_texture_t vk_create_texture_depth(uint32_t width, uint32_t height, const void* data)
{
    return {};
}

vk_texture_t vk_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data)
{
    return {};
}

void vk_destroy_texture(vk_texture_t& texture)
{
}

void vk_bind_texture(vk_texture_t texture, vk_texture_bind_t bind)
{
}

void vk_bind_texture_storage(vk_texture_t texture, vk_texture_storage_bind_t bind)
{
}

vk_texture_t vk_load_texture(vk_image_t const& image)
{
    return {};
}

vk_image_t vk_load_image(vk_texture_t const& texture, void* buffer, size_t length)
{
    return {};
}

vk_sampler_t vk_create_sampler(vk_sampler_desc_t const& info)
{
    return {};
}

void vk_destroy_sampler(vk_sampler_t& sampler)
{
}

void vk_bind_sampler(vk_sampler_t sampler, vk_sampler_bind_t bind)
{
}

vk_module_t vk_create_module_compute(const char* comp_src)
{
    return {};
}

vk_module_t vk_create_module_render(const char* vert_src, const char* frag_src)
{
    return {};
}

vk_module_t vk_create_module_meshlet(const char* task_src, const char* mesh_src, const char* frag_src)
{
    return {};
}

void vk_destroy_module(vk_module_t& module)
{
}

void vk_push_const_int(const char* name, int32_t value)
{
}

void vk_push_const_uint(const char* name, uint32_t value)
{
}

void vk_push_const_float(const char* name, float value)
{
}

void vk_push_const_vec2(const char* name, const float* value)
{
}

void vk_push_const_vec3(const char* name, const float* value)
{
}

void vk_push_const_vec4(const char* name, const float* value)
{
}

void vk_push_const_mat3(const char* name, const float* value)
{
}

void vk_push_const_mat4(const char* name, const float* value)
{
}

void vk_begin_compute(vk_pass_t& pass)
{
}

void vk_end_compute(vk_pass_t& pass)
{
}

void vk_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
}

void vk_begin_render(vk_pass_t& pass)
{
}

void vk_end_render(vk_pass_t& pass)
{
}

void vk_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
}

void vk_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
}

void vk_draw_mesh_task(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
}

vk_mesh_t vk_create_mesh(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    return {};
}

void vk_destroy_mesh(vk_mesh_t& mesh)
{
}

void vk_draw_mesh(vk_mesh_t const& mesh)
{
}

vk_meshlet_t vk_create_meshlet(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    return {};
}

void vk_destroy_meshlet(vk_meshlet_t& meshlet)
{
}

void vk_draw_meshlet(vk_meshlet_t const& meshlet)
{
}

vk_mesh_t vk_create_mesh_screen()
{
    return {};
}

void vk_draw_screen(int width, int height, vk_texture_t texture, vk_color_t clear)
{
}

#endif