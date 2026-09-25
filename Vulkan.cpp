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
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <numeric>
#include <vector>

enum rt_module_type_t : uint32_t
{
    GL_MODULE_RENDER = 1,
    GL_MODULE_COMPUTE = 2,
    GL_MODULE_MESHLET = 3,
    GL_MODULE_TRANSFER = 4,
};

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

static bool gl_has_mipmap_filter(GLenum minFilter)
{
    return minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST ||
           minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR;
}

static VkFormat gl_to_vk_format(GLenum glFormat, GLenum glType, GLenum internalFormat = 0)
{
    switch (internalFormat)
    {
        case GL_R8: return VK_FORMAT_R8_UNORM;
        case GL_RG8: return VK_FORMAT_R8G8_UNORM;
        case GL_RGB8: return VK_FORMAT_R8G8B8_UNORM;
        case GL_RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
        case GL_SRGB8_ALPHA8: return VK_FORMAT_R8G8B8A8_SRGB;
        case GL_R16F: return VK_FORMAT_R16_SFLOAT;
        case GL_RG16F: return VK_FORMAT_R16G16_SFLOAT;
        case GL_RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case GL_R32F: return VK_FORMAT_R32_SFLOAT;
        case GL_RG32F: return VK_FORMAT_R32G32_SFLOAT;
        case GL_RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case GL_DEPTH_COMPONENT16: return VK_FORMAT_D16_UNORM;
        case GL_DEPTH_COMPONENT24: return VK_FORMAT_X8_D24_UNORM_PACK32;
        case GL_DEPTH_COMPONENT32F: return VK_FORMAT_D32_SFLOAT;
        case GL_DEPTH24_STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
        case GL_DEPTH32F_STENCIL8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
        default: break;
    }

    switch (glFormat)
    {
        case GL_RED:
            return (glType == GL_FLOAT) ? VK_FORMAT_R32_SFLOAT : VK_FORMAT_R8_UNORM;
        case GL_RG:
            return (glType == GL_FLOAT) ? VK_FORMAT_R32G32_SFLOAT : VK_FORMAT_R8G8_UNORM;
        case GL_RGB:
            return (glType == GL_FLOAT) ? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM;
        case GL_RGBA:
            return (glType == GL_FLOAT) ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM;
        case GL_DEPTH_COMPONENT:
            return VK_FORMAT_D32_SFLOAT;
        case GL_DEPTH_STENCIL:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        default:
            return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

static VkImageAspectFlags vk_format_aspect(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case VK_FORMAT_S8_UINT:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        default:
            return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

static VkImageUsageFlags gl_to_vk_image_usage(GLenum format)
{
    VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    switch (format)
    {
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_STENCIL:
            flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            break;
        default:
            flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
            break;
    }
    return flags;
}

static VkBufferUsageFlags rt_to_vk_buffer_usage(uint32_t usage)
{
    VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
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
    if (type == GL_INT)
    {
        switch (count)
        {
            case 1: return VK_FORMAT_R32_SINT;
            case 2: return VK_FORMAT_R32G32_SINT;
            case 3: return VK_FORMAT_R32G32B32_SINT;
            case 4: return VK_FORMAT_R32G32B32A32_SINT;
            default: return VK_FORMAT_R32_SINT;
        }
    }
    if (type == GL_UNSIGNED_INT)
    {
        switch (count)
        {
            case 1: return VK_FORMAT_R32_UINT;
            case 2: return VK_FORMAT_R32G32_UINT;
            case 3: return VK_FORMAT_R32G32B32_UINT;
            case 4: return VK_FORMAT_R32G32B32A32_UINT;
            default: return VK_FORMAT_R32_UINT;
        }
    }
    return VK_FORMAT_R32_SFLOAT;
}

static VkDescriptorType gl_to_vk_descriptor_type(rt_binding_type_t bindingType)
{
    switch (bindingType)
    {
        case GL_BINDING_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case GL_BINDING_TEXTURE: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case GL_BINDING_STORAGE_TEXTURE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case GL_BINDING_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
        default: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
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
        case GL_LINE_LOOP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
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
    if (type == GL_FLOAT || type == GL_INT || type == GL_UNSIGNED_INT)
        return count * 4;
    if (type == GL_SHORT || type == GL_UNSIGNED_SHORT || type == GL_HALF_FLOAT)
        return count * 2;
    if (type == GL_BYTE || type == GL_UNSIGNED_BYTE)
        return count;
    if (type == GL_DOUBLE)
        return count * 8;
    return count * 4;
}

static uint32_t gl_index_type_size(GLenum type)
{
    switch (type)
    {
        case GL_UNSIGNED_SHORT:
            return 2;
        default:
            return 4;
    }
}

static VkIndexType gl_to_vk_index_type(GLenum type)
{
    switch (type)
    {
        case GL_UNSIGNED_SHORT:
            return VK_INDEX_TYPE_UINT16;
        default:
            return VK_INDEX_TYPE_UINT32;
    }
}

static uint32_t vk_format_bytes(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_R8_UNORM: case VK_FORMAT_S8_UINT: return 1;
        case VK_FORMAT_R8G8_UNORM: case VK_FORMAT_D16_UNORM: return 2;
        case VK_FORMAT_R8G8B8_UNORM: return 3;
        case VK_FORMAT_R8G8B8A8_UNORM: case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R32_SFLOAT: case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D24_UNORM_S8_UINT: case VK_FORMAT_X8_D24_UNORM_PACK32: return 4;
        case VK_FORMAT_R16G16B16A16_SFLOAT: case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return 8;
        case VK_FORMAT_R32G32B32_SFLOAT: return 12;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return 16;
        default: return 4;
    }
}

static VkImageAspectFlags vk_image_aspect(GLenum aspect, GLenum format)
{
    if (format == GL_DEPTH_STENCIL)
    {
        if (aspect == GL_STENCIL_INDEX) return VK_IMAGE_ASPECT_STENCIL_BIT;
        if (aspect == GL_DEPTH_COMPONENT) return VK_IMAGE_ASPECT_DEPTH_BIT;
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    if (aspect == GL_STENCIL_INDEX || format == GL_STENCIL_INDEX)
        return VK_IMAGE_ASPECT_STENCIL_BIT;
    if (aspect == GL_DEPTH_COMPONENT || format == GL_DEPTH_COMPONENT)
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    return VK_IMAGE_ASPECT_COLOR_BIT;
}

// ====================================================================

struct rt_buffer_native_t
{
    VkBuffer handle = nullptr;
    VkDeviceMemory memory = nullptr;
    VkMemoryPropertyFlags memFlags = 0;
    VkBufferUsageFlags usage = 0;
    void* mapped = nullptr;
    VkDeviceSize mappedOffset = 0;
    VkDeviceSize mappedSize = 0;
    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags access = 0;
};

struct rt_texture_native_t
{
    VkImage handle = nullptr;
    VkDeviceMemory memory = nullptr;
    VkImageView imageView = nullptr;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t mipLevels = 1;
    uint32_t layers = 1;
    VkExtent3D extent = {1, 1, 1};
    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags access = 0;
};

struct rt_sampler_native_t
{
    VkSampler handle = nullptr;
};

struct rt_module_native_t
{
    VkShaderModule vshader = nullptr;
    VkShaderModule tshader = nullptr;
    VkShaderModule mshader = nullptr;
    VkShaderModule fshader = nullptr;
    VkShaderModule cshader = nullptr;
    VkPipeline pipeline = nullptr;
    VkPipelineLayout pipelineLayout = nullptr;
    VkDescriptorSetLayout descriptorSetLayout = nullptr;
    VkDescriptorSet descriptorSet = nullptr;
    VkShaderStageFlags shaderStages = 0;
    bool isMeshlet = false;
    bool isCompute = false;

    VkDescriptorType descriptorTypes[GL_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t descriptorBindings[GL_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t descriptorCount = 0;
};

struct rt_mesh_native_t
{
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

struct rt_meshlet_native_t
{
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

struct rt_pass_compute_native_t
{
    VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
};

struct rt_pass_render_native_t
{
    bool rendering = false;
    bool offscreen = false;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct rt_pass_transfer_native_t
{
    uint32_t dummy = 0;
};

struct vk_staging_t
{
    VkBuffer buffer = nullptr;
    VkDeviceMemory memory = nullptr;
};

struct vk_native_t
{
    uint32_t bufferID = 0;
    uint32_t textureID = 0;
    uint32_t samplerID = 0;
    uint32_t moduleID = 0;
    uint32_t meshID = 0;
    uint32_t meshletID = 0;
    uint32_t passID = 0;

    std::map<uint32_t, rt_buffer_native_t> buffers;
    std::map<uint32_t, rt_texture_native_t> textures;
    std::map<uint32_t, rt_sampler_native_t> samplers;
    std::map<uint32_t, rt_module_native_t> modules;
    std::map<uint32_t, rt_mesh_native_t> meshes;
    std::map<uint32_t, rt_meshlet_native_t> meshlets;
    std::map<uint32_t, rt_pass_compute_native_t> computePasses;
    std::map<uint32_t, rt_pass_render_native_t> renderPasses;
    std::map<uint32_t, rt_pass_transfer_native_t> transferPasses;

    VkInstance instance = nullptr;
    VkPhysicalDevice physicalDevice = nullptr;
    VkPhysicalDeviceMemoryProperties memProps = {};
    VkDevice device = nullptr;
    VkQueue queue = nullptr;
    uint32_t queueFamily = 0;
    VkAllocationCallbacks* allocator = nullptr;
    VkCommandPool cmdPool = nullptr;
    VkCommandBuffer cmdBuffer = nullptr;
    bool inRendering = false;
    VkDescriptorPool descriptorPool = nullptr;
    VkPipelineCache pipelineCache = nullptr;
    VkSampler defaultSampler = nullptr;
    std::vector<vk_staging_t> pendingStaging;

    PFN_vkCmdDrawMeshTasksNV fnDrawMeshTasksNV = nullptr;

    struct Binding
    {
        GLenum type = GL_NONE;
        rt_buffer_t buffer = {};
        rt_buffer_bind_t buffer_bind = {};
        rt_texture_t texture = {};
        rt_texture_bind_t texture_bind = {};
        rt_texture_t storage_texture = {};
        rt_texture_storage_bind_t storage_texture_bind = {};
        rt_sampler_t sampler = {};
        rt_sampler_bind_t sampler_bind = {};
    } currentBinding[GL_MAX_BINDING_HANDLE_NUM] = {};

    GLenum currentPassType = GL_NONE;
    union
    {
        void* currentPipeline = nullptr;
        rt_pass_render_t* currentRenderPass;
        rt_pass_compute_t* currentComputePass;
        rt_pass_transfer_t* currentTransferPass;
    };

} static vulkan;

static uint32_t vk_find_memory_type(uint32_t typeBits, VkMemoryPropertyFlags flags)
{
    for (uint32_t i = 0; i < vulkan.memProps.memoryTypeCount; ++i)
    {
        if ((typeBits & (1u << i)) && (vulkan.memProps.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    }
    for (uint32_t i = 0; i < vulkan.memProps.memoryTypeCount; ++i)
    {
        if (typeBits & (1u << i))
            return i;
    }
    fprintf(stderr, "Vulkan: no compatible memory type\n");
    abort();
}

static void vk_destroy_staging(vk_staging_t& staging)
{
    if (!vulkan.device) return;
    if (staging.buffer) vkDestroyBuffer(vulkan.device, staging.buffer, vulkan.allocator);
    if (staging.memory) vkFreeMemory(vulkan.device, staging.memory, vulkan.allocator);
    staging = {};
}

static void vk_flush_staging()
{
    for (auto& staging : vulkan.pendingStaging)
        vk_destroy_staging(staging);
    vulkan.pendingStaging.clear();
}

static bool vk_create_staging(VkDeviceSize size, vk_staging_t& staging, void** mapped)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(vulkan.device, &bufferInfo, vulkan.allocator, &staging.buffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements req = {};
    vkGetBufferMemoryRequirements(vulkan.device, staging.buffer, &req);
    VkMemoryAllocateInfo alloc = {};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = vk_find_memory_type(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(vulkan.device, &alloc, vulkan.allocator, &staging.memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(vulkan.device, staging.buffer, vulkan.allocator);
        staging.buffer = nullptr;
        return false;
    }
    vkBindBufferMemory(vulkan.device, staging.buffer, staging.memory, 0);
    if (mapped)
    {
        if (vkMapMemory(vulkan.device, staging.memory, 0, size, 0, mapped) != VK_SUCCESS)
            return false;
    }
    return true;
}

static void vk_transition_image(rt_texture_native_t& image, VkImageLayout newLayout)
{
    if (!image.handle)
        return;

    VkAccessFlags dstAccess = 0;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    switch (newLayout)
    {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            dstAccess = VK_ACCESS_TRANSFER_READ_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            dstAccess = VK_ACCESS_SHADER_READ_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            dstAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_GENERAL:
            dstAccess = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        default:
            dstAccess = 0;
            dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
    }

    if (image.layout != newLayout)
    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = image.access;
        barrier.dstAccessMask = dstAccess;
        barrier.oldLayout = image.layout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.handle;
        barrier.subresourceRange.aspectMask = image.aspect;
        barrier.subresourceRange.levelCount = image.mipLevels;
        barrier.subresourceRange.layerCount = image.layers;
        vkCmdPipelineBarrier(vulkan.cmdBuffer, image.stage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        image.layout = newLayout;
        image.stage = dstStage;
        image.access = dstAccess;
    }
}

static void vk_transition_buffer(rt_buffer_native_t& buffer, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess)
{
    if (!buffer.handle)
        return;
    if (buffer.stage == dstStage && buffer.access == dstAccess)
        return;

    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = buffer.access;
    barrier.dstAccessMask = dstAccess;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = buffer.handle;
    barrier.offset = 0;
    barrier.size = VK_WHOLE_SIZE;
    vkCmdPipelineBarrier(vulkan.cmdBuffer, buffer.stage, dstStage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
    buffer.stage = dstStage;
    buffer.access = dstAccess;
}

static rt_buffer_native_t* vk_buffer_native(rt_buffer_t const& buffer)
{
    if (!buffer.native || buffer.handle == 0) return nullptr;
    auto it = vulkan.buffers.find(buffer.handle);
    if (it == vulkan.buffers.end()) return nullptr;
    return &it->second;
}

static rt_texture_native_t* vk_texture_native(rt_texture_t const& texture)
{
    if (!texture.native || texture.handle == 0) return nullptr;
    auto it = vulkan.textures.find(texture.handle);
    if (it == vulkan.textures.end()) return nullptr;
    return &it->second;
}

static rt_sampler_native_t* vk_sampler_native(rt_sampler_t const& sampler)
{
    if (!sampler.native || sampler.handle == 0) return nullptr;
    auto it = vulkan.samplers.find(sampler.handle);
    if (it == vulkan.samplers.end()) return nullptr;
    return &it->second;
}

static rt_module_native_t* vk_current_module_native()
{
    if (vulkan.currentPassType == GL_MODULE_COMPUTE && vulkan.currentComputePass)
        return (rt_module_native_t*)vulkan.currentComputePass->module.native;
    if (vulkan.currentPassType == GL_MODULE_RENDER && vulkan.currentRenderPass)
        return (rt_module_native_t*)vulkan.currentRenderPass->module.native;
    return nullptr;
}

static VkShaderModule vk_create_shader_module(const char* spirv, uint32_t length)
{
    if (!spirv || !length || !vulkan.device)
        return nullptr;

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = length;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(spirv);
    VkShaderModule module = nullptr;
    if (vkCreateShaderModule(vulkan.device, &createInfo, vulkan.allocator, &module) != VK_SUCCESS)
        return nullptr;
    return module;
}

static bool vk_setup_descriptors(rt_module_native_t& native, rt_binding_t const* bindings, VkShaderStageFlags stages)
{
    VkDescriptorSetLayoutBinding layoutBindings[GL_MAX_BINDING_HANDLE_NUM] = {};
    native.descriptorCount = 0;
    for (uint32_t i = 0; i < GL_MAX_BINDING_HANDLE_NUM; ++i)
    {
        if (!bindings || bindings[i].type == GL_NONE)
            continue;
        auto& item = layoutBindings[native.descriptorCount];
        item.binding = bindings[i].binding;
        item.descriptorType = gl_to_vk_descriptor_type(bindings[i].type);
        item.descriptorCount = 1;
        item.stageFlags = stages;
        native.descriptorBindings[native.descriptorCount] = bindings[i].binding;
        native.descriptorTypes[native.descriptorCount] = item.descriptorType;
        native.descriptorCount++;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = native.descriptorCount;
    layoutInfo.pBindings = native.descriptorCount ? layoutBindings : nullptr;
    if (vkCreateDescriptorSetLayout(vulkan.device, &layoutInfo, vulkan.allocator, &native.descriptorSetLayout) != VK_SUCCESS)
        return false;

    if (native.descriptorCount > 0)
    {
        VkDescriptorSetAllocateInfo alloc = {};
        alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc.descriptorPool = vulkan.descriptorPool;
        alloc.descriptorSetCount = 1;
        alloc.pSetLayouts = &native.descriptorSetLayout;
        if (vkAllocateDescriptorSets(vulkan.device, &alloc, &native.descriptorSet) != VK_SUCCESS)
            native.descriptorSet = nullptr;
    }
    return true;
}

static bool vk_create_graphics_pipeline(rt_module_native_t& native, rt_module_render_info_t const& info, bool meshlet)
{
    VkPipelineShaderStageCreateInfo shaderStages[3] = {};
    uint32_t shaderCount = 0;
    auto add_stage = [&](VkShaderModule module, VkShaderStageFlagBits stage)
    {
        if (!module) return;
        shaderStages[shaderCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[shaderCount].stage = stage;
        shaderStages[shaderCount].module = module;
        shaderStages[shaderCount].pName = "main";
        shaderCount++;
    };
    add_stage(native.vshader, VK_SHADER_STAGE_VERTEX_BIT);
    add_stage(native.tshader, VK_SHADER_STAGE_TASK_BIT_NV);
    add_stage(native.mshader, VK_SHADER_STAGE_MESH_BIT_NV);
    add_stage(native.fshader, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkVertexInputBindingDescription bindingDescs[GL_MAX_VERTEX_BUFFER_NUM] = {};
    VkVertexInputAttributeDescription attrDescs[GL_MAX_VERTEX_BUFFER_NUM] = {};
    uint32_t attrCount = 0;
    if (!meshlet)
    {
        for (uint32_t i = 0; i < GL_MAX_VERTEX_BUFFER_NUM; ++i)
        {
            if (info.vertex[i].type == GL_NONE || info.vertex[i].count == 0)
                continue;
            bindingDescs[attrCount].binding = info.vertex[i].location;
            bindingDescs[attrCount].stride = gl_to_vk_vertex_size(info.vertex[i].type, info.vertex[i].count);
            bindingDescs[attrCount].inputRate = info.vertex[i].instance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
            attrDescs[attrCount].location = info.vertex[i].location;
            attrDescs[attrCount].binding = info.vertex[i].location;
            attrDescs[attrCount].format = gl_to_vk_vertex_format(info.vertex[i].type, info.vertex[i].count);
            attrDescs[attrCount].offset = 0;
            attrCount++;
        }
    }

    VkPipelineVertexInputStateCreateInfo vertexInput = {};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = meshlet ? 0 : attrCount;
    vertexInput.pVertexBindingDescriptions = meshlet || attrCount == 0 ? nullptr : bindingDescs;
    vertexInput.vertexAttributeDescriptionCount = meshlet ? 0 : attrCount;
    vertexInput.pVertexAttributeDescriptions = meshlet || attrCount == 0 ? nullptr : attrDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = gl_to_vk_primitive_topology(info.primitive);

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = gl_to_vk_polygon_mode(info.fill_mode);
    rasterizer.cullMode = gl_to_vk_cull_mode(info.cull_mode);
    rasterizer.frontFace = gl_to_vk_front_face(info.front_face);
    rasterizer.depthBiasEnable = (info.depth.bias != 0.0f || info.depth.biasSlope != 0.0f) ? VK_TRUE : VK_FALSE;
    rasterizer.depthBiasConstantFactor = info.depth.bias;
    rasterizer.depthBiasClamp = info.depth.biasClamp;
    rasterizer.depthBiasSlopeFactor = info.depth.biasSlope;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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
    depthStencil.stencilTestEnable = stencilEnabled ? VK_TRUE : VK_FALSE;
    depthStencil.front.failOp = gl_to_vk_stencil_op(info.stencil.front.sfail);
    depthStencil.front.passOp = gl_to_vk_stencil_op(info.stencil.front.zpass);
    depthStencil.front.depthFailOp = gl_to_vk_stencil_op(info.stencil.front.zfail);
    depthStencil.front.compareOp = gl_to_vk_compare_op(info.stencil.front.func);
    depthStencil.front.compareMask = info.stencil.read;
    depthStencil.front.writeMask = info.stencil.write;
    depthStencil.back.failOp = gl_to_vk_stencil_op(info.stencil.back.sfail);
    depthStencil.back.passOp = gl_to_vk_stencil_op(info.stencil.back.zpass);
    depthStencil.back.depthFailOp = gl_to_vk_stencil_op(info.stencil.back.zfail);
    depthStencil.back.compareOp = gl_to_vk_compare_op(info.stencil.back.func);
    depthStencil.back.compareMask = info.stencil.read;
    depthStencil.back.writeMask = info.stencil.write;

    VkPipelineColorBlendAttachmentState colorBlendAttachments[GL_MAX_COLOR_TEXTURE_NUM] = {};
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        bool blendEnabled =
            (info.colors[i].color.func != GL_FUNC_ADD || info.colors[i].color.src != GL_ONE ||
             info.colors[i].color.dst != GL_ZERO || info.colors[i].alpha.func != GL_FUNC_ADD ||
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
    colorBlending.attachmentCount = GL_MAX_COLOR_TEXTURE_NUM;
    colorBlending.pAttachments = colorBlendAttachments;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkFormat colorFormats[GL_MAX_COLOR_TEXTURE_NUM];
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
        colorFormats[i] = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;
    if (stencilEnabled) depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    else if (depthEnabled) depthStencilFormat = VK_FORMAT_D32_SFLOAT;

    VkPipelineRenderingCreateInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = GL_MAX_COLOR_TEXTURE_NUM;
    renderingInfo.pColorAttachmentFormats = colorFormats;
    renderingInfo.depthAttachmentFormat = depthStencilFormat;
    renderingInfo.stencilAttachmentFormat = stencilEnabled ? depthStencilFormat : VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = shaderCount;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = meshlet ? nullptr : &vertexInput;
    pipelineInfo.pInputAssemblyState = meshlet ? nullptr : &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = native.pipelineLayout;
    return vkCreateGraphicsPipelines(vulkan.device, vulkan.pipelineCache, 1, &pipelineInfo, vulkan.allocator, &native.pipeline) == VK_SUCCESS;
}

static void vk_destroy_module_native(uint32_t handle, void*& native)
{
    if (!vulkan.device)
    {
        native = nullptr;
        return;
    }
    auto it = vulkan.modules.find(handle);
    if (it == vulkan.modules.end())
    {
        native = nullptr;
        return;
    }
    auto& module = it->second;
    if (module.descriptorSet && vulkan.descriptorPool)
        vkFreeDescriptorSets(vulkan.device, vulkan.descriptorPool, 1, &module.descriptorSet);
    if (module.vshader) vkDestroyShaderModule(vulkan.device, module.vshader, vulkan.allocator);
    if (module.tshader) vkDestroyShaderModule(vulkan.device, module.tshader, vulkan.allocator);
    if (module.mshader) vkDestroyShaderModule(vulkan.device, module.mshader, vulkan.allocator);
    if (module.fshader) vkDestroyShaderModule(vulkan.device, module.fshader, vulkan.allocator);
    if (module.cshader) vkDestroyShaderModule(vulkan.device, module.cshader, vulkan.allocator);
    if (module.pipeline) vkDestroyPipeline(vulkan.device, module.pipeline, vulkan.allocator);
    if (module.pipelineLayout) vkDestroyPipelineLayout(vulkan.device, module.pipelineLayout, vulkan.allocator);
    if (module.descriptorSetLayout) vkDestroyDescriptorSetLayout(vulkan.device, module.descriptorSetLayout, vulkan.allocator);
    vulkan.modules.erase(it);
    native = nullptr;
}

static void vk_flush_descriptors()
{
    auto* mod = vk_current_module_native();
    if (!mod || !mod->descriptorSet || !mod->pipelineLayout)
        return;

    VkWriteDescriptorSet writes[GL_MAX_BINDING_HANDLE_NUM] = {};
    VkDescriptorBufferInfo bufferInfos[GL_MAX_BINDING_HANDLE_NUM] = {};
    VkDescriptorImageInfo imageInfos[GL_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t writeCount = 0;

    for (uint32_t i = 0; i < mod->descriptorCount; ++i)
    {
        uint32_t binding = mod->descriptorBindings[i];
        VkDescriptorType type = mod->descriptorTypes[i];
        auto& slot = vulkan.currentBinding[binding];
        writes[writeCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[writeCount].dstSet = mod->descriptorSet;
        writes[writeCount].dstBinding = binding;
        writes[writeCount].descriptorCount = 1;
        writes[writeCount].descriptorType = type;

        if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        {
            auto* buf = vk_buffer_native(slot.buffer);
            if (!buf) continue;
            if (slot.buffer_bind.target == GL_SHADER_STORAGE_BUFFER)
                writes[writeCount].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bufferInfos[writeCount] = {buf->handle, 0, VK_WHOLE_SIZE};
            writes[writeCount].pBufferInfo = &bufferInfos[writeCount];
            VkPipelineStageFlags dstStage = (vulkan.currentPassType == GL_MODULE_COMPUTE) ?
                                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT :
                                           (VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                            VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV | VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV);
            if (writes[writeCount].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                vk_transition_buffer(*buf, dstStage, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
            else
                vk_transition_buffer(*buf, dstStage, VK_ACCESS_UNIFORM_READ_BIT);
        }
        else if (type == VK_DESCRIPTOR_TYPE_SAMPLER)
        {
            auto* samp = vk_sampler_native(slot.sampler);
            if (!samp) continue;
            imageInfos[writeCount] = {samp->handle, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
            writes[writeCount].pImageInfo = &imageInfos[writeCount];
        }
        else if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
        {
            auto* tex = vk_texture_native(slot.texture);
            if (!tex || !tex->imageView) continue;
            vk_transition_image(*tex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            VkSampler sampler = vulkan.defaultSampler;
            if (auto* samp = vk_sampler_native(slot.sampler))
                sampler = samp->handle;
            imageInfos[writeCount] = {sampler, tex->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes[writeCount].pImageInfo = &imageInfos[writeCount];
        }
        else if (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
        {
            auto* tex = vk_texture_native(slot.storage_texture);
            if (!tex || !tex->imageView) continue;
            vk_transition_image(*tex, VK_IMAGE_LAYOUT_GENERAL);
            imageInfos[writeCount] = {VK_NULL_HANDLE, tex->imageView, VK_IMAGE_LAYOUT_GENERAL};
            writes[writeCount].pImageInfo = &imageInfos[writeCount];
        }
        else continue;
        writeCount++;
    }

    if (writeCount)
        vkUpdateDescriptorSets(vulkan.device, writeCount, writes, 0, nullptr);

    VkPipelineBindPoint bindPoint = (vulkan.currentPassType == GL_MODULE_COMPUTE) ?
                                    VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
    vkCmdBindDescriptorSets(vulkan.cmdBuffer, bindPoint, mod->pipelineLayout, 0, 1, &mod->descriptorSet, 0, nullptr);
}

static void vk_require_pass(GLenum type)
{
    if (vulkan.currentPipeline == nullptr || vulkan.currentPassType != type)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
}

static void vk_clear_bindings()
{
    for (auto& binding : vulkan.currentBinding)
        binding = {};
}

// ====================================================================

void vk_load_library(VkInstance instance, VkDevice device, uint32_t family)
{
    vulkan.instance = instance;
    vulkan.device = device;
    vulkan.queueFamily = family;

    uint32_t physCount = 0;
    vkEnumeratePhysicalDevices(instance, &physCount, nullptr);
    std::vector<VkPhysicalDevice> devices(physCount);
    if (physCount) vkEnumeratePhysicalDevices(instance, &physCount, devices.data());
    for (auto phys : devices)
    {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &familyCount, nullptr);
        if (family < familyCount)
        {
            vulkan.physicalDevice = phys;
            break;
        }
    }
    if (!vulkan.physicalDevice && !devices.empty())
        vulkan.physicalDevice = devices[0];
    if (vulkan.physicalDevice)
        vkGetPhysicalDeviceMemoryProperties(vulkan.physicalDevice, &vulkan.memProps);

    vkGetDeviceQueue(vulkan.device, family, 0, &vulkan.queue);
    vulkan.fnDrawMeshTasksNV = (PFN_vkCmdDrawMeshTasksNV)vkGetDeviceProcAddr(vulkan.device, "vkCmdDrawMeshTasksNV");

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = family;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(vulkan.device, &poolInfo, vulkan.allocator, &vulkan.cmdPool) == VK_SUCCESS)
    {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = vulkan.cmdPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(vulkan.device, &allocInfo, &vulkan.cmdBuffer);
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo) != VK_SUCCESS)
        {
            fprintf(stderr, "Vulkan: failed to begin command buffer\n");
            abort();
        }
    }

    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 256},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 256},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256},
        {VK_DESCRIPTOR_TYPE_SAMPLER, 256},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 256}
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = (uint32_t)std::size(poolSizes);
    descriptorPoolInfo.pPoolSizes = poolSizes;
    descriptorPoolInfo.maxSets = 256;
    descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    vkCreateDescriptorPool(vulkan.device, &descriptorPoolInfo, vulkan.allocator, &vulkan.descriptorPool);

    VkPipelineCacheCreateInfo cacheInfo = {};
    cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    vkCreatePipelineCache(vulkan.device, &cacheInfo, vulkan.allocator, &vulkan.pipelineCache);

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    vkCreateSampler(vulkan.device, &samplerInfo, vulkan.allocator, &vulkan.defaultSampler);

    rt_create_buffer = vk_create_buffer;
    rt_destroy_buffer = vk_destroy_buffer;
    rt_bind_buffer = vk_bind_buffer;
    rt_map_buffer = vk_map_buffer;
    rt_unmap_buffer = vk_unmap_buffer;
    rt_create_texture = vk_create_texture;
    rt_create_texture_color = vk_create_texture_color;
    rt_create_texture_depth = vk_create_texture_depth;
    rt_create_texture_depth_stencil = vk_create_texture_depth_stencil;
    rt_destroy_texture = vk_destroy_texture;
    rt_bind_texture = vk_bind_texture;
    rt_bind_texture_storage = vk_bind_texture_storage;
    rt_create_sampler = vk_create_sampler;
    rt_destroy_sampler = vk_destroy_sampler;
    rt_bind_sampler = vk_bind_sampler;
    rt_create_module_compute = vk_create_module_compute;
    rt_create_module_render = vk_create_module_render;
    rt_create_module_meshlet = vk_create_module_meshlet;
    rt_destroy_module_render = vk_destroy_module_render;
    rt_destroy_module_compute = vk_destroy_module_compute;
    rt_begin_compute = vk_begin_compute;
    rt_end_compute = vk_end_compute;
    rt_dispatch_compute = vk_dispatch_compute;
    rt_begin_render = vk_begin_render;
    rt_end_render = vk_end_render;
    rt_set_viewport = vk_set_viewport;
    rt_set_scissor = vk_set_scissor;
    rt_draw_mesh_task = vk_draw_mesh_task;
    rt_push_constant = vk_push_constant;
    rt_push_const_int = vk_push_const_int;
    rt_push_const_uint = vk_push_const_uint;
    rt_push_const_float = vk_push_const_float;
    rt_push_const_vec2 = vk_push_const_vec2;
    rt_push_const_vec3 = vk_push_const_vec3;
    rt_push_const_vec4 = vk_push_const_vec4;
    rt_push_const_mat3 = vk_push_const_mat3;
    rt_push_const_mat4 = vk_push_const_mat4;
    rt_begin_transfer = vk_begin_transfer;
    rt_end_transfer = vk_end_transfer;
    rt_copy_buffer = vk_copy_buffer;
    rt_copy_buffer_data = vk_copy_buffer_data;
    rt_copy_buffer_texture = vk_copy_buffer_texture;
    rt_copy_texture = vk_copy_texture;
    rt_copy_texture_data = vk_copy_texture_data;
    rt_copy_texture_buffer = vk_copy_texture_buffer;
    rt_create_mesh = vk_create_mesh;
    rt_destroy_mesh = vk_destroy_mesh;
    rt_draw_mesh = vk_draw_mesh;
    rt_create_meshlet = vk_create_meshlet;
    rt_destroy_meshlet = vk_destroy_meshlet;
    rt_draw_meshlet = vk_draw_meshlet;
    rt_create_mesh_screen = vk_create_mesh_screen;
    rt_draw_screen = vk_draw_screen;
    rt_submit = vk_submit;
}

void vk_unload_library()
{
    if (!vulkan.device) return;
    vkDeviceWaitIdle(vulkan.device);
    if (vulkan.cmdBuffer)
        vkEndCommandBuffer(vulkan.cmdBuffer);
    vk_flush_staging();
    if (vulkan.defaultSampler)
    {
        vkDestroySampler(vulkan.device, vulkan.defaultSampler, vulkan.allocator);
        vulkan.defaultSampler = nullptr;
    }
    if (vulkan.descriptorPool)
    {
        vkDestroyDescriptorPool(vulkan.device, vulkan.descriptorPool, vulkan.allocator);
        vulkan.descriptorPool = nullptr;
    }
    if (vulkan.pipelineCache)
    {
        vkDestroyPipelineCache(vulkan.device, vulkan.pipelineCache, vulkan.allocator);
        vulkan.pipelineCache = nullptr;
    }
    if (vulkan.cmdPool && vulkan.cmdBuffer)
    {
        vkFreeCommandBuffers(vulkan.device, vulkan.cmdPool, 1, &vulkan.cmdBuffer);
        vulkan.cmdBuffer = nullptr;
    }
    if (vulkan.cmdPool)
    {
        vkDestroyCommandPool(vulkan.device, vulkan.cmdPool, vulkan.allocator);
        vulkan.cmdPool = nullptr;
    }

    vulkan.buffers.clear();
    vulkan.textures.clear();
    vulkan.samplers.clear();
    vulkan.modules.clear();
    vulkan.meshes.clear();
    vulkan.meshlets.clear();
    vulkan.computePasses.clear();
    vulkan.renderPasses.clear();
    vulkan.transferPasses.clear();
    vulkan.queue = nullptr;
    vulkan.device = nullptr;
    vulkan.instance = nullptr;
    vulkan.physicalDevice = nullptr;
    vulkan.bufferID = vulkan.textureID = vulkan.samplerID = 0;
    vulkan.moduleID = vulkan.meshID = vulkan.meshletID = vulkan.passID = 0;
    vulkan.currentPassType = GL_NONE;
    vulkan.currentPipeline = nullptr;
    vulkan.inRendering = false;

    if (rt_create_buffer == vk_create_buffer) rt_create_buffer = nullptr;
    if (rt_destroy_buffer == vk_destroy_buffer) rt_destroy_buffer = nullptr;
    if (rt_bind_buffer == vk_bind_buffer) rt_bind_buffer = nullptr;
    if (rt_map_buffer == vk_map_buffer) rt_map_buffer = nullptr;
    if (rt_unmap_buffer == vk_unmap_buffer) rt_unmap_buffer = nullptr;
    if (rt_create_texture == vk_create_texture) rt_create_texture = nullptr;
    if (rt_create_texture_color == vk_create_texture_color) rt_create_texture_color = nullptr;
    if (rt_create_texture_depth == vk_create_texture_depth) rt_create_texture_depth = nullptr;
    if (rt_create_texture_depth_stencil == vk_create_texture_depth_stencil) rt_create_texture_depth_stencil = nullptr;
    if (rt_destroy_texture == vk_destroy_texture) rt_destroy_texture = nullptr;
    if (rt_bind_texture == vk_bind_texture) rt_bind_texture = nullptr;
    if (rt_bind_texture_storage == vk_bind_texture_storage) rt_bind_texture_storage = nullptr;
    if (rt_create_sampler == vk_create_sampler) rt_create_sampler = nullptr;
    if (rt_destroy_sampler == vk_destroy_sampler) rt_destroy_sampler = nullptr;
    if (rt_bind_sampler == vk_bind_sampler) rt_bind_sampler = nullptr;
    if (rt_create_module_compute == vk_create_module_compute) rt_create_module_compute = nullptr;
    if (rt_create_module_render == vk_create_module_render) rt_create_module_render = nullptr;
    if (rt_create_module_meshlet == vk_create_module_meshlet) rt_create_module_meshlet = nullptr;
    if (rt_destroy_module_render == vk_destroy_module_render) rt_destroy_module_render = nullptr;
    if (rt_destroy_module_compute == vk_destroy_module_compute) rt_destroy_module_compute = nullptr;
    if (rt_begin_compute == vk_begin_compute) rt_begin_compute = nullptr;
    if (rt_end_compute == vk_end_compute) rt_end_compute = nullptr;
    if (rt_dispatch_compute == vk_dispatch_compute) rt_dispatch_compute = nullptr;
    if (rt_begin_render == vk_begin_render) rt_begin_render = nullptr;
    if (rt_end_render == vk_end_render) rt_end_render = nullptr;
    if (rt_set_viewport == vk_set_viewport) rt_set_viewport = nullptr;
    if (rt_set_scissor == vk_set_scissor) rt_set_scissor = nullptr;
    if (rt_draw_mesh_task == vk_draw_mesh_task) rt_draw_mesh_task = nullptr;
    if (rt_push_constant == vk_push_constant) rt_push_constant = nullptr;
    if (rt_push_const_int == vk_push_const_int) rt_push_const_int = nullptr;
    if (rt_push_const_uint == vk_push_const_uint) rt_push_const_uint = nullptr;
    if (rt_push_const_float == vk_push_const_float) rt_push_const_float = nullptr;
    if (rt_push_const_vec2 == vk_push_const_vec2) rt_push_const_vec2 = nullptr;
    if (rt_push_const_vec3 == vk_push_const_vec3) rt_push_const_vec3 = nullptr;
    if (rt_push_const_vec4 == vk_push_const_vec4) rt_push_const_vec4 = nullptr;
    if (rt_push_const_mat3 == vk_push_const_mat3) rt_push_const_mat3 = nullptr;
    if (rt_push_const_mat4 == vk_push_const_mat4) rt_push_const_mat4 = nullptr;
    if (rt_begin_transfer == vk_begin_transfer) rt_begin_transfer = nullptr;
    if (rt_end_transfer == vk_end_transfer) rt_end_transfer = nullptr;
    if (rt_copy_buffer == vk_copy_buffer) rt_copy_buffer = nullptr;
    if (rt_copy_buffer_data == vk_copy_buffer_data) rt_copy_buffer_data = nullptr;
    if (rt_copy_buffer_texture == vk_copy_buffer_texture) rt_copy_buffer_texture = nullptr;
    if (rt_copy_texture == vk_copy_texture) rt_copy_texture = nullptr;
    if (rt_copy_texture_data == vk_copy_texture_data) rt_copy_texture_data = nullptr;
    if (rt_copy_texture_buffer == vk_copy_texture_buffer) rt_copy_texture_buffer = nullptr;
    if (rt_create_mesh == vk_create_mesh) rt_create_mesh = nullptr;
    if (rt_destroy_mesh == vk_destroy_mesh) rt_destroy_mesh = nullptr;
    if (rt_draw_mesh == vk_draw_mesh) rt_draw_mesh = nullptr;
    if (rt_create_meshlet == vk_create_meshlet) rt_create_meshlet = nullptr;
    if (rt_destroy_meshlet == vk_destroy_meshlet) rt_destroy_meshlet = nullptr;
    if (rt_draw_meshlet == vk_draw_meshlet) rt_draw_meshlet = nullptr;
    if (rt_create_mesh_screen == vk_create_mesh_screen) rt_create_mesh_screen = nullptr;
    if (rt_draw_screen == vk_draw_screen) rt_draw_screen = nullptr;
    if (rt_submit == vk_submit) rt_submit = nullptr;
}

rt_buffer_t vk_create_buffer(rt_buffer_info_t const& info)
{
    if (info.usage == 0)
    {
        fprintf(stderr, "Buffer usage must not be 0");
        abort();
    }
    rt_buffer_t result = {};
    if (!vulkan.device || info.size == 0) return result;

    uint32_t handle = vulkan.bufferID + 1;
    auto& native = vulkan.buffers[handle];
    VkBufferCreateInfo vkInfo = {};
    vkInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkInfo.size = info.size;
    vkInfo.usage = rt_to_vk_buffer_usage(info.usage);
    vkInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    native.usage = vkInfo.usage;
    if (vkCreateBuffer(vulkan.device, &vkInfo, vulkan.allocator, &native.handle) != VK_SUCCESS)
    {
        vulkan.buffers.erase(handle);
        return {};
    }

    VkMemoryRequirements req = {};
    vkGetBufferMemoryRequirements(vulkan.device, native.handle, &req);
    bool hostVisible = (info.usage & (GL_BUFFER_USAGE_MAP_READ | GL_BUFFER_USAGE_MAP_WRITE)) || info.data;
    native.memFlags = hostVisible ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                                  : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkMemoryAllocateInfo alloc = {};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = vk_find_memory_type(req.memoryTypeBits, native.memFlags);
    if (vkAllocateMemory(vulkan.device, &alloc, vulkan.allocator, &native.memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(vulkan.device, native.handle, vulkan.allocator);
        vulkan.buffers.erase(handle);
        return {};
    }
    vkBindBufferMemory(vulkan.device, native.handle, native.memory, 0);
    if (info.data)
    {
        void* mapped = nullptr;
        if (native.memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            vkMapMemory(vulkan.device, native.memory, 0, info.size, 0, &mapped);
            std::memcpy(mapped, info.data, info.size);
            vkUnmapMemory(vulkan.device, native.memory);
            native.stage = VK_PIPELINE_STAGE_HOST_BIT;
            native.access = VK_ACCESS_HOST_WRITE_BIT;
        }
        else
        {
            vk_staging_t staging = {};
            void* stagingPtr = nullptr;
            if (vk_create_staging(info.size, staging, &stagingPtr))
            {
                std::memcpy(stagingPtr, info.data, info.size);
                vkUnmapMemory(vulkan.device, staging.memory);
                vk_transition_buffer(native, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
                VkBufferCopy region = {0, 0, info.size};
                vkCmdCopyBuffer(vulkan.cmdBuffer, staging.buffer, native.handle, 1, &region);
                vulkan.pendingStaging.push_back(staging);
            }
        }
    }

    vulkan.bufferID = handle;
    result.handle = handle;
    result.size = info.size;
    result.usage = info.usage;
    result.native = &native;
    return result;
}

void vk_destroy_buffer(rt_buffer_t& buffer)
{
    if (vulkan.device && buffer.native)
    {
        auto it = vulkan.buffers.find(buffer.handle);
        if (it != vulkan.buffers.end())
        {
            auto& native = it->second;
            if (native.mapped)
            {
                vkUnmapMemory(vulkan.device, native.memory);
                native.mapped = nullptr;
            }
            if (native.handle) vkDestroyBuffer(vulkan.device, native.handle, vulkan.allocator);
            if (native.memory) vkFreeMemory(vulkan.device, native.memory, vulkan.allocator);
            vulkan.buffers.erase(it);
        }
    }
    buffer = {};
}

void vk_bind_buffer(rt_buffer_t& buffer, rt_buffer_bind_t bind)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vulkan.currentBinding[bind.binding].type = GL_BINDING_BUFFER;
    vulkan.currentBinding[bind.binding].buffer = buffer;
    vulkan.currentBinding[bind.binding].buffer_bind = bind;
}

void* vk_map_buffer(rt_buffer_t& buffer, GLenum mode, size_t offset, size_t size)
{
    (void)mode;
    auto* native = vk_buffer_native(buffer);
    if (!native || !native->memory) return nullptr;
    if (offset > buffer.size) return nullptr;
    if (size == 0) size = buffer.size - offset;
    if (size == 0 || offset + size > buffer.size) return nullptr;
    if (native->mapped)
        vkUnmapMemory(vulkan.device, native->memory);
    VkAccessFlags hostAccess = 0;
    if (mode != GL_WRITE_ONLY)
        hostAccess |= VK_ACCESS_HOST_READ_BIT;
    if (mode != GL_READ_ONLY)
        hostAccess |= VK_ACCESS_HOST_WRITE_BIT;
    vk_transition_buffer(*native, VK_PIPELINE_STAGE_HOST_BIT, hostAccess);
    void* ptr = nullptr;
    if (vkMapMemory(vulkan.device, native->memory, offset, size, 0, &ptr) != VK_SUCCESS)
        return nullptr;
    native->mapped = ptr;
    native->mappedOffset = offset;
    native->mappedSize = size;
    return ptr;
}

void vk_unmap_buffer(rt_buffer_t& buffer)
{
    auto* native = vk_buffer_native(buffer);
    if (!native || !native->mapped || !native->memory) return;
    vkUnmapMemory(vulkan.device, native->memory);
    native->mapped = nullptr;
    native->mappedOffset = 0;
    native->mappedSize = 0;
}

rt_texture_t vk_create_texture(rt_texture_info_t const& info)
{
    rt_texture_t result = {};
    if (!vulkan.device || info.width == 0 || info.height == 0) return result;

    uint32_t handle = vulkan.textureID + 1;
    auto& native = vulkan.textures[handle];
    native.format = gl_to_vk_format(info.format, info.type, info.internal_format);
    native.aspect = vk_format_aspect(native.format);
    native.extent = {info.width, info.height, info.depth ? info.depth : 1};
    native.layers = 1;
    native.mipLevels = 1;
    if (gl_has_mipmap_filter(info.min_filter))
    {
        uint32_t maxDim = std::max(info.width, info.height);
        native.mipLevels = 1;
        while (maxDim >>= 1) native.mipLevels++;
    }

    VkImageCreateInfo vkInfo = {};
    vkInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vkInfo.imageType = (info.target == GL_TEXTURE_3D) ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    vkInfo.extent = native.extent;
    vkInfo.mipLevels = native.mipLevels;
    vkInfo.arrayLayers = native.layers;
    vkInfo.format = native.format;
    vkInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    vkInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkInfo.usage = gl_to_vk_image_usage(info.format);
    vkInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    vkInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(vulkan.device, &vkInfo, vulkan.allocator, &native.handle) != VK_SUCCESS)
    {
        vulkan.textures.erase(handle);
        return {};
    }

    VkMemoryRequirements req = {};
    vkGetImageMemoryRequirements(vulkan.device, native.handle, &req);
    VkMemoryAllocateInfo alloc = {};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = vk_find_memory_type(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(vulkan.device, &alloc, vulkan.allocator, &native.memory) != VK_SUCCESS)
    {
        vkDestroyImage(vulkan.device, native.handle, vulkan.allocator);
        vulkan.textures.erase(handle);
        return {};
    }
    vkBindImageMemory(vulkan.device, native.handle, native.memory, 0);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = native.handle;
    viewInfo.viewType = (info.target == GL_TEXTURE_3D) ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = native.format;
    viewInfo.subresourceRange.aspectMask = native.aspect;
    viewInfo.subresourceRange.levelCount = native.mipLevels;
    viewInfo.subresourceRange.layerCount = native.layers;
    if (vkCreateImageView(vulkan.device, &viewInfo, vulkan.allocator, &native.imageView) != VK_SUCCESS)
        native.imageView = nullptr;

    if (info.data)
    {
        VkDeviceSize bytes = (VkDeviceSize)info.width * info.height * native.extent.depth * vk_format_bytes(native.format);
        vk_staging_t staging = {};
        void* ptr = nullptr;
        if (vk_create_staging(bytes, staging, &ptr))
        {
            std::memcpy(ptr, info.data, (size_t)bytes);
            vkUnmapMemory(vulkan.device, staging.memory);
            vk_transition_image(native, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            VkBufferImageCopy region = {};
            region.imageSubresource.aspectMask = native.aspect;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = native.extent;
            vkCmdCopyBufferToImage(vulkan.cmdBuffer, staging.buffer, native.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            vulkan.pendingStaging.push_back(staging);
        }
        vk_transition_image(native, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    else
    {
        VkImageLayout layout = (native.aspect & VK_IMAGE_ASPECT_DEPTH_BIT) ?
                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        vk_transition_image(native, layout);
    }

    vulkan.textureID = handle;
    result.handle = handle;
    result.width = info.width;
    result.height = info.height;
    result.depth = info.depth;
    result.format = info.format;
    result.internal_format = info.internal_format;
    result.type = info.type;
    result.target = info.target;
    result.mipmaps = native.mipLevels;
    result.samples = info.samples ? info.samples : 1;
    result.native = &native;
    return result;
}

rt_texture_t vk_create_texture_color(uint32_t width, uint32_t height, const void* data)
{
    return vk_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_RGBA, .internal_format = GL_RGBA8, .type = GL_UNSIGNED_BYTE,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

rt_texture_t vk_create_texture_depth(uint32_t width, uint32_t height, const void* data)
{
    return vk_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_DEPTH_COMPONENT, .internal_format = GL_DEPTH_COMPONENT32F, .type = GL_FLOAT,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

rt_texture_t vk_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data)
{
    return vk_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_DEPTH_STENCIL, .internal_format = GL_DEPTH24_STENCIL8, .type = GL_UNSIGNED_INT_24_8,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

void vk_destroy_texture(rt_texture_t& texture)
{
    if (vulkan.device && texture.native)
    {
        auto it = vulkan.textures.find(texture.handle);
        if (it != vulkan.textures.end())
        {
            auto& native = it->second;
            if (native.imageView) vkDestroyImageView(vulkan.device, native.imageView, vulkan.allocator);
            if (native.handle) vkDestroyImage(vulkan.device, native.handle, vulkan.allocator);
            if (native.memory) vkFreeMemory(vulkan.device, native.memory, vulkan.allocator);
            vulkan.textures.erase(it);
        }
    }
    texture = {};
}

void vk_bind_texture(rt_texture_t& texture, rt_texture_bind_t bind)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vulkan.currentBinding[bind.binding].type = GL_BINDING_TEXTURE;
    vulkan.currentBinding[bind.binding].texture = texture;
    vulkan.currentBinding[bind.binding].texture_bind = bind;
}

void vk_bind_texture_storage(rt_texture_t& texture, rt_texture_storage_bind_t bind)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vulkan.currentBinding[bind.binding].type = GL_BINDING_STORAGE_TEXTURE;
    vulkan.currentBinding[bind.binding].storage_texture = texture;
    vulkan.currentBinding[bind.binding].storage_texture_bind = bind;
}

rt_sampler_t vk_create_sampler(rt_sampler_info_t const& info)
{
    rt_sampler_t result = {};
    if (!vulkan.device) return result;
    uint32_t handle = vulkan.samplerID + 1;
    auto& native = vulkan.samplers[handle];
    VkSamplerCreateInfo vkInfo = {};
    vkInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    vkInfo.magFilter = gl_to_vk_filter(info.mag_filter);
    vkInfo.minFilter = gl_to_vk_min_filter(info.min_filter);
    vkInfo.addressModeU = gl_to_vk_address_mode(info.wrap_s);
    vkInfo.addressModeV = gl_to_vk_address_mode(info.wrap_t);
    vkInfo.addressModeW = gl_to_vk_address_mode(info.wrap_r);
    vkInfo.mipmapMode = gl_to_vk_mipmap_mode(info.min_filter);
    vkInfo.maxLod = gl_has_mipmap_filter(info.min_filter) ? VK_LOD_CLAMP_NONE : 0.0f;
    if (vkCreateSampler(vulkan.device, &vkInfo, vulkan.allocator, &native.handle) != VK_SUCCESS)
    {
        vulkan.samplers.erase(handle);
        return {};
    }
    vulkan.samplerID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void vk_destroy_sampler(rt_sampler_t& sampler)
{
    if (vulkan.device && sampler.native)
    {
        auto it = vulkan.samplers.find(sampler.handle);
        if (it != vulkan.samplers.end())
        {
            if (it->second.handle) vkDestroySampler(vulkan.device, it->second.handle, vulkan.allocator);
            vulkan.samplers.erase(it);
        }
    }
    sampler = {};
}

void vk_bind_sampler(rt_sampler_t& sampler, rt_sampler_bind_t bind)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vulkan.currentBinding[bind.binding].type = GL_BINDING_SAMPLER;
    vulkan.currentBinding[bind.binding].sampler = sampler;
    vulkan.currentBinding[bind.binding].sampler_bind = bind;
}

rt_module_compute_t vk_create_module_compute(rt_module_compute_info_t const& info)
{
    rt_module_compute_t result = {};
    if (!info.cshader || !info.clength || !vulkan.device) return result;
    uint32_t handle = vulkan.moduleID + 1;
    auto& native = vulkan.modules[handle];
    native.isCompute = true;
    native.shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
    native.cshader = vk_create_shader_module(info.cshader, info.clength);
    if (!native.cshader)
    {
        vulkan.modules.erase(handle);
        return {};
    }
    if (!vk_setup_descriptors(native, nullptr, VK_SHADER_STAGE_COMPUTE_BIT))
    {
        vk_destroy_module_native(handle, result.native);
        return {};
    }
    VkPushConstantRange push = {};
    push.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push.offset = 0;
    push.size = 128;
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &push;
    layoutInfo.setLayoutCount = native.descriptorSetLayout ? 1 : 0;
    layoutInfo.pSetLayouts = native.descriptorSetLayout ? &native.descriptorSetLayout : nullptr;
    if (vkCreatePipelineLayout(vulkan.device, &layoutInfo, vulkan.allocator, &native.pipelineLayout) != VK_SUCCESS)
    {
        vk_destroy_module_native(handle, result.native);
        return {};
    }
    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = native.pipelineLayout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = native.cshader;
    pipelineInfo.stage.pName = "main";
    if (vkCreateComputePipelines(vulkan.device, vulkan.pipelineCache, 1, &pipelineInfo, vulkan.allocator, &native.pipeline) != VK_SUCCESS)
    {
        result.native = &native;
        vk_destroy_module_native(handle, result.native);
        return {};
    }
    vulkan.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

rt_module_render_t vk_create_module_render(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};
    if (!vulkan.device) return result;
    uint32_t handle = vulkan.moduleID + 1;
    auto& native = vulkan.modules[handle];
    native.shaderStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    if (info.vshader)
    {
        native.vshader = vk_create_shader_module(info.vshader, info.vlength);
        if (!native.vshader) { vulkan.modules.erase(handle); return {}; }
    }
    if (info.fshader)
    {
        native.fshader = vk_create_shader_module(info.fshader, info.flength);
        if (!native.fshader)
        {
            result.native = &native;
            vk_destroy_module_native(handle, result.native);
            return {};
        }
    }
    if (!vk_setup_descriptors(native, info.binding, native.shaderStages))
    {
        result.native = &native;
        vk_destroy_module_native(handle, result.native);
        return {};
    }
    VkPushConstantRange push = {};
    push.stageFlags = native.shaderStages;
    push.offset = 0;
    push.size = 128;
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &push;
    layoutInfo.setLayoutCount = native.descriptorSetLayout ? 1 : 0;
    layoutInfo.pSetLayouts = native.descriptorSetLayout ? &native.descriptorSetLayout : nullptr;
    if (vkCreatePipelineLayout(vulkan.device, &layoutInfo, vulkan.allocator, &native.pipelineLayout) != VK_SUCCESS ||
        !vk_create_graphics_pipeline(native, info, false))
    {
        result.native = &native;
        vk_destroy_module_native(handle, result.native);
        return {};
    }
    vulkan.moduleID = handle;
    result.handle = handle;
    result.native = &native;

    for (size_t i = 0; i < std::size(info.colors); ++i)
    {
        result.colors[i].color.func = info.colors[i].color.func;
        result.colors[i].color.src = info.colors[i].color.src;
        result.colors[i].color.dst = info.colors[i].color.dst;
        result.colors[i].alpha.func = info.colors[i].alpha.func;
        result.colors[i].alpha.src = info.colors[i].alpha.src;
        result.colors[i].alpha.dst = info.colors[i].alpha.dst;
    }
    result.depth.write = info.depth.write;
    result.depth.bias = info.depth.bias;
    result.depth.biasSlope = info.depth.biasSlope;
    result.depth.biasClamp = info.depth.biasClamp;
    result.depth.func = info.depth.func;
    result.stencil.read = info.stencil.read;
    result.stencil.write = info.stencil.write;
    result.stencil.back.func = info.stencil.back.func;
    result.stencil.back.sfail = info.stencil.back.sfail;
    result.stencil.back.zfail = info.stencil.back.zfail;
    result.stencil.back.zpass = info.stencil.back.zpass;
    result.stencil.front.func = info.stencil.front.func;
    result.stencil.front.sfail = info.stencil.front.sfail;
    result.stencil.front.zfail = info.stencil.front.zfail;
    result.stencil.front.zpass = info.stencil.front.zpass;
    result.index_type = info.index_type;
    for (size_t i = 0; i < std::size(info.vertex); ++i)
        result.vertex[i] = info.vertex[i];
    for (size_t i = 0; i < std::size(info.binding); ++i)
        result.binding[i] = info.binding[i];
    result.cull_mode = info.cull_mode;
    result.front_face = info.front_face;
    result.fill_mode = info.fill_mode;
    result.primitive = info.primitive;
    return result;
}

rt_module_render_t vk_create_module_meshlet(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};
    if (!info.mshader || !info.mlength || !vulkan.device) return result;
    uint32_t handle = vulkan.moduleID + 1;
    auto& native = vulkan.modules[handle];
    native.isMeshlet = true;
    native.shaderStages = VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT;
    if (info.tshader)
    {
        native.shaderStages |= VK_SHADER_STAGE_TASK_BIT_NV;
        native.tshader = vk_create_shader_module(info.tshader, info.tlength);
        if (!native.tshader) { vulkan.modules.erase(handle); return {}; }
    }
    native.mshader = vk_create_shader_module(info.mshader, info.mlength);
    if (!native.mshader)
    {
        result.native = &native;
        vk_destroy_module_native(handle, result.native);
        return {};
    }
    if (info.fshader)
    {
        native.fshader = vk_create_shader_module(info.fshader, info.flength);
        if (!native.fshader)
        {
            result.native = &native;
            vk_destroy_module_native(handle, result.native);
            return {};
        }
    }
    if (!vk_setup_descriptors(native, info.binding, native.shaderStages))
    {
        result.native = &native;
        vk_destroy_module_native(handle, result.native);
        return {};
    }
    VkPushConstantRange push = {};
    push.stageFlags = native.shaderStages;
    push.offset = 0;
    push.size = 128;
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &push;
    layoutInfo.setLayoutCount = native.descriptorSetLayout ? 1 : 0;
    layoutInfo.pSetLayouts = native.descriptorSetLayout ? &native.descriptorSetLayout : nullptr;
    if (vkCreatePipelineLayout(vulkan.device, &layoutInfo, vulkan.allocator, &native.pipelineLayout) != VK_SUCCESS ||
        !vk_create_graphics_pipeline(native, info, true))
    {
        result.native = &native;
        vk_destroy_module_native(handle, result.native);
        return {};
    }
    vulkan.moduleID = handle;
    result.handle = handle;
    result.native = &native;

    for (size_t i = 0; i < std::size(info.colors); ++i)
    {
        result.colors[i].color.func = info.colors[i].color.func;
        result.colors[i].color.src = info.colors[i].color.src;
        result.colors[i].color.dst = info.colors[i].color.dst;
        result.colors[i].alpha.func = info.colors[i].alpha.func;
        result.colors[i].alpha.src = info.colors[i].alpha.src;
        result.colors[i].alpha.dst = info.colors[i].alpha.dst;
    }
    result.depth.write = info.depth.write;
    result.depth.bias = info.depth.bias;
    result.depth.biasSlope = info.depth.biasSlope;
    result.depth.biasClamp = info.depth.biasClamp;
    result.depth.func = info.depth.func;
    result.stencil.read = info.stencil.read;
    result.stencil.write = info.stencil.write;
    result.stencil.back.func = info.stencil.back.func;
    result.stencil.back.sfail = info.stencil.back.sfail;
    result.stencil.back.zfail = info.stencil.back.zfail;
    result.stencil.back.zpass = info.stencil.back.zpass;
    result.stencil.front.func = info.stencil.front.func;
    result.stencil.front.sfail = info.stencil.front.sfail;
    result.stencil.front.zfail = info.stencil.front.zfail;
    result.stencil.front.zpass = info.stencil.front.zpass;
    result.index_type = info.index_type;
    for (size_t i = 0; i < std::size(info.vertex); ++i)
        result.vertex[i] = info.vertex[i];
    for (size_t i = 0; i < std::size(info.binding); ++i)
        result.binding[i] = info.binding[i];
    result.cull_mode = info.cull_mode;
    result.front_face = info.front_face;
    result.fill_mode = info.fill_mode;
    result.primitive = info.primitive;
    return result;
}

void vk_destroy_module_render(rt_module_render_t& module)
{
    vk_destroy_module_native(module.handle, module.native);
    module.handle = 0;
    module.vertex_vao = 0;
}

void vk_destroy_module_compute(rt_module_compute_t& module)
{
    vk_destroy_module_native(module.handle, module.native);
    module.handle = 0;
}

void vk_push_constant(uint8_t const* buffer, size_t length)
{
    vk_require_pass(vulkan.currentPassType);
    auto* mod = vk_current_module_native();
    if (!buffer || length == 0 || !mod || !mod->pipelineLayout) return;
    vkCmdPushConstants(vulkan.cmdBuffer, mod->pipelineLayout, mod->shaderStages, 0, (uint32_t)length, buffer);
}

void vk_push_const_int(const char*, int32_t) {}
void vk_push_const_uint(const char*, uint32_t) {}
void vk_push_const_float(const char*, float) {}
void vk_push_const_vec2(const char*, const float*) {}
void vk_push_const_vec3(const char*, const float*) {}
void vk_push_const_vec4(const char*, const float*) {}
void vk_push_const_mat3(const char*, const float*) {}
void vk_push_const_mat4(const char*, const float*) {}

void vk_begin_compute(rt_pass_compute_t& pass)
{
    if (pass.module.handle == 0 || !pass.module.native)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (vulkan.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    vk_clear_bindings();
    pass.handle = ++vulkan.passID;
    auto& native = vulkan.computePasses[pass.handle];
    native.bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    pass.native = &native;
    vulkan.currentPassType = GL_MODULE_COMPUTE;
    vulkan.currentComputePass = &pass;
    auto* mod = (rt_module_native_t*)pass.module.native;
    if (mod && mod->pipeline)
        vkCmdBindPipeline(vulkan.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mod->pipeline);
}

void vk_end_compute(rt_pass_compute_t& pass)
{
    if (vulkan.currentComputePass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vulkan.computePasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    vulkan.currentPassType = GL_NONE;
    vulkan.currentPipeline = nullptr;
    vk_clear_bindings();
}

void vk_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    vk_require_pass(GL_MODULE_COMPUTE);
    vk_flush_descriptors();
    vkCmdDispatch(vulkan.cmdBuffer, std::max(1u, groupX), std::max(1u, groupY), std::max(1u, groupZ));
}

void vk_begin_render(rt_pass_render_t& pass)
{
    if (pass.module.handle == 0 || !pass.module.native)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (vulkan.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    vk_clear_bindings();
    pass.handle = ++vulkan.passID;
    auto& native = vulkan.renderPasses[pass.handle];
    pass.native = &native;
    vulkan.currentPassType = GL_MODULE_RENDER;
    vulkan.currentRenderPass = &pass;

    auto* mod = (rt_module_native_t*)pass.module.native;
    if (mod && mod->pipeline)
        vkCmdBindPipeline(vulkan.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mod->pipeline);

    native.offscreen = pass.depth.texture.handle != 0;
    for (auto& color : pass.colors)
        if (color.texture.handle) native.offscreen = true;

    VkRenderingAttachmentInfo colorAttachments[GL_MAX_COLOR_TEXTURE_NUM] = {};
    uint32_t colorCount = 0;
    uint32_t width = 0, height = 0;
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        colorAttachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if (auto* tex = vk_texture_native(pass.colors[i].texture))
        {
            vk_transition_image(*tex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            colorAttachments[i].imageView = tex->imageView;
            colorAttachments[i].loadOp = pass.colors[i].clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            colorAttachments[i].clearValue.color = {{pass.colors[i].value.r, pass.colors[i].value.g, pass.colors[i].value.b, pass.colors[i].value.a}};
            width = std::max(width, pass.colors[i].texture.width);
            height = std::max(height, pass.colors[i].texture.height);
            colorCount = i + 1;
        }
    }

    VkRenderingAttachmentInfo depthAttachment = {};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    bool hasDepth = false;
    if (auto* tex = vk_texture_native(pass.depth.texture))
    {
        vk_transition_image(*tex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        depthAttachment.imageView = tex->imageView;
        depthAttachment.loadOp = pass.depth.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.clearValue.depthStencil.depth = pass.depth.value;
        depthAttachment.clearValue.depthStencil.stencil = (uint32_t)pass.stencil.value;
        width = std::max(width, pass.depth.texture.width);
        height = std::max(height, pass.depth.texture.height);
        hasDepth = true;
    }

    if (!native.offscreen)
    {
        native.width = native.height = 0;
        native.rendering = false;
        return;
    }

    native.width = width;
    native.height = height;
    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = {std::max(1u, width), std::max(1u, height)};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = std::max(colorCount, (uint32_t)GL_MAX_COLOR_TEXTURE_NUM);
    renderingInfo.pColorAttachments = colorAttachments;
    renderingInfo.pDepthAttachment = hasDepth ? &depthAttachment : nullptr;
    renderingInfo.pStencilAttachment = (hasDepth && pass.depth.texture.format == GL_DEPTH_STENCIL) ? &depthAttachment : nullptr;
    vkCmdBeginRendering(vulkan.cmdBuffer, &renderingInfo);
    native.rendering = true;
    vulkan.inRendering = true;
    vk_set_viewport(0, 0, (int32_t)width, (int32_t)height);
    vk_set_scissor(0, 0, (int32_t)width, (int32_t)height);
}

void vk_end_render(rt_pass_render_t& pass)
{
    if (vulkan.currentRenderPass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* native = (rt_pass_render_native_t*)pass.native;
    if (native && native->rendering)
    {
        vkCmdEndRendering(vulkan.cmdBuffer);
        for (auto& color : pass.colors)
            if (auto* tex = vk_texture_native(color.texture))
                vk_transition_image(*tex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        if (auto* tex = vk_texture_native(pass.depth.texture))
            vk_transition_image(*tex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        native->rendering = false;
        vulkan.inRendering = false;
    }
    vulkan.renderPasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    vulkan.currentPassType = GL_NONE;
    vulkan.currentPipeline = nullptr;
    vk_clear_bindings();
}

void vk_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    VkViewport viewport = {};
    viewport.x = (float)x;
    viewport.y = (float)y;
    viewport.width = (float)width;
    viewport.height = (float)height;
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
    scissor.extent = {(uint32_t)std::max(0, width), (uint32_t)std::max(0, height)};
    vkCmdSetScissor(vulkan.cmdBuffer, 0, 1, &scissor);
}

void vk_draw_mesh_task(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    vk_require_pass(GL_MODULE_RENDER);
    vk_flush_descriptors();
    if (vulkan.fnDrawMeshTasksNV)
        vulkan.fnDrawMeshTasksNV(vulkan.cmdBuffer, std::max(1u, groupX) * std::max(1u, groupY) * std::max(1u, groupZ), 0);
}

void vk_begin_transfer(rt_pass_transfer_t& pass)
{
    if (vulkan.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    pass.handle = ++vulkan.passID;
    auto& native = vulkan.transferPasses[pass.handle];
    pass.native = &native;
    vulkan.currentPassType = GL_MODULE_TRANSFER;
    vulkan.currentTransferPass = &pass;
}

void vk_end_transfer(rt_pass_transfer_t& pass)
{
    if (vulkan.currentTransferPass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vulkan.transferPasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    vulkan.currentPassType = GL_NONE;
    vulkan.currentPipeline = nullptr;
}

void vk_copy_buffer(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize)
{
    vk_require_pass(GL_MODULE_TRANSFER);
    auto* src = vk_buffer_native(source.buffer);
    auto* dst = vk_buffer_native(destination.buffer);
    if (!src || !dst || copySize == 0) return;
    if (source.offset + copySize > source.buffer.size || destination.offset + copySize > destination.buffer.size)
        return;
    vk_transition_buffer(*src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    vk_transition_buffer(*dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
    VkBufferCopy region = {source.offset, destination.offset, copySize};
    vkCmdCopyBuffer(vulkan.cmdBuffer, src->handle, dst->handle, 1, &region);
}

void vk_copy_buffer_data(rt_buffer_data_t source, rt_buffer_copy_t destination, size_t copySize)
{
    vk_require_pass(GL_MODULE_TRANSFER);
    auto* dst = vk_buffer_native(destination.buffer);
    if (!source.data || !dst || copySize == 0) return;
    if (source.offset + copySize > source.size || destination.offset + copySize > destination.buffer.size)
        return;
    if ((dst->memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && dst->memory)
    {
        void* mapped = nullptr;
        if (vkMapMemory(vulkan.device, dst->memory, destination.offset, copySize, 0, &mapped) == VK_SUCCESS)
        {
            std::memcpy(mapped, source.data + source.offset, copySize);
            vkUnmapMemory(vulkan.device, dst->memory);
            dst->stage = VK_PIPELINE_STAGE_HOST_BIT;
            dst->access = VK_ACCESS_HOST_WRITE_BIT;
            return;
        }
    }
    vk_staging_t staging = {};
    void* ptr = nullptr;
    if (!vk_create_staging(copySize, staging, &ptr)) return;
    std::memcpy(ptr, source.data + source.offset, copySize);
    vkUnmapMemory(vulkan.device, staging.memory);
    vk_transition_buffer(*dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
    VkBufferCopy region = {0, destination.offset, copySize};
    vkCmdCopyBuffer(vulkan.cmdBuffer, staging.buffer, dst->handle, 1, &region);
    vulkan.pendingStaging.push_back(staging);
}

void vk_copy_buffer_texture(rt_texture_copy_t source, rt_buffer_texel_t destination, rt_size_t copySize)
{
    vk_require_pass(GL_MODULE_TRANSFER);
    auto* src = vk_texture_native(source.texture);
    auto* dst = vk_buffer_native(destination.buffer);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    vk_transition_image(*src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vk_transition_buffer(*dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
    VkBufferImageCopy region = {};
    region.bufferOffset = destination.offset;
    region.bufferRowLength = destination.bytesPerRow ? destination.bytesPerRow / vk_format_bytes(src->format) : 0;
    region.bufferImageHeight = destination.rowsPerImage;
    region.imageSubresource.aspectMask = vk_image_aspect(source.aspect, source.texture.format);
    region.imageSubresource.mipLevel = source.mipLevel;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {(int32_t)source.origin.x, (int32_t)source.origin.y, (int32_t)source.origin.z};
    region.imageExtent = {copySize.x, copySize.y, copySize.z ? copySize.z : 1};
    vkCmdCopyImageToBuffer(vulkan.cmdBuffer, src->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->handle, 1, &region);
}

void vk_copy_texture(rt_texture_copy_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    vk_require_pass(GL_MODULE_TRANSFER);
    auto* src = vk_texture_native(source.texture);
    auto* dst = vk_texture_native(destination.texture);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    vk_transition_image(*src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vk_transition_image(*dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageCopy region = {};
    region.srcSubresource.aspectMask = vk_image_aspect(source.aspect, source.texture.format);
    region.srcSubresource.mipLevel = source.mipLevel;
    region.srcSubresource.layerCount = 1;
    region.srcOffset = {(int32_t)source.origin.x, (int32_t)source.origin.y, (int32_t)source.origin.z};
    region.dstSubresource.aspectMask = vk_image_aspect(destination.aspect, destination.texture.format);
    region.dstSubresource.mipLevel = destination.mipLevel;
    region.dstSubresource.layerCount = 1;
    region.dstOffset = {(int32_t)destination.origin.x, (int32_t)destination.origin.y, (int32_t)destination.origin.z};
    region.extent = {copySize.x, copySize.y, copySize.z ? copySize.z : 1};
    vkCmdCopyImage(vulkan.cmdBuffer, src->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dst->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void vk_copy_texture_data(rt_texture_data_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    vk_require_pass(GL_MODULE_TRANSFER);
    auto* dst = vk_texture_native(destination.texture);
    if (!source.data || !dst || copySize.x == 0 || copySize.y == 0) return;
    uint32_t bpp = vk_format_bytes(dst->format);
    VkDeviceSize bytes = source.size ? source.size : (VkDeviceSize)std::max(copySize.x * bpp, 1u) * copySize.y * std::max(1u, copySize.z);
    vk_staging_t staging = {};
    void* ptr = nullptr;
    if (!vk_create_staging(bytes, staging, &ptr)) return;
    std::memcpy(ptr, source.data + source.offset, (size_t)std::min(bytes, (VkDeviceSize)(source.size ? source.size - source.offset : bytes)));
    vkUnmapMemory(vulkan.device, staging.memory);
    vk_transition_image(*dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = source.bytesPerRow ? source.bytesPerRow / bpp : 0;
    region.bufferImageHeight = source.rowsPerImage;
    region.imageSubresource.aspectMask = vk_image_aspect(destination.aspect, destination.texture.format);
    region.imageSubresource.mipLevel = destination.mipLevel;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {(int32_t)destination.origin.x, (int32_t)destination.origin.y, (int32_t)destination.origin.z};
    region.imageExtent = {copySize.x, copySize.y, copySize.z ? copySize.z : 1};
    vkCmdCopyBufferToImage(vulkan.cmdBuffer, staging.buffer, dst->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    vulkan.pendingStaging.push_back(staging);
}

void vk_copy_texture_buffer(rt_buffer_texel_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    vk_require_pass(GL_MODULE_TRANSFER);
    auto* src = vk_buffer_native(source.buffer);
    auto* dst = vk_texture_native(destination.texture);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    vk_transition_image(*dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vk_transition_buffer(*src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    VkBufferImageCopy region = {};
    region.bufferOffset = source.offset;
    region.bufferRowLength = source.bytesPerRow ? source.bytesPerRow / vk_format_bytes(dst->format) : 0;
    region.bufferImageHeight = source.rowsPerImage;
    region.imageSubresource.aspectMask = vk_image_aspect(destination.aspect, destination.texture.format);
    region.imageSubresource.mipLevel = destination.mipLevel;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {(int32_t)destination.origin.x, (int32_t)destination.origin.y, (int32_t)destination.origin.z};
    region.imageExtent = {copySize.x, copySize.y, copySize.z ? copySize.z : 1};
    vkCmdCopyBufferToImage(vulkan.cmdBuffer, src->handle, dst->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

rt_mesh_t vk_create_mesh(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    rt_mesh_t result = {};
    if (!vertices || vertex_count == 0) return result;
    uint32_t handle = vulkan.meshID + 1;
    auto& native = vulkan.meshes[handle];
    native.vertexCount = (uint32_t)vertex_count;
    native.indexCount = (uint32_t)index_count;
    if (vertices)
        result.vertex[0] = vk_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = vk_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = vk_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = vk_create_buffer({.size = index_count * sizeof(uint32_t), .usage = GL_BUFFER_USAGE_INDEX | GL_BUFFER_USAGE_COPY_DST, .data = indices});
    std::iota(result.location, result.location + std::size(result.location), 0);
    vulkan.meshID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void vk_destroy_mesh(rt_mesh_t& mesh)
{
    for (auto& vertex : mesh.vertex)
        vk_destroy_buffer(vertex);
    vk_destroy_buffer(mesh.index);
    vulkan.meshes.erase(mesh.handle);
    mesh.handle = 0;
    mesh.native = nullptr;
}

void vk_draw_mesh(rt_mesh_t& mesh)
{
    vk_require_pass(GL_MODULE_RENDER);
    vk_flush_descriptors();
    rt_module_render_t const& module = vulkan.currentRenderPass->module;
    uint32_t vertex_count = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.type == GL_NONE || layout.count == 0) continue;
        for (uint32_t k = 0; k < std::size(mesh.vertex); ++k)
        {
            if (mesh.vertex[k].handle == 0 || mesh.location[k] != layout.location) continue;
            auto* native = vk_buffer_native(mesh.vertex[k]);
            if (!native) break;
            vk_transition_buffer(*native, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(vulkan.cmdBuffer, layout.location, 1, &native->handle, &offset);
            uint32_t stride = gl_to_vk_vertex_size(layout.type, layout.count);
            if (vertex_count == 0 && stride)
                vertex_count = (uint32_t)(mesh.vertex[k].size / stride);
            break;
        }
    }
    if (mesh.index.handle)
    {
        auto* native = vk_buffer_native(mesh.index);
        if (!native) return;
        uint32_t indexStride = gl_index_type_size(module.index_type);
        vk_transition_buffer(*native, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT);
        vkCmdBindIndexBuffer(vulkan.cmdBuffer, native->handle, 0, gl_to_vk_index_type(module.index_type));
        vkCmdDrawIndexed(vulkan.cmdBuffer, (uint32_t)(mesh.index.size / indexStride), 1, 0, 0, 0);
    }
    else
        vkCmdDraw(vulkan.cmdBuffer, vertex_count, 1, 0, 0);
}

rt_meshlet_t vk_create_meshlet(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    rt_meshlet_t result = {};
    if (!vertices || vertex_count == 0) return result;
    uint32_t handle = vulkan.meshletID + 1;
    auto& native = vulkan.meshlets[handle];
    native.vertexCount = (uint32_t)vertex_count;
    native.indexCount = (uint32_t)index_count;
    if (vertices)
        result.vertex[0] = vk_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = vk_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = vk_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = vk_create_buffer({.size = index_count * sizeof(uint32_t), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = indices});
    std::iota(result.location, result.location + std::size(result.location), 0);
    vulkan.meshletID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void vk_destroy_meshlet(rt_meshlet_t& meshlet)
{
    for (auto& vertex : meshlet.vertex)
        vk_destroy_buffer(vertex);
    vk_destroy_buffer(meshlet.index);
    vulkan.meshlets.erase(meshlet.handle);
    meshlet.handle = 0;
    meshlet.native = nullptr;
}

void vk_draw_meshlet(rt_meshlet_t& meshlet)
{
    vk_require_pass(GL_MODULE_RENDER);
    rt_module_render_t const& module = vulkan.currentRenderPass->module;
    uint32_t index_binding = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.type == GL_NONE || layout.count == 0) continue;
        for (uint32_t k = 0; k < std::size(meshlet.vertex); ++k)
        {
            if (meshlet.vertex[k].handle == 0 || meshlet.location[k] != layout.location) continue;
            vk_bind_buffer(meshlet.vertex[k], {.binding = layout.location, .target = GL_SHADER_STORAGE_BUFFER});
            break;
        }
        if (layout.location + 1 > index_binding)
            index_binding = layout.location + 1;
    }
    if (meshlet.index.handle)
        vk_bind_buffer(meshlet.index, {.binding = index_binding, .target = GL_SHADER_STORAGE_BUFFER});
    vk_flush_descriptors();
    auto* native = (rt_meshlet_native_t*)meshlet.native;
    uint32_t tasks = native && native->indexCount ? native->indexCount / 3 : 1;
    if (vulkan.fnDrawMeshTasksNV)
        vulkan.fnDrawMeshTasksNV(vulkan.cmdBuffer, std::max(1u, tasks), 0);
}

rt_mesh_t vk_create_mesh_screen()
{
    const float points[]
    {
        -1.0f, -1.0f, 0.0f,
        +3.0f, -1.0f, 0.0f,
        -1.0f, +3.0f, 0.0f,
    };
    const float uvs[]
    {
        0.0f, 0.0f,
        2.0f, 0.0f,
        0.0f, 2.0f,
    };
    return vk_create_mesh(points, nullptr, uvs, 3, nullptr, 0);
}

void vk_draw_screen(int width, int height, rt_color_t clear, rt_texture_t& texture)
{
    static auto module = vk_create_module_render({
        .vertex = {rt_vertex_vertex, {}, rt_vertex_uv},
        .binding = {{.binding = 0, .type = GL_BINDING_TEXTURE}},
    });
    if (!module.handle) return;

    rt_pass_render_t pass = {.module = module, .screen = {.color = {.clear = true, .value = clear}}};
    vk_begin_render(pass);
    vk_set_viewport(0, 0, width, height);
    vk_bind_texture(texture, {.binding = 0});
    static auto mesh = vk_create_mesh_screen();
    vk_draw_mesh(mesh);
    vk_end_render(pass);
}

void vk_submit()
{
    if (!vulkan.cmdBuffer)
        return;
    if (vulkan.inRendering)
    {
        vkCmdEndRendering(vulkan.cmdBuffer);
        vulkan.inRendering = false;
    }
    if (vkEndCommandBuffer(vulkan.cmdBuffer) != VK_SUCCESS)
    {
        fprintf(stderr, "Vulkan: failed to end command buffer\n");
        abort();
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkan.cmdBuffer;
    if (vulkan.queue)
    {
        vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(vulkan.queue);
    }
    vk_flush_staging();
    vkResetCommandBuffer(vulkan.cmdBuffer, 0);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo) != VK_SUCCESS)
    {
        fprintf(stderr, "Vulkan: failed to begin command buffer\n");
        abort();
    }
}

#endif
