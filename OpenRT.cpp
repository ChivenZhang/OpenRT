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
#include <cstdio>
#include <cstdlib>
#ifdef OPENGL_IMPLEMENTATION
#include "OpenGL.h"
#endif
#ifdef VULKAN_IMPLEMENTATION
#include "Vulkan.h"
#endif
#ifdef DIRECTX_IMPLEMENTATION
#include "DirectX.h"
#endif
#ifdef METAL_IMPLEMENTATION
#include "Metal.h"
#endif
#ifdef WEBGPU_IMPLEMENTATION
#include "WebGPU.h"
#endif
#include <string>

void rt_load_library(rt_load_info_t const& info)
{
    switch (info.backend)
    {
    case RT_OPENGL:
#ifdef OPENGL_IMPLEMENTATION
        gl_load_library();
#else
        fprintf(stderr, "OpenRT: OpenGL backend is not built\n");
        abort();
#endif
        return;
    case RT_VULKAN:
#ifdef VULKAN_IMPLEMENTATION
        vk_load_library(
            static_cast<VkInstance>(info.vulkan.instance),
            static_cast<VkPhysicalDevice>(info.vulkan.physical),
            static_cast<VkDevice>(info.vulkan.device),
            static_cast<VkQueue>(info.vulkan.queue),
            static_cast<VkCommandBuffer>(info.vulkan.cmdbuf),
            info.vulkan.family);
#else
        fprintf(stderr, "OpenRT: Vulkan backend is not built\n");
        abort();
#endif
        return;
    case RT_DIRECTX:
#ifdef DIRECTX_IMPLEMENTATION
        dx_load_library(static_cast<ID3D12Device*>(info.directx.device), static_cast<ID3D12CommandQueue*>(info.directx.queue));
#else
        fprintf(stderr, "OpenRT: DirectX backend is not built\n");
        abort();
#endif
        return;
    case RT_METAL:
#ifdef METAL_IMPLEMENTATION
        mt_load_library(static_cast<MTL::Device*>(info.metal.device), static_cast<MTL::CommandQueue*>(info.metal.queue));
#else
        fprintf(stderr, "OpenRT: Metal backend is not built\n");
        abort();
#endif
        return;
    case RT_WEBGPU:
#ifdef WEBGPU_IMPLEMENTATION
        wg_load_library(static_cast<WGPUDevice>(info.webgpu.device), static_cast<WGPUQueue>(info.webgpu.queue));
#else
        fprintf(stderr, "OpenRT: WebGPU backend is not built\n");
        abort();
#endif
        return;
    }
    fprintf(stderr, "OpenRT: unknown backend %u\n", (uint32_t)info.backend);
    abort();
}

void (*rt_unload_library)() = nullptr;

rt_buffer_t (*rt_create_buffer)(rt_buffer_info_t const& info) = nullptr;
void (*rt_destroy_buffer)(rt_buffer_t& buffer) = nullptr;
void (*rt_bind_buffer)(rt_buffer_t& buffer, rt_buffer_bind_t bind) = nullptr;
void* (*rt_map_buffer)(rt_buffer_t& buffer, rt_access_t mode, size_t offset, size_t size) = nullptr; // mode: RT_READ_ONLY / RT_WRITE_ONLY / RT_READ_WRITE
void (*rt_unmap_buffer)(rt_buffer_t& buffer) = nullptr;

rt_texture_t (*rt_create_texture)(rt_texture_info_t const& info) = nullptr;
void (*rt_destroy_texture)(rt_texture_t& texture) = nullptr;
void (*rt_bind_texture)(rt_texture_t& texture, rt_texture_bind_t bind) = nullptr;

rt_texture_view_t (*rt_create_texture_view)(rt_texture_t& texture, rt_texture_view_info_t const& info) = nullptr;
void (*rt_destroy_texture_view)(rt_texture_view_t& view) = nullptr;
void (*rt_bind_texture_view)(rt_texture_view_t& view, rt_texture_view_bind_t bind) = nullptr;
void (*rt_bind_texture_storage)(rt_texture_view_t& view, rt_texture_storage_bind_t bind) = nullptr;

rt_sampler_t (*rt_create_sampler)(rt_sampler_info_t const& info) = nullptr;
void (*rt_destroy_sampler)(rt_sampler_t& sampler) = nullptr;
void (*rt_bind_sampler)(rt_sampler_t& sampler, rt_sampler_bind_t bind) = nullptr;

rt_module_compute_t (*rt_create_module_compute)(rt_module_compute_info_t const& info) = nullptr;
rt_module_render_t (*rt_create_module_render)(rt_module_render_info_t const& info) = nullptr;
void (*rt_destroy_module_render)(rt_module_render_t& module) = nullptr;
void (*rt_destroy_module_compute)(rt_module_compute_t& module) = nullptr;

void (*rt_begin_compute)(rt_pass_compute_t& pass) = nullptr;
void (*rt_end_compute)(rt_pass_compute_t& pass) = nullptr;
void (*rt_bind_module_compute)(rt_module_compute_t& module) = nullptr;
void (*rt_dispatch_compute)(uint32_t groupX, uint32_t groupY, uint32_t groupZ) = nullptr;
void (*rt_dispatch_compute_indirect)(rt_buffer_t& indirect, size_t offset) = nullptr;

void (*rt_begin_render)(rt_pass_render_t& pass) = nullptr;
void (*rt_end_render)(rt_pass_render_t& pass) = nullptr;
void (*rt_bind_module_render)(rt_module_render_t& module) = nullptr;
void (*rt_set_viewport)(float x, float y, float width, float height, float minDepth, float maxDepth) = nullptr;
void (*rt_set_scissor)(int32_t x, int32_t y, int32_t width, int32_t height) = nullptr;
void (*rt_set_blend_constant)(float r, float g, float b, float a) = nullptr;
void (*rt_set_stencil_reference)(int32_t refer) = nullptr;
void (*rt_draw_array)(rt_buffer_t vbo[], uint32_t vbo_num, uint32_t vertex_num, uint32_t instance_num, uint32_t vertex_start, uint32_t instance_start) = nullptr;
void (*rt_draw_index)(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& ebo, uint32_t vertex_num, uint32_t instance_num, uint32_t vertex_start, uint32_t instance_start) = nullptr;
void (*rt_draw_array_indirect)(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& indirect, size_t offset) = nullptr;
void (*rt_draw_index_indirect)(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& ebo, rt_buffer_t& indirect, size_t offset) = nullptr;
void (*rt_draw_mesh_task)(uint32_t groupX, uint32_t groupY, uint32_t groupZ) = nullptr;
void (*rt_draw_mesh_task_indirect)(rt_buffer_t& indirect, size_t offset, uint32_t draw_count, uint32_t draw_stride) = nullptr;

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
void (*rt_draw_mesh)(rt_mesh_t& mesh) = nullptr;
void (*rt_draw_mesh_multi)(rt_mesh_t& mesh, uint32_t count) = nullptr;

rt_meshlet_t (*rt_create_meshlet)(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count) = nullptr;
void (*rt_destroy_meshlet)(rt_meshlet_t& meshlet) = nullptr;
void (*rt_draw_meshlet)(rt_meshlet_t& meshlet) = nullptr;

void (*rt_submit)() = nullptr;