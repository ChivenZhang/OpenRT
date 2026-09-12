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
#include <map>

struct vk_buffer_native_t
{
    VkBuffer handle = nullptr;
};
struct vk_texture_native_t
{
    VkImage handle = nullptr;
};
struct vk_sampler_native_t
{
    VkSampler handle = nullptr;
};
struct vk_module_native_t
{
    VkShaderModule vshader = nullptr;   // Vertex Shader
    VkShaderModule tshader = nullptr;   // Task Shader
    VkShaderModule mshader = nullptr;   // Mesh Shader
    VkShaderModule fshader = nullptr;   // Fragment Shader
    VkShaderModule cshader = nullptr;   // Compute Shader
};
struct vk_mesh_native_t
{

};
struct vk_meshlet_native_t
{

};

struct vk_native_t
{
    uint32_t bufferID = 0;
    uint32_t textureID = 0;
    uint32_t samplerID = 0;
    uint32_t moduleID = 0;
    uint32_t meshID = 0;
    uint32_t meshletID = 0;

    std::map<uint32_t, vk_buffer_native_t> buffers;
    std::map<uint32_t, vk_texture_native_t> textures;
    std::map<uint32_t, vk_sampler_native_t> samplers;
    std::map<uint32_t, vk_module_native_t> modules;
    std::map<uint32_t, vk_mesh_native_t> meshes;
    std::map<uint32_t, vk_meshlet_native_t> meshlets;

    VkDevice device = nullptr;
    VkCommandBuffer cmdBuf = nullptr;
    // Other Resource here...

} static vulkan;

void vk_load_library(VkDevice device)
{
    vulkan.device = device;

    // ====================================================================

    // Buffer 相关
    rhi_create_buffer = vk_create_buffer;
    rhi_destroy_buffer = vk_destroy_buffer;
    rhi_bind_buffer = vk_bind_buffer;
    rhi_read_buffer = vk_read_buffer;
    rhi_write_buffer = vk_write_buffer;

    // Texture 相关
    rhi_create_texture = vk_create_texture;
    rhi_create_texture_color = vk_create_texture_color;
    rhi_create_texture_depth = vk_create_texture_depth;
    rhi_create_texture_depth_stencil = vk_create_texture_depth_stencil;
    rhi_destroy_texture = vk_destroy_texture;
    rhi_bind_texture = vk_bind_texture;
    rhi_bind_texture_storage = vk_bind_texture_storage;
    rhi_load_texture = vk_load_texture;
    rhi_load_image = vk_load_image;

    // Sampler 相关
    rhi_create_sampler = vk_create_sampler;
    rhi_destroy_sampler = vk_destroy_sampler;
    rhi_bind_sampler = vk_bind_sampler;

    // Module 相关
    rhi_create_module_compute = vk_create_module_compute;
    rhi_create_module_render = vk_create_module_render;
    rhi_create_module_meshlet = vk_create_module_meshlet;
    rhi_destroy_module = vk_destroy_module;

    // Uniform 相关
    rhi_push_const_int = vk_push_const_int;
    rhi_push_const_uint = vk_push_const_uint;
    rhi_push_const_float = vk_push_const_float;
    rhi_push_const_vec2 = vk_push_const_vec2;
    rhi_push_const_vec3 = vk_push_const_vec3;
    rhi_push_const_vec4 = vk_push_const_vec4;
    rhi_push_const_mat3 = vk_push_const_mat3;
    rhi_push_const_mat4 = vk_push_const_mat4;

    // Compute Pass 相关
    rhi_begin_compute = vk_begin_compute;
    rhi_end_compute = vk_end_compute;
    rhi_dispatch_compute = vk_dispatch_compute;

    // Render Pass 相关
    rhi_begin_render = vk_begin_render;
    rhi_end_render = vk_end_render;
    rhi_set_viewport = vk_set_viewport;
    rhi_set_scissor = vk_set_scissor;
    rhi_draw_mesh_task = vk_draw_mesh_task;

    // Mesh 相关
    rhi_create_mesh = vk_create_mesh;
    rhi_destroy_mesh = vk_destroy_mesh;
    rhi_draw_mesh = vk_draw_mesh;

    // Meshlet 相关
    rhi_create_meshlet = vk_create_meshlet;
    rhi_destroy_meshlet = vk_destroy_meshlet;
    rhi_draw_meshlet = vk_draw_meshlet;

    // Screen 相关
    rhi_create_mesh_screen = vk_create_mesh_screen;
    rhi_draw_screen = vk_draw_screen;
}

vk_buffer_t vk_create_buffer(vk_buffer_desc_t const& info)
{
    vk_buffer_t result = {};
    result.handle = vulkan.bufferID + 1;
    auto& native = vulkan.buffers[result.handle];
    result.native = &native;
    VkBufferCreateInfo vkInfo = {};
    vkInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkInfo.size = info.size;
    vkInfo.usage =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    vkInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(vulkan.device, &vkInfo, nullptr, &native.handle) != VK_SUCCESS) return {};

    vulkan.bufferID += 1;
    result.size = info.size;
    result.usage = info.usage;
    return result;
}

void vk_destroy_buffer(vk_buffer_t& buffer)
{
    if (vulkan.device && buffer.native)
    {
        auto& native = vulkan.buffers[buffer.handle];
        vkDestroyBuffer(vulkan.device, native.handle, nullptr);

        vulkan.buffers.erase(buffer.handle);
    }
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