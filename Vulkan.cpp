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
#include "Vulkan.h"
#ifdef VULKAN_IMPLEMENTATION
#include <map>

static VkFilter gl_to_vk_filter(GLenum filter)
{
    return (filter == GL_NEAREST) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
}

static VkFilter gl_to_vk_min_filter(GLenum minFilter)
{
    return (minFilter == GL_NEAREST || minFilter == GL_NEAREST_MIPMAP_NEAREST ||
            minFilter == GL_NEAREST_MIPMAP_LINEAR) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
}

static VkSamplerAddressMode gl_to_vk_address_mode(GLenum wrap)
{
    switch (wrap)
    {
        case GL_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case GL_MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case GL_CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case GL_CLAMP_TO_EDGE:
        default: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }
}

static VkSamplerMipmapMode gl_to_vk_mipmap_mode(GLenum minFilter)
{
    return (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST) ?
           VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
}

static VkFormat gl_to_vk_format(GLenum glFormat, GLenum glType)
{
    switch (glFormat)
    {
        case GL_RGBA:
            return (glType == GL_UNSIGNED_BYTE) ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
        case GL_RGB:
            return (glType == GL_UNSIGNED_BYTE) ? VK_FORMAT_R8G8B8_UNORM : VK_FORMAT_R8G8B8_UNORM;
        case GL_DEPTH_COMPONENT:
            return (glType == GL_FLOAT) ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_D32_SFLOAT;
        case GL_DEPTH_STENCIL:
            return (glType == GL_UNSIGNED_INT_24_8) ? VK_FORMAT_D24_UNORM_S8_UINT : VK_FORMAT_D24_UNORM_S8_UINT;
        default:
            return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

static VkImageUsageFlags gl_to_vk_image_usage(GLenum format)
{
    switch (format)
    {
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_STENCIL:
            return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        default:
            return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }
}

static VkBufferUsageFlags rt_to_vk_buffer_usage(uint32_t usage)
{
    VkBufferUsageFlags flags = 0;
    if (usage & GL_BUFFER_USAGE_COPY_SRC) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (usage & GL_BUFFER_USAGE_COPY_DST) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (usage & GL_BUFFER_USAGE_INDEX) flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (usage & GL_BUFFER_USAGE_VERTEX) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (usage & GL_BUFFER_USAGE_UNIFORM) flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (usage & GL_BUFFER_USAGE_STORAGE) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (usage & GL_BUFFER_USAGE_INDIRECT) flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    return flags;
}

static VkFormat gl_to_vk_vertex_format(GLenum type, GLenum count)
{
    if (type == GL_FLOAT)
    {
        switch (count)
        {
            case 1: return VK_FORMAT_R32_SFLOAT;
            case 2: return VK_FORMAT_R32G32_SFLOAT;
            case 3: return VK_FORMAT_R32G32B32_SFLOAT;
            case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            default: return VK_FORMAT_R32_SFLOAT;
        }
    }
    return VK_FORMAT_R32_SFLOAT;
}

static VkDescriptorType gl_to_vk_descriptor_type(GLenum bindingType)
{
    switch (bindingType)
    {
        case GL_BINDING_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case GL_BINDING_TEXTURE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case GL_BINDING_STORAGE_TEXTURE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case GL_BINDING_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
        default: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
}

static VkShaderStageFlagBits gl_to_vk_shader_stage(const char* source)
{
    return VK_SHADER_STAGE_FRAGMENT_BIT;
}

static VkCompareOp gl_to_vk_compare_op(GLenum func)
{
    switch (func)
    {
        case GL_NEVER: return VK_COMPARE_OP_NEVER;
        case GL_LESS: return VK_COMPARE_OP_LESS;
        case GL_EQUAL: return VK_COMPARE_OP_EQUAL;
        case GL_LEQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
        case GL_GREATER: return VK_COMPARE_OP_GREATER;
        case GL_NOTEQUAL: return VK_COMPARE_OP_NOT_EQUAL;
        case GL_GEQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case GL_ALWAYS:
        default: return VK_COMPARE_OP_ALWAYS;
    }
}

static VkBlendFactor gl_to_vk_blend_factor(GLenum factor)
{
    switch (factor)
    {
        case GL_ZERO: return VK_BLEND_FACTOR_ZERO;
        case GL_ONE: return VK_BLEND_FACTOR_ONE;
        case GL_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
        case GL_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case GL_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
        case GL_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case GL_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case GL_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
        case GL_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case GL_CONSTANT_COLOR: return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case GL_ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case GL_CONSTANT_ALPHA: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case GL_ONE_MINUS_CONSTANT_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case GL_SRC_ALPHA_SATURATE: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        default: return VK_BLEND_FACTOR_ONE;
    }
}

static VkBlendOp gl_to_vk_blend_op(GLenum func)
{
    switch (func)
    {
        case GL_FUNC_SUBTRACT: return VK_BLEND_OP_SUBTRACT;
        case GL_FUNC_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case GL_MIN: return VK_BLEND_OP_MIN;
        case GL_MAX: return VK_BLEND_OP_MAX;
        case GL_FUNC_ADD:
        default: return VK_BLEND_OP_ADD;
    }
}

static VkCullModeFlags gl_to_vk_cull_mode(GLenum mode)
{
    switch (mode)
    {
        case GL_FRONT: return VK_CULL_MODE_FRONT_BIT;
        case GL_BACK: return VK_CULL_MODE_BACK_BIT;
        case GL_FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
        case GL_NONE:
        default: return VK_CULL_MODE_NONE;
    }
}

static VkFrontFace gl_to_vk_front_face(GLenum face)
{
    return (face == GL_CW) ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

static VkPolygonMode gl_to_vk_polygon_mode(GLenum fill)
{
    switch (fill)
    {
        case GL_POINT: return VK_POLYGON_MODE_POINT;
        case GL_LINE: return VK_POLYGON_MODE_LINE;
        case GL_FILL:
        default: return VK_POLYGON_MODE_FILL;
    }
}

static VkPrimitiveTopology gl_to_vk_primitive_topology(GLenum primitive)
{
    switch (primitive)
    {
        case GL_POINTS: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case GL_LINES: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case GL_LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case GL_LINE_LOOP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; // Vulkan has no line loop
        case GL_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case GL_TRIANGLE_FAN: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        case GL_TRIANGLES:
        default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

static VkStencilOp gl_to_vk_stencil_op(GLenum op)
{
    switch (op)
    {
        case GL_ZERO: return VK_STENCIL_OP_ZERO;
        case GL_REPLACE: return VK_STENCIL_OP_REPLACE;
        case GL_INCR: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case GL_INCR_WRAP: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case GL_DECR: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case GL_DECR_WRAP: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        case GL_INVERT: return VK_STENCIL_OP_INVERT;
        case GL_KEEP:
        default: return VK_STENCIL_OP_KEEP;
    }
}

static uint32_t gl_to_vk_vertex_size(GLenum type, GLenum count)
{
    if (type == GL_FLOAT)
        return count * sizeof(float);
    if (type == GL_INT || type == GL_UNSIGNED_INT)
        return count * sizeof(int32_t);
    if (type == GL_SHORT || type == GL_UNSIGNED_SHORT)
        return count * sizeof(int16_t);
    if (type == GL_BYTE || type == GL_UNSIGNED_BYTE)
        return count * sizeof(int8_t);
    return count * sizeof(float);
}

// ====================================================================

struct rt_buffer_native_t
{
    VkBuffer handle = nullptr;
    VkDeviceMemory memory = nullptr;
};
struct rt_texture_native_t
{
    VkImage handle = nullptr;
    VkDeviceMemory memory = nullptr;
    VkImageView imageView = nullptr;
};
struct rt_sampler_native_t
{
    VkSampler handle = nullptr;
};
struct rt_module_native_t
{
    VkShaderModule vshader = nullptr;   // Vertex Shader
    VkShaderModule tshader = nullptr;   // Task Shader
    VkShaderModule mshader = nullptr;   // Mesh Shader
    VkShaderModule fshader = nullptr;   // Fragment Shader
    VkShaderModule cshader = nullptr;   // Compute Shader
    VkPipeline pipeline = nullptr;
    VkPipelineLayout pipelineLayout = nullptr;
    VkDescriptorSetLayout descriptorSetLayout = nullptr;
};
struct rt_mesh_native_t
{
    VkVertexInputBindingDescription bindingDescription = {};
    VkVertexInputAttributeDescription attributeDescriptions[3] = {};
};
struct rt_meshlet_native_t
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

    std::map<uint32_t, rt_buffer_native_t> buffers;
    std::map<uint32_t, rt_texture_native_t> textures;
    std::map<uint32_t, rt_sampler_native_t> samplers;
    std::map<uint32_t, rt_module_native_t> modules;
    std::map<uint32_t, rt_mesh_native_t> meshes;
    std::map<uint32_t, rt_meshlet_native_t> meshlets;

    VkInstance instance = nullptr;
    VkDevice device = nullptr;
    VkQueue queue = nullptr;
    VkAllocationCallbacks* allocator = nullptr;
    VkCommandPool cmdPool = nullptr;
    VkCommandBuffer cmdBuffer = nullptr;
    VkDescriptorPool descriptorPool = nullptr;
	VkPipelineCache pipelineCache = nullptr;

    struct
    {
        GLenum type = GL_NONE;
        union
        {
            struct
            {
                rt_buffer_t* buffer;
                rt_buffer_bind_t buffer_bind;
            };
            struct
            {
                rt_texture_t* texture;
                rt_texture_bind_t texture_bind;
            };
            struct
            {
                rt_texture_t* storage_texture;
                rt_texture_storage_bind_t storage_texture_bind;
            };
            struct
            {
                rt_sampler_t* sampler;
                rt_sampler_bind_t sampler_bind;
            };
        };
    } currentBinding[GL_MAX_BINDING_HANDLE_NUM] = {};

    rt_pass_t* currentPipeline = nullptr;

} static vulkan;

void vk_load_library(VkInstance instance, VkDevice device, uint32_t family)
{
    vulkan.instance = instance;
    vulkan.device = device;

    // Get Device Queue
    {
        vkGetDeviceQueue(vulkan.device, family, 0, &vulkan.queue);
    }

    // Create Command Pool
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = family;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        
        if (vkCreateCommandPool(vulkan.device, &poolInfo, vulkan.allocator, &vulkan.cmdPool) == VK_SUCCESS)
        {
            // Allocate Command Buffer
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = vulkan.cmdPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;
            
            vkAllocateCommandBuffers(vulkan.device, &allocInfo, &vulkan.cmdBuffer);
        }
    }

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize poolSizes[] = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
            {VK_DESCRIPTOR_TYPE_SAMPLER, 100},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100}
        };
        
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 5;
        poolInfo.pPoolSizes = poolSizes;
        poolInfo.maxSets = 100;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        
        vkCreateDescriptorPool(vulkan.device, &poolInfo, vulkan.allocator, &vulkan.descriptorPool);
    }

    // Create Pipeline Cache
    {
        VkPipelineCacheCreateInfo cacheInfo = {};
        cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        vkCreatePipelineCache(vulkan.device, &cacheInfo, vulkan.allocator, &vulkan.pipelineCache);
    }

    // ====================================================================

    // Buffer 相关
    rt_create_buffer = vk_create_buffer;
    rt_destroy_buffer = vk_destroy_buffer;
    rt_bind_buffer = vk_bind_buffer;
    rt_map_buffer = vk_map_buffer;
    rt_unmap_buffer = vk_unmap_buffer;

    // Texture 相关
    rt_create_texture = vk_create_texture;
    rt_create_texture_color = vk_create_texture_color;
    rt_create_texture_depth = vk_create_texture_depth;
    rt_create_texture_depth_stencil = vk_create_texture_depth_stencil;
    rt_destroy_texture = vk_destroy_texture;
    rt_bind_texture = vk_bind_texture;
    rt_bind_texture_storage = vk_bind_texture_storage;

    // Sampler 相关
    rt_create_sampler = vk_create_sampler;
    rt_destroy_sampler = vk_destroy_sampler;
    rt_bind_sampler = vk_bind_sampler;

    // Module 相关
    rt_create_module_compute = vk_create_module_compute;
    rt_create_module_render = vk_create_module_render;
    rt_create_module_meshlet = vk_create_module_meshlet;
    rt_destroy_module = vk_destroy_module;

    // Compute Pass 相关
    rt_begin_compute = vk_begin_compute;
    rt_end_compute = vk_end_compute;
    rt_dispatch_compute = vk_dispatch_compute;

    // Render Pass 相关
    rt_begin_render = vk_begin_render;
    rt_end_render = vk_end_render;
    rt_set_viewport = vk_set_viewport;
    rt_set_scissor = vk_set_scissor;
    rt_draw_mesh_task = vk_draw_mesh_task;

    // Uniform 相关
    rt_push_constant = vk_push_constant;
    rt_push_const_int = vk_push_const_int;
    rt_push_const_uint = vk_push_const_uint;
    rt_push_const_float = vk_push_const_float;
    rt_push_const_vec2 = vk_push_const_vec2;
    rt_push_const_vec3 = vk_push_const_vec3;
    rt_push_const_vec4 = vk_push_const_vec4;
    rt_push_const_mat3 = vk_push_const_mat3;
    rt_push_const_mat4 = vk_push_const_mat4;

    // Mesh 相关
    rt_create_mesh = vk_create_mesh;
    rt_destroy_mesh = vk_destroy_mesh;
    rt_draw_mesh = vk_draw_mesh;

    // Meshlet 相关
    rt_create_meshlet = vk_create_meshlet;
    rt_destroy_meshlet = vk_destroy_meshlet;
    rt_draw_meshlet = vk_draw_meshlet;

    // Screen 相关
    rt_create_mesh_screen = vk_create_mesh_screen;
    rt_draw_screen = vk_draw_screen;
}

void vk_unload_library()
{
    if (!vulkan.device) return;

    // Wait for device to finish all operations
    vkDeviceWaitIdle(vulkan.device);

    // Destroy Descriptor Pool
    if (vulkan.descriptorPool)
    {
        vkDestroyDescriptorPool(vulkan.device, vulkan.descriptorPool, vulkan.allocator);
        vulkan.descriptorPool = nullptr;
    }

    // Destroy Pipeline Cache
    if (vulkan.pipelineCache)
    {
        vkDestroyPipelineCache(vulkan.device, vulkan.pipelineCache, vulkan.allocator);
        vulkan.pipelineCache = nullptr;
    }

    // Free Command Buffer
    if (vulkan.cmdPool && vulkan.cmdBuffer)
    {
        vkFreeCommandBuffers(vulkan.device, vulkan.cmdPool, 1, &vulkan.cmdBuffer);
        vulkan.cmdBuffer = nullptr;
    }

    // Destroy Command Pool
    if (vulkan.cmdPool)
    {
        vkDestroyCommandPool(vulkan.device, vulkan.cmdPool, vulkan.allocator);
        vulkan.cmdPool = nullptr;
    }

    // Reset Vulkan state
    vulkan.queue = nullptr;
    vulkan.device = nullptr;
    vulkan.instance = nullptr;
    vulkan.bufferID = 0;
    vulkan.textureID = 0;
    vulkan.samplerID = 0;
    vulkan.moduleID = 0;
    vulkan.meshID = 0;
    vulkan.meshletID = 0;
}

rt_buffer_t vk_create_buffer(rt_buffer_info_t const& info)
{
    if (info.usage == 0)
    {
        fprintf(stderr, "Buffer usage must not be 0");
        abort();
    }

    rt_buffer_t result = {};
    result.handle = vulkan.bufferID + 1;
    auto& native = vulkan.buffers[result.handle];
    result.native = &native;
    VkBufferCreateInfo vkInfo = {};
    vkInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkInfo.size = info.size;
    vkInfo.usage = rt_to_vk_buffer_usage(info.usage);
    if (info.data)
        vkInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vulkan.device, &vkInfo, vulkan.allocator, &native.handle) != VK_SUCCESS) return {};

    vulkan.bufferID += 1;
    result.size = info.size;
    result.usage = info.usage;
    return result;
}

void vk_destroy_buffer(rt_buffer_t& buffer)
{
    if (vulkan.device && buffer.native)
    {
        auto& native = vulkan.buffers[buffer.handle];
        vkDestroyBuffer(vulkan.device, native.handle, vulkan.allocator);

        vulkan.buffers.erase(buffer.handle);
    }
}

void vk_bind_buffer(rt_buffer_t buffer, rt_buffer_bind_t bind)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    vulkan.currentBinding[bind.binding].buffer = &buffer;
    vulkan.currentBinding[bind.binding].buffer_bind = bind;
    vulkan.currentBinding[bind.binding].type = GL_BINDING_BUFFER;
}

void* vk_map_buffer(rt_buffer_t& buffer, GLenum mode, size_t offset, size_t size)
{
    return nullptr;
}

void vk_unmap_buffer(rt_buffer_t& buffer)
{

}

rt_texture_t vk_create_texture(rt_texture_info_t const& info)
{
    rt_texture_t result = {};
    result.handle = vulkan.textureID + 1;
    result.width = info.width;
    result.height = info.height;
    result.format = info.format;
    result.internal_format = info.internal_format;
    result.type = info.type;
    result.target = info.target;
    
    auto& native = vulkan.textures[result.handle];
    result.native = &native;
    
    VkImageCreateInfo vkInfo = {};
    vkInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vkInfo.imageType = VK_IMAGE_TYPE_2D;
    vkInfo.extent.width = info.width;
    vkInfo.extent.height = info.height;
    vkInfo.extent.depth = 1;
    vkInfo.mipLevels = 1;
    vkInfo.arrayLayers = 1;
    vkInfo.format = gl_to_vk_format(info.format, info.type);
    vkInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    vkInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkInfo.usage = gl_to_vk_image_usage(info.format);
    vkInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    vkInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(vulkan.device, &vkInfo, nullptr, &native.handle) != VK_SUCCESS) return {};
    
    vulkan.textureID += 1;
    result.mipmaps = false;
    return result;
}

rt_texture_t vk_create_texture_color(uint32_t width, uint32_t height, const void* data)
{
    rt_texture_info_t desc = {};
    desc.width = width;
    desc.height = height;
    desc.target = GL_TEXTURE_2D;
    desc.format = GL_RGBA;
    desc.internal_format = GL_RGBA8;
    desc.type = GL_UNSIGNED_BYTE;
    desc.min_filter = GL_LINEAR;
    desc.mag_filter = GL_LINEAR;
    desc.wrap_s = GL_CLAMP_TO_EDGE;
    desc.wrap_t = GL_CLAMP_TO_EDGE;
    desc.wrap_r = GL_CLAMP_TO_EDGE;
    desc.data = data;
    
    return vk_create_texture(desc);
}

rt_texture_t vk_create_texture_depth(uint32_t width, uint32_t height, const void* data)
{
    rt_texture_info_t desc = {};
    desc.width = width;
    desc.height = height;
    desc.target = GL_TEXTURE_2D;
    desc.format = GL_DEPTH_COMPONENT;
    desc.internal_format = GL_DEPTH_COMPONENT32F;
    desc.type = GL_FLOAT;
    desc.min_filter = GL_LINEAR;
    desc.mag_filter = GL_LINEAR;
    desc.wrap_s = GL_CLAMP_TO_EDGE;
    desc.wrap_t = GL_CLAMP_TO_EDGE;
    desc.wrap_r = GL_CLAMP_TO_EDGE;
    desc.data = data;
    
    return vk_create_texture(desc);
}

rt_texture_t vk_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data)
{
    rt_texture_info_t desc = {};
    desc.width = width;
    desc.height = height;
    desc.target = GL_TEXTURE_2D;
    desc.format = GL_DEPTH_STENCIL;
    desc.internal_format = GL_DEPTH24_STENCIL8;
    desc.type = GL_UNSIGNED_INT_24_8;
    desc.min_filter = GL_LINEAR;
    desc.mag_filter = GL_LINEAR;
    desc.wrap_s = GL_CLAMP_TO_EDGE;
    desc.wrap_t = GL_CLAMP_TO_EDGE;
    desc.wrap_r = GL_CLAMP_TO_EDGE;
    desc.data = data;
    
    return vk_create_texture(desc);
}

void vk_destroy_texture(rt_texture_t& texture)
{
    if (vulkan.device && texture.native)
    {
        auto& native = vulkan.textures[texture.handle];
        if (native.handle) vkDestroyImage(vulkan.device, native.handle, nullptr);
        
        vulkan.textures.erase(texture.handle);
        texture.handle = 0;
    }
}

void vk_bind_texture(rt_texture_t texture, rt_texture_bind_t bind)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    vulkan.currentBinding[bind.binding].texture = &texture;
    vulkan.currentBinding[bind.binding].texture_bind = bind;
    vulkan.currentBinding[bind.binding].type = GL_BINDING_TEXTURE;
}

void vk_bind_texture_storage(rt_texture_t texture, rt_texture_storage_bind_t bind)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    vulkan.currentBinding[bind.binding].storage_texture = &texture;
    vulkan.currentBinding[bind.binding].storage_texture_bind = bind;
    vulkan.currentBinding[bind.binding].type = GL_BINDING_STORAGE_TEXTURE;
}

rt_sampler_t vk_create_sampler(rt_sampler_info_t const& info)
{
    rt_sampler_t result = {};
    result.handle = vulkan.samplerID + 1;
    auto& native = vulkan.samplers[result.handle];
    result.native = &native;
    
    VkSamplerCreateInfo vkInfo = {};
    vkInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    vkInfo.magFilter = gl_to_vk_filter(info.mag_filter);
    vkInfo.minFilter = gl_to_vk_min_filter(info.min_filter);
    vkInfo.addressModeU = gl_to_vk_address_mode(info.wrap_s);
    vkInfo.addressModeV = gl_to_vk_address_mode(info.wrap_t);
    vkInfo.addressModeW = gl_to_vk_address_mode(info.wrap_r);
    vkInfo.anisotropyEnable = VK_FALSE;
    vkInfo.maxAnisotropy = 1.0f;
    vkInfo.compareEnable = VK_FALSE;
    vkInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    vkInfo.mipmapMode = gl_to_vk_mipmap_mode(info.min_filter);
    vkInfo.mipLodBias = 0.0f;
    vkInfo.minLod = 0.0f;
    vkInfo.maxLod = 0.0f;
    vkInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    vkInfo.unnormalizedCoordinates = VK_FALSE;
    
    if (vkCreateSampler(vulkan.device, &vkInfo, nullptr, &native.handle) != VK_SUCCESS) return {};
    
    vulkan.samplerID += 1;
    return result;
}

void vk_destroy_sampler(rt_sampler_t& sampler)
{
    if (vulkan.device && sampler.native)
    {
        auto& native = vulkan.samplers[sampler.handle];
        if (native.handle) vkDestroySampler(vulkan.device, native.handle, nullptr);
        
        vulkan.samplers.erase(sampler.handle);
        sampler.handle = 0;
    }
}

void vk_bind_sampler(rt_sampler_t sampler, rt_sampler_bind_t bind)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    vulkan.currentBinding[bind.binding].sampler = &sampler;
    vulkan.currentBinding[bind.binding].sampler_bind = bind;
    vulkan.currentBinding[bind.binding].type = GL_BINDING_SAMPLER;
}

static VkShaderModule vk_create_shader_module(const char* source)
{
    if (!source) return nullptr;

    // TODO: The RHI API currently accepts GLSL source strings, while Vulkan only
    // accepts precompiled SPIR-V bytecode. Integrate a GLSL -> SPIR-V compiler
    // (glslang / shaderc) here and fill the create info with the compiled code.
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = 0;     // placeholder: compiled SPIR-V size in bytes
    createInfo.pCode = nullptr;  // placeholder: compiled SPIR-V bytecode

    VkShaderModule module = nullptr;
    if (vkCreateShaderModule(vulkan.device, &createInfo, vulkan.allocator, &module) != VK_SUCCESS)
    {
        fprintf(stderr, "Vulkan: failed to create shader module (GLSL source must be compiled to SPIR-V first)\n");
        return nullptr;
    }
    return module;
}

rt_module_t vk_create_module_compute(const char* comp_src, rt_compute_info_t const& info)
{
    rt_module_t result = {};
    if (!comp_src || !vulkan.device) return result;
    
    result.handle = vulkan.moduleID + 1;
    result.target = GL_MODULE_COMPUTE;
    auto& native = vulkan.modules[result.handle];
    result.native = &native;
    result.compute = info;
    
    // Create shader module from GLSL source (SPIR-V compilation pending)
    native.cshader = vk_create_shader_module(comp_src);
    if (!native.cshader)
    {
        return {};
    }
    
    // Create pipeline layout with push constant range
    VkPushConstantRange pushConstRange = {};
    pushConstRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstRange.offset = 0;
    pushConstRange.size = 128; // 128 bytes max for push constants
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstRange;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    
    if (vkCreatePipelineLayout(vulkan.device, &pipelineLayoutInfo, vulkan.allocator, &native.pipelineLayout) != VK_SUCCESS)
    {
        vkDestroyShaderModule(vulkan.device, native.cshader, vulkan.allocator);
        return {};
    }
    
    // Create compute pipeline
    VkComputePipelineCreateInfo computePipelineInfo = {};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.layout = native.pipelineLayout;
    computePipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computePipelineInfo.stage.module = native.cshader;
    computePipelineInfo.stage.pName = "main";
    
    if (vkCreateComputePipelines(vulkan.device, vulkan.pipelineCache, 1, &computePipelineInfo, vulkan.allocator, &native.pipeline) != VK_SUCCESS)
    {
        vkDestroyPipelineLayout(vulkan.device, native.pipelineLayout, vulkan.allocator);
        vkDestroyShaderModule(vulkan.device, native.cshader, vulkan.allocator);
        return {};
    }
    
    vulkan.moduleID += 1;
    return result;
}

rt_module_t vk_create_module_render(const char* vert_src, const char* frag_src, rt_render_info_t const& info)
{
    rt_module_t result = {};
    if (!vulkan.device) return result;
    
    result.handle = vulkan.moduleID + 1;
    result.target = GL_MODULE_RENDER;
    auto& native = vulkan.modules[result.handle];
    result.native = &native;
    result.render = info;
    
    // Create shader modules from GLSL source (SPIR-V compilation pending)
    if (vert_src)
    {
        native.vshader = vk_create_shader_module(vert_src);
        if (!native.vshader)
        {
            return {};
        }
    }
    
    if (frag_src)
    {
        native.fshader = vk_create_shader_module(frag_src);
        if (!native.fshader)
        {
            if (native.vshader) vkDestroyShaderModule(vulkan.device, native.vshader, vulkan.allocator);
            return {};
        }
    }
    
    // Create descriptor set layout from resource bindings
    VkDescriptorSetLayoutBinding bindings[GL_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t bindingCount = 0;
    
    for (uint32_t i = 0; i < GL_MAX_BINDING_HANDLE_NUM; ++i)
    {
        if (info.layout[i].type == GL_NONE) continue;
        
        bindings[bindingCount].binding = info.layout[i].binding;
        bindings[bindingCount].descriptorType = gl_to_vk_descriptor_type(info.layout[i].type);
        bindings[bindingCount].descriptorCount = 1;
        bindings[bindingCount].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindingCount++;
    }
    
    if (bindingCount > 0)
    {
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindingCount;
        layoutInfo.pBindings = bindings;
        
        if (vkCreateDescriptorSetLayout(vulkan.device, &layoutInfo, vulkan.allocator, &native.descriptorSetLayout) != VK_SUCCESS)
        {
            if (native.vshader) vkDestroyShaderModule(vulkan.device, native.vshader, vulkan.allocator);
            if (native.fshader) vkDestroyShaderModule(vulkan.device, native.fshader, vulkan.allocator);
            return {};
        }
    }
    
    // Create pipeline layout with push constant range and descriptor set layout
    VkPushConstantRange pushConstRange = {};
    pushConstRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstRange.offset = 0;
    pushConstRange.size = 128; // 128 bytes max for push constants
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstRange;
    pipelineLayoutInfo.setLayoutCount = (bindingCount > 0) ? 1 : 0;
    pipelineLayoutInfo.pSetLayouts = (bindingCount > 0) ? &native.descriptorSetLayout : nullptr;
    
    if (vkCreatePipelineLayout(vulkan.device, &pipelineLayoutInfo, vulkan.allocator, &native.pipelineLayout) != VK_SUCCESS)
    {
        if (native.descriptorSetLayout) vkDestroyDescriptorSetLayout(vulkan.device, native.descriptorSetLayout, vulkan.allocator);
        if (native.vshader) vkDestroyShaderModule(vulkan.device, native.vshader, vulkan.allocator);
        if (native.fshader) vkDestroyShaderModule(vulkan.device, native.fshader, vulkan.allocator);
        return {};
    }
    
    // Create vertex input state from vertex layout
    VkVertexInputBindingDescription bindingDesc = {};
    bindingDesc.binding = 0;
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attrDescs[GL_MAX_VERTEX_BUFFER_NUM] = {};
    uint32_t attrCount = 0;
    uint32_t stride = 0;
    
    for (uint32_t i = 0; i < GL_MAX_VERTEX_BUFFER_NUM; ++i)
    {
        if (info.vertex[i].type == GL_NONE) continue;
        
        attrDescs[attrCount].location = info.vertex[i].location;
        attrDescs[attrCount].binding = 0;
        attrDescs[attrCount].format = gl_to_vk_vertex_format(info.vertex[i].type, info.vertex[i].count);
        attrDescs[attrCount].offset = stride;
        stride += gl_to_vk_vertex_size(info.vertex[i].type, info.vertex[i].count);
        attrCount++;
    }
    bindingDesc.stride = stride;
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = (attrCount > 0) ? 1 : 0;
    vertexInputInfo.pVertexBindingDescriptions = (attrCount > 0) ? &bindingDesc : nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = attrCount;
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs;
    
    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = gl_to_vk_primitive_topology(info.primitive);
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state (viewport and scissor are dynamic)
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = gl_to_vk_polygon_mode(info.fill_mode);
    rasterizer.cullMode = gl_to_vk_cull_mode(info.cull_mode);
    rasterizer.frontFace = gl_to_vk_front_face(info.front_face);
    rasterizer.depthBiasEnable = (info.depth.bias != 0.0f || info.depth.biasSlope != 0.0f) ? VK_TRUE : VK_FALSE;
    rasterizer.depthBiasConstantFactor = info.depth.bias;
    rasterizer.depthBiasClamp = info.depth.biasClamp;
    rasterizer.depthBiasSlopeFactor = info.depth.biasSlope;
    rasterizer.lineWidth = 1.0f;

    // Multisample state
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_FALSE;

    // Depth stencil state
    const bool depthEnabled = (info.depth.func != GL_ALWAYS || info.depth.write);
    const bool stencilEnabled =
        (info.stencil.back.func != GL_ALWAYS || info.stencil.back.sfail != GL_KEEP ||
         info.stencil.back.zfail != GL_KEEP || info.stencil.back.zpass != GL_KEEP ||
         info.stencil.front.func != GL_ALWAYS || info.stencil.front.sfail != GL_KEEP ||
         info.stencil.front.zfail != GL_KEEP || info.stencil.front.zpass != GL_KEEP);

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = depthEnabled ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = info.depth.write ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = gl_to_vk_compare_op(info.depth.func);
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = stencilEnabled ? VK_TRUE : VK_FALSE;
    depthStencil.front.failOp = gl_to_vk_stencil_op(info.stencil.front.sfail);
    depthStencil.front.passOp = gl_to_vk_stencil_op(info.stencil.front.zpass);
    depthStencil.front.depthFailOp = gl_to_vk_stencil_op(info.stencil.front.zfail);
    depthStencil.front.compareOp = gl_to_vk_compare_op(info.stencil.front.func);
    depthStencil.front.compareMask = info.stencil.read;
    depthStencil.front.writeMask = info.stencil.write;
    depthStencil.front.reference = 0;
    depthStencil.back.failOp = gl_to_vk_stencil_op(info.stencil.back.sfail);
    depthStencil.back.passOp = gl_to_vk_stencil_op(info.stencil.back.zpass);
    depthStencil.back.depthFailOp = gl_to_vk_stencil_op(info.stencil.back.zfail);
    depthStencil.back.compareOp = gl_to_vk_compare_op(info.stencil.back.func);
    depthStencil.back.compareMask = info.stencil.read;
    depthStencil.back.writeMask = info.stencil.write;
    depthStencil.back.reference = 0;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    // Color blend state
    VkPipelineColorBlendAttachmentState colorBlendAttachments[GL_MAX_COLOR_TEXTURE_NUM] = {};
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        bool blendEnabled =
            (info.colors[i].color.func != GL_ADD || info.colors[i].color.src != GL_ONE ||
             info.colors[i].color.dst != GL_ZERO || info.colors[i].alpha.func != GL_ADD ||
             info.colors[i].alpha.src != GL_ONE || info.colors[i].alpha.dst != GL_ZERO);

        colorBlendAttachments[i].blendEnable = blendEnabled ? VK_TRUE : VK_FALSE;
        colorBlendAttachments[i].srcColorBlendFactor = gl_to_vk_blend_factor(info.colors[i].color.src);
        colorBlendAttachments[i].dstColorBlendFactor = gl_to_vk_blend_factor(info.colors[i].color.dst);
        colorBlendAttachments[i].colorBlendOp = gl_to_vk_blend_op(info.colors[i].color.func);
        colorBlendAttachments[i].srcAlphaBlendFactor = gl_to_vk_blend_factor(info.colors[i].alpha.src);
        colorBlendAttachments[i].dstAlphaBlendFactor = gl_to_vk_blend_factor(info.colors[i].alpha.dst);
        colorBlendAttachments[i].alphaBlendOp = gl_to_vk_blend_op(info.colors[i].alpha.func);
        colorBlendAttachments[i].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = GL_MAX_COLOR_TEXTURE_NUM;
    colorBlending.pAttachments = colorBlendAttachments;

    // Dynamic state (viewport and scissor are set per-pass)
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(sizeof(dynamicStates) / sizeof(dynamicStates[0]));
    dynamicState.pDynamicStates = dynamicStates;

    // Create graphics pipeline
    VkPipelineShaderStageCreateInfo shaderStages[2] = {};
    uint32_t shaderCount = 0;
    
    if (native.vshader)
    {
        shaderStages[shaderCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[shaderCount].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[shaderCount].module = native.vshader;
        shaderStages[shaderCount].pName = "main";
        shaderCount++;
    }
    
    if (native.fshader)
    {
        shaderStages[shaderCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[shaderCount].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[shaderCount].module = native.fshader;
        shaderStages[shaderCount].pName = "main";
        shaderCount++;
    }
    
    // Dynamic rendering: describe the attachment formats up front so the pipeline
    // can be created without a VkRenderPass object. The dynamicRendering device
    // feature must be enabled when the logical device is created.
    VkFormat colorFormats[GL_MAX_COLOR_TEXTURE_NUM];
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
        colorFormats[i] = VK_FORMAT_R8G8B8A8_UNORM;

    VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;
    if (stencilEnabled)
        depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    else if (depthEnabled)
        depthStencilFormat = VK_FORMAT_D32_SFLOAT;

    VkPipelineRenderingCreateInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = GL_MAX_COLOR_TEXTURE_NUM;
    renderingInfo.pColorAttachmentFormats = colorFormats;
    renderingInfo.depthAttachmentFormat = depthStencilFormat;
    renderingInfo.stencilAttachmentFormat = stencilEnabled ? depthStencilFormat : VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {};
    graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineInfo.pNext = &renderingInfo;
    graphicsPipelineInfo.stageCount = shaderCount;
    graphicsPipelineInfo.pStages = shaderStages;
    graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
    graphicsPipelineInfo.pInputAssemblyState = &inputAssembly;
    graphicsPipelineInfo.pViewportState = &viewportState;
    graphicsPipelineInfo.pRasterizationState = &rasterizer;
    graphicsPipelineInfo.pMultisampleState = &multisampling;
    graphicsPipelineInfo.pDepthStencilState = &depthStencil;
    graphicsPipelineInfo.pColorBlendState = &colorBlending;
    graphicsPipelineInfo.pDynamicState = &dynamicState;
    graphicsPipelineInfo.layout = native.pipelineLayout;
    
    if (vkCreateGraphicsPipelines(vulkan.device, vulkan.pipelineCache, 1, &graphicsPipelineInfo, vulkan.allocator, &native.pipeline) != VK_SUCCESS)
    {
        vkDestroyPipelineLayout(vulkan.device, native.pipelineLayout, vulkan.allocator);
        if (native.descriptorSetLayout) vkDestroyDescriptorSetLayout(vulkan.device, native.descriptorSetLayout, vulkan.allocator);
        if (native.vshader) vkDestroyShaderModule(vulkan.device, native.vshader, vulkan.allocator);
        if (native.fshader) vkDestroyShaderModule(vulkan.device, native.fshader, vulkan.allocator);
        return {};
    }
    
    vulkan.moduleID += 1;
    return result;
}

rt_module_t vk_create_module_meshlet(const char* task_src, const char* mesh_src, const char* frag_src, rt_render_info_t const& info)
{
    rt_module_t result = {};
    if (!mesh_src || !vulkan.device) return result;
    
    result.handle = vulkan.moduleID + 1;
    auto& native = vulkan.modules[result.handle];
    result.native = &native;
    result.render = info;
    
    // Create shader modules from GLSL source (SPIR-V compilation pending)
    if (task_src)
    {
        native.tshader = vk_create_shader_module(task_src);
        if (!native.tshader)
        {
            return {};
        }
    }
    
    if (mesh_src)
    {
        native.mshader = vk_create_shader_module(mesh_src);
        if (!native.mshader)
        {
            if (native.tshader) vkDestroyShaderModule(vulkan.device, native.tshader, vulkan.allocator);
            return {};
        }
    }
    
    if (frag_src)
    {
        native.fshader = vk_create_shader_module(frag_src);
        if (!native.fshader)
        {
            if (native.tshader) vkDestroyShaderModule(vulkan.device, native.tshader, vulkan.allocator);
            if (native.mshader) vkDestroyShaderModule(vulkan.device, native.mshader, vulkan.allocator);
            return {};
        }
    }
    
    // Create descriptor set layout from resource bindings
    VkDescriptorSetLayoutBinding bindings[GL_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t bindingCount = 0;
    
    for (uint32_t i = 0; i < GL_MAX_BINDING_HANDLE_NUM; ++i)
    {
        if (info.layout[i].type == GL_NONE) continue;
        
        bindings[bindingCount].binding = info.layout[i].binding;
        bindings[bindingCount].descriptorType = gl_to_vk_descriptor_type(info.layout[i].type);
        bindings[bindingCount].descriptorCount = 1;
        bindings[bindingCount].stageFlags = VK_SHADER_STAGE_TASK_BIT_NV | VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindingCount++;
    }
    
    if (bindingCount > 0)
    {
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindingCount;
        layoutInfo.pBindings = bindings;
        
        if (vkCreateDescriptorSetLayout(vulkan.device, &layoutInfo, vulkan.allocator, &native.descriptorSetLayout) != VK_SUCCESS)
        {
            if (native.tshader) vkDestroyShaderModule(vulkan.device, native.tshader, vulkan.allocator);
            if (native.mshader) vkDestroyShaderModule(vulkan.device, native.mshader, vulkan.allocator);
            if (native.fshader) vkDestroyShaderModule(vulkan.device, native.fshader, vulkan.allocator);
            return {};
        }
    }
    
    // Create pipeline layout with push constant range and descriptor set layout
    VkPushConstantRange pushConstRange = {};
    pushConstRange.stageFlags = VK_SHADER_STAGE_TASK_BIT_NV | VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstRange.offset = 0;
    pushConstRange.size = 128; // 128 bytes max for push constants
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstRange;
    pipelineLayoutInfo.setLayoutCount = (bindingCount > 0) ? 1 : 0;
    pipelineLayoutInfo.pSetLayouts = (bindingCount > 0) ? &native.descriptorSetLayout : nullptr;
    
    if (vkCreatePipelineLayout(vulkan.device, &pipelineLayoutInfo, vulkan.allocator, &native.pipelineLayout) != VK_SUCCESS)
    {
        if (native.descriptorSetLayout) vkDestroyDescriptorSetLayout(vulkan.device, native.descriptorSetLayout, vulkan.allocator);
        if (native.tshader) vkDestroyShaderModule(vulkan.device, native.tshader, vulkan.allocator);
        if (native.mshader) vkDestroyShaderModule(vulkan.device, native.mshader, vulkan.allocator);
        if (native.fshader) vkDestroyShaderModule(vulkan.device, native.fshader, vulkan.allocator);
        return {};
    }
    
    // Note: Mesh shaders generate primitives directly, so there is no vertex
    // input state; the primitive topology comes from the mesh shader itself.
    
    // Viewport state (viewport and scissor are dynamic)
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    
    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = gl_to_vk_polygon_mode(info.fill_mode);
    rasterizer.cullMode = gl_to_vk_cull_mode(info.cull_mode);
    rasterizer.frontFace = gl_to_vk_front_face(info.front_face);
    rasterizer.depthBiasEnable = (info.depth.bias != 0.0f || info.depth.biasSlope != 0.0f) ? VK_TRUE : VK_FALSE;
    rasterizer.depthBiasConstantFactor = info.depth.bias;
    rasterizer.depthBiasClamp = info.depth.biasClamp;
    rasterizer.depthBiasSlopeFactor = info.depth.biasSlope;
    rasterizer.lineWidth = 1.0f;
    
    // Multisample state
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_FALSE;
    
    // Depth stencil state
    const bool depthEnabled = (info.depth.func != GL_ALWAYS || info.depth.write);
    const bool stencilEnabled =
        (info.stencil.back.func != GL_ALWAYS || info.stencil.back.sfail != GL_KEEP ||
         info.stencil.back.zfail != GL_KEEP || info.stencil.back.zpass != GL_KEEP ||
         info.stencil.front.func != GL_ALWAYS || info.stencil.front.sfail != GL_KEEP ||
         info.stencil.front.zfail != GL_KEEP || info.stencil.front.zpass != GL_KEEP);
    
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = depthEnabled ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = info.depth.write ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = gl_to_vk_compare_op(info.depth.func);
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = stencilEnabled ? VK_TRUE : VK_FALSE;
    depthStencil.front.failOp = gl_to_vk_stencil_op(info.stencil.front.sfail);
    depthStencil.front.passOp = gl_to_vk_stencil_op(info.stencil.front.zpass);
    depthStencil.front.depthFailOp = gl_to_vk_stencil_op(info.stencil.front.zfail);
    depthStencil.front.compareOp = gl_to_vk_compare_op(info.stencil.front.func);
    depthStencil.front.compareMask = info.stencil.read;
    depthStencil.front.writeMask = info.stencil.write;
    depthStencil.front.reference = 0;
    depthStencil.back.failOp = gl_to_vk_stencil_op(info.stencil.back.sfail);
    depthStencil.back.passOp = gl_to_vk_stencil_op(info.stencil.back.zpass);
    depthStencil.back.depthFailOp = gl_to_vk_stencil_op(info.stencil.back.zfail);
    depthStencil.back.compareOp = gl_to_vk_compare_op(info.stencil.back.func);
    depthStencil.back.compareMask = info.stencil.read;
    depthStencil.back.writeMask = info.stencil.write;
    depthStencil.back.reference = 0;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    
    // Color blend state
    VkPipelineColorBlendAttachmentState colorBlendAttachments[GL_MAX_COLOR_TEXTURE_NUM] = {};
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        bool blendEnabled =
            (info.colors[i].color.func != GL_ADD || info.colors[i].color.src != GL_ONE ||
             info.colors[i].color.dst != GL_ZERO || info.colors[i].alpha.func != GL_ADD ||
             info.colors[i].alpha.src != GL_ONE || info.colors[i].alpha.dst != GL_ZERO);
    
        colorBlendAttachments[i].blendEnable = blendEnabled ? VK_TRUE : VK_FALSE;
        colorBlendAttachments[i].srcColorBlendFactor = gl_to_vk_blend_factor(info.colors[i].color.src);
        colorBlendAttachments[i].dstColorBlendFactor = gl_to_vk_blend_factor(info.colors[i].color.dst);
        colorBlendAttachments[i].colorBlendOp = gl_to_vk_blend_op(info.colors[i].color.func);
        colorBlendAttachments[i].srcAlphaBlendFactor = gl_to_vk_blend_factor(info.colors[i].alpha.src);
        colorBlendAttachments[i].dstAlphaBlendFactor = gl_to_vk_blend_factor(info.colors[i].alpha.dst);
        colorBlendAttachments[i].alphaBlendOp = gl_to_vk_blend_op(info.colors[i].alpha.func);
        colorBlendAttachments[i].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = GL_MAX_COLOR_TEXTURE_NUM;
    colorBlending.pAttachments = colorBlendAttachments;
    
    // Dynamic state (viewport and scissor are set per-pass)
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(sizeof(dynamicStates) / sizeof(dynamicStates[0]));
    dynamicState.pDynamicStates = dynamicStates;
    
    // Shader stages
    VkPipelineShaderStageCreateInfo shaderStages[3] = {};
    uint32_t shaderCount = 0;
    
    if (native.tshader)
    {
        shaderStages[shaderCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[shaderCount].stage = VK_SHADER_STAGE_TASK_BIT_NV;
        shaderStages[shaderCount].module = native.tshader;
        shaderStages[shaderCount].pName = "main";
        shaderCount++;
    }
    
    if (native.mshader)
    {
        shaderStages[shaderCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[shaderCount].stage = VK_SHADER_STAGE_MESH_BIT_NV;
        shaderStages[shaderCount].module = native.mshader;
        shaderStages[shaderCount].pName = "main";
        shaderCount++;
    }
    
    if (native.fshader)
    {
        shaderStages[shaderCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[shaderCount].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[shaderCount].module = native.fshader;
        shaderStages[shaderCount].pName = "main";
        shaderCount++;
    }
    
    // Dynamic rendering: describe the attachment formats up front so the pipeline
    // can be created without a VkRenderPass object. The dynamicRendering device
    // feature must be enabled when the logical device is created.
    VkFormat colorFormats[GL_MAX_COLOR_TEXTURE_NUM];
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
        colorFormats[i] = VK_FORMAT_R8G8B8A8_UNORM;
    
    VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;
    if (stencilEnabled)
        depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    else if (depthEnabled)
        depthStencilFormat = VK_FORMAT_D32_SFLOAT;
    
    VkPipelineRenderingCreateInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = GL_MAX_COLOR_TEXTURE_NUM;
    renderingInfo.pColorAttachmentFormats = colorFormats;
    renderingInfo.depthAttachmentFormat = depthStencilFormat;
    renderingInfo.stencilAttachmentFormat = stencilEnabled ? depthStencilFormat : VK_FORMAT_UNDEFINED;
    
    VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {};
    graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineInfo.pNext = &renderingInfo;
    graphicsPipelineInfo.stageCount = shaderCount;
    graphicsPipelineInfo.pStages = shaderStages;
    graphicsPipelineInfo.pViewportState = &viewportState;
    graphicsPipelineInfo.pRasterizationState = &rasterizer;
    graphicsPipelineInfo.pMultisampleState = &multisampling;
    graphicsPipelineInfo.pDepthStencilState = &depthStencil;
    graphicsPipelineInfo.pColorBlendState = &colorBlending;
    graphicsPipelineInfo.pDynamicState = &dynamicState;
    graphicsPipelineInfo.layout = native.pipelineLayout;
    
    if (vkCreateGraphicsPipelines(vulkan.device, vulkan.pipelineCache, 1, &graphicsPipelineInfo, vulkan.allocator, &native.pipeline) != VK_SUCCESS)
    {
        vkDestroyPipelineLayout(vulkan.device, native.pipelineLayout, vulkan.allocator);
        if (native.descriptorSetLayout) vkDestroyDescriptorSetLayout(vulkan.device, native.descriptorSetLayout, vulkan.allocator);
        if (native.tshader) vkDestroyShaderModule(vulkan.device, native.tshader, vulkan.allocator);
        if (native.mshader) vkDestroyShaderModule(vulkan.device, native.mshader, vulkan.allocator);
        if (native.fshader) vkDestroyShaderModule(vulkan.device, native.fshader, vulkan.allocator);
        return {};
    }
    
    vulkan.moduleID += 1;
    result.target = GL_MODULE_MESHLET;
    return result;
}

void vk_destroy_module(rt_module_t& module)
{
    if (vulkan.device && module.native)
    {
        auto& native = vulkan.modules[module.handle];
        if (native.vshader) vkDestroyShaderModule(vulkan.device, native.vshader, vulkan.allocator);
        if (native.tshader) vkDestroyShaderModule(vulkan.device, native.tshader, vulkan.allocator);
        if (native.mshader) vkDestroyShaderModule(vulkan.device, native.mshader, vulkan.allocator);
        if (native.fshader) vkDestroyShaderModule(vulkan.device, native.fshader, vulkan.allocator);
        if (native.cshader) vkDestroyShaderModule(vulkan.device, native.cshader, vulkan.allocator);
        
        if (native.pipeline) vkDestroyPipeline(vulkan.device, native.pipeline, vulkan.allocator);
        if (native.pipelineLayout) vkDestroyPipelineLayout(vulkan.device, native.pipelineLayout, vulkan.allocator);
        if (native.descriptorSetLayout) vkDestroyDescriptorSetLayout(vulkan.device, native.descriptorSetLayout, vulkan.allocator);
        
        vulkan.modules.erase(module.handle);
        module.handle = 0;
    }
}

void vk_push_constant(uint8_t const* buffer, size_t length)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    if (!buffer || length == 0) return;
    auto& pass = *vulkan.currentPipeline;
    if (!pass.module.native) return;
    auto mod = (rt_module_native_t*)pass.module.native;
    if (!mod->pipelineLayout) return;

    VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_TASK_BIT_NV;
    vkCmdPushConstants(vulkan.cmdBuffer, mod->pipelineLayout, stages, 0, static_cast<uint32_t>(length), buffer);
}

void vk_push_const_int(const char* name, int32_t value)
{
    fprintf(stderr, "Unsupported implementation");
    abort();
}

void vk_push_const_uint(const char* name, uint32_t value)
{
    fprintf(stderr, "Unsupported implementation");
    abort();
}

void vk_push_const_float(const char* name, float value)
{
    fprintf(stderr, "Unsupported implementation");
    abort();
}

void vk_push_const_vec2(const char* name, const float* value)
{
    fprintf(stderr, "Unsupported implementation");
    abort();
}

void vk_push_const_vec3(const char* name, const float* value)
{
    fprintf(stderr, "Unsupported implementation");
    abort();
}

void vk_push_const_vec4(const char* name, const float* value)
{
    fprintf(stderr, "Unsupported implementation");
    abort();
}

void vk_push_const_mat3(const char* name, const float* value)
{
    fprintf(stderr, "Unsupported implementation");
    abort();
}

void vk_push_const_mat4(const char* name, const float* value)
{
    fprintf(stderr, "Unsupported implementation");
    abort();
}

void vk_begin_compute(rt_pass_t& pass)
{
    if (vulkan.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    vulkan.currentPipeline = &pass;
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo);
}

void vk_end_compute(rt_pass_t& pass)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vulkan.currentPipeline = nullptr;

    vkEndCommandBuffer(vulkan.cmdBuffer);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkan.cmdBuffer;
    
    if (vulkan.queue) vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE);
    if (vulkan.queue) vkQueueWaitIdle(vulkan.queue);
}

void vk_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    vkCmdDispatch(vulkan.cmdBuffer, groupX, groupY, groupZ);
}

void vk_begin_render(rt_pass_t& pass)
{
    if (vulkan.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    vulkan.currentPipeline = &pass;
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo);
}

void vk_end_render(rt_pass_t& pass)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vulkan.currentPipeline = nullptr;

    vkEndCommandBuffer(vulkan.cmdBuffer);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkan.cmdBuffer;
    
    if (vulkan.queue) vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE);
    if (vulkan.queue) vkQueueWaitIdle(vulkan.queue);
}

void vk_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    VkViewport viewport = {};
    viewport.x = static_cast<float>(x);
    viewport.y = static_cast<float>(y);
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    vkCmdSetViewport(vulkan.cmdBuffer, 0, 1, &viewport);
}

void vk_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    
    VkRect2D scissor = {};
    scissor.offset = {x, y};
    scissor.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    
    vkCmdSetScissor(vulkan.cmdBuffer, 0, 1, &scissor);
}

void vk_draw_mesh_task(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    
    // vkCmdDrawMeshTasksNV(vulkan.cmdBuffer, groupX * groupY * groupZ, 0);
}

rt_mesh_t vk_create_mesh(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    rt_mesh_t result = {};
    if (!vertices || vertex_count == 0) return result;
    
    result.handle = vulkan.meshID + 1;
    result.vertex_count = static_cast<GLsizei>(vertex_count);
    result.index_count = static_cast<GLsizei>(index_count);

    auto& native = vulkan.meshes[result.handle];
    result.native = &native;
    
    if (vertices)
    {
        rt_buffer_info_t vertexDesc = {};
        vertexDesc.size = vertex_count * sizeof(float) * 3;
        vertexDesc.usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST;
        vertexDesc.data = vertices;
        result.vertex_vbo = vk_create_buffer(vertexDesc);
    }
    
    if (normals)
    {
        rt_buffer_info_t normalDesc = {};
        normalDesc.size = vertex_count * sizeof(float) * 3;
        normalDesc.usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST;
        normalDesc.data = normals;
        result.normal_vbo = vk_create_buffer(normalDesc);
    }
    
    if (uvs)
    {
        rt_buffer_info_t uvDesc = {};
        uvDesc.size = vertex_count * sizeof(float) * 2;
        uvDesc.usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST;
        uvDesc.data = uvs;
        result.uv_vbo = vk_create_buffer(uvDesc);
    }
    
    if (indices)
    {
        rt_buffer_info_t indexDesc = {};
        indexDesc.size = index_count * sizeof(unsigned int);
        indexDesc.usage = GL_BUFFER_USAGE_INDEX | GL_BUFFER_USAGE_COPY_DST;
        indexDesc.data = indices;
        result.index_vbo = vk_create_buffer(indexDesc);
    }
    
    vulkan.meshID += 1;
    return result;
}

void vk_destroy_mesh(rt_mesh_t& mesh)
{
    if (mesh.vertex_vbo.handle) vk_destroy_buffer(mesh.vertex_vbo);
    if (mesh.normal_vbo.handle) vk_destroy_buffer(mesh.normal_vbo);
    if (mesh.uv_vbo.handle) vk_destroy_buffer(mesh.uv_vbo);
    if (mesh.index_vbo.handle) vk_destroy_buffer(mesh.index_vbo);
    
    vulkan.meshes.erase(mesh.handle);
    mesh.handle = 0;
}

void vk_draw_mesh(rt_mesh_t const& mesh)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    
    if (mesh.index_count)
    {
        vkCmdDrawIndexed(vulkan.cmdBuffer, mesh.index_count, 1, 0, 0, 0);
    }
    else
    {
        vkCmdDraw(vulkan.cmdBuffer, mesh.vertex_count, 1, 0, 0);
    }
}

rt_meshlet_t vk_create_meshlet(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    rt_meshlet_t result = {};
    if (!vertices || vertex_count == 0) return result;
    
    result.vertex_count = static_cast<GLsizei>(vertex_count);
    result.index_count = static_cast<GLsizei>(index_count);
    result.index_type = GL_UNSIGNED_INT;
    result.primitive_type = GL_TRIANGLES;
    
    if (vertices)
    {
        rt_buffer_info_t vertexDesc = {};
        vertexDesc.size = vertex_count * sizeof(float) * 4;
        vertexDesc.usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST;
        vertexDesc.data = vertices;
        result.vertex_vbo = vk_create_buffer(vertexDesc);
    }
    
    if (normals)
    {
        rt_buffer_info_t normalDesc = {};
        normalDesc.size = vertex_count * sizeof(float) * 4;
        normalDesc.usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST;
        normalDesc.data = normals;
        result.normal_vbo = vk_create_buffer(normalDesc);
    }
    
    if (uvs)
    {
        rt_buffer_info_t uvDesc = {};
        uvDesc.size = vertex_count * sizeof(float) * 2;
        uvDesc.usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST;
        uvDesc.data = uvs;
        result.uv_vbo = vk_create_buffer(uvDesc);
    }
    
    if (indices)
    {
        rt_buffer_info_t indexDesc = {};
        indexDesc.size = index_count * sizeof(unsigned int);
        indexDesc.usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST;
        indexDesc.data = indices;
        result.index_vbo = vk_create_buffer(indexDesc);
    }
    
    return result;
}

void vk_destroy_meshlet(rt_meshlet_t& meshlet)
{
    if (meshlet.vertex_vbo.handle) vk_destroy_buffer(meshlet.vertex_vbo);
    if (meshlet.normal_vbo.handle) vk_destroy_buffer(meshlet.normal_vbo);
    if (meshlet.uv_vbo.handle) vk_destroy_buffer(meshlet.uv_vbo);
    if (meshlet.index_vbo.handle) vk_destroy_buffer(meshlet.index_vbo);
    
    vulkan.meshletID += 1;
}

void vk_draw_meshlet(rt_meshlet_t const& meshlet)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
}

rt_mesh_t vk_create_mesh_screen()
{
    rt_mesh_t result = {};
    
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,
         3.0f, -1.0f, 0.0f,
        -1.0f,  3.0f, 0.0f
    };
    
    unsigned int indices[] = { 0, 1, 2 };
    
    result = vk_create_mesh(vertices, nullptr, nullptr, 3, indices, 3);
    return result;
}

void vk_draw_screen(int width, int height, rt_texture_t texture, rt_color_t clear)
{
    constexpr auto VS = R"(
        #version 460
        layout(location = 0) in vec3 in_vertex;
        layout(location = 1) in vec3 in_normal;
        layout(location = 2) in vec2 in_uv;
        layout(location = 0) out vec3 vertex;
        layout(location = 1) out vec3 normal;
        layout(location = 2) out vec2 uv;

        void main()
        {
            vertex = in_vertex;
            normal = in_normal;
            uv = in_uv;
            gl_Position = vec4(in_vertex, 1.0);
        }
    )";
    constexpr auto FS = R"(
        #version 460
        layout(location = 0) in vec3 vertex;
        layout(location = 1) in vec3 normal;
        layout(location = 2) in vec2 uv;
        layout(location = 0) out vec4 final;
        layout(binding = 0) uniform sampler2D texture0;

        void main()
        {
            final = texture(texture0, uv);
        }
    )";
    auto module = vk_create_module_render(VS, FS, {.vertex = {rt_vertex_layout, rt_normal_layout, rt_uv_layout,},});
    rt_pass_t pass = {.module = module, .screen = {.color = { .clear = true, .value = clear, }}};
    vk_begin_render(pass);
    vk_set_viewport(0, 0, width, height);
    vk_bind_texture(texture, { .binding = 0, });
    vk_draw_mesh(vk_create_mesh_screen());
    vk_end_render(pass);
}

#endif