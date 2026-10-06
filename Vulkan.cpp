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

static VkFilter rt_to_vk_filter(rt_filter_t filter)
{
    switch (filter)
    {
        case RT_NEAREST: return VK_FILTER_NEAREST;
        case RT_LINEAR: return VK_FILTER_LINEAR;
        case RT_NEAREST_MIPMAP_NEAREST: return VK_FILTER_LINEAR;
        case RT_LINEAR_MIPMAP_NEAREST: return VK_FILTER_LINEAR;
        case RT_NEAREST_MIPMAP_LINEAR: return VK_FILTER_LINEAR;
        case RT_LINEAR_MIPMAP_LINEAR: return VK_FILTER_LINEAR;
        default: return VK_FILTER_LINEAR;
    }
}

static VkFilter rt_to_vk_min_filter(rt_filter_t minFilter)
{
    switch (minFilter)
    {
        case RT_NEAREST: return VK_FILTER_NEAREST;
        case RT_LINEAR: return VK_FILTER_LINEAR;
        case RT_NEAREST_MIPMAP_NEAREST: return VK_FILTER_NEAREST;
        case RT_LINEAR_MIPMAP_NEAREST: return VK_FILTER_LINEAR;
        case RT_NEAREST_MIPMAP_LINEAR: return VK_FILTER_NEAREST;
        case RT_LINEAR_MIPMAP_LINEAR: return VK_FILTER_LINEAR;
        default: return VK_FILTER_LINEAR;
    }
}

static VkSamplerAddressMode rt_to_vk_address(rt_address_t address)
{
    switch (address)
    {
        case RT_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case RT_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case RT_CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case RT_MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case RT_MIRROR_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        default: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }
}

static VkSamplerMipmapMode rt_to_vk_mipmap(rt_filter_t minFilter)
{
    switch (minFilter)
    {
        case RT_NEAREST: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        case RT_LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        case RT_NEAREST_MIPMAP_NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case RT_LINEAR_MIPMAP_NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case RT_NEAREST_MIPMAP_LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        case RT_LINEAR_MIPMAP_LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        default: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

static bool rt_has_mipmap_filter(rt_filter_t minFilter)
{
    switch (minFilter)
    {
        case RT_NEAREST: return false;
        case RT_LINEAR: return false;
        case RT_NEAREST_MIPMAP_NEAREST: return true;
        case RT_LINEAR_MIPMAP_NEAREST: return true;
        case RT_NEAREST_MIPMAP_LINEAR: return true;
        case RT_LINEAR_MIPMAP_LINEAR: return true;
        default: return false;
    }
}

static VkFormat rt_to_vk_texture_format(rt_texture_format_t format)
{
    switch (format)
    {
        case RT_TEXTURE_NONE: return VK_FORMAT_UNDEFINED;
        case RT_TEXTURE_R8UNORM: return VK_FORMAT_R8_UNORM;
        case RT_TEXTURE_R8SNORM: return VK_FORMAT_R8_SNORM;
        case RT_TEXTURE_R8UINT: return VK_FORMAT_R8_UINT;
        case RT_TEXTURE_R8SINT: return VK_FORMAT_R8_SINT;
        case RT_TEXTURE_R16UNORM: return VK_FORMAT_R16_UNORM;
        case RT_TEXTURE_R16SNORM: return VK_FORMAT_R16_SNORM;
        case RT_TEXTURE_R16UINT: return VK_FORMAT_R16_UINT;
        case RT_TEXTURE_R16SINT: return VK_FORMAT_R16_SINT;
        case RT_TEXTURE_R16FLOAT: return VK_FORMAT_R16_SFLOAT;
        case RT_TEXTURE_RG8UNORM: return VK_FORMAT_R8G8_UNORM;
        case RT_TEXTURE_RG8SNORM: return VK_FORMAT_R8G8_SNORM;
        case RT_TEXTURE_RG8UINT: return VK_FORMAT_R8G8_UINT;
        case RT_TEXTURE_RG8SINT: return VK_FORMAT_R8G8_SINT;
        case RT_TEXTURE_R32UINT: return VK_FORMAT_R32_UINT;
        case RT_TEXTURE_R32SINT: return VK_FORMAT_R32_SINT;
        case RT_TEXTURE_R32FLOAT: return VK_FORMAT_R32_SFLOAT;
        case RT_TEXTURE_RG16UNORM: return VK_FORMAT_R16G16_UNORM;
        case RT_TEXTURE_RG16SNORM: return VK_FORMAT_R16G16_SNORM;
        case RT_TEXTURE_RG16UINT: return VK_FORMAT_R16G16_UINT;
        case RT_TEXTURE_RG16SINT: return VK_FORMAT_R16G16_SINT;
        case RT_TEXTURE_RG16FLOAT: return VK_FORMAT_R16G16_SFLOAT;
        case RT_TEXTURE_RGBA8UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case RT_TEXTURE_RGBA8UNORM_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
        case RT_TEXTURE_RGBA8SNORM: return VK_FORMAT_R8G8B8A8_SNORM;
        case RT_TEXTURE_RGBA8UINT: return VK_FORMAT_R8G8B8A8_UINT;
        case RT_TEXTURE_RGBA8SINT: return VK_FORMAT_R8G8B8A8_SINT;
        case RT_TEXTURE_BGRA8UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
        case RT_TEXTURE_BGRA8UNORM_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
        case RT_TEXTURE_RGB10A2UINT: return VK_FORMAT_A2B10G10R10_UINT_PACK32;
        case RT_TEXTURE_RGB10A2UNORM: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case RT_TEXTURE_RG11B10UFLOAT: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case RT_TEXTURE_RGB9E5UFLOAT: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
        case RT_TEXTURE_RG32UINT: return VK_FORMAT_R32G32_UINT;
        case RT_TEXTURE_RG32SINT: return VK_FORMAT_R32G32_SINT;
        case RT_TEXTURE_RG32FLOAT: return VK_FORMAT_R32G32_SFLOAT;
        case RT_TEXTURE_RGBA16UNORM: return VK_FORMAT_R16G16B16A16_UNORM;
        case RT_TEXTURE_RGBA16SNORM: return VK_FORMAT_R16G16B16A16_SNORM;
        case RT_TEXTURE_RGBA16UINT: return VK_FORMAT_R16G16B16A16_UINT;
        case RT_TEXTURE_RGBA16SINT: return VK_FORMAT_R16G16B16A16_SINT;
        case RT_TEXTURE_RGBA16FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case RT_TEXTURE_RGBA32UINT: return VK_FORMAT_R32G32B32A32_UINT;
        case RT_TEXTURE_RGBA32SINT: return VK_FORMAT_R32G32B32A32_SINT;
        case RT_TEXTURE_RGBA32FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case RT_TEXTURE_STENCIL8: return VK_FORMAT_S8_UINT;
        case RT_TEXTURE_DEPTH16UNORM: return VK_FORMAT_D16_UNORM;
        case RT_TEXTURE_DEPTH24PLUS: return VK_FORMAT_X8_D24_UNORM_PACK32;
        case RT_TEXTURE_DEPTH24PLUS_STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
        case RT_TEXTURE_DEPTH32FLOAT: return VK_FORMAT_D32_SFLOAT;
        case RT_TEXTURE_DEPTH32FLOAT_STENCIL8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
        default: return VK_FORMAT_UNDEFINED;
    }
}

static VkSampleCountFlagBits rt_to_vk_sample_count(rt_texture_sample_t samples)
{
    switch (samples)
    {
        case RT_TEXTURE_SAMPLE_1X: return VK_SAMPLE_COUNT_1_BIT;
        case RT_TEXTURE_SAMPLE_4X: return VK_SAMPLE_COUNT_4_BIT;
        default: return VK_SAMPLE_COUNT_1_BIT;
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

static VkImageUsageFlags rt_to_vk_image_usage(rt_texture_usages_t usage, rt_texture_format_t format)
{
    VkImageUsageFlags flags = 0;
    bool depthStencil = rt_texture_has_depth(format) || rt_texture_has_stencil(format);
    if (usage & RT_TEXTURE_USAGE_COPY_SRC) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (usage & RT_TEXTURE_USAGE_COPY_DST) flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (usage & RT_TEXTURE_USAGE_TEXTURE_BINDING) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if ((usage & RT_TEXTURE_USAGE_STORAGE_BINDING) && !depthStencil) flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (usage & RT_TEXTURE_USAGE_RENDER_ATTACHMENT)
        flags |= depthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    return flags;
}

static VkBufferUsageFlags rt_to_vk_buffer_usage(uint32_t usage)
{
    VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (usage & RT_BUFFER_USAGE_INDEX) flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (usage & RT_BUFFER_USAGE_VERTEX) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (usage & RT_BUFFER_USAGE_UNIFORM) flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (usage & RT_BUFFER_USAGE_STORAGE) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (usage & RT_BUFFER_USAGE_INDIRECT) flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    return flags;
}

static VkFormat rt_to_vk_vertex_format(rt_vertex_format_t format)
{
    switch (format)
    {
        case RT_VERTEX_NONE: return VK_FORMAT_UNDEFINED;
        case RT_VERTEX_UINT8: return VK_FORMAT_R8_UINT;
        case RT_VERTEX_UINT8X2: return VK_FORMAT_R8G8_UINT;
        case RT_VERTEX_UINT8X4: return VK_FORMAT_R8G8B8A8_UINT;
        case RT_VERTEX_SINT8: return VK_FORMAT_R8_SINT;
        case RT_VERTEX_SINT8X2: return VK_FORMAT_R8G8_SINT;
        case RT_VERTEX_SINT8X4: return VK_FORMAT_R8G8B8A8_SINT;
        case RT_VERTEX_UNORM8: return VK_FORMAT_R8_UNORM;
        case RT_VERTEX_UNORM8X2: return VK_FORMAT_R8G8_UNORM;
        case RT_VERTEX_UNORM8X4: return VK_FORMAT_R8G8B8A8_UNORM;
        case RT_VERTEX_SNORM8: return VK_FORMAT_R8_SNORM;
        case RT_VERTEX_SNORM8X2: return VK_FORMAT_R8G8_SNORM;
        case RT_VERTEX_SNORM8X4: return VK_FORMAT_R8G8B8A8_SNORM;
        case RT_VERTEX_UINT16: return VK_FORMAT_R16_UINT;
        case RT_VERTEX_UINT16X2: return VK_FORMAT_R16G16_UINT;
        case RT_VERTEX_UINT16X4: return VK_FORMAT_R16G16B16A16_UINT;
        case RT_VERTEX_SINT16: return VK_FORMAT_R16_SINT;
        case RT_VERTEX_SINT16X2: return VK_FORMAT_R16G16_SINT;
        case RT_VERTEX_SINT16X4: return VK_FORMAT_R16G16B16A16_SINT;
        case RT_VERTEX_UNORM16: return VK_FORMAT_R16_UNORM;
        case RT_VERTEX_UNORM16X2: return VK_FORMAT_R16G16_UNORM;
        case RT_VERTEX_UNORM16X4: return VK_FORMAT_R16G16B16A16_UNORM;
        case RT_VERTEX_SNORM16: return VK_FORMAT_R16_SNORM;
        case RT_VERTEX_SNORM16X2: return VK_FORMAT_R16G16_SNORM;
        case RT_VERTEX_SNORM16X4: return VK_FORMAT_R16G16B16A16_SNORM;
        case RT_VERTEX_FLOAT16: return VK_FORMAT_R16_SFLOAT;
        case RT_VERTEX_FLOAT16X2: return VK_FORMAT_R16G16_SFLOAT;
        case RT_VERTEX_FLOAT16X4: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case RT_VERTEX_FLOAT32: return VK_FORMAT_R32_SFLOAT;
        case RT_VERTEX_FLOAT32X2: return VK_FORMAT_R32G32_SFLOAT;
        case RT_VERTEX_FLOAT32X3: return VK_FORMAT_R32G32B32_SFLOAT;
        case RT_VERTEX_FLOAT32X4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case RT_VERTEX_UINT32: return VK_FORMAT_R32_UINT;
        case RT_VERTEX_UINT32X2: return VK_FORMAT_R32G32_UINT;
        case RT_VERTEX_UINT32X3: return VK_FORMAT_R32G32B32_UINT;
        case RT_VERTEX_UINT32X4: return VK_FORMAT_R32G32B32A32_UINT;
        case RT_VERTEX_SINT32: return VK_FORMAT_R32_SINT;
        case RT_VERTEX_SINT32X2: return VK_FORMAT_R32G32_SINT;
        case RT_VERTEX_SINT32X3: return VK_FORMAT_R32G32B32_SINT;
        case RT_VERTEX_SINT32X4: return VK_FORMAT_R32G32B32A32_SINT;
        default: return VK_FORMAT_UNDEFINED;
    }
}

static VkDescriptorType rt_to_vk_descriptor(rt_binding_type_t bindingType)
{
    switch (bindingType)
    {
        case RT_BINDING_NONE: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case RT_BINDING_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case RT_BINDING_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case RT_BINDING_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
        case RT_BINDING_TEXTURE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case RT_BINDING_STORAGE_TEXTURE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        default: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
}

static VkCompareOp rt_to_vk_compare(rt_compare_op_t func)
{
    switch (func)
    {
        case RT_NEVER: return VK_COMPARE_OP_NEVER;
        case RT_LESS: return VK_COMPARE_OP_LESS;
        case RT_EQUAL: return VK_COMPARE_OP_EQUAL;
        case RT_LEQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
        case RT_GREATER: return VK_COMPARE_OP_GREATER;
        case RT_NOTEQUAL: return VK_COMPARE_OP_NOT_EQUAL;
        case RT_GEQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case RT_ALWAYS: return VK_COMPARE_OP_ALWAYS;
        default: return VK_COMPARE_OP_ALWAYS;
    }
}

static VkBlendFactor rt_to_vk_blend_factor(rt_blend_factor_t factor)
{
    switch (factor)
    {
        case RT_BLEND_ZERO: return VK_BLEND_FACTOR_ZERO;
        case RT_BLEND_ONE: return VK_BLEND_FACTOR_ONE;
        case RT_BLEND_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
        case RT_BLEND_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case RT_BLEND_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
        case RT_BLEND_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case RT_BLEND_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
        case RT_BLEND_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case RT_BLEND_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
        case RT_BLEND_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case RT_BLEND_SRC_ALPHA_SATURATE: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case RT_BLEND_CONSTANT_COLOR: return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case RT_BLEND_ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case RT_BLEND_CONSTANT_ALPHA: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case RT_BLEND_ONE_MINUS_CONSTANT_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        default: return VK_BLEND_FACTOR_ONE;
    }
}

static VkBlendOp rt_to_vk_blend_op(rt_blend_op_t func)
{
    switch (func)
    {
        case RT_FUNC_ADD: return VK_BLEND_OP_ADD;
        case RT_MIN: return VK_BLEND_OP_MIN;
        case RT_MAX: return VK_BLEND_OP_MAX;
        case RT_FUNC_SUBTRACT: return VK_BLEND_OP_SUBTRACT;
        case RT_FUNC_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
        default: return VK_BLEND_OP_ADD;
    }
}

static VkCullModeFlags rt_to_vk_cull(rt_cull_mode_t mode)
{
    switch (mode)
    {
        case RT_CULL_NONE: return VK_CULL_MODE_NONE;
        case RT_CULL_FRONT: return VK_CULL_MODE_FRONT_BIT;
        case RT_CULL_BACK: return VK_CULL_MODE_BACK_BIT;
        case RT_CULL_FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
        default: return VK_CULL_MODE_NONE;
    }
}

static VkFrontFace rt_to_vk_wind_mode(rt_wind_mode_t face)
{
    switch (face)
    {
        case RT_CW: return VK_FRONT_FACE_CLOCKWISE;
        case RT_CCW: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        default: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
}

static VkPolygonMode rt_to_vk_fill(rt_fill_mode_t fill)
{
    switch (fill)
    {
        case RT_POINT: return VK_POLYGON_MODE_POINT;
        case RT_LINE: return VK_POLYGON_MODE_LINE;
        case RT_FILL: return VK_POLYGON_MODE_FILL;
        default: return VK_POLYGON_MODE_FILL;
    }
}

static VkPrimitiveTopology rt_to_vk_primitive(rt_primitive_t primitive)
{
    switch (primitive)
    {
        case RT_POINTS: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case RT_LINES: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case RT_LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case RT_TRIANGLES: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case RT_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

static VkStencilOp rt_to_vk_stencil_op(rt_stencil_op_t op)
{
    switch (op)
    {
        case RT_STENCIL_ZERO: return VK_STENCIL_OP_ZERO;
        case RT_STENCIL_INVERT: return VK_STENCIL_OP_INVERT;
        case RT_STENCIL_KEEP: return VK_STENCIL_OP_KEEP;
        case RT_STENCIL_REPLACE: return VK_STENCIL_OP_REPLACE;
        case RT_STENCIL_INCR: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case RT_STENCIL_DECR: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case RT_STENCIL_INCR_WRAP: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case RT_STENCIL_DECR_WRAP: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        default: return VK_STENCIL_OP_KEEP;
    }
}

static uint32_t rt_to_vk_vertex_size(rt_vertex_format_t format)
{
    switch (format)
    {
        case RT_VERTEX_NONE: return 0;
        case RT_VERTEX_UINT8: return 1;
        case RT_VERTEX_UINT8X2: return 2;
        case RT_VERTEX_UINT8X4: return 4;
        case RT_VERTEX_SINT8: return 1;
        case RT_VERTEX_SINT8X2: return 2;
        case RT_VERTEX_SINT8X4: return 4;
        case RT_VERTEX_UNORM8: return 1;
        case RT_VERTEX_UNORM8X2: return 2;
        case RT_VERTEX_UNORM8X4: return 4;
        case RT_VERTEX_SNORM8: return 1;
        case RT_VERTEX_SNORM8X2: return 2;
        case RT_VERTEX_SNORM8X4: return 4;
        case RT_VERTEX_UINT16: return 2;
        case RT_VERTEX_UINT16X2: return 4;
        case RT_VERTEX_UINT16X4: return 8;
        case RT_VERTEX_SINT16: return 2;
        case RT_VERTEX_SINT16X2: return 4;
        case RT_VERTEX_SINT16X4: return 8;
        case RT_VERTEX_UNORM16: return 2;
        case RT_VERTEX_UNORM16X2: return 4;
        case RT_VERTEX_UNORM16X4: return 8;
        case RT_VERTEX_SNORM16: return 2;
        case RT_VERTEX_SNORM16X2: return 4;
        case RT_VERTEX_SNORM16X4: return 8;
        case RT_VERTEX_FLOAT16: return 2;
        case RT_VERTEX_FLOAT16X2: return 4;
        case RT_VERTEX_FLOAT16X4: return 8;
        case RT_VERTEX_FLOAT32: return 4;
        case RT_VERTEX_FLOAT32X2: return 8;
        case RT_VERTEX_FLOAT32X3: return 12;
        case RT_VERTEX_FLOAT32X4: return 16;
        case RT_VERTEX_UINT32: return 4;
        case RT_VERTEX_UINT32X2: return 8;
        case RT_VERTEX_UINT32X3: return 12;
        case RT_VERTEX_UINT32X4: return 16;
        case RT_VERTEX_SINT32: return 4;
        case RT_VERTEX_SINT32X2: return 8;
        case RT_VERTEX_SINT32X3: return 12;
        case RT_VERTEX_SINT32X4: return 16;
        default: return 0;
    }
}

static uint32_t rt_to_vk_vertex_stride(rt_vertex_t const& layout)
{
    // stride != 0: use it as-is; stride == 0: tight-pack from the attributes
    if (layout.stride)
        return layout.stride;
    uint32_t stride = 0;
    for (auto& attrib : layout.attrib)
    {
        if (attrib.format == RT_VERTEX_NONE)
            continue;
        uint32_t end = attrib.offset + rt_to_vk_vertex_size(attrib.format);
        if (end > stride)
            stride = end;
    }
    return stride;
}

static uint32_t rt_to_vk_index_size(rt_index_type_t type)
{
    switch (type)
    {
        case RT_INDEX_UINT16: return 2;
        case RT_INDEX_UINT32: return 4;
        default: return 4;
    }
}

static VkIndexType rt_to_vk_index_type(rt_index_type_t type)
{
    switch (type)
    {
        case RT_INDEX_UINT16: return VK_INDEX_TYPE_UINT16;
        case RT_INDEX_UINT32: return VK_INDEX_TYPE_UINT32;
        default: return VK_INDEX_TYPE_UINT32;
    }
}

static uint32_t vk_format_bytes(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
            return 1;
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
            return 2;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
            return 4;
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
            return 4;
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
            return 2;
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
            return 4;
        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 8;
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            return 8;
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return 4;
        case VK_FORMAT_D16_UNORM:
            return 2;
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:
            return 4;
        case VK_FORMAT_S8_UINT:
            return 1;
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return 4;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return 8;
        default: return 4;
    }
}

static VkImageAspectFlags rt_to_vk_aspect(rt_texture_aspect_t aspect, rt_texture_format_t format)
{
    switch (aspect)
    {
        case RT_TEXTURE_ASPECT_ALL:
            if (rt_texture_has_depth(format) && rt_texture_has_stencil(format))
                return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            if (rt_texture_has_stencil(format))
                return VK_IMAGE_ASPECT_STENCIL_BIT;
            if (rt_texture_has_depth(format))
                return VK_IMAGE_ASPECT_DEPTH_BIT;
            return VK_IMAGE_ASPECT_COLOR_BIT;
        case RT_TEXTURE_ASPECT_STENCIL:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        case RT_TEXTURE_ASPECT_DEPTH:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        default:
            return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

// ====================================================================

struct vk_buffer_native_t
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

struct vk_texture_native_t
{
    VkImage handle = nullptr;
    VkDeviceMemory memory = nullptr;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t levels = 1;
    uint32_t layers = 1;
    VkExtent3D extent = {1, 1, 1};
    rt_texture_target_t target = RT_TEXTURE_2D;
    rt_texture_format_t rtFormat = RT_TEXTURE_NONE;
    rt_texture_usages_t usage = 0;
    rt_texture_sample_t samples = RT_TEXTURE_SAMPLE_1X;
    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags access = 0;
};

struct vk_texture_view_native_t
{
    VkImageView handle = nullptr;
    vk_texture_native_t* texture = nullptr;
};

struct vk_sampler_native_t
{
    VkSampler handle = nullptr;
};

struct vk_module_native_t
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

    VkDescriptorType descriptorTypes[RT_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t descriptorBindings[RT_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t descriptorCount = 0;
};

struct vk_mesh_native_t
{
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

struct vk_meshlet_native_t
{
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

struct vk_pass_compute_native_t
{
    VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
};

struct vk_pass_render_native_t
{
    bool rendering = false;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t colorCount = 0;
    bool hasDepth = false;
    bool hasStencil = false;
    VkRenderingAttachmentInfo colorAttachments[RT_MAX_COLOR_TEXTURE_NUM] = {};
    VkRenderingAttachmentInfo depthAttachment = {};
};

struct vk_pass_transfer_native_t
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
    uint32_t textureViewID = 0;
    uint32_t samplerID = 0;
    uint32_t moduleID = 0;
    uint32_t meshID = 0;
    uint32_t meshletID = 0;
    uint32_t passID = 0;

    std::map<uint32_t, vk_buffer_native_t> buffers;
    std::map<uint32_t, vk_texture_native_t> textures;
    std::map<uint32_t, vk_texture_view_native_t> textureViews;
    std::map<uint32_t, vk_sampler_native_t> samplers;
    std::map<uint32_t, vk_module_native_t> modules;
    std::map<uint32_t, vk_mesh_native_t> meshes;
    std::map<uint32_t, vk_meshlet_native_t> meshlets;
    std::map<uint32_t, vk_pass_compute_native_t> computePasses;
    std::map<uint32_t, vk_pass_transfer_native_t> transferPasses;
    vk_pass_render_native_t renderPass = {};

    VkInstance instance = nullptr;
    VkPhysicalDevice physicalDevice = nullptr;
    VkPhysicalDeviceMemoryProperties memProps = {};
    VkDevice device = nullptr;
    VkQueue queue = nullptr;
    uint32_t queueFamily = 0;
    VkAllocationCallbacks* allocator = nullptr;
    VkCommandBuffer cmdBuffer = nullptr;
    VkDescriptorPool descriptorPool = nullptr;
    VkPipelineCache pipelineCache = nullptr;
    std::vector<vk_staging_t> pendingStaging;

    PFN_vkCmdDrawMeshTasksNV fnDrawMeshTasksNV = nullptr;
    PFN_vkCmdDrawMeshTasksIndirectEXT fnDrawMeshTasksIndirectEXT = nullptr;

    struct
    {
        rt_binding_type_t type = RT_BINDING_NONE;
        union
        {
            struct
            {
                rt_buffer_t buffer;
                rt_buffer_bind_t buffer_bind;
            };
            struct
            {
                rt_texture_view_t texture_view;
                rt_texture_bind_t texture_bind;
            };
            struct
            {
                rt_texture_view_t storage_view;
                rt_texture_storage_bind_t storage_texture_bind;
            };
            struct
            {
                rt_sampler_t sampler;
                rt_sampler_bind_t sampler_bind;
            };
        };
    } currentBinding[RT_MAX_BINDING_HANDLE_NUM] = {};

    rt_module_type_t currentPassType = RT_MODULE_NONE;
    union
    {
        void* currentPipeline = nullptr;
        rt_pass_render_t* currentRenderPass;
        rt_pass_compute_t* currentComputePass;
        rt_pass_transfer_t* currentTransferPass;
    };
    rt_module_render_t* currentRenderModule = nullptr;
    rt_module_compute_t* currentComputeModule = nullptr;

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
    fprintf(stderr, "Vulkan: no compatible memory type");
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

static void vk_suspend_rendering()
{
    if (vulkan.currentPassType != RT_MODULE_RENDER || !vulkan.currentRenderPass || !vulkan.currentRenderPass->native)
        return;
    auto* pass = (vk_pass_render_native_t*)vulkan.currentRenderPass->native;
    if (!pass->rendering)
        return;
    vkCmdEndRendering(vulkan.cmdBuffer);
    pass->rendering = false;
    for (uint32_t i = 0; i < pass->colorCount; ++i)
    {
        if (pass->colorAttachments[i].imageView)
            pass->colorAttachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    }
    if (pass->hasDepth)
        pass->depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
}

static void vk_transition_image(vk_texture_native_t& image, VkImageLayout newLayout)
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
        vk_suspend_rendering();
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
        barrier.subresourceRange.levelCount = image.levels;
        barrier.subresourceRange.layerCount = image.layers;
        vkCmdPipelineBarrier(vulkan.cmdBuffer, image.stage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        image.layout = newLayout;
        image.stage = dstStage;
        image.access = dstAccess;
    }
}

static void vk_transition_buffer(vk_buffer_native_t& buffer, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess)
{
    if (!buffer.handle)
        return;
    if (buffer.stage == dstStage && buffer.access == dstAccess)
        return;

    vk_suspend_rendering();
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

static vk_buffer_native_t* vk_buffer_native(rt_buffer_t const& buffer)
{
    if (!buffer.native || buffer.handle == 0) return nullptr;
    return (vk_buffer_native_t*)buffer.native;
}

static vk_texture_native_t* vk_texture_native(rt_texture_t const& texture)
{
    if (!texture.native || texture.handle == 0) return nullptr;
    return (vk_texture_native_t*)texture.native;
}

static vk_texture_view_native_t* vk_texture_view_native(rt_texture_view_t const& view)
{
    if (!view.native || view.handle == 0) return nullptr;
    return (vk_texture_view_native_t*)view.native;
}

static void vk_begin_rendering()
{
    if (vulkan.currentPassType != RT_MODULE_RENDER || !vulkan.currentRenderPass || !vulkan.currentRenderPass->native)
        return;
    auto* renderPass = (vk_pass_render_native_t*)vulkan.currentRenderPass->native;
    if (renderPass->rendering)
        return;

    auto& pass = *vulkan.currentRenderPass;
    for (uint32_t i = 0; i < RT_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        if (pass.colors[i].texture_view.format == RT_TEXTURE_NONE)
            continue;
        if (auto* view = vk_texture_view_native(pass.colors[i].texture_view))
        {
            if (view->texture && view->handle)
                vk_transition_image(*view->texture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
    }
    if (auto* view = vk_texture_view_native(pass.depth.texture_view))
    {
        if (view->texture && view->handle)
            vk_transition_image(*view->texture, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = {std::max(1u, renderPass->width), std::max(1u, renderPass->height)};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = renderPass->colorCount;
    renderingInfo.pColorAttachments = renderPass->colorCount ? renderPass->colorAttachments : nullptr;
    renderingInfo.pDepthAttachment = renderPass->hasDepth ? &renderPass->depthAttachment : nullptr;
    renderingInfo.pStencilAttachment = renderPass->hasStencil ? &renderPass->depthAttachment : nullptr;
    vkCmdBeginRendering(vulkan.cmdBuffer, &renderingInfo);
    renderPass->rendering = true;
}

static VkImageView vk_image_view(rt_texture_view_t const& view)
{
    auto* native = vk_texture_view_native(view);
    return native ? native->handle : nullptr;
}

static vk_sampler_native_t* vk_sampler_native(rt_sampler_t const& sampler)
{
    if (!sampler.native || sampler.handle == 0) return nullptr;
    return (vk_sampler_native_t*)sampler.native;
}

static vk_module_native_t* vk_current_module_native()
{
    if (vulkan.currentPassType == RT_MODULE_COMPUTE && vulkan.currentComputeModule)
        return (vk_module_native_t*)vulkan.currentComputeModule->native;
    if (vulkan.currentPassType == RT_MODULE_RENDER && vulkan.currentRenderModule)
        return (vk_module_native_t*)vulkan.currentRenderModule->native;
    return nullptr;
}

static rt_module_render_t const& vk_current_render_module()
{
    if (vulkan.currentRenderModule == nullptr)
    {
        fprintf(stderr, "Pipeline module not bound");
        abort();
    }
    return *vulkan.currentRenderModule;
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

static void vk_destroy_module_native(uint32_t handle, void*& native)
{
    if (!vulkan.device)
    {
        native = nullptr;
        return;
    }
    if (!native || handle == 0)
    {
        native = nullptr;
        return;
    }
    auto& module = *(vk_module_native_t*)native;
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
    vulkan.modules.erase(handle);
    native = nullptr;
}

static void vk_flush_descriptors()
{
    auto* mod = vk_current_module_native();
    if (!mod || !mod->descriptorSet || !mod->pipelineLayout)
        return;

    VkWriteDescriptorSet writes[RT_MAX_BINDING_HANDLE_NUM] = {};
    VkDescriptorBufferInfo bufferInfos[RT_MAX_BINDING_HANDLE_NUM] = {};
    VkDescriptorImageInfo imageInfos[RT_MAX_BINDING_HANDLE_NUM] = {};
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
            bufferInfos[writeCount] = {buf->handle, 0, VK_WHOLE_SIZE};
            writes[writeCount].pBufferInfo = &bufferInfos[writeCount];
            VkPipelineStageFlags dstStage = (vulkan.currentPassType == RT_MODULE_COMPUTE) ?
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
        else if (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
        {
            auto* view = vk_texture_view_native(slot.texture_view);
            if (!view || !view->handle || !view->texture) continue;
            auto* tex = view->texture;
            VkImageView imageView = view->handle;
            vk_transition_image(*tex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            imageInfos[writeCount] = {VK_NULL_HANDLE, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes[writeCount].pImageInfo = &imageInfos[writeCount];
        }
        else if (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
        {
            auto* view = vk_texture_view_native(slot.storage_view);
            if (!view || !view->handle || !view->texture) continue;
            auto* tex = view->texture;
            vk_transition_image(*tex, VK_IMAGE_LAYOUT_GENERAL);
            imageInfos[writeCount] = {VK_NULL_HANDLE, view->handle, VK_IMAGE_LAYOUT_GENERAL};
            writes[writeCount].pImageInfo = &imageInfos[writeCount];
        }
        else continue;
        writeCount++;
    }

    if (writeCount)
        vkUpdateDescriptorSets(vulkan.device, writeCount, writes, 0, nullptr);

    VkPipelineBindPoint bindPoint = (vulkan.currentPassType == RT_MODULE_COMPUTE) ?
                                    VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
    vkCmdBindDescriptorSets(vulkan.cmdBuffer, bindPoint, mod->pipelineLayout, 0, 1, &mod->descriptorSet, 0, nullptr);
}

// ====================================================================

void vk_load_library(VkInstance instance, VkPhysicalDevice physical, VkDevice device, VkQueue queue, VkCommandBuffer cmdbuf, uint32_t family)
{
    if (!instance)
    {
        fprintf(stderr, "Vulkan: instance is null");
        abort();
    }
    if (!physical)
    {
        fprintf(stderr, "Vulkan: physical device is null");
        abort();
    }
    if (!device)
    {
        fprintf(stderr, "Vulkan: device is null");
        abort();
    }
    if (!queue)
    {
        fprintf(stderr, "Vulkan: queue is null");
        abort();
    }
    if (!cmdbuf)
    {
        fprintf(stderr, "Vulkan: command buffer is null");
        abort();
    }
    vulkan.instance = instance;
    vulkan.physicalDevice = physical;
    vulkan.device = device;
    vulkan.queueFamily = family;
    vulkan.queue = queue;
    vulkan.cmdBuffer = cmdbuf;
    vkGetPhysicalDeviceMemoryProperties(vulkan.physicalDevice, &vulkan.memProps);

    VkPhysicalDeviceDriverProperties driverProps = {};
    driverProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
    VkPhysicalDeviceMeshShaderPropertiesNV meshProps = {};
    meshProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
    meshProps.pNext = &driverProps;
    VkPhysicalDeviceProperties2 props2 = {};
    props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props2.pNext = &meshProps;
    vkGetPhysicalDeviceProperties2(vulkan.physicalDevice, &props2);
    uint32_t apiMajor = VK_VERSION_MAJOR(props2.properties.apiVersion);
    uint32_t apiMinor = VK_VERSION_MINOR(props2.properties.apiVersion);
    uint32_t apiPatch = VK_VERSION_PATCH(props2.properties.apiVersion);
    uint32_t spirvMinor = apiMinor >= 3 ? 6 : apiMinor == 2 ? 5 : apiMinor == 1 ? 3 : 0;
    fprintf(stdout, "Vulkan Version: %u.%u.%u\n", apiMajor, apiMinor, apiPatch);
    fprintf(stdout, "SPIR-V Version: 1.%u\n", spirvMinor);
    fprintf(stdout, "Vulkan Render : %s\n", props2.properties.deviceName);
    fprintf(stdout, "Vulkan Vendor : %s\n", driverProps.driverName);
    fprintf(stdout, "Meshlet Primitives: %u\n", meshProps.maxMeshOutputPrimitives);
    fprintf(stdout, "Meshlet Vertices  : %u\n", meshProps.maxMeshOutputVertices);
    fflush(stdout);

    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 256},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 256},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256},
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

    vulkan.fnDrawMeshTasksNV = (PFN_vkCmdDrawMeshTasksNV)vkGetDeviceProcAddr(vulkan.device, "vkCmdDrawMeshTasksNV");
    vulkan.fnDrawMeshTasksIndirectEXT = (PFN_vkCmdDrawMeshTasksIndirectEXT)vkGetDeviceProcAddr(vulkan.device, "vkCmdDrawMeshTasksIndirectEXT");

    rt_unload_library = vk_unload_library;
    rt_create_buffer = vk_create_buffer;
    rt_destroy_buffer = vk_destroy_buffer;
    rt_bind_buffer = vk_bind_buffer;
    rt_map_buffer = vk_map_buffer;
    rt_unmap_buffer = vk_unmap_buffer;
    rt_create_texture = vk_create_texture;
    rt_destroy_texture = vk_destroy_texture;
    rt_bind_texture = vk_bind_texture;
    rt_create_texture_view = vk_create_texture_view;
    rt_destroy_texture_view = vk_destroy_texture_view;
    rt_bind_texture_view = vk_bind_texture_view;
    rt_bind_texture_storage = vk_bind_texture_storage;
    rt_create_sampler = vk_create_sampler;
    rt_destroy_sampler = vk_destroy_sampler;
    rt_bind_sampler = vk_bind_sampler;
    rt_create_module_compute = vk_create_module_compute;
    rt_create_module_render = vk_create_module_render;
    rt_destroy_module_render = vk_destroy_module_render;
    rt_destroy_module_compute = vk_destroy_module_compute;
    rt_begin_compute = vk_begin_compute;
    rt_end_compute = vk_end_compute;
    rt_bind_module_compute = vk_bind_module_compute;
    rt_dispatch_compute = vk_dispatch_compute;
    rt_dispatch_compute_indirect = vk_dispatch_compute_indirect;
    rt_begin_render = vk_begin_render;
    rt_end_render = vk_end_render;
    rt_bind_module_render = vk_bind_module_render;
    rt_set_viewport = vk_set_viewport;
    rt_set_scissor = vk_set_scissor;
    rt_set_blend_constant = vk_set_blend_constant;
    rt_set_stencil_reference = vk_set_stencil_reference;
    rt_draw_mesh_task = vk_draw_mesh_task;
    rt_draw_mesh_task_indirect = vk_draw_mesh_task_indirect;
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
    rt_draw_array = vk_draw_array;
    rt_draw_index = vk_draw_index;
    rt_draw_array_indirect = vk_draw_array_indirect;
    rt_draw_index_indirect = vk_draw_index_indirect;
    rt_create_meshlet = vk_create_meshlet;
    rt_destroy_meshlet = vk_destroy_meshlet;
    rt_draw_meshlet = vk_draw_meshlet;
    rt_submit = vk_submit;
}

void vk_unload_library()
{
    if (!vulkan.device) return;
    vkDeviceWaitIdle(vulkan.device);
    vkResetCommandBuffer(vulkan.cmdBuffer, 0);
    vk_flush_staging();

    for (auto& item : vulkan.modules)
    {
        auto& module = item.second;
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
    }
    vulkan.modules.clear();

    for (auto& item : vulkan.buffers)
    {
        auto& buffer = item.second;
        if (buffer.mapped)
            vkUnmapMemory(vulkan.device, buffer.memory);
        if (buffer.handle) vkDestroyBuffer(vulkan.device, buffer.handle, vulkan.allocator);
        if (buffer.memory) vkFreeMemory(vulkan.device, buffer.memory, vulkan.allocator);
    }
    vulkan.buffers.clear();

    for (auto& item : vulkan.textureViews)
    {
        if (item.second.handle)
            vkDestroyImageView(vulkan.device, item.second.handle, vulkan.allocator);
    }
    vulkan.textureViews.clear();

    for (auto& item : vulkan.textures)
    {
        auto& texture = item.second;
        if (texture.handle) vkDestroyImage(vulkan.device, texture.handle, vulkan.allocator);
        if (texture.memory) vkFreeMemory(vulkan.device, texture.memory, vulkan.allocator);
    }
    vulkan.textures.clear();

    for (auto& item : vulkan.samplers)
    {
        if (item.second.handle) vkDestroySampler(vulkan.device, item.second.handle, vulkan.allocator);
    }
    vulkan.samplers.clear();

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

    vulkan.meshes.clear();
    vulkan.meshlets.clear();
    vulkan.computePasses.clear();
    vulkan.renderPass = {};
    vulkan.transferPasses.clear();
    vulkan.queue = nullptr;
    vulkan.device = nullptr;
    vulkan.instance = nullptr;
    vulkan.physicalDevice = nullptr;
    vulkan.bufferID = vulkan.textureID = vulkan.textureViewID = vulkan.samplerID = 0;
    vulkan.moduleID = vulkan.meshID = vulkan.meshletID = vulkan.passID = 0;
    vulkan.currentPassType = RT_MODULE_NONE;
    vulkan.currentPipeline = nullptr;

    if (rt_unload_library == vk_unload_library) rt_unload_library = nullptr;
    if (rt_create_buffer == vk_create_buffer) rt_create_buffer = nullptr;
    if (rt_destroy_buffer == vk_destroy_buffer) rt_destroy_buffer = nullptr;
    if (rt_bind_buffer == vk_bind_buffer) rt_bind_buffer = nullptr;
    if (rt_map_buffer == vk_map_buffer) rt_map_buffer = nullptr;
    if (rt_unmap_buffer == vk_unmap_buffer) rt_unmap_buffer = nullptr;
    if (rt_create_texture == vk_create_texture) rt_create_texture = nullptr;
    if (rt_destroy_texture == vk_destroy_texture) rt_destroy_texture = nullptr;
    if (rt_bind_texture == vk_bind_texture) rt_bind_texture = nullptr;
    if (rt_create_texture_view == vk_create_texture_view) rt_create_texture_view = nullptr;
    if (rt_destroy_texture_view == vk_destroy_texture_view) rt_destroy_texture_view = nullptr;
    if (rt_bind_texture_view == vk_bind_texture_view) rt_bind_texture_view = nullptr;
    if (rt_bind_texture_storage == vk_bind_texture_storage) rt_bind_texture_storage = nullptr;
    if (rt_create_sampler == vk_create_sampler) rt_create_sampler = nullptr;
    if (rt_destroy_sampler == vk_destroy_sampler) rt_destroy_sampler = nullptr;
    if (rt_bind_sampler == vk_bind_sampler) rt_bind_sampler = nullptr;
    if (rt_create_module_compute == vk_create_module_compute) rt_create_module_compute = nullptr;
    if (rt_create_module_render == vk_create_module_render) rt_create_module_render = nullptr;
    if (rt_destroy_module_render == vk_destroy_module_render) rt_destroy_module_render = nullptr;
    if (rt_destroy_module_compute == vk_destroy_module_compute) rt_destroy_module_compute = nullptr;
    if (rt_begin_compute == vk_begin_compute) rt_begin_compute = nullptr;
    if (rt_end_compute == vk_end_compute) rt_end_compute = nullptr;
    if (rt_bind_module_compute == vk_bind_module_compute) rt_bind_module_compute = nullptr;
    if (rt_dispatch_compute == vk_dispatch_compute) rt_dispatch_compute = nullptr;
    if (rt_dispatch_compute_indirect == vk_dispatch_compute_indirect) rt_dispatch_compute_indirect = nullptr;
    if (rt_begin_render == vk_begin_render) rt_begin_render = nullptr;
    if (rt_end_render == vk_end_render) rt_end_render = nullptr;
    if (rt_bind_module_render == vk_bind_module_render) rt_bind_module_render = nullptr;
    if (rt_set_viewport == vk_set_viewport) rt_set_viewport = nullptr;
    if (rt_set_scissor == vk_set_scissor) rt_set_scissor = nullptr;
    if (rt_set_blend_constant == vk_set_blend_constant) rt_set_blend_constant = nullptr;
    if (rt_set_stencil_reference == vk_set_stencil_reference) rt_set_stencil_reference = nullptr;
    if (rt_draw_mesh_task == vk_draw_mesh_task) rt_draw_mesh_task = nullptr;
    if (rt_draw_mesh_task_indirect == vk_draw_mesh_task_indirect) rt_draw_mesh_task_indirect = nullptr;
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
    if (rt_draw_array == vk_draw_array) rt_draw_array = nullptr;
    if (rt_draw_index == vk_draw_index) rt_draw_index = nullptr;
    if (rt_draw_array_indirect == vk_draw_array_indirect) rt_draw_array_indirect = nullptr;
    if (rt_draw_index_indirect == vk_draw_index_indirect) rt_draw_index_indirect = nullptr;
    if (rt_create_meshlet == vk_create_meshlet) rt_create_meshlet = nullptr;
    if (rt_destroy_meshlet == vk_destroy_meshlet) rt_destroy_meshlet = nullptr;
    if (rt_draw_meshlet == vk_draw_meshlet) rt_draw_meshlet = nullptr;
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
    bool hostVisible = (info.usage & (RT_BUFFER_USAGE_MAP_READ | RT_BUFFER_USAGE_MAP_WRITE)) || info.data;
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
                bool immediate = vulkan.currentPassType == RT_MODULE_NONE;
                if (immediate)
                {
                    VkCommandBufferBeginInfo beginInfo = {};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                    if (vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo) != VK_SUCCESS)
                    {
                        fprintf(stderr, "Vulkan: failed to begin command buffer");
                        abort();
                    }
                }
                vk_transition_buffer(native, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
                VkBufferCopy region = {0, 0, info.size};
                vkCmdCopyBuffer(vulkan.cmdBuffer, staging.buffer, native.handle, 1, &region);
                vulkan.pendingStaging.push_back(staging);
                if (immediate)
                {
                    if (vkEndCommandBuffer(vulkan.cmdBuffer) != VK_SUCCESS)
                    {
                        fprintf(stderr, "Vulkan: failed to end command buffer");
                        abort();
                    }
                    VkSubmitInfo submitInfo = {};
                    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    submitInfo.commandBufferCount = 1;
                    submitInfo.pCommandBuffers = &vulkan.cmdBuffer;
                    vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE);
                    vkQueueWaitIdle(vulkan.queue);
                    vk_flush_staging();
                    vkResetCommandBuffer(vulkan.cmdBuffer, 0);
                }
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
    if (vulkan.device)
    {
        if (auto* native = vk_buffer_native(buffer))
        {
            if (native->mapped)
            {
                vkUnmapMemory(vulkan.device, native->memory);
                native->mapped = nullptr;
            }
            if (native->handle) vkDestroyBuffer(vulkan.device, native->handle, vulkan.allocator);
            if (native->memory) vkFreeMemory(vulkan.device, native->memory, vulkan.allocator);
            vulkan.buffers.erase(buffer.handle);
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
    vulkan.currentBinding[bind.binding].buffer = buffer;
    vulkan.currentBinding[bind.binding].buffer_bind = bind;
}

void* vk_map_buffer(rt_buffer_t& buffer, rt_access_t mode, size_t offset, size_t size)
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
    if (mode != RT_WRITE_ONLY)
        hostAccess |= VK_ACCESS_HOST_READ_BIT;
    if (mode != RT_READ_ONLY)
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
    if (info.usage == 0)
    {
        fprintf(stderr, "Texture usage must not be 0");
        abort();
    }
    rt_texture_t result = {};
    if (!vulkan.device || info.width == 0 || info.height == 0) return result;

    uint32_t handle = vulkan.textureID + 1;
    auto& native = vulkan.textures[handle];
    native.format = rt_to_vk_texture_format(info.format);
    native.rtFormat = info.format;
    native.usage = info.usage;
    native.target = info.target;
    native.aspect = vk_format_aspect(native.format);
    native.extent = {info.width, info.height, info.depth ? info.depth : 1};
    native.layers = 1;
    native.levels = 1;
    if (rt_has_mipmap_filter(info.min_filter))
    {
        uint32_t maxDim = std::max(info.width, info.height);
        native.levels = 1;
        while (maxDim >>= 1) native.levels++;
    }
    rt_texture_sample_t samples = info.samples == RT_TEXTURE_SAMPLE_4X ? RT_TEXTURE_SAMPLE_4X : RT_TEXTURE_SAMPLE_1X;
    if (info.target == RT_TEXTURE_1D || info.target == RT_TEXTURE_3D)
        samples = RT_TEXTURE_SAMPLE_1X;
    native.samples = samples;
    if (samples == RT_TEXTURE_SAMPLE_4X)
        native.levels = 1;

    VkImageCreateInfo vkInfo = {};
    vkInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vkInfo.imageType = (info.target == RT_TEXTURE_3D) ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    vkInfo.extent = native.extent;
    vkInfo.mipLevels = native.levels;
    vkInfo.arrayLayers = native.layers;
    vkInfo.format = native.format;
    vkInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    vkInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkInfo.usage = rt_to_vk_image_usage(info.usage, info.format);
    if (info.data && samples == RT_TEXTURE_SAMPLE_1X)
        vkInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    vkInfo.samples = rt_to_vk_sample_count(samples);
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

    if (info.data && samples == RT_TEXTURE_SAMPLE_1X)
    {
        VkDeviceSize bytes = (VkDeviceSize)info.width * info.height * native.extent.depth * vk_format_bytes(native.format);
        vk_staging_t staging = {};
        void* ptr = nullptr;
        bool immediate = vulkan.currentPassType == RT_MODULE_NONE;
        if (immediate)
        {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            if (vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo) != VK_SUCCESS)
            {
                fprintf(stderr, "Vulkan: failed to begin command buffer");
                abort();
            }
        }
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
        if (immediate)
        {
            if (vkEndCommandBuffer(vulkan.cmdBuffer) != VK_SUCCESS)
            {
                fprintf(stderr, "Vulkan: failed to end command buffer");
                abort();
            }
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &vulkan.cmdBuffer;
            vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(vulkan.queue);
            vk_flush_staging();
            vkResetCommandBuffer(vulkan.cmdBuffer, 0);
        }
    }

    if (native.samples == RT_TEXTURE_SAMPLE_4X && (native.target == RT_TEXTURE_2D || native.target == RT_TEXTURE_2D_MULTISAMPLE))
        native.target = RT_TEXTURE_2D_MULTISAMPLE;
    vulkan.textureID = handle;
    result.handle = handle;
    result.width = native.extent.width;
    result.height = native.extent.height;
    result.depth = native.extent.depth;
    result.target = native.target;
    result.format = native.rtFormat;
    result.samples = native.samples;
    result.usage = native.usage;
    result.mipmaps = native.levels;
    result.native = &native;
    result.default_view = vk_create_texture_view(result, {
        .target = native.target,
        .format = native.rtFormat,
        .aspect = RT_TEXTURE_ASPECT_ALL,
        .usage = native.usage,
        .layer_count = native.layers,
        .level_count = native.levels,
    });
    return result;
}

void vk_destroy_texture(rt_texture_t& texture)
{
    if (vulkan.device)
    {
        if (auto* native = vk_texture_native(texture))
        {
            for (auto view = vulkan.textureViews.begin(); view != vulkan.textureViews.end(); )
            {
                if (view->second.texture == native)
                {
                    if (view->second.handle)
                        vkDestroyImageView(vulkan.device, view->second.handle, vulkan.allocator);
                    view = vulkan.textureViews.erase(view);
                }
                else
                    ++view;
            }
            if (native->handle) vkDestroyImage(vulkan.device, native->handle, vulkan.allocator);
            if (native->memory) vkFreeMemory(vulkan.device, native->memory, vulkan.allocator);
            vulkan.textures.erase(texture.handle);
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
    vulkan.currentBinding[bind.binding].type = RT_BINDING_TEXTURE;
    vulkan.currentBinding[bind.binding].texture_view = texture.default_view;
    vulkan.currentBinding[bind.binding].texture_bind = bind;
}

rt_texture_view_t vk_create_texture_view(rt_texture_t& texture, rt_texture_view_info_t const& info)
{
    auto* tex = vk_texture_native(texture);
    rt_texture_format_t format = info.format != RT_TEXTURE_NONE ? info.format : (tex ? tex->rtFormat : RT_TEXTURE_NONE);
    rt_texture_usages_t usage = info.usage ? info.usage : (tex ? tex->usage : 0);
    rt_texture_view_t result{
        0, info.target, format,
        info.aspect, usage,
        info.base_layer, info.layer_count, info.base_level, info.level_count, nullptr};
    if (!vulkan.device || !tex || !tex->handle) return result;
    uint32_t levelCount = info.level_count ? info.level_count : tex->levels - info.base_level;
    if (info.base_level >= tex->levels || levelCount == 0) return result;
    if (info.base_level + levelCount > tex->levels)
        levelCount = tex->levels - info.base_level;
    uint32_t baseLayer = 0;
    uint32_t layerCount = 1;
    if (info.target != RT_TEXTURE_3D)
    {
        baseLayer = info.base_layer;
        layerCount = info.layer_count ? info.layer_count : (tex->layers > baseLayer ? tex->layers - baseLayer : 0);
        if (baseLayer >= tex->layers || layerCount == 0) return result;
        if (baseLayer + layerCount > tex->layers)
            layerCount = tex->layers - baseLayer;
    }
    result.base_layer = baseLayer;
    result.layer_count = layerCount;
    result.base_level = info.base_level;
    result.level_count = levelCount;

    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
    if (info.target == RT_TEXTURE_3D) viewType = VK_IMAGE_VIEW_TYPE_3D;
    else if (info.target == RT_TEXTURE_1D) viewType = VK_IMAGE_VIEW_TYPE_1D;
    else if (info.target == RT_TEXTURE_2D_ARRAY || layerCount > 1) viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;

    VkFormat imageFormat = result.format != RT_TEXTURE_NONE ? rt_to_vk_texture_format(result.format) : tex->format;
    VkImageViewUsageCreateInfo usageInfo = {};
    usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
    usageInfo.usage = rt_to_vk_image_usage(result.usage, result.format);
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = info.usage ? &usageInfo : nullptr;
    viewInfo.image = tex->handle;
    viewInfo.viewType = viewType;
    viewInfo.format = imageFormat;
    viewInfo.subresourceRange.aspectMask = rt_to_vk_aspect(info.aspect, result.format);
    viewInfo.subresourceRange.baseMipLevel = info.base_level;
    viewInfo.subresourceRange.levelCount = levelCount;
    viewInfo.subresourceRange.baseArrayLayer = baseLayer;
    viewInfo.subresourceRange.layerCount = layerCount;
    VkImageView imageView = nullptr;
    if (vkCreateImageView(vulkan.device, &viewInfo, vulkan.allocator, &imageView) != VK_SUCCESS)
        return result;
    uint32_t handle = vulkan.textureViewID + 1;
    auto& native = vulkan.textureViews[handle];
    native.handle = imageView;
    native.texture = tex;
    vulkan.textureViewID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void vk_destroy_texture_view(rt_texture_view_t& view)
{
    if (auto* native = vk_texture_view_native(view))
    {
        if (vulkan.device && native->handle)
            vkDestroyImageView(vulkan.device, native->handle, vulkan.allocator);
        vulkan.textureViews.erase(view.handle);
    }
    view = {};
}

void vk_bind_texture_view(rt_texture_view_t& view, rt_texture_view_bind_t bind)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vulkan.currentBinding[bind.binding].type = RT_BINDING_TEXTURE;
    vulkan.currentBinding[bind.binding].texture_view = view;
    vulkan.currentBinding[bind.binding].texture_bind = {};
    vulkan.currentBinding[bind.binding].texture_bind.binding = bind.binding;
}

void vk_bind_texture_storage(rt_texture_view_t& view, rt_texture_storage_bind_t bind)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vulkan.currentBinding[bind.binding].type = RT_BINDING_STORAGE_TEXTURE;
    vulkan.currentBinding[bind.binding].storage_view = view;
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
    vkInfo.magFilter = rt_to_vk_filter(info.mag_filter);
    vkInfo.minFilter = rt_to_vk_min_filter(info.min_filter);
    vkInfo.addressModeU = rt_to_vk_address(info.address_u);
    vkInfo.addressModeV = rt_to_vk_address(info.address_v);
    vkInfo.addressModeW = rt_to_vk_address(info.address_w);
    vkInfo.mipmapMode = rt_to_vk_mipmap(info.min_filter);
    vkInfo.maxLod = rt_has_mipmap_filter(info.min_filter) ? VK_LOD_CLAMP_NONE : 0.0f;
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
    if (vulkan.device)
    {
        if (auto* native = vk_sampler_native(sampler))
        {
            if (native->handle) vkDestroySampler(vulkan.device, native->handle, vulkan.allocator);
            vulkan.samplers.erase(sampler.handle);
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
    vulkan.currentBinding[bind.binding].type = RT_BINDING_SAMPLER;
    vulkan.currentBinding[bind.binding].sampler = sampler;
    vulkan.currentBinding[bind.binding].sampler_bind = bind;
}

rt_module_compute_t vk_create_module_compute(rt_module_compute_info_t const& info)
{
    rt_module_compute_t result = {};
    if (!info.cshader.code || !info.cshader.size || !vulkan.device) return result;
    uint32_t handle = vulkan.moduleID + 1;
    auto& native = vulkan.modules[handle];
    native.shaderStages = VK_SHADER_STAGE_COMPUTE_BIT;
    native.cshader = vk_create_shader_module(info.cshader.code, info.cshader.size);
    if (!native.cshader)
    {
        vulkan.modules.erase(handle);
        return {};
    }
    VkDescriptorSetLayoutBinding layoutBindings[RT_MAX_BINDING_HANDLE_NUM] = {};
    native.descriptorCount = 0;
    for (uint32_t i = 0; i < RT_MAX_BINDING_HANDLE_NUM; ++i)
    {
        if (info.binding[i].type == RT_BINDING_NONE)
            continue;
        auto& item = layoutBindings[native.descriptorCount];
        item.binding = info.binding[i].binding;
        item.descriptorType = rt_to_vk_descriptor(info.binding[i].type);
        item.descriptorCount = 1;
        item.stageFlags = native.shaderStages;
        native.descriptorBindings[native.descriptorCount] = info.binding[i].binding;
        native.descriptorTypes[native.descriptorCount] = item.descriptorType;
        native.descriptorCount++;
    }

    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = native.descriptorCount;
    setLayoutInfo.pBindings = native.descriptorCount ? layoutBindings : nullptr;
    if (vkCreateDescriptorSetLayout(vulkan.device, &setLayoutInfo, vulkan.allocator, &native.descriptorSetLayout) != VK_SUCCESS)
    {
        result.native = &native;
        vk_destroy_module_native(handle, result.native);
        return {};
    }

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
    pipelineInfo.stage.pName = (info.cshader.entry && info.cshader.entry[0]) ? info.cshader.entry : "main";
    if (vkCreateComputePipelines(vulkan.device, vulkan.pipelineCache, 1, &pipelineInfo, vulkan.allocator, &native.pipeline) != VK_SUCCESS)
    {
        result.native = &native;
        vk_destroy_module_native(handle, result.native);
        return {};
    }
    vulkan.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    for (size_t i = 0; i < std::size(info.binding); ++i)
        result.binding[i] = info.binding[i];
    return result;
}

rt_module_render_t vk_create_module_render(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};
    if (!vulkan.device) return result;
    uint32_t handle = vulkan.moduleID + 1;
    auto& native = vulkan.modules[handle];
    if (info.vshader.code)
    {
        native.shaderStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        native.vshader = vk_create_shader_module(info.vshader.code, info.vshader.size);
        if (!native.vshader) { vulkan.modules.erase(handle); return {}; }
    }
    else if (info.mshader.code)
    {
        native.shaderStages = VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT;
        if (info.tshader.code)
        {
            native.shaderStages |= VK_SHADER_STAGE_TASK_BIT_NV;
            native.tshader = vk_create_shader_module(info.tshader.code, info.tshader.size);
            if (!native.tshader) { vulkan.modules.erase(handle); return {}; }
        }
        native.mshader = vk_create_shader_module(info.mshader.code, info.mshader.size);
        if (!native.mshader)
        {
            result.native = &native;
            vk_destroy_module_native(handle, result.native);
            return {};
        }
    }
    else
    {
        vulkan.modules.erase(handle);
        fprintf(stderr, "Render module requires a vertex shader or a mesh shader");
        abort();
    }
    if (info.fshader.code)
    {
        native.fshader = vk_create_shader_module(info.fshader.code, info.fshader.size);
        if (!native.fshader)
        {
            result.native = &native;
            vk_destroy_module_native(handle, result.native);
            return {};
        }
    }
    VkDescriptorSetLayoutBinding layoutBindings[RT_MAX_BINDING_HANDLE_NUM] = {};
    native.descriptorCount = 0;
    for (uint32_t i = 0; i < RT_MAX_BINDING_HANDLE_NUM; ++i)
    {
        if (info.binding[i].type == RT_BINDING_NONE)
            continue;
        auto& item = layoutBindings[native.descriptorCount];
        item.binding = info.binding[i].binding;
        item.descriptorType = rt_to_vk_descriptor(info.binding[i].type);
        item.descriptorCount = 1;
        item.stageFlags = native.shaderStages;
        native.descriptorBindings[native.descriptorCount] = info.binding[i].binding;
        native.descriptorTypes[native.descriptorCount] = item.descriptorType;
        native.descriptorCount++;
    }

    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = native.descriptorCount;
    setLayoutInfo.pBindings = native.descriptorCount ? layoutBindings : nullptr;
    if (vkCreateDescriptorSetLayout(vulkan.device, &setLayoutInfo, vulkan.allocator, &native.descriptorSetLayout) != VK_SUCCESS)
    {
        result.native = &native;
        vk_destroy_module_native(handle, result.native);
        return {};
    }

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
    if (vkCreatePipelineLayout(vulkan.device, &layoutInfo, vulkan.allocator, &native.pipelineLayout) != VK_SUCCESS)
    {
        result.native = &native;
        vk_destroy_module_native(handle, result.native);
        return {};
    }

    VkPipelineShaderStageCreateInfo stages[3] = {};
    uint32_t shaderCount = 0;
    if (native.vshader)
    {
        stages[shaderCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[shaderCount].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[shaderCount].module = native.vshader;
        stages[shaderCount].pName = (info.vshader.entry && info.vshader.entry[0]) ? info.vshader.entry : "main";
        shaderCount++;
    }
    if (native.tshader)
    {
        stages[shaderCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[shaderCount].stage = VK_SHADER_STAGE_TASK_BIT_NV;
        stages[shaderCount].module = native.tshader;
        stages[shaderCount].pName = (info.tshader.entry && info.tshader.entry[0]) ? info.tshader.entry : "main";
        shaderCount++;
    }
    if (native.mshader)
    {
        stages[shaderCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[shaderCount].stage = VK_SHADER_STAGE_MESH_BIT_NV;
        stages[shaderCount].module = native.mshader;
        stages[shaderCount].pName = (info.mshader.entry && info.mshader.entry[0]) ? info.mshader.entry : "main";
        shaderCount++;
    }
    if (native.fshader)
    {
        stages[shaderCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[shaderCount].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[shaderCount].module = native.fshader;
        stages[shaderCount].pName = (info.fshader.entry && info.fshader.entry[0]) ? info.fshader.entry : "main";
        shaderCount++;
    }

    VkVertexInputBindingDescription bindingDescs[RT_MAX_VERTEX_BUFFER_NUM] = {};
    VkVertexInputAttributeDescription attribDescs[RT_MAX_VERTEX_BUFFER_NUM * RT_MAX_VERTEX_ATTRIB_NUM] = {};
    uint32_t bindingCount = 0;
    uint32_t attribCount = 0;
    for (uint32_t i = 0; i < RT_MAX_VERTEX_BUFFER_NUM; ++i)
    {
        uint32_t stride = rt_to_vk_vertex_stride(info.vertex[i]);
        if (stride == 0)
            continue;
        bindingDescs[bindingCount].binding = i;
        bindingDescs[bindingCount].stride = stride;
        bindingDescs[bindingCount].inputRate = info.vertex[i].instance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
        bindingCount++;
        for (auto& attrib : info.vertex[i].attrib)
        {
            if (attrib.format == RT_VERTEX_NONE)
                continue;
            attribDescs[attribCount].location = attrib.location;
            attribDescs[attribCount].binding = i;
            attribDescs[attribCount].format = rt_to_vk_vertex_format(attrib.format);
            attribDescs[attribCount].offset = attrib.offset;
            attribCount++;
        }
    }

    VkPipelineVertexInputStateCreateInfo vertexInput = {};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = bindingCount;
    vertexInput.pVertexBindingDescriptions = bindingCount == 0 ? nullptr : bindingDescs;
    vertexInput.vertexAttributeDescriptionCount = attribCount;
    vertexInput.pVertexAttributeDescriptions = attribCount == 0 ? nullptr : attribDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = rt_to_vk_primitive(info.primitive);

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = rt_to_vk_fill(info.fill_mode);
    rasterizer.cullMode = rt_to_vk_cull(info.cull_mode);
    rasterizer.frontFace = rt_to_vk_wind_mode(info.wind_mode);
    rasterizer.depthBiasEnable = (info.depth.bias != 0.0f || info.depth.biasSlope != 0.0f) ? VK_TRUE : VK_FALSE;
    rasterizer.depthBiasConstantFactor = info.depth.bias;
    rasterizer.depthBiasClamp = info.depth.biasClamp;
    rasterizer.depthBiasSlopeFactor = info.depth.biasSlope;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    const bool depthEnabled = (info.depth.func != RT_ALWAYS || info.depth.write);
    const bool stencilEnabled =
        (info.stencil.back.func != RT_ALWAYS || info.stencil.back.sfail != RT_STENCIL_KEEP ||
         info.stencil.back.zfail != RT_STENCIL_KEEP || info.stencil.back.zpass != RT_STENCIL_KEEP ||
         info.stencil.front.func != RT_ALWAYS || info.stencil.front.sfail != RT_STENCIL_KEEP ||
         info.stencil.front.zfail != RT_STENCIL_KEEP || info.stencil.front.zpass != RT_STENCIL_KEEP);

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = depthEnabled ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = info.depth.write ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = rt_to_vk_compare(info.depth.func);
    depthStencil.stencilTestEnable = stencilEnabled ? VK_TRUE : VK_FALSE;
    depthStencil.front.failOp = rt_to_vk_stencil_op(info.stencil.front.sfail);
    depthStencil.front.passOp = rt_to_vk_stencil_op(info.stencil.front.zpass);
    depthStencil.front.depthFailOp = rt_to_vk_stencil_op(info.stencil.front.zfail);
    depthStencil.front.compareOp = rt_to_vk_compare(info.stencil.front.func);
    depthStencil.front.compareMask = info.stencil.read;
    depthStencil.front.writeMask = info.stencil.write;
    depthStencil.back.failOp = rt_to_vk_stencil_op(info.stencil.back.sfail);
    depthStencil.back.passOp = rt_to_vk_stencil_op(info.stencil.back.zpass);
    depthStencil.back.depthFailOp = rt_to_vk_stencil_op(info.stencil.back.zfail);
    depthStencil.back.compareOp = rt_to_vk_compare(info.stencil.back.func);
    depthStencil.back.compareMask = info.stencil.read;
    depthStencil.back.writeMask = info.stencil.write;

    uint32_t colorCount = 0;
    VkPipelineColorBlendAttachmentState colorBlendAttachments[RT_MAX_COLOR_TEXTURE_NUM] = {};
    VkFormat colorFormats[RT_MAX_COLOR_TEXTURE_NUM] = {};
    for (uint32_t i = 0; i < RT_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        colorFormats[i] = rt_to_vk_texture_format(info.colors[i].format);
        if (info.colors[i].format == RT_TEXTURE_NONE)
            continue;
        colorCount = i + 1;
        bool blendEnabled =
            (info.colors[i].color.func != RT_FUNC_ADD || info.colors[i].color.src != RT_BLEND_ONE ||
             info.colors[i].color.dst != RT_BLEND_ZERO || info.colors[i].alpha.func != RT_FUNC_ADD ||
             info.colors[i].alpha.src != RT_BLEND_ONE || info.colors[i].alpha.dst != RT_BLEND_ZERO);
        colorBlendAttachments[i].blendEnable = blendEnabled ? VK_TRUE : VK_FALSE;
        colorBlendAttachments[i].srcColorBlendFactor = rt_to_vk_blend_factor(info.colors[i].color.src);
        colorBlendAttachments[i].dstColorBlendFactor = rt_to_vk_blend_factor(info.colors[i].color.dst);
        colorBlendAttachments[i].colorBlendOp = rt_to_vk_blend_op(info.colors[i].color.func);
        colorBlendAttachments[i].srcAlphaBlendFactor = rt_to_vk_blend_factor(info.colors[i].alpha.src);
        colorBlendAttachments[i].dstAlphaBlendFactor = rt_to_vk_blend_factor(info.colors[i].alpha.dst);
        colorBlendAttachments[i].alphaBlendOp = rt_to_vk_blend_op(info.colors[i].alpha.func);
        colorBlendAttachments[i].colorWriteMask =
            (info.colors[i].write.r ? VK_COLOR_COMPONENT_R_BIT : 0) | (info.colors[i].write.g ? VK_COLOR_COMPONENT_G_BIT : 0) |
            (info.colors[i].write.b ? VK_COLOR_COMPONENT_B_BIT : 0) | (info.colors[i].write.a ? VK_COLOR_COMPONENT_A_BIT : 0);
    }

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = colorCount;
    colorBlending.pAttachments = colorCount ? colorBlendAttachments : nullptr;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_BLEND_CONSTANTS, VK_DYNAMIC_STATE_STENCIL_REFERENCE};
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (uint32_t)std::size(dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;

    VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;
    if (stencilEnabled) depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    else if (depthEnabled) depthStencilFormat = VK_FORMAT_D32_SFLOAT;

    VkPipelineRenderingCreateInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = colorCount;
    renderingInfo.pColorAttachmentFormats = colorCount ? colorFormats : nullptr;
    renderingInfo.depthAttachmentFormat = depthStencilFormat;
    renderingInfo.stencilAttachmentFormat = stencilEnabled ? depthStencilFormat : VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = shaderCount;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = native.vshader ? &vertexInput : nullptr;
    pipelineInfo.pInputAssemblyState = native.vshader ? &inputAssembly : nullptr;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = native.pipelineLayout;
    if (vkCreateGraphicsPipelines(vulkan.device, vulkan.pipelineCache, 1, &pipelineInfo, vulkan.allocator, &native.pipeline) != VK_SUCCESS)
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
        result.colors[i].format = info.colors[i].format;
        result.colors[i].write.r = info.colors[i].write.r;
        result.colors[i].write.g = info.colors[i].write.g;
        result.colors[i].write.b = info.colors[i].write.b;
        result.colors[i].write.a = info.colors[i].write.a;
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
    result.wind_mode = info.wind_mode;
    result.fill_mode = info.fill_mode;
    result.primitive = info.primitive;
    return result;
}

void vk_destroy_module_render(rt_module_render_t& module)
{
    vk_destroy_module_native(module.handle, module.native);
    module.handle = 0;
}

void vk_destroy_module_compute(rt_module_compute_t& module)
{
    vk_destroy_module_native(module.handle, module.native);
    module.handle = 0;
}

void vk_begin_compute(rt_pass_compute_t& pass)
{
    if (vulkan.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo) != VK_SUCCESS)
    {
        fprintf(stderr, "Vulkan: failed to begin command buffer");
        abort();
    }
    for (auto& binding : vulkan.currentBinding)
        binding = {};
    auto handle = vulkan.passID + 1;
    auto& native = vulkan.computePasses[handle];
    native.bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    pass.handle = handle;
    pass.native = &native;
    vulkan.currentPassType = RT_MODULE_COMPUTE;
    vulkan.currentComputePass = &pass;
    vulkan.currentComputeModule = nullptr;
    vulkan.passID = handle;
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
    vulkan.currentPassType = RT_MODULE_NONE;
    vulkan.currentPipeline = nullptr;
    vulkan.currentComputeModule = nullptr;
    for (auto& binding : vulkan.currentBinding)
        binding = {};
    if (vkEndCommandBuffer(vulkan.cmdBuffer) != VK_SUCCESS)
    {
        fprintf(stderr, "Vulkan: failed to end command buffer");
        abort();
    }
}

void vk_bind_module_compute(rt_module_compute_t& module)
{
    if (vulkan.currentPipeline == nullptr || vulkan.currentPassType != RT_MODULE_COMPUTE)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* mod = (vk_module_native_t*)module.native;
    if (module.handle == 0 || !mod || !mod->pipeline)
    {
        fprintf(stderr, "Pipeline module is not created");
        abort();
    }
    vulkan.currentComputeModule = &module;
    vkCmdBindPipeline(vulkan.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mod->pipeline);
}

void vk_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_COMPUTE)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentComputeModule == nullptr)
    {
        fprintf(stderr, "Pipeline module not bound");
        abort();
    }
    vk_flush_descriptors();
    vkCmdDispatch(vulkan.cmdBuffer, std::max(1u, groupX), std::max(1u, groupY), std::max(1u, groupZ));
}

void vk_dispatch_compute_indirect(rt_buffer_t& indirect, size_t offset)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_COMPUTE)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentComputeModule == nullptr)
    {
        fprintf(stderr, "Pipeline module not bound");
        abort();
    }
    auto* native = vk_buffer_native(indirect);
    if (!native)
        return;
    vk_flush_descriptors();
    vk_transition_buffer(*native, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
    vkCmdDispatchIndirect(vulkan.cmdBuffer, native->handle, (VkDeviceSize)offset);
}

static void vk_bind_draw_vbos(rt_buffer_t vbo[], uint32_t vbo_num)
{
    rt_module_render_t const& module = vk_current_render_module();
    uint32_t count = vbo_num;
    if (count > (uint32_t)std::size(module.vertex))
        count = (uint32_t)std::size(module.vertex);

    VkBuffer buffers[RT_MAX_VERTEX_BUFFER_NUM] = {};
    uint32_t slots[RT_MAX_VERTEX_BUFFER_NUM] = {};
    uint32_t bindCount = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        if (rt_to_vk_vertex_stride(module.vertex[i]) == 0 || !vbo || vbo[i].handle == 0)
            continue;
        auto* native = vk_buffer_native(vbo[i]);
        if (!native)
            continue;
        vk_transition_buffer(*native, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
        buffers[bindCount] = native->handle;
        slots[bindCount] = i;
        bindCount++;
    }
    vk_begin_rendering();
    for (uint32_t i = 0; i < bindCount; ++i)
    {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(vulkan.cmdBuffer, slots[i], 1, &buffers[i], &offset);
    }
}

void vk_begin_render(rt_pass_render_t& pass)
{
    if (vulkan.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo) != VK_SUCCESS)
    {
        fprintf(stderr, "Vulkan: failed to begin command buffer");
        abort();
    }
    for (auto& binding : vulkan.currentBinding)
        binding = {};
    vulkan.renderPass = {};
    auto& native = vulkan.renderPass;
    pass.native = &native;
    vulkan.currentPassType = RT_MODULE_RENDER;
    vulkan.currentRenderPass = &pass;
    vulkan.currentRenderModule = nullptr;

    uint32_t colorCount = 0;
    uint32_t width = 0, height = 0;
    VkRenderingAttachmentInfo colorAttachments[RT_MAX_COLOR_TEXTURE_NUM] = {};
    for (uint32_t i = 0; i < RT_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        if (pass.colors[i].texture_view.format == RT_TEXTURE_NONE)
            continue;
        colorAttachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if (auto* view = vk_texture_view_native(pass.colors[i].texture_view))
        {
            if (view->texture && view->handle)
            {
                auto* tex = view->texture;
                colorAttachments[i].imageView = view->handle;
                colorAttachments[i].loadOp = pass.colors[i].clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                colorAttachments[i].clearValue.color = {{pass.colors[i].value.r, pass.colors[i].value.g, pass.colors[i].value.b, pass.colors[i].value.a}};
                uint32_t mip = std::min(pass.colors[i].texture_view.base_level, 31u);
                width = std::max(width, std::max(1u, tex->extent.width >> mip));
                height = std::max(height, std::max(1u, tex->extent.height >> mip));
            }
        }
        colorCount = i + 1;
    }

    VkRenderingAttachmentInfo depthAttachment = {};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    bool hasDepth = false;
    if (auto* view = vk_texture_view_native(pass.depth.texture_view))
    {
        if (view->texture && view->handle)
        {
            auto* tex = view->texture;
            depthAttachment.imageView = view->handle;
            depthAttachment.loadOp = pass.depth.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            depthAttachment.clearValue.depthStencil.depth = pass.depth.value;
            depthAttachment.clearValue.depthStencil.stencil = (uint32_t)pass.stencil.value;
            uint32_t mip = std::min(pass.depth.texture_view.base_level, 31u);
            width = std::max(width, std::max(1u, tex->extent.width >> mip));
            height = std::max(height, std::max(1u, tex->extent.height >> mip));
            hasDepth = true;
        }
    }

    native.width = width;
    native.height = height;
    native.colorCount = colorCount;
    native.hasDepth = hasDepth;
    native.hasStencil = hasDepth && rt_texture_has_stencil(pass.depth.texture_view.format);
    std::memcpy(native.colorAttachments, colorAttachments, sizeof(colorAttachments));
    native.depthAttachment = depthAttachment;
    vk_set_viewport(0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f);
    vk_set_scissor(0, 0, (int32_t)width, (int32_t)height);
    vk_set_blend_constant(0.0f, 0.0f, 0.0f, 0.0f);
    vk_set_stencil_reference(pass.stencil.refer);
}

void vk_end_render(rt_pass_render_t& pass)
{
    if (vulkan.currentRenderPass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }

    for (auto& color : pass.colors)
        if (auto* view = vk_texture_view_native(color.texture_view))
            if (view->texture)
                vk_transition_image(*view->texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if (auto* view = vk_texture_view_native(pass.depth.texture_view))
        if (view->texture)
            vk_transition_image(*view->texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    pass.handle = 0;
    pass.native = nullptr;
    vulkan.renderPass = {};
    vulkan.currentPassType = RT_MODULE_NONE;
    vulkan.currentPipeline = nullptr;
    vulkan.currentRenderModule = nullptr;
    for (auto& binding : vulkan.currentBinding)
        binding = {};
    if (vkEndCommandBuffer(vulkan.cmdBuffer) != VK_SUCCESS)
    {
        fprintf(stderr, "Vulkan: failed to end command buffer");
        abort();
    }
}

void vk_bind_module_render(rt_module_render_t& module)
{
    if (vulkan.currentPipeline == nullptr || vulkan.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* mod = (vk_module_native_t*)module.native;
    if (module.handle == 0 || !mod || !mod->pipeline)
    {
        fprintf(stderr, "Pipeline module is not created");
        abort();
    }
    vulkan.currentRenderModule = &module;
    vkCmdBindPipeline(vulkan.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mod->pipeline);
}

void vk_set_viewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    VkViewport viewport = {};
#ifdef VULKAN_FLIPPING_VIEWPORT
    viewport.x = x;
    viewport.y = y + height;
    viewport.width = width;
    viewport.height = -height;
#else
    viewport.x = x;
    viewport.y = y;
    viewport.width = width;
    viewport.height = height;
#endif
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
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

void vk_set_blend_constant(float r, float g, float b, float a)
{
    if (vulkan.currentPipeline == nullptr || vulkan.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    const float constants[4] = {r, g, b, a};
    vkCmdSetBlendConstants(vulkan.cmdBuffer, constants);
}

void vk_set_stencil_reference(int32_t refer)
{
    if (vulkan.currentPipeline == nullptr || vulkan.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vkCmdSetStencilReference(vulkan.cmdBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, (uint32_t)refer);
}

void vk_draw_array(rt_buffer_t vbo[], uint32_t vbo_num, uint32_t vertex_num, uint32_t instance_num, uint32_t vertex_start, uint32_t instance_start)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vk_flush_descriptors();
    vk_bind_draw_vbos(vbo, vbo_num);
    vkCmdDraw(vulkan.cmdBuffer, vertex_num, instance_num, vertex_start, instance_start);
}

void vk_draw_index(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& ebo, uint32_t vertex_num, uint32_t instance_num, uint32_t vertex_start, uint32_t instance_start)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* indexNative = vk_buffer_native(ebo);
    if (!indexNative)
        return;
    vk_flush_descriptors();
    vk_transition_buffer(*indexNative, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT);
    vk_bind_draw_vbos(vbo, vbo_num);
    rt_module_render_t const& module = vk_current_render_module();
    vkCmdBindIndexBuffer(vulkan.cmdBuffer, indexNative->handle, 0, rt_to_vk_index_type(module.index_type));
    vkCmdDrawIndexed(vulkan.cmdBuffer, vertex_num, instance_num, 0, (int32_t)vertex_start, instance_start);
}

void vk_draw_array_indirect(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& indirect, size_t offset)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* indirectNative = vk_buffer_native(indirect);
    if (!indirectNative)
        return;
    vk_flush_descriptors();
    vk_transition_buffer(*indirectNative, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
    vk_bind_draw_vbos(vbo, vbo_num);
    vkCmdDrawIndirect(vulkan.cmdBuffer, indirectNative->handle, (VkDeviceSize)offset, 1, sizeof(VkDrawIndirectCommand));
}

void vk_draw_index_indirect(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& ebo, rt_buffer_t& indirect, size_t offset)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* indexNative = vk_buffer_native(ebo);
    auto* indirectNative = vk_buffer_native(indirect);
    if (!indexNative || !indirectNative)
        return;
    vk_flush_descriptors();
    vk_transition_buffer(*indexNative, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT);
    vk_transition_buffer(*indirectNative, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
    vk_bind_draw_vbos(vbo, vbo_num);
    rt_module_render_t const& module = vk_current_render_module();
    vkCmdBindIndexBuffer(vulkan.cmdBuffer, indexNative->handle, 0, rt_to_vk_index_type(module.index_type));
    vkCmdDrawIndexedIndirect(vulkan.cmdBuffer, indirectNative->handle, (VkDeviceSize)offset, 1, sizeof(VkDrawIndexedIndirectCommand));
}

void vk_draw_mesh_task(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    (void)vk_current_render_module();
    vk_flush_descriptors();
    vk_begin_rendering();
    if (vulkan.fnDrawMeshTasksNV)
        vulkan.fnDrawMeshTasksNV(vulkan.cmdBuffer, std::max(1u, groupX) * std::max(1u, groupY) * std::max(1u, groupZ), 0);
}

void vk_draw_mesh_task_indirect(rt_buffer_t& indirect, size_t offset, uint32_t draw_count, uint32_t draw_stride)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    (void)vk_current_render_module();
    auto* native = vk_buffer_native(indirect);
    if (!native)
        return;
    vk_flush_descriptors();
    vk_transition_buffer(*native, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
    vk_begin_rendering();
    if (vulkan.fnDrawMeshTasksIndirectEXT)
        vulkan.fnDrawMeshTasksIndirectEXT(vulkan.cmdBuffer, native->handle, (VkDeviceSize)offset, draw_count, draw_stride);
}

void vk_push_constant(uint8_t const* buffer, size_t length)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* mod = vk_current_module_native();
    if (!buffer || length == 0 || !mod || !mod->pipelineLayout) return;
    vkCmdPushConstants(vulkan.cmdBuffer, mod->pipelineLayout, mod->shaderStages, 0, (uint32_t)length, buffer);
}

void vk_push_const_int(const char*, int32_t)
{
    fprintf(stderr, "Vulkan: does not support rt_push_const_int");
    abort();
}

void vk_push_const_uint(const char*, uint32_t)
{
    fprintf(stderr, "Vulkan: does not support rt_push_const_uint");
    abort();
}

void vk_push_const_float(const char*, float)
{
    fprintf(stderr, "Vulkan: does not support rt_push_const_float");
    abort();
}

void vk_push_const_vec2(const char*, const float*)
{
    fprintf(stderr, "Vulkan: does not support rt_push_const_vec2");
    abort();
}

void vk_push_const_vec3(const char*, const float*)
{
    fprintf(stderr, "Vulkan: does not support rt_push_const_vec3");
    abort();
}

void vk_push_const_vec4(const char*, const float*)
{
    fprintf(stderr, "Vulkan: does not support rt_push_const_vec4");
    abort();
}

void vk_push_const_mat3(const char*, const float*)
{
    fprintf(stderr, "Vulkan: does not support rt_push_const_mat3");
    abort();
}

void vk_push_const_mat4(const char*, const float*)
{
    fprintf(stderr, "Vulkan: does not support rt_push_const_mat4");
    abort();
}

void vk_begin_transfer(rt_pass_transfer_t& pass)
{
    if (vulkan.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo) != VK_SUCCESS)
    {
        fprintf(stderr, "Vulkan: failed to begin command buffer");
        abort();
    }
    auto handle = vulkan.passID + 1;
    auto& native = vulkan.transferPasses[handle];
    pass.handle = handle;
    pass.native = &native;
    vulkan.currentPassType = RT_MODULE_TRANSFER;
    vulkan.currentTransferPass = &pass;
    vulkan.passID = handle;
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
    vulkan.currentPassType = RT_MODULE_NONE;
    vulkan.currentPipeline = nullptr;
    if (vkEndCommandBuffer(vulkan.cmdBuffer) != VK_SUCCESS)
    {
        fprintf(stderr, "Vulkan: failed to end command buffer");
        abort();
    }
}

void vk_copy_buffer(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
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
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
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
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* src = vk_texture_native(source.texture);
    auto* dst = vk_buffer_native(destination.buffer);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    vk_transition_image(*src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vk_transition_buffer(*dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
    VkBufferImageCopy region = {};
    region.bufferOffset = destination.offset;
    region.bufferRowLength = destination.bytesPerRow ? destination.bytesPerRow / vk_format_bytes(src->format) : 0;
    region.bufferImageHeight = destination.rowsPerImage;
    region.imageSubresource.aspectMask = rt_to_vk_aspect(source.aspect, src->rtFormat);
    region.imageSubresource.mipLevel = source.mipLevel;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {(int32_t)source.origin.x, (int32_t)source.origin.y, (int32_t)source.origin.z};
    region.imageExtent = {copySize.x, copySize.y, copySize.z ? copySize.z : 1};
    vkCmdCopyImageToBuffer(vulkan.cmdBuffer, src->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->handle, 1, &region);
}

void vk_copy_texture(rt_texture_copy_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* src = vk_texture_native(source.texture);
    auto* dst = vk_texture_native(destination.texture);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    vk_transition_image(*src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vk_transition_image(*dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageCopy region = {};
    region.srcSubresource.aspectMask = rt_to_vk_aspect(source.aspect, src->rtFormat);
    region.srcSubresource.mipLevel = source.mipLevel;
    region.srcSubresource.layerCount = 1;
    region.srcOffset = {(int32_t)source.origin.x, (int32_t)source.origin.y, (int32_t)source.origin.z};
    region.dstSubresource.aspectMask = rt_to_vk_aspect(destination.aspect, dst->rtFormat);
    region.dstSubresource.mipLevel = destination.mipLevel;
    region.dstSubresource.layerCount = 1;
    region.dstOffset = {(int32_t)destination.origin.x, (int32_t)destination.origin.y, (int32_t)destination.origin.z};
    region.extent = {copySize.x, copySize.y, copySize.z ? copySize.z : 1};
    vkCmdCopyImage(vulkan.cmdBuffer, src->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dst->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void vk_copy_texture_data(rt_texture_data_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
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
    region.imageSubresource.aspectMask = rt_to_vk_aspect(destination.aspect, dst->rtFormat);
    region.imageSubresource.mipLevel = destination.mipLevel;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {(int32_t)destination.origin.x, (int32_t)destination.origin.y, (int32_t)destination.origin.z};
    region.imageExtent = {copySize.x, copySize.y, copySize.z ? copySize.z : 1};
    vkCmdCopyBufferToImage(vulkan.cmdBuffer, staging.buffer, dst->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    vulkan.pendingStaging.push_back(staging);
}

void vk_copy_texture_buffer(rt_buffer_texel_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* src = vk_buffer_native(source.buffer);
    auto* dst = vk_texture_native(destination.texture);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    vk_transition_image(*dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vk_transition_buffer(*src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    VkBufferImageCopy region = {};
    region.bufferOffset = source.offset;
    region.bufferRowLength = source.bytesPerRow ? source.bytesPerRow / vk_format_bytes(dst->format) : 0;
    region.bufferImageHeight = source.rowsPerImage;
    region.imageSubresource.aspectMask = rt_to_vk_aspect(destination.aspect, dst->rtFormat);
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
        result.vertex[0] = vk_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = vk_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = vk_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = vk_create_buffer({.size = index_count * sizeof(uint32_t), .usage = RT_BUFFER_USAGE_INDEX | RT_BUFFER_USAGE_COPY_DST, .data = indices});
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
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    vk_flush_descriptors();
    rt_module_render_t const& module = vk_current_render_module();
    uint32_t vertex_count = 0;
    VkBuffer vertexBuffers[RT_MAX_VERTEX_BUFFER_NUM] = {};
    uint32_t vertexBindings[RT_MAX_VERTEX_BUFFER_NUM] = {};
    uint32_t vertexBindCount = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        uint32_t stride = rt_to_vk_vertex_stride(layout);
        if (stride == 0) continue;
        bool bound = false;
        for (uint32_t k = 0; k < std::size(mesh.vertex) && !bound; ++k)
        {
            if (mesh.vertex[k].handle == 0) continue;
            for (auto& attrib : layout.attrib)
            {
                if (attrib.format == RT_VERTEX_NONE || mesh.location[k] != attrib.location) continue;
                auto* native = vk_buffer_native(mesh.vertex[k]);
                if (!native) break;
                vk_transition_buffer(*native, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
                vertexBuffers[vertexBindCount] = native->handle;
                vertexBindings[vertexBindCount] = i;
                vertexBindCount++;
                if (vertex_count == 0)
                    vertex_count = (uint32_t)(mesh.vertex[k].size / stride);
                bound = true;
                break;
            }
        }
    }
    vk_buffer_native_t* indexNative = nullptr;
    if (mesh.index.handle)
        indexNative = vk_buffer_native(mesh.index);
    if (indexNative)
        vk_transition_buffer(*indexNative, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT);
    vk_begin_rendering();
    for (uint32_t i = 0; i < vertexBindCount; ++i)
    {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(vulkan.cmdBuffer, vertexBindings[i], 1, &vertexBuffers[i], &offset);
    }
    if (indexNative)
    {
        uint32_t indexStride = rt_to_vk_index_size(module.index_type);
        vkCmdBindIndexBuffer(vulkan.cmdBuffer, indexNative->handle, 0, rt_to_vk_index_type(module.index_type));
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
        result.vertex[0] = vk_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = vk_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = vk_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = vk_create_buffer({.size = index_count * sizeof(uint32_t), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = indices});
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
    if (vulkan.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (vulkan.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    rt_module_render_t const& module = vk_current_render_module();
    uint32_t index_binding = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        for (auto& attrib : module.vertex[i].attrib)
        {
            if (attrib.format == RT_VERTEX_NONE) continue;
            for (uint32_t k = 0; k < std::size(meshlet.vertex); ++k)
            {
                if (meshlet.vertex[k].handle == 0 || meshlet.location[k] != attrib.location) continue;
                vk_bind_buffer(meshlet.vertex[k], {.binding = attrib.location});
                break;
            }
            if (attrib.location + 1 > index_binding)
                index_binding = attrib.location + 1;
        }
    }
    if (meshlet.index.handle)
        vk_bind_buffer(meshlet.index, {.binding = index_binding});
    vk_flush_descriptors();
    vk_begin_rendering();
    auto* native = (vk_meshlet_native_t*)meshlet.native;
    uint32_t tasks = native && native->indexCount ? native->indexCount / 3 : 1;
    if (vulkan.fnDrawMeshTasksNV)
        vulkan.fnDrawMeshTasksNV(vulkan.cmdBuffer, std::max(1u, tasks), 0);
}

void vk_submit()
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkan.cmdBuffer;
    vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vulkan.queue);
    vk_flush_staging();
    vkResetCommandBuffer(vulkan.cmdBuffer, 0);
}

#endif
