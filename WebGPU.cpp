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
#ifdef WEBGPU_IMPLEMENTATION
#include "WebGPU.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <numeric>
#include <string>
#include <vector>

enum wg_res_state_t : uint32_t
{
    WG_STATE_UNKNOWN = 0,
    WG_STATE_HOST,
    WG_STATE_COPY_SRC,
    WG_STATE_COPY_DST,
    WG_STATE_SHADER_READ,
    WG_STATE_SHADER_WRITE,
    WG_STATE_COLOR,
    WG_STATE_DEPTH,
    WG_STATE_VERTEX,
    WG_STATE_INDEX,
};

enum wg_binding_kind_t : uint32_t
{
    WG_KIND_NONE = 0,
    WG_KIND_UNIFORM,
    WG_KIND_STORAGE,
    WG_KIND_TEXTURE,
    WG_KIND_STORAGE_TEXTURE,
    WG_KIND_SAMPLER,
};

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

static WGPUFilterMode rt_to_wg_filter(rt_filter_t filter)
{
    switch (filter)
    {
        case RT_NEAREST: return WGPUFilterMode_Nearest;
        case RT_LINEAR: return WGPUFilterMode_Linear;
        case RT_NEAREST_MIPMAP_NEAREST: return WGPUFilterMode_Nearest;
        case RT_LINEAR_MIPMAP_NEAREST: return WGPUFilterMode_Linear;
        case RT_NEAREST_MIPMAP_LINEAR: return WGPUFilterMode_Nearest;
        case RT_LINEAR_MIPMAP_LINEAR: return WGPUFilterMode_Linear;
        default: return WGPUFilterMode_Linear;
    }
}

static WGPUMipmapFilterMode rt_to_wg_mip(rt_filter_t minFilter)
{
    switch (minFilter)
    {
        case RT_NEAREST: return WGPUMipmapFilterMode_Nearest;
        case RT_LINEAR: return WGPUMipmapFilterMode_Nearest;
        case RT_NEAREST_MIPMAP_NEAREST: return WGPUMipmapFilterMode_Nearest;
        case RT_LINEAR_MIPMAP_NEAREST: return WGPUMipmapFilterMode_Nearest;
        case RT_NEAREST_MIPMAP_LINEAR: return WGPUMipmapFilterMode_Linear;
        case RT_LINEAR_MIPMAP_LINEAR: return WGPUMipmapFilterMode_Linear;
        default: return WGPUMipmapFilterMode_Nearest;
    }
}

static WGPUAddressMode rt_to_wg_address(rt_address_t address)
{
    switch (address)
    {
        case RT_REPEAT: return WGPUAddressMode_Repeat;
        case RT_CLAMP_TO_EDGE: return WGPUAddressMode_ClampToEdge;
        case RT_CLAMP_TO_BORDER: return WGPUAddressMode_ClampToEdge;
        case RT_MIRRORED_REPEAT: return WGPUAddressMode_MirrorRepeat;
        case RT_MIRROR_CLAMP_TO_EDGE: return WGPUAddressMode_ClampToEdge;
        default: return WGPUAddressMode_ClampToEdge;
    }
}

static WGPUTextureFormat rt_to_wg_texture_format(rt_texture_format_t format)
{
    switch (format)
    {
        case RT_TEXTURE_NONE: return WGPUTextureFormat_Undefined;
        case RT_TEXTURE_R8UNORM: return WGPUTextureFormat_R8Unorm;
        case RT_TEXTURE_R8SNORM: return WGPUTextureFormat_R8Snorm;
        case RT_TEXTURE_R8UINT: return WGPUTextureFormat_R8Uint;
        case RT_TEXTURE_R8SINT: return WGPUTextureFormat_R8Sint;
        case RT_TEXTURE_R16UNORM: return WGPUTextureFormat_R16Unorm;
        case RT_TEXTURE_R16SNORM: return WGPUTextureFormat_R16Snorm;
        case RT_TEXTURE_R16UINT: return WGPUTextureFormat_R16Uint;
        case RT_TEXTURE_R16SINT: return WGPUTextureFormat_R16Sint;
        case RT_TEXTURE_R16FLOAT: return WGPUTextureFormat_R16Float;
        case RT_TEXTURE_RG8UNORM: return WGPUTextureFormat_RG8Unorm;
        case RT_TEXTURE_RG8SNORM: return WGPUTextureFormat_RG8Snorm;
        case RT_TEXTURE_RG8UINT: return WGPUTextureFormat_RG8Uint;
        case RT_TEXTURE_RG8SINT: return WGPUTextureFormat_RG8Sint;
        case RT_TEXTURE_R32UINT: return WGPUTextureFormat_R32Uint;
        case RT_TEXTURE_R32SINT: return WGPUTextureFormat_R32Sint;
        case RT_TEXTURE_R32FLOAT: return WGPUTextureFormat_R32Float;
        case RT_TEXTURE_RG16UNORM: return WGPUTextureFormat_RG16Unorm;
        case RT_TEXTURE_RG16SNORM: return WGPUTextureFormat_RG16Snorm;
        case RT_TEXTURE_RG16UINT: return WGPUTextureFormat_RG16Uint;
        case RT_TEXTURE_RG16SINT: return WGPUTextureFormat_RG16Sint;
        case RT_TEXTURE_RG16FLOAT: return WGPUTextureFormat_RG16Float;
        case RT_TEXTURE_RGBA8UNORM: return WGPUTextureFormat_RGBA8Unorm;
        case RT_TEXTURE_RGBA8UNORM_SRGB: return WGPUTextureFormat_RGBA8UnormSrgb;
        case RT_TEXTURE_RGBA8SNORM: return WGPUTextureFormat_RGBA8Snorm;
        case RT_TEXTURE_RGBA8UINT: return WGPUTextureFormat_RGBA8Uint;
        case RT_TEXTURE_RGBA8SINT: return WGPUTextureFormat_RGBA8Sint;
        case RT_TEXTURE_BGRA8UNORM: return WGPUTextureFormat_BGRA8Unorm;
        case RT_TEXTURE_BGRA8UNORM_SRGB: return WGPUTextureFormat_BGRA8UnormSrgb;
        case RT_TEXTURE_RGB10A2UINT: return WGPUTextureFormat_RGB10A2Uint;
        case RT_TEXTURE_RGB10A2UNORM: return WGPUTextureFormat_RGB10A2Unorm;
        case RT_TEXTURE_RG11B10UFLOAT: return WGPUTextureFormat_RG11B10Ufloat;
        case RT_TEXTURE_RGB9E5UFLOAT: return WGPUTextureFormat_RGB9E5Ufloat;
        case RT_TEXTURE_RG32UINT: return WGPUTextureFormat_RG32Uint;
        case RT_TEXTURE_RG32SINT: return WGPUTextureFormat_RG32Sint;
        case RT_TEXTURE_RG32FLOAT: return WGPUTextureFormat_RG32Float;
        case RT_TEXTURE_RGBA16UNORM: return WGPUTextureFormat_RGBA16Unorm;
        case RT_TEXTURE_RGBA16SNORM: return WGPUTextureFormat_RGBA16Snorm;
        case RT_TEXTURE_RGBA16UINT: return WGPUTextureFormat_RGBA16Uint;
        case RT_TEXTURE_RGBA16SINT: return WGPUTextureFormat_RGBA16Sint;
        case RT_TEXTURE_RGBA16FLOAT: return WGPUTextureFormat_RGBA16Float;
        case RT_TEXTURE_RGBA32UINT: return WGPUTextureFormat_RGBA32Uint;
        case RT_TEXTURE_RGBA32SINT: return WGPUTextureFormat_RGBA32Sint;
        case RT_TEXTURE_RGBA32FLOAT: return WGPUTextureFormat_RGBA32Float;
        case RT_TEXTURE_STENCIL8: return WGPUTextureFormat_Stencil8;
        case RT_TEXTURE_DEPTH16UNORM: return WGPUTextureFormat_Depth16Unorm;
        case RT_TEXTURE_DEPTH24PLUS: return WGPUTextureFormat_Depth24Plus;
        case RT_TEXTURE_DEPTH24PLUS_STENCIL8: return WGPUTextureFormat_Depth24PlusStencil8;
        case RT_TEXTURE_DEPTH32FLOAT: return WGPUTextureFormat_Depth32Float;
        case RT_TEXTURE_DEPTH32FLOAT_STENCIL8: return WGPUTextureFormat_Depth32FloatStencil8;
        default: return WGPUTextureFormat_Undefined;
    }
}

static uint32_t rt_to_wg_sample_count(rt_texture_sample_t samples)
{
    switch (samples)
    {
        case RT_TEXTURE_SAMPLE_1X: return 1;
        case RT_TEXTURE_SAMPLE_4X: return 4;
        default: return 1;
    }
}

static WGPUVertexFormat rt_to_wg_vertex_format(rt_vertex_format_t format)
{
    switch (format)
    {
        case RT_VERTEX_NONE: return WGPUVertexFormat_Undefined;
        case RT_VERTEX_UINT8: return WGPUVertexFormat_Uint8;
        case RT_VERTEX_UINT8X2: return WGPUVertexFormat_Uint8x2;
        case RT_VERTEX_UINT8X4: return WGPUVertexFormat_Uint8x4;
        case RT_VERTEX_SINT8: return WGPUVertexFormat_Sint8;
        case RT_VERTEX_SINT8X2: return WGPUVertexFormat_Sint8x2;
        case RT_VERTEX_SINT8X4: return WGPUVertexFormat_Sint8x4;
        case RT_VERTEX_UNORM8: return WGPUVertexFormat_Unorm8;
        case RT_VERTEX_UNORM8X2: return WGPUVertexFormat_Unorm8x2;
        case RT_VERTEX_UNORM8X4: return WGPUVertexFormat_Unorm8x4;
        case RT_VERTEX_SNORM8: return WGPUVertexFormat_Snorm8;
        case RT_VERTEX_SNORM8X2: return WGPUVertexFormat_Snorm8x2;
        case RT_VERTEX_SNORM8X4: return WGPUVertexFormat_Snorm8x4;
        case RT_VERTEX_UINT16: return WGPUVertexFormat_Uint16;
        case RT_VERTEX_UINT16X2: return WGPUVertexFormat_Uint16x2;
        case RT_VERTEX_UINT16X4: return WGPUVertexFormat_Uint16x4;
        case RT_VERTEX_SINT16: return WGPUVertexFormat_Sint16;
        case RT_VERTEX_SINT16X2: return WGPUVertexFormat_Sint16x2;
        case RT_VERTEX_SINT16X4: return WGPUVertexFormat_Sint16x4;
        case RT_VERTEX_UNORM16: return WGPUVertexFormat_Unorm16;
        case RT_VERTEX_UNORM16X2: return WGPUVertexFormat_Unorm16x2;
        case RT_VERTEX_UNORM16X4: return WGPUVertexFormat_Unorm16x4;
        case RT_VERTEX_SNORM16: return WGPUVertexFormat_Snorm16;
        case RT_VERTEX_SNORM16X2: return WGPUVertexFormat_Snorm16x2;
        case RT_VERTEX_SNORM16X4: return WGPUVertexFormat_Snorm16x4;
        case RT_VERTEX_FLOAT16: return WGPUVertexFormat_Float16;
        case RT_VERTEX_FLOAT16X2: return WGPUVertexFormat_Float16x2;
        case RT_VERTEX_FLOAT16X4: return WGPUVertexFormat_Float16x4;
        case RT_VERTEX_FLOAT32: return WGPUVertexFormat_Float32;
        case RT_VERTEX_FLOAT32X2: return WGPUVertexFormat_Float32x2;
        case RT_VERTEX_FLOAT32X3: return WGPUVertexFormat_Float32x3;
        case RT_VERTEX_FLOAT32X4: return WGPUVertexFormat_Float32x4;
        case RT_VERTEX_UINT32: return WGPUVertexFormat_Uint32;
        case RT_VERTEX_UINT32X2: return WGPUVertexFormat_Uint32x2;
        case RT_VERTEX_UINT32X3: return WGPUVertexFormat_Uint32x3;
        case RT_VERTEX_UINT32X4: return WGPUVertexFormat_Uint32x4;
        case RT_VERTEX_SINT32: return WGPUVertexFormat_Sint32;
        case RT_VERTEX_SINT32X2: return WGPUVertexFormat_Sint32x2;
        case RT_VERTEX_SINT32X3: return WGPUVertexFormat_Sint32x3;
        case RT_VERTEX_SINT32X4: return WGPUVertexFormat_Sint32x4;
        default: return WGPUVertexFormat_Undefined;
    }
}

static WGPUCompareFunction rt_to_wg_compare(rt_compare_op_t func)
{
    switch (func)
    {
        case RT_NEVER: return WGPUCompareFunction_Never;
        case RT_LESS: return WGPUCompareFunction_Less;
        case RT_EQUAL: return WGPUCompareFunction_Equal;
        case RT_LEQUAL: return WGPUCompareFunction_LessEqual;
        case RT_GREATER: return WGPUCompareFunction_Greater;
        case RT_NOTEQUAL: return WGPUCompareFunction_NotEqual;
        case RT_GEQUAL: return WGPUCompareFunction_GreaterEqual;
        case RT_ALWAYS: return WGPUCompareFunction_Always;
        default: return WGPUCompareFunction_Always;
    }
}

static WGPUBlendFactor rt_to_wg_blend(rt_blend_factor_t factor)
{
    switch (factor)
    {
        case RT_BLEND_ZERO: return WGPUBlendFactor_Zero;
        case RT_BLEND_ONE: return WGPUBlendFactor_One;
        case RT_BLEND_SRC_COLOR: return WGPUBlendFactor_Src;
        case RT_BLEND_ONE_MINUS_SRC_COLOR: return WGPUBlendFactor_OneMinusSrc;
        case RT_BLEND_SRC_ALPHA: return WGPUBlendFactor_SrcAlpha;
        case RT_BLEND_ONE_MINUS_SRC_ALPHA: return WGPUBlendFactor_OneMinusSrcAlpha;
        case RT_BLEND_DST_ALPHA: return WGPUBlendFactor_DstAlpha;
        case RT_BLEND_ONE_MINUS_DST_ALPHA: return WGPUBlendFactor_OneMinusDstAlpha;
        case RT_BLEND_DST_COLOR: return WGPUBlendFactor_Dst;
        case RT_BLEND_ONE_MINUS_DST_COLOR: return WGPUBlendFactor_OneMinusDst;
        case RT_BLEND_SRC_ALPHA_SATURATE: return WGPUBlendFactor_SrcAlphaSaturated;
        case RT_BLEND_CONSTANT_COLOR: return WGPUBlendFactor_Constant;
        case RT_BLEND_ONE_MINUS_CONSTANT_COLOR: return WGPUBlendFactor_OneMinusConstant;
        case RT_BLEND_CONSTANT_ALPHA: return WGPUBlendFactor_One;
        case RT_BLEND_ONE_MINUS_CONSTANT_ALPHA: return WGPUBlendFactor_One;
        default: return WGPUBlendFactor_One;
    }
}

static WGPUBlendOperation rt_to_wg_blend_op(rt_blend_op_t func)
{
    switch (func)
    {
        case RT_FUNC_ADD: return WGPUBlendOperation_Add;
        case RT_MIN: return WGPUBlendOperation_Min;
        case RT_MAX: return WGPUBlendOperation_Max;
        case RT_FUNC_SUBTRACT: return WGPUBlendOperation_Subtract;
        case RT_FUNC_REVERSE_SUBTRACT: return WGPUBlendOperation_ReverseSubtract;
        default: return WGPUBlendOperation_Add;
    }
}

static WGPUCullMode rt_to_wg_cull(rt_cull_mode_t mode)
{
    switch (mode)
    {
        case RT_CULL_NONE: return WGPUCullMode_None;
        case RT_CULL_FRONT: return WGPUCullMode_Front;
        case RT_CULL_BACK: return WGPUCullMode_Back;
        case RT_CULL_FRONT_AND_BACK: return WGPUCullMode_None;
        default: return WGPUCullMode_None;
    }
}

static WGPUPrimitiveTopology rt_to_wg_primitive(rt_primitive_t primitive)
{
    switch (primitive)
    {
        case RT_POINTS: return WGPUPrimitiveTopology_PointList;
        case RT_LINES: return WGPUPrimitiveTopology_LineList;
        case RT_LINE_STRIP: return WGPUPrimitiveTopology_LineStrip;
        case RT_TRIANGLES: return WGPUPrimitiveTopology_TriangleList;
        case RT_TRIANGLE_STRIP: return WGPUPrimitiveTopology_TriangleStrip;
        default: return WGPUPrimitiveTopology_TriangleList;
    }
}

static uint32_t rt_to_wg_vertex_size(rt_vertex_format_t format)
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

static uint32_t rt_to_wg_index_size(rt_index_type_t type)
{
    switch (type)
    {
        case RT_INDEX_UINT16: return 2;
        case RT_INDEX_UINT32: return 4;
        default: return 4;
    }
}

static WGPUIndexFormat rt_to_wg_index_type(rt_index_type_t type)
{
    switch (type)
    {
        case RT_INDEX_UINT16: return WGPUIndexFormat_Uint16;
        case RT_INDEX_UINT32: return WGPUIndexFormat_Uint32;
        default: return WGPUIndexFormat_Uint32;
    }
}

static uint32_t wg_format_bytes(WGPUTextureFormat format)
{
    switch (format)
    {
        case WGPUTextureFormat_R8Unorm:
        case WGPUTextureFormat_R8Snorm:
        case WGPUTextureFormat_R8Uint:
        case WGPUTextureFormat_R8Sint:
            return 1;
        case WGPUTextureFormat_R16Unorm:
        case WGPUTextureFormat_R16Snorm:
        case WGPUTextureFormat_R16Uint:
        case WGPUTextureFormat_R16Sint:
        case WGPUTextureFormat_R16Float:
            return 2;
        case WGPUTextureFormat_RG8Unorm:
        case WGPUTextureFormat_RG8Snorm:
        case WGPUTextureFormat_RG8Uint:
        case WGPUTextureFormat_RG8Sint:
            return 2;
        case WGPUTextureFormat_R32Uint:
        case WGPUTextureFormat_R32Sint:
        case WGPUTextureFormat_R32Float:
            return 4;
        case WGPUTextureFormat_RG16Unorm:
        case WGPUTextureFormat_RG16Snorm:
        case WGPUTextureFormat_RG16Uint:
        case WGPUTextureFormat_RG16Sint:
        case WGPUTextureFormat_RG16Float:
            return 4;
        case WGPUTextureFormat_RGBA8Unorm:
        case WGPUTextureFormat_RGBA8UnormSrgb:
        case WGPUTextureFormat_RGBA8Snorm:
        case WGPUTextureFormat_RGBA8Uint:
        case WGPUTextureFormat_RGBA8Sint:
        case WGPUTextureFormat_BGRA8Unorm:
        case WGPUTextureFormat_BGRA8UnormSrgb:
        case WGPUTextureFormat_RGB10A2Uint:
        case WGPUTextureFormat_RGB10A2Unorm:
        case WGPUTextureFormat_RG11B10Ufloat:
        case WGPUTextureFormat_RGB9E5Ufloat:
            return 4;
        case WGPUTextureFormat_RG32Uint:
        case WGPUTextureFormat_RG32Sint:
        case WGPUTextureFormat_RG32Float:
            return 8;
        case WGPUTextureFormat_RGBA16Unorm:
        case WGPUTextureFormat_RGBA16Snorm:
        case WGPUTextureFormat_RGBA16Uint:
        case WGPUTextureFormat_RGBA16Sint:
        case WGPUTextureFormat_RGBA16Float:
            return 8;
        case WGPUTextureFormat_RGBA32Uint:
        case WGPUTextureFormat_RGBA32Sint:
        case WGPUTextureFormat_RGBA32Float:
            return 16;
        case WGPUTextureFormat_Stencil8:
            return 1;
        case WGPUTextureFormat_Depth16Unorm:
            return 2;
        case WGPUTextureFormat_Depth24Plus:
        case WGPUTextureFormat_Depth24PlusStencil8:
        case WGPUTextureFormat_Depth32Float:
            return 4;
        case WGPUTextureFormat_Depth32FloatStencil8:
            return 8;
        default: return 4;
    }
}

static bool wg_is_depth(WGPUTextureFormat format)
{
    return format == WGPUTextureFormat_Stencil8 || format == WGPUTextureFormat_Depth16Unorm ||
           format == WGPUTextureFormat_Depth24Plus || format == WGPUTextureFormat_Depth24PlusStencil8 ||
           format == WGPUTextureFormat_Depth32Float || format == WGPUTextureFormat_Depth32FloatStencil8;
}

struct wg_buffer_native_t
{
    WGPUBuffer handle = nullptr;
    wg_res_state_t state = WG_STATE_UNKNOWN;
    std::vector<uint8_t> mappedCpu;
    void* mapped = nullptr;
    size_t mappedOffset = 0;
    size_t mappedSize = 0;
    ~wg_buffer_native_t() { if (handle) wgpuBufferRelease(handle); }
};

struct wg_texture_native_t
{
    WGPUTexture handle = nullptr;
    WGPUTextureFormat format = WGPUTextureFormat_Undefined;
    wg_res_state_t state = WG_STATE_UNKNOWN;
    uint32_t levels = 1;
    uint32_t layers = 1;
    uint32_t width = 1, height = 1, depth = 1;
    rt_texture_target_t target = RT_TEXTURE_2D;
    rt_texture_format_t rtFormat = RT_TEXTURE_NONE;
    rt_texture_usages_t usage = 0;
    rt_texture_sample_t samples = RT_TEXTURE_SAMPLE_1X;
    ~wg_texture_native_t()
    {
        if (handle) wgpuTextureRelease(handle);
    }
};

struct wg_texture_view_native_t
{
    WGPUTextureView handle = nullptr;
    uint32_t texture = 0;

    wg_texture_view_native_t() = default;
    wg_texture_view_native_t(const wg_texture_view_native_t&) = delete;
    wg_texture_view_native_t& operator=(const wg_texture_view_native_t&) = delete;
    wg_texture_view_native_t(wg_texture_view_native_t&& other) noexcept : handle(other.handle), texture(other.texture)
    {
        other.handle = nullptr;
    }
    wg_texture_view_native_t& operator=(wg_texture_view_native_t&& other) noexcept
    {
        if (this != &other)
        {
            if (handle) wgpuTextureViewRelease(handle);
            handle = other.handle;
            texture = other.texture;
            other.handle = nullptr;
        }
        return *this;
    }
    ~wg_texture_view_native_t() { if (handle) wgpuTextureViewRelease(handle); }
};

struct wg_sampler_native_t
{
    WGPUSampler handle = nullptr;
    ~wg_sampler_native_t() { if (handle) wgpuSamplerRelease(handle); }
};

struct wg_module_native_t
{
    WGPUShaderModule vshader = nullptr;
    WGPUShaderModule tshader = nullptr;
    WGPUShaderModule mshader = nullptr;
    WGPUShaderModule fshader = nullptr;
    WGPUShaderModule cshader = nullptr;
    WGPURenderPipeline renderPipeline = nullptr;
    WGPUComputePipeline computePipeline = nullptr;
    WGPUBindGroupLayout bindGroupLayout = nullptr;
    WGPUPipelineLayout pipelineLayout = nullptr;
    WGPUBindGroup bindGroup = nullptr;
    WGPUPrimitiveTopology topology = WGPUPrimitiveTopology_TriangleList;
    wg_binding_kind_t kinds[RT_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t descriptorBindings[RT_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t descriptorCount = 0;
};

struct wg_mesh_native_t { uint32_t vertexCount = 0, indexCount = 0; };
struct wg_meshlet_native_t { uint32_t vertexCount = 0, indexCount = 0; };
struct wg_pass_compute_native_t { uint32_t dummy = 0; };
struct wg_pass_transfer_native_t { uint32_t dummy = 0; };

struct wg_native_t
{
    uint32_t bufferID = 0, textureID = 0, textureViewID = 0, samplerID = 0, moduleID = 0;
    uint32_t meshID = 0, meshletID = 0, passID = 0;

    std::map<uint32_t, wg_buffer_native_t> buffers;
    std::map<uint32_t, wg_texture_native_t> textures;
    std::map<uint32_t, wg_texture_view_native_t> textureViews;
    std::map<uint32_t, wg_sampler_native_t> samplers;
    std::map<uint32_t, wg_module_native_t> modules;
    std::map<uint32_t, wg_mesh_native_t> meshes;
    std::map<uint32_t, wg_meshlet_native_t> meshlets;
    std::map<uint32_t, wg_pass_compute_native_t> computePasses;
    std::map<uint32_t, wg_pass_transfer_native_t> transferPasses;

    WGPUDevice device = nullptr;
    WGPUQueue queue = nullptr;
    WGPUCommandEncoder encoder = nullptr;
    WGPURenderPassEncoder renderPass = nullptr;
    WGPUComputePassEncoder computePass = nullptr;
    WGPUBuffer pushBuffer = nullptr;
    uint8_t pushData[256] = {};
    uint32_t pushLength = 0;

    struct
    {
        rt_binding_type_t type = RT_BINDING_NONE;
        rt_buffer_t buffer = {};
        rt_buffer_bind_t buffer_bind = {};
        rt_texture_view_t texture_view = {};
        rt_texture_bind_t texture_bind = {};
        rt_texture_view_t storage_view = {};
        rt_texture_storage_bind_t storage_texture_bind = {};
        rt_sampler_t sampler = {};
        rt_sampler_bind_t sampler_bind = {};
    } currentBinding[RT_MAX_BINDING_HANDLE_NUM] = {};

    rt_module_type_t currentPassType = RT_MODULE_NONE;
    union
    {
        void* currentPipeline = nullptr;
        rt_pass_render_t* currentRenderPass;
        rt_pass_compute_t* currentComputePass;
        rt_pass_transfer_t* currentTransferPass;
    };
} static webgpu;

static WGPUStringView wg_string(char const* text)
{
    WGPUStringView view = {};
    view.data = text;
    view.length = WGPU_STRLEN;
    return view;
}

static WGPUShaderModule wg_create_shader(const char* data, uint32_t length)
{
    if (!data || !length || !webgpu.device) return nullptr;
    std::string source(data, length);
    WGPUShaderSourceWGSL wgsl = {};
    wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgsl.code.data = source.c_str();
    wgsl.code.length = source.size();
    WGPUShaderModuleDescriptor desc = {};
    desc.nextInChain = &wgsl.chain;
    return wgpuDeviceCreateShaderModule(webgpu.device, &desc);
}

static WGPUBufferUsage wg_buffer_usage(rt_buffer_usages_t usage)
{
    WGPUBufferUsage flags = WGPUBufferUsage_None;
    if (usage & RT_BUFFER_USAGE_MAP_READ) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst);
    if (usage & RT_BUFFER_USAGE_MAP_WRITE) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc);
    if (usage & RT_BUFFER_USAGE_COPY_SRC) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_CopySrc);
    if (usage & RT_BUFFER_USAGE_COPY_DST) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_CopyDst);
    if (usage & RT_BUFFER_USAGE_INDEX) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_Index);
    if (usage & RT_BUFFER_USAGE_VERTEX) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_Vertex);
    if (usage & RT_BUFFER_USAGE_UNIFORM) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_Uniform);
    if (usage & RT_BUFFER_USAGE_STORAGE) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_Storage);
    if (usage & RT_BUFFER_USAGE_INDIRECT) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_Indirect);
    return flags;
}

static bool wg_storage_format(WGPUTextureFormat format)
{
    switch (format)
    {
        case WGPUTextureFormat_R8Unorm:
        case WGPUTextureFormat_R8Snorm:
        case WGPUTextureFormat_R8Uint:
        case WGPUTextureFormat_R8Sint:
        case WGPUTextureFormat_R16Uint:
        case WGPUTextureFormat_R16Sint:
        case WGPUTextureFormat_R16Float:
        case WGPUTextureFormat_RG8Unorm:
        case WGPUTextureFormat_RG8Snorm:
        case WGPUTextureFormat_RG8Uint:
        case WGPUTextureFormat_RG8Sint:
        case WGPUTextureFormat_R32Uint:
        case WGPUTextureFormat_R32Sint:
        case WGPUTextureFormat_R32Float:
        case WGPUTextureFormat_RG16Uint:
        case WGPUTextureFormat_RG16Sint:
        case WGPUTextureFormat_RG16Float:
        case WGPUTextureFormat_RGBA8Unorm:
        case WGPUTextureFormat_RGBA8Snorm:
        case WGPUTextureFormat_RGBA8Uint:
        case WGPUTextureFormat_RGBA8Sint:
        case WGPUTextureFormat_RGBA16Uint:
        case WGPUTextureFormat_RGBA16Sint:
        case WGPUTextureFormat_RGBA16Float:
        case WGPUTextureFormat_RG32Uint:
        case WGPUTextureFormat_RG32Sint:
        case WGPUTextureFormat_RG32Float:
        case WGPUTextureFormat_RGBA32Uint:
        case WGPUTextureFormat_RGBA32Sint:
        case WGPUTextureFormat_RGBA32Float:
            return true;
        default:
            return false;
    }
}

static WGPUTextureUsage wg_texture_usage(rt_texture_usages_t usage, WGPUTextureFormat format, rt_texture_sample_t samples)
{
    if (samples != RT_TEXTURE_SAMPLE_1X || !wg_storage_format(format))
        usage &= ~RT_TEXTURE_USAGE_STORAGE_BINDING;
    WGPUTextureUsage flags = WGPUTextureUsage_None;
    if (usage & RT_TEXTURE_USAGE_COPY_SRC) flags = (WGPUTextureUsage)(flags | WGPUTextureUsage_CopySrc);
    if (usage & RT_TEXTURE_USAGE_COPY_DST) flags = (WGPUTextureUsage)(flags | WGPUTextureUsage_CopyDst);
    if (usage & RT_TEXTURE_USAGE_TEXTURE_BINDING) flags = (WGPUTextureUsage)(flags | WGPUTextureUsage_TextureBinding);
    if (usage & RT_TEXTURE_USAGE_STORAGE_BINDING) flags = (WGPUTextureUsage)(flags | WGPUTextureUsage_StorageBinding);
    if (usage & RT_TEXTURE_USAGE_RENDER_ATTACHMENT) flags = (WGPUTextureUsage)(flags | WGPUTextureUsage_RenderAttachment);
    return flags;
}

static void wg_end_pass_encoders()
{
    if (webgpu.renderPass)
    {
        wgpuRenderPassEncoderEnd(webgpu.renderPass);
        wgpuRenderPassEncoderRelease(webgpu.renderPass);
        webgpu.renderPass = nullptr;
    }
    if (webgpu.computePass)
    {
        wgpuComputePassEncoderEnd(webgpu.computePass);
        wgpuComputePassEncoderRelease(webgpu.computePass);
        webgpu.computePass = nullptr;
    }
}

static void wg_ensure_encoder()
{
    if (!webgpu.encoder && webgpu.device)
        webgpu.encoder = wgpuDeviceCreateCommandEncoder(webgpu.device, nullptr);
}

static void wg_transition_buffer(wg_buffer_native_t& buffer, wg_res_state_t dst)
{
    if (!buffer.handle || buffer.state == dst) return;
    buffer.state = dst;
}

static void wg_transition_image(wg_texture_native_t& image, wg_res_state_t dst)
{
    if (!image.handle || image.state == dst) return;
    image.state = dst;
}

static wg_buffer_native_t* wg_buffer_native(rt_buffer_t const& buffer)
{
    if (!buffer.native || buffer.handle == 0) return nullptr;
    auto it = webgpu.buffers.find(buffer.handle);
    return it == webgpu.buffers.end() ? nullptr : &it->second;
}

static wg_texture_native_t* wg_texture_native(rt_texture_t const& texture)
{
    if (!texture.native || texture.handle == 0) return nullptr;
    auto it = webgpu.textures.find(texture.handle);
    return it == webgpu.textures.end() ? nullptr : &it->second;
}

static wg_texture_view_native_t* wg_texture_view_native(uint32_t handle)
{
    if (handle == 0) return nullptr;
    auto it = webgpu.textureViews.find(handle);
    return it == webgpu.textureViews.end() ? nullptr : &it->second;
}

static WGPUTextureView wg_image_view(rt_texture_view_t const& view)
{
    auto* native = wg_texture_view_native(view.handle);
    return native ? native->handle : nullptr;
}

static wg_sampler_native_t* wg_sampler_native(rt_sampler_t const& sampler)
{
    if (!sampler.native || sampler.handle == 0) return nullptr;
    auto it = webgpu.samplers.find(sampler.handle);
    return it == webgpu.samplers.end() ? nullptr : &it->second;
}

static wg_module_native_t* wg_current_module_native()
{
    if (webgpu.currentPassType == RT_MODULE_COMPUTE && webgpu.currentComputePass)
        return (wg_module_native_t*)webgpu.currentComputePass->module.native;
    if (webgpu.currentPassType == RT_MODULE_RENDER && webgpu.currentRenderPass)
        return (wg_module_native_t*)webgpu.currentRenderPass->module.native;
    return nullptr;
}

static bool wg_create_pipeline_layout(wg_module_native_t& native, rt_binding_t const* bindings)
{
    WGPUBindGroupLayoutEntry entries[RT_MAX_BINDING_HANDLE_NUM + 1] = {};
    native.descriptorCount = 0;
    WGPUShaderStage visibility = (WGPUShaderStage)(WGPUShaderStage_Vertex | WGPUShaderStage_Fragment | WGPUShaderStage_Compute);
    if (bindings)
    {
        for (uint32_t i = 0; i < RT_MAX_BINDING_HANDLE_NUM; ++i)
        {
            if (bindings[i].type == RT_BINDING_NONE) continue;
            auto& entry = entries[native.descriptorCount];
            entry.binding = bindings[i].binding;
            entry.visibility = visibility;
            if (bindings[i].type == RT_BINDING_UNIFORM_BUFFER)
            {
                native.kinds[native.descriptorCount] = WG_KIND_UNIFORM;
                entry.buffer.type = WGPUBufferBindingType_Uniform;
            }
            else if (bindings[i].type == RT_BINDING_STORAGE_BUFFER)
            {
                native.kinds[native.descriptorCount] = WG_KIND_STORAGE;
                entry.buffer.type = WGPUBufferBindingType_Storage;
            }
            else if (bindings[i].type == RT_BINDING_TEXTURE)
            {
                native.kinds[native.descriptorCount] = WG_KIND_TEXTURE;
                entry.texture.sampleType = WGPUTextureSampleType_Float;
                entry.texture.viewDimension = WGPUTextureViewDimension_2D;
            }
            else if (bindings[i].type == RT_BINDING_STORAGE_TEXTURE)
            {
                native.kinds[native.descriptorCount] = WG_KIND_STORAGE_TEXTURE;
                entry.storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
                entry.storageTexture.format = WGPUTextureFormat_RGBA8Unorm;
                entry.storageTexture.viewDimension = WGPUTextureViewDimension_2D;
            }
            else if (bindings[i].type == RT_BINDING_SAMPLER)
            {
                native.kinds[native.descriptorCount] = WG_KIND_SAMPLER;
                entry.sampler.type = WGPUSamplerBindingType_Filtering;
            }
            else
                continue;
            native.descriptorBindings[native.descriptorCount] = bindings[i].binding;
            native.descriptorCount++;
        }
    }
    auto& push = entries[native.descriptorCount];
    push.binding = 16;
    push.visibility = visibility;
    push.buffer.type = WGPUBufferBindingType_Uniform;
    push.buffer.minBindingSize = 256;
    uint32_t entryCount = native.descriptorCount + 1;

    WGPUBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entryCount = entryCount;
    layoutDesc.entries = entries;
    native.bindGroupLayout = wgpuDeviceCreateBindGroupLayout(webgpu.device, &layoutDesc);
    if (!native.bindGroupLayout) return false;

    WGPUPipelineLayoutDescriptor pipeDesc = {};
    pipeDesc.bindGroupLayoutCount = 1;
    pipeDesc.bindGroupLayouts = &native.bindGroupLayout;
    native.pipelineLayout = wgpuDeviceCreatePipelineLayout(webgpu.device, &pipeDesc);
    return native.pipelineLayout != nullptr;
}

static void wg_destroy_module_native(uint32_t handle, void*& native)
{
    auto it = webgpu.modules.find(handle);
    if (it != webgpu.modules.end())
    {
        auto& module = it->second;
        if (module.bindGroup) wgpuBindGroupRelease(module.bindGroup);
        if (module.renderPipeline) wgpuRenderPipelineRelease(module.renderPipeline);
        if (module.computePipeline) wgpuComputePipelineRelease(module.computePipeline);
        if (module.pipelineLayout) wgpuPipelineLayoutRelease(module.pipelineLayout);
        if (module.bindGroupLayout) wgpuBindGroupLayoutRelease(module.bindGroupLayout);
        if (module.vshader) wgpuShaderModuleRelease(module.vshader);
        if (module.tshader) wgpuShaderModuleRelease(module.tshader);
        if (module.mshader) wgpuShaderModuleRelease(module.mshader);
        if (module.fshader) wgpuShaderModuleRelease(module.fshader);
        if (module.cshader) wgpuShaderModuleRelease(module.cshader);
        webgpu.modules.erase(it);
    }
    native = nullptr;
}

static void wg_flush_descriptors()
{
    auto* mod = wg_current_module_native();
    if (!mod || !mod->bindGroupLayout) return;
    if (mod->bindGroup)
    {
        wgpuBindGroupRelease(mod->bindGroup);
        mod->bindGroup = nullptr;
    }

    WGPUBindGroupEntry entries[RT_MAX_BINDING_HANDLE_NUM + 1] = {};
    uint32_t count = 0;
    for (uint32_t i = 0; i < mod->descriptorCount; ++i)
    {
        uint32_t binding = mod->descriptorBindings[i];
        auto& slot = webgpu.currentBinding[binding];
        entries[count].binding = binding;
        if (mod->kinds[i] == WG_KIND_UNIFORM || mod->kinds[i] == WG_KIND_STORAGE)
        {
            auto* buf = wg_buffer_native(slot.buffer);
            if (!buf) continue;
            bool storage = (mod->kinds[i] == WG_KIND_STORAGE);
            wg_transition_buffer(*buf, storage ? WG_STATE_SHADER_WRITE : WG_STATE_SHADER_READ);
            entries[count].buffer = buf->handle;
            entries[count].offset = 0;
            entries[count].size = slot.buffer.size;
        }
        else if (mod->kinds[i] == WG_KIND_TEXTURE)
        {
            auto* view = wg_texture_view_native(slot.texture_view.handle);
            if (!view || !view->handle) continue;
            auto texIt = webgpu.textures.find(view->texture);
            if (texIt == webgpu.textures.end()) continue;
            auto* tex = &texIt->second;
            WGPUTextureView gpuView = view->handle;
            wg_transition_image(*tex, WG_STATE_SHADER_READ);
            entries[count].textureView = gpuView;
        }
        else if (mod->kinds[i] == WG_KIND_STORAGE_TEXTURE)
        {
            auto* view = wg_texture_view_native(slot.storage_view.handle);
            if (!view || !view->handle) continue;
            auto texIt = webgpu.textures.find(view->texture);
            if (texIt == webgpu.textures.end()) continue;
            auto* tex = &texIt->second;
            wg_transition_image(*tex, WG_STATE_SHADER_WRITE);
            entries[count].textureView = view->handle;
        }
        else if (mod->kinds[i] == WG_KIND_SAMPLER)
        {
            auto* samp = wg_sampler_native(slot.sampler);
            if (!samp || !samp->handle) continue;
            entries[count].sampler = samp->handle;
        }
        else
            continue;
        count++;
    }
    entries[count].binding = 16;
    entries[count].buffer = webgpu.pushBuffer;
    entries[count].offset = 0;
    entries[count].size = 256;
    count++;

    WGPUBindGroupDescriptor desc = {};
    desc.layout = mod->bindGroupLayout;
    desc.entryCount = count;
    desc.entries = entries;
    mod->bindGroup = wgpuDeviceCreateBindGroup(webgpu.device, &desc);
    if (!mod->bindGroup) return;
    if (webgpu.renderPass)
        wgpuRenderPassEncoderSetBindGroup(webgpu.renderPass, 0, mod->bindGroup, 0, nullptr);
    if (webgpu.computePass)
        wgpuComputePassEncoderSetBindGroup(webgpu.computePass, 0, mod->bindGroup, 0, nullptr);
}

void wg_load_library(WGPUDevice device, WGPUQueue queue)
{
    webgpu.device = device;
    webgpu.queue = queue;
    if (!webgpu.device)
    {
        fprintf(stderr, "WebGPU: device is null\n");
        abort();
    }
    if (!webgpu.queue)
    {
        fprintf(stderr, "WebGPU: queue is null\n");
        abort();
    }
    wg_ensure_encoder();

    WGPUBufferDescriptor push = {};
    push.size = 256;
    push.usage = (WGPUBufferUsage)(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    webgpu.pushBuffer = wgpuDeviceCreateBuffer(webgpu.device, &push);

    rt_unload_library = wg_unload_library;
    rt_create_buffer = wg_create_buffer;
    rt_destroy_buffer = wg_destroy_buffer;
    rt_bind_buffer = wg_bind_buffer;
    rt_map_buffer = wg_map_buffer;
    rt_unmap_buffer = wg_unmap_buffer;
    rt_create_texture = wg_create_texture;
    rt_destroy_texture = wg_destroy_texture;
    rt_bind_texture = wg_bind_texture;
    rt_create_texture_view = wg_create_texture_view;
    rt_destroy_texture_view = wg_destroy_texture_view;
    rt_bind_texture_view = wg_bind_texture_view;
    rt_bind_texture_storage = wg_bind_texture_storage;
    rt_create_sampler = wg_create_sampler;
    rt_destroy_sampler = wg_destroy_sampler;
    rt_bind_sampler = wg_bind_sampler;
    rt_create_module_compute = wg_create_module_compute;
    rt_create_module_render = wg_create_module_render;
    rt_create_module_meshlet = wg_create_module_meshlet;
    rt_destroy_module_render = wg_destroy_module_render;
    rt_destroy_module_compute = wg_destroy_module_compute;
    rt_begin_compute = wg_begin_compute;
    rt_end_compute = wg_end_compute;
    rt_dispatch_compute = wg_dispatch_compute;
    rt_dispatch_compute_indirect = wg_dispatch_compute_indirect;
    rt_begin_render = wg_begin_render;
    rt_end_render = wg_end_render;
    rt_set_viewport = wg_set_viewport;
    rt_set_scissor = wg_set_scissor;
    rt_draw_mesh_task = wg_draw_mesh_task;
    rt_push_constant = wg_push_constant;
    rt_push_const_int = wg_push_const_int;
    rt_push_const_uint = wg_push_const_uint;
    rt_push_const_float = wg_push_const_float;
    rt_push_const_vec2 = wg_push_const_vec2;
    rt_push_const_vec3 = wg_push_const_vec3;
    rt_push_const_vec4 = wg_push_const_vec4;
    rt_push_const_mat3 = wg_push_const_mat3;
    rt_push_const_mat4 = wg_push_const_mat4;
    rt_begin_transfer = wg_begin_transfer;
    rt_end_transfer = wg_end_transfer;
    rt_copy_buffer = wg_copy_buffer;
    rt_copy_buffer_data = wg_copy_buffer_data;
    rt_copy_buffer_texture = wg_copy_buffer_texture;
    rt_copy_texture = wg_copy_texture;
    rt_copy_texture_data = wg_copy_texture_data;
    rt_copy_texture_buffer = wg_copy_texture_buffer;
    rt_create_mesh = wg_create_mesh;
    rt_destroy_mesh = wg_destroy_mesh;
    rt_draw_mesh = wg_draw_mesh;
    rt_draw_array = wg_draw_array;
    rt_draw_index = wg_draw_index;
    rt_draw_array_indirect = wg_draw_array_indirect;
    rt_draw_index_indirect = wg_draw_index_indirect;
    rt_draw_mesh_multi = wg_draw_mesh_multi;
    rt_create_meshlet = wg_create_meshlet;
    rt_destroy_meshlet = wg_destroy_meshlet;
    rt_draw_meshlet = wg_draw_meshlet;
    rt_submit = wg_submit;
}

void wg_unload_library()
{
    wg_end_pass_encoders();
    if (webgpu.encoder)
    {
        wgpuCommandEncoderRelease(webgpu.encoder);
        webgpu.encoder = nullptr;
    }
    while (!webgpu.modules.empty())
    {
        uint32_t handle = webgpu.modules.begin()->first;
        void* dummy = nullptr;
        wg_destroy_module_native(handle, dummy);
    }
    for (auto& item : webgpu.buffers)
    {
        if (item.second.handle)
        {
            wgpuBufferRelease(item.second.handle);
            item.second.handle = nullptr;
        }
    }
    webgpu.buffers.clear();
    webgpu.textureViews.clear();
    for (auto& item : webgpu.textures)
    {
        if (item.second.handle)
        {
            wgpuTextureRelease(item.second.handle);
            item.second.handle = nullptr;
        }
    }
    webgpu.textures.clear();
    for (auto& item : webgpu.samplers)
    {
        if (item.second.handle)
        {
            wgpuSamplerRelease(item.second.handle);
            item.second.handle = nullptr;
        }
    }
    webgpu.samplers.clear();
    webgpu.meshes.clear();
    webgpu.meshlets.clear();
    webgpu.computePasses.clear();
    webgpu.transferPasses.clear();
    if (webgpu.pushBuffer) { wgpuBufferRelease(webgpu.pushBuffer); webgpu.pushBuffer = nullptr; }
    webgpu.queue = nullptr;
    webgpu.device = nullptr;
    webgpu.bufferID = webgpu.textureID = webgpu.textureViewID = webgpu.samplerID = webgpu.moduleID = 0;
    webgpu.meshID = webgpu.meshletID = webgpu.passID = 0;
    webgpu.currentPassType = RT_MODULE_NONE;
    webgpu.currentPipeline = nullptr;

    if (rt_unload_library == wg_unload_library) rt_unload_library = nullptr;
    if (rt_create_buffer == wg_create_buffer) rt_create_buffer = nullptr;
    if (rt_destroy_buffer == wg_destroy_buffer) rt_destroy_buffer = nullptr;
    if (rt_bind_buffer == wg_bind_buffer) rt_bind_buffer = nullptr;
    if (rt_map_buffer == wg_map_buffer) rt_map_buffer = nullptr;
    if (rt_unmap_buffer == wg_unmap_buffer) rt_unmap_buffer = nullptr;
    if (rt_create_texture == wg_create_texture) rt_create_texture = nullptr;
    if (rt_destroy_texture == wg_destroy_texture) rt_destroy_texture = nullptr;
    if (rt_bind_texture == wg_bind_texture) rt_bind_texture = nullptr;
    if (rt_create_texture_view == wg_create_texture_view) rt_create_texture_view = nullptr;
    if (rt_destroy_texture_view == wg_destroy_texture_view) rt_destroy_texture_view = nullptr;
    if (rt_bind_texture_view == wg_bind_texture_view) rt_bind_texture_view = nullptr;
    if (rt_bind_texture_storage == wg_bind_texture_storage) rt_bind_texture_storage = nullptr;
    if (rt_create_sampler == wg_create_sampler) rt_create_sampler = nullptr;
    if (rt_destroy_sampler == wg_destroy_sampler) rt_destroy_sampler = nullptr;
    if (rt_bind_sampler == wg_bind_sampler) rt_bind_sampler = nullptr;
    if (rt_create_module_compute == wg_create_module_compute) rt_create_module_compute = nullptr;
    if (rt_create_module_render == wg_create_module_render) rt_create_module_render = nullptr;
    if (rt_create_module_meshlet == wg_create_module_meshlet) rt_create_module_meshlet = nullptr;
    if (rt_destroy_module_render == wg_destroy_module_render) rt_destroy_module_render = nullptr;
    if (rt_destroy_module_compute == wg_destroy_module_compute) rt_destroy_module_compute = nullptr;
    if (rt_begin_compute == wg_begin_compute) rt_begin_compute = nullptr;
    if (rt_end_compute == wg_end_compute) rt_end_compute = nullptr;
    if (rt_dispatch_compute == wg_dispatch_compute) rt_dispatch_compute = nullptr;
    if (rt_dispatch_compute_indirect == wg_dispatch_compute_indirect) rt_dispatch_compute_indirect = nullptr;
    if (rt_begin_render == wg_begin_render) rt_begin_render = nullptr;
    if (rt_end_render == wg_end_render) rt_end_render = nullptr;
    if (rt_set_viewport == wg_set_viewport) rt_set_viewport = nullptr;
    if (rt_set_scissor == wg_set_scissor) rt_set_scissor = nullptr;
    if (rt_draw_mesh_task == wg_draw_mesh_task) rt_draw_mesh_task = nullptr;
    if (rt_push_constant == wg_push_constant) rt_push_constant = nullptr;
    if (rt_push_const_int == wg_push_const_int) rt_push_const_int = nullptr;
    if (rt_push_const_uint == wg_push_const_uint) rt_push_const_uint = nullptr;
    if (rt_push_const_float == wg_push_const_float) rt_push_const_float = nullptr;
    if (rt_push_const_vec2 == wg_push_const_vec2) rt_push_const_vec2 = nullptr;
    if (rt_push_const_vec3 == wg_push_const_vec3) rt_push_const_vec3 = nullptr;
    if (rt_push_const_vec4 == wg_push_const_vec4) rt_push_const_vec4 = nullptr;
    if (rt_push_const_mat3 == wg_push_const_mat3) rt_push_const_mat3 = nullptr;
    if (rt_push_const_mat4 == wg_push_const_mat4) rt_push_const_mat4 = nullptr;
    if (rt_begin_transfer == wg_begin_transfer) rt_begin_transfer = nullptr;
    if (rt_end_transfer == wg_end_transfer) rt_end_transfer = nullptr;
    if (rt_copy_buffer == wg_copy_buffer) rt_copy_buffer = nullptr;
    if (rt_copy_buffer_data == wg_copy_buffer_data) rt_copy_buffer_data = nullptr;
    if (rt_copy_buffer_texture == wg_copy_buffer_texture) rt_copy_buffer_texture = nullptr;
    if (rt_copy_texture == wg_copy_texture) rt_copy_texture = nullptr;
    if (rt_copy_texture_data == wg_copy_texture_data) rt_copy_texture_data = nullptr;
    if (rt_copy_texture_buffer == wg_copy_texture_buffer) rt_copy_texture_buffer = nullptr;
    if (rt_create_mesh == wg_create_mesh) rt_create_mesh = nullptr;
    if (rt_destroy_mesh == wg_destroy_mesh) rt_destroy_mesh = nullptr;
    if (rt_draw_mesh == wg_draw_mesh) rt_draw_mesh = nullptr;
    if (rt_draw_array == wg_draw_array) rt_draw_array = nullptr;
    if (rt_draw_index == wg_draw_index) rt_draw_index = nullptr;
    if (rt_draw_array_indirect == wg_draw_array_indirect) rt_draw_array_indirect = nullptr;
    if (rt_draw_index_indirect == wg_draw_index_indirect) rt_draw_index_indirect = nullptr;
    if (rt_draw_mesh_multi == wg_draw_mesh_multi) rt_draw_mesh_multi = nullptr;
    if (rt_create_meshlet == wg_create_meshlet) rt_create_meshlet = nullptr;
    if (rt_destroy_meshlet == wg_destroy_meshlet) rt_destroy_meshlet = nullptr;
    if (rt_draw_meshlet == wg_draw_meshlet) rt_draw_meshlet = nullptr;
    if (rt_submit == wg_submit) rt_submit = nullptr;
}

rt_buffer_t wg_create_buffer(rt_buffer_info_t const& info)
{
    if (info.usage == 0)
    {
        fprintf(stderr, "Buffer usage must not be 0");
        abort();
    }
    rt_buffer_t result = {};
    if (!webgpu.device || info.size == 0) return result;
    uint32_t handle = webgpu.bufferID + 1;
    auto& native = webgpu.buffers[handle];
    WGPUBufferDescriptor desc = {};
    desc.size = info.size;
    desc.usage = wg_buffer_usage(info.usage);
    if (info.data)
        desc.usage = (WGPUBufferUsage)(desc.usage | WGPUBufferUsage_CopyDst);
    desc.mappedAtCreation = info.data ? 1 : 0;
    native.handle = wgpuDeviceCreateBuffer(webgpu.device, &desc);
    if (!native.handle)
    {
        webgpu.buffers.erase(handle);
        return {};
    }
    if (info.data)
    {
        void* mapped = wgpuBufferGetMappedRange(native.handle, 0, info.size);
        if (mapped) std::memcpy(mapped, info.data, info.size);
        wgpuBufferUnmap(native.handle);
        native.state = WG_STATE_HOST;
    }
    if (info.usage & (RT_BUFFER_USAGE_MAP_READ | RT_BUFFER_USAGE_MAP_WRITE))
        native.mappedCpu.resize(info.size);

    webgpu.bufferID = handle;
    result.handle = handle;
    result.size = info.size;
    result.usage = info.usage;
    result.native = &native;
    return result;
}

void wg_destroy_buffer(rt_buffer_t& buffer)
{
    if (buffer.native)
    {
        auto it = webgpu.buffers.find(buffer.handle);
        if (it != webgpu.buffers.end())
        {
            webgpu.buffers.erase(it);
        }
    }
    buffer = {};
}

void wg_bind_buffer(rt_buffer_t& buffer, rt_buffer_bind_t bind)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    webgpu.currentBinding[bind.binding].buffer = buffer;
    webgpu.currentBinding[bind.binding].buffer_bind = bind;
}

void* wg_map_buffer(rt_buffer_t& buffer, rt_access_t mode, size_t offset, size_t size)
{
    (void)mode;
    auto* native = wg_buffer_native(buffer);
    if (!native || !native->handle) return nullptr;
    if (offset > buffer.size) return nullptr;
    if (size == 0) size = buffer.size - offset;
    if (size == 0 || offset + size > buffer.size) return nullptr;
    if (native->mappedCpu.size() < buffer.size)
        native->mappedCpu.resize(buffer.size);
    wg_transition_buffer(*native, WG_STATE_HOST);
    native->mapped = native->mappedCpu.data() + offset;
    native->mappedOffset = offset;
    native->mappedSize = size;
    return native->mapped;
}

void wg_unmap_buffer(rt_buffer_t& buffer)
{
    auto* native = wg_buffer_native(buffer);
    if (!native || !native->mapped) return;
    if (native->mappedSize && webgpu.queue)
        wgpuQueueWriteBuffer(webgpu.queue, native->handle, native->mappedOffset, native->mapped, native->mappedSize);
    native->mapped = nullptr;
    native->mappedOffset = 0;
    native->mappedSize = 0;
}

rt_texture_t wg_create_texture(rt_texture_info_t const& info)
{
    if (info.usage == 0)
    {
        fprintf(stderr, "Texture usage must not be 0");
        abort();
    }
    rt_texture_t result = {};
    if (!webgpu.device || info.width == 0) return result;
    if (info.target != RT_TEXTURE_1D && info.height == 0) return result;

    uint32_t handle = webgpu.textureID + 1;
    auto& native = webgpu.textures[handle];
    native.format = rt_to_wg_texture_format(info.format);
    native.rtFormat = info.format;
    native.usage = info.usage;
    native.width = info.width;
    native.height = info.target == RT_TEXTURE_1D ? 1 : info.height;
    native.depth = info.depth ? info.depth : 1;
    native.target = info.target;
    native.layers = (info.target == RT_TEXTURE_2D_ARRAY) ? native.depth : 1;
    rt_texture_sample_t samples = info.samples == RT_TEXTURE_SAMPLE_4X ? RT_TEXTURE_SAMPLE_4X : RT_TEXTURE_SAMPLE_1X;
    if (info.target == RT_TEXTURE_1D || info.target == RT_TEXTURE_3D || info.target == RT_TEXTURE_2D_ARRAY)
        samples = RT_TEXTURE_SAMPLE_1X;
    native.samples = samples;
    native.levels = 1;
    if (samples == RT_TEXTURE_SAMPLE_1X && info.mipmaps == 0 && rt_has_mipmap_filter(info.min_filter))
    {
        uint32_t maxDim = std::max(native.width, native.height);
        while (maxDim >>= 1) native.levels++;
    }
    else if (samples == RT_TEXTURE_SAMPLE_1X && info.mipmaps > 1)
        native.levels = info.mipmaps;

    WGPUTextureDescriptor desc = {};
    desc.size.width = native.width;
    desc.size.height = native.height;
    desc.size.depthOrArrayLayers = (info.target == RT_TEXTURE_3D) ? native.depth : native.layers;
    desc.mipLevelCount = native.levels;
    desc.sampleCount = rt_to_wg_sample_count(samples);
    desc.dimension = (info.target == RT_TEXTURE_3D) ? WGPUTextureDimension_3D :
                     (info.target == RT_TEXTURE_1D) ? WGPUTextureDimension_1D :
                     WGPUTextureDimension_2D;
    desc.format = native.format;
    desc.usage = wg_texture_usage(info.usage, native.format, samples);
    if (info.data && samples == RT_TEXTURE_SAMPLE_1X)
        desc.usage = (WGPUTextureUsage)(desc.usage | WGPUTextureUsage_CopyDst);
    native.handle = wgpuDeviceCreateTexture(webgpu.device, &desc);
    if (!native.handle)
    {
        webgpu.textures.erase(handle);
        return {};
    }

    if (info.data && samples == RT_TEXTURE_SAMPLE_1X)
    {
        size_t bpp = wg_format_bytes(native.format);
        size_t bytes = (size_t)native.width * native.height * ((info.target == RT_TEXTURE_3D) ? native.depth : 1) * bpp;
        WGPUTexelCopyTextureInfo dst = {};
        dst.texture = native.handle;
        WGPUTexelCopyBufferLayout layout = {};
        layout.bytesPerRow = (uint32_t)(native.width * bpp);
        layout.rowsPerImage = native.height;
        WGPUExtent3D size = {native.width, native.height, (info.target == RT_TEXTURE_3D) ? native.depth : 1};
        wgpuQueueWriteTexture(webgpu.queue, &dst, info.data, bytes, &layout, &size);
        wg_transition_image(native, WG_STATE_SHADER_READ);
    }
    else
        wg_transition_image(native, wg_is_depth(native.format) ? WG_STATE_DEPTH : WG_STATE_COLOR);

    if (native.samples == RT_TEXTURE_SAMPLE_4X && (native.target == RT_TEXTURE_2D || native.target == RT_TEXTURE_2D_MULTISAMPLE))
        native.target = RT_TEXTURE_2D_MULTISAMPLE;
    webgpu.textureID = handle;
    result.handle = handle;
    result.width = native.width;
    result.height = native.height;
    result.depth = native.depth;
    result.target = native.target;
    result.format = native.rtFormat;
    result.samples = native.samples;
    result.usage = native.usage;
    result.mipmaps = native.levels;
    result.native = &native;
    result.default_view = wg_create_texture_view(result, {
        .target = native.target,
        .format = native.rtFormat,
        .aspect = RT_TEXTURE_ASPECT_ALL,
        .usage = native.usage,
        .layer_count = native.layers,
        .level_count = native.levels,
    });
    return result;
}

void wg_destroy_texture(rt_texture_t& texture)
{
    if (texture.handle)
    {
        for (auto it = webgpu.textureViews.begin(); it != webgpu.textureViews.end(); )
        {
            if (it->second.texture == texture.handle)
                it = webgpu.textureViews.erase(it);
            else
                ++it;
        }
    }
    if (texture.native)
        webgpu.textures.erase(texture.handle);
    texture = {};
}

void wg_bind_texture(rt_texture_t& texture, rt_texture_bind_t bind)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    webgpu.currentBinding[bind.binding].type = RT_BINDING_TEXTURE;
    webgpu.currentBinding[bind.binding].texture_view = texture.default_view;
    webgpu.currentBinding[bind.binding].texture_bind = bind;
}

rt_texture_view_t wg_create_texture_view(rt_texture_t& texture, rt_texture_view_info_t const& info)
{
    auto* tex = wg_texture_native(texture);
    rt_texture_format_t format = info.format != RT_TEXTURE_NONE ? info.format : (tex ? tex->rtFormat : RT_TEXTURE_NONE);
    rt_texture_usages_t usage = info.usage ? info.usage : (tex ? tex->usage : 0);
    rt_texture_view_t result{
        0, info.target, format,
        info.aspect, usage,
        info.base_layer, info.layer_count, info.base_level, info.level_count, nullptr};
    if (!tex || !tex->handle) return result;
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

    WGPUTextureViewDescriptor desc = {};
    desc.format = info.format != RT_TEXTURE_NONE ? rt_to_wg_texture_format(info.format) : tex->format;
    if (info.target == RT_TEXTURE_1D) desc.dimension = WGPUTextureViewDimension_1D;
    else if (info.target == RT_TEXTURE_3D) desc.dimension = WGPUTextureViewDimension_3D;
    else if (info.target == RT_TEXTURE_2D_ARRAY || layerCount > 1) desc.dimension = WGPUTextureViewDimension_2DArray;
    else desc.dimension = WGPUTextureViewDimension_2D;
    desc.baseMipLevel = info.base_level;
    desc.mipLevelCount = levelCount;
    dewc.baseArrayLayer = baseLayer;
    desc.arrAyLayerCount = layerCounu;
    if (info,aspect = RT_TEXTURE_ASPECT_STENCIL) desc.aspect = WGPUTextureAspect_StencilOnly;
    else if (info.aspect == RT_TEXTURE_ASPECT_DEPTH) desc.aspect = WGPUTextureAspect_DepthOnly;
    else desc.aspect = WGPUTextureAspect_All;
    WGPUTextureView view = wgpuTextureCreateView(tex->handle, &desc);
    if (!view) return result;
    uint32_t handle = webgpu.textureViewID + 1;
    auto& native = webgpu.textureViews[handle];
    native.handle = view;
    native.texture = texture.handle;
    webgpu.textureViewID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void wg_destroy_texture_view(rt_texture_view_t& view)
{
    webgpu.textureViews.erase(view.handle);
    view.handle = 0;
    view.native = nullptr;
}

void wg_bind_texture_view(rt_texture_view_t& view, rt_texture_view_bind_t bind)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    webgpu.currentBinding[bind.binding].type = RT_BINDING_TEXTURE;
    webgpu.currentBinding[bind.binding].texture_view = view;
    webgpu.currentBinding[bind.binding].texture_bind = {};
    webgpu.currentBinding[bind.binding].texture_bind.binding = bind.binding;
}

void wg_bind_texture_storage(rt_texture_view_t& view, rt_texture_storage_bind_t bind)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    webgpu.currentBinding[bind.binding].type = RT_BINDING_STORAGE_TEXTURE;
    webgpu.currentBinding[bind.binding].storage_view = view;
    webgpu.currentBinding[bind.binding].storage_texture_bind = bind;
}

rt_sampler_t wg_create_sampler(rt_sampler_info_t const& info)
{
    rt_sampler_t result = {};
    uint32_t handle = webgpu.samplerID + 1;
    auto& native = webgpu.samplers[handle];
    WGPUSamplerDescriptor desc = {};
    desc.minFilter = rt_to_wg_filter(info.min_filter);
    desc.magFilter = rt_to_wg_filter(info.mag_filter);
    desc.mipmapFilter = rt_to_wg_mip(info.min_filter);
    desc.addressModeU = rt_to_wg_address(info.address_u);
    desc.addressModeV = rt_to_wg_address(info.address_v);
    desc.addressModeW = rt_to_wg_address(info.address_w);
    desc.maxAnisotropy = 1;
    native.handle = wgpuDeviceCreateSampler(webgpu.device, &desc);
    webgpu.samplerID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void wg_destroy_sampler(rt_sampler_t& sampler)
{
    if (sampler.native)
    {
        auto it = webgpu.samplers.find(sampler.handle);
        if (it != webgpu.samplers.end())
        {
            webgpu.samplers.erase(it);
        }
    }
    sampler = {};
}

void wg_bind_sampler(rt_sampler_t& sampler, rt_sampler_bind_t bind)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    webgpu.currentBinding[bind.binding].type = RT_BINDING_SAMPLER;
    webgpu.currentBinding[bind.binding].sampler = sampler;
    webgpu.currentBinding[bind.binding].sampler_bind = bind;
}

rt_module_compute_t wg_create_module_compute(rt_module_compute_info_t const& info)
{
    rt_module_compute_t result = {};
    if (!info.cshader.code || !info.cshader.size || !webgpu.device) return result;
    uint32_t handle = webgpu.moduleID + 1;
    auto& native = webgpu.modules[handle];
    native.cshader = wg_create_shader(info.cshader.code, info.cshader.size);
    if (!native.cshader || !wg_create_pipeline_layout(native, info.binding))
    {
        result.native = &native;
        wg_destroy_module_native(handle, result.native);
        return {};
    }
    WGPUComputePipelineDescriptor desc = {};
    desc.layout = native.pipelineLayout;
    desc.compute.module = native.cshader;
    desc.compute.entryPoint = wg_string((info.cshader.entry && info.cshader.entry[0]) ? info.cshader.entry : "main");
    native.computePipeline = wgpuDeviceCreateComputePipeline(webgpu.device, &desc);
    if (!native.computePipeline)
    {
        result.native = &native;
        wg_destroy_module_native(handle, result.native);
        return {};
    }
    webgpu.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    for (size_t i = 0; i < std::size(info.binding); ++i)
        result.binding[i] = info.binding[i];
    return result;
}

rt_module_render_t wg_create_module_render(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};
    if (!webgpu.device) return result;
    uint32_t handle = webgpu.moduleID + 1;
    auto& native = webgpu.modules[handle];
    if (info.vshader.code) native.vshader = wg_create_shader(info.vshader.code, info.vshader.size);
    if (info.fshader.code) native.fshader = wg_create_shader(info.fshader.code, info.fshader.size);
    if (!wg_create_pipeline_layout(native, info.binding) || !native.vshader)
    {
        result.native = &native;
        wg_destroy_module_native(handle, result.native);
        return {};
    }
    WGPUVertexAttribute attributes[RT_MAX_VERTEX_BUFFER_NUM] = {};
    WGPUVertexBufferLayout layouts[RT_MAX_VERTEX_BUFFER_NUM] = {};
    uint32_t attrCount = 0;
    for (uint32_t i = 0; i < RT_MAX_VERTEX_BUFFER_NUM; ++i)
    {
        if (info.vertex[i].format == RT_VERTEX_NONE) continue;
        attributes[attrCount].format = rt_to_wg_vertex_format(info.vertex[i].format);
        attributes[attrCount].offset = 0;
        attributes[attrCount].shaderLocation = info.vertex[i].location;
        layouts[attrCount].arrayStride = rt_to_wg_vertex_size(info.vertex[i].format);
        layouts[attrCount].stepMode = info.vertex[i].instance ? WGPUVertexStepMode_Instance : WGPUVertexStepMode_Vertex;
        layouts[attrCount].attributeCount = 1;
        layouts[attrCount].attributes = &attributes[attrCount];
        attrCount++;
    }
    WGPUColorTargetState targets[RT_MAX_COLOR_TEXTURE_NUM] = {};
    WGPUBlendState blends[RT_MAX_COLOR_TEXTURE_NUM] = {};
    uint32_t colorCount = 0;
    for (uint32_t i = 0; i < RT_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        if (info.colors[i].format == RT_TEXTURE_NONE)
            continue;
        colorCount = i + 1;
        targets[i].format = rt_to_wg_texture_format(info.colors[i].format);
        targets[i].writeMask = WGPUColorWriteMask_All;
        bool blend =
            (info.colors[i].color.func != RT_FUNC_ADD || info.colors[i].color.src != RT_BLEND_ONE ||
             info.colors[i].color.dst != RT_BLEND_ZERO || info.colors[i].alpha.func != RT_FUNC_ADD ||
             info.colors[i].alpha.src != RT_BLEND_ONE || info.colors[i].alpha.dst != RT_BLEND_ZERO);
        if (blend)
        {
            blends[i].color.srcFactor = rt_to_wg_blend(info.colors[i].color.src);
            blends[i].color.dstFactor = rt_to_wg_blend(info.colors[i].color.dst);
            blends[i].color.operation = rt_to_wg_blend_op(info.colors[i].color.func);
            blends[i].alpha.srcFactor = rt_to_wg_blend(info.colors[i].alpha.src);
            blends[i].alpha.dstFactor = rt_to_wg_blend(info.colors[i].alpha.dst);
            blends[i].alpha.operation = rt_to_wg_blend_op(info.colors[i].alpha.func);
            targets[i].blend = &blends[i];
        }
    }
    WGPUFragmentState fragment = {};
    fragment.module = native.fshader;
    fragment.entryPoint = wg_string((info.fshader.entry && info.fshader.entry[0]) ? info.fshader.entry : "main");
    fragment.targetCount = colorCount;
    fragment.targets = colorCount ? targets : nullptr;
    const bool depthEnabled = (info.depth.func != RT_ALWAYS || info.depth.write);
    const bool stencilEnabled =
        (info.stencil.back.func != RT_ALWAYS || info.stencil.back.sfail != RT_STENCIL_KEEP ||
         info.stencil.back.zfail != RT_STENCIL_KEEP || info.stencil.back.zpass != RT_STENCIL_KEEP ||
         info.stencil.front.func != RT_ALWAYS || info.stencil.front.sfail != RT_STENCIL_KEEP ||
         info.stencil.front.zfail != RT_STENCIL_KEEP || info.stencil.front.zpass != RT_STENCIL_KEEP);
    WGPUDepthStencilState depth = {};
    depth.format = stencilEnabled ? WGPUTextureFormat_Depth32FloatStencil8 : WGPUTextureFormat_Depth32Float;
    depth.depthWriteEnabled = info.depth.write ? WGPUOptionalBool_True : WGPUOptionalBool_False;
    depth.depthCompare = rt_to_wg_compare(info.depth.func);
    depth.stencilFront.compare = rt_to_wg_compare(info.stencil.front.func);
    depth.stencilBack.compare = rt_to_wg_compare(info.stencil.back.func);
    depth.stencilReadMask = info.stencil.read;
    depth.stencilWriteMask = info.stencil.write;
    WGPURenderPipelineDescriptor desc = {};
    desc.layout = native.pipelineLayout;
    desc.vertex.module = native.vshader;
    desc.vertex.entryPoint = wg_string((info.vshader.entry && info.vshader.entry[0]) ? info.vshader.entry : "main");
    desc.vertex.bufferCount = attrCount;
    desc.vertex.buffers = layouts;
    desc.primitive.topology = rt_to_wg_primitive(info.primitive);
    desc.primitive.frontFace = (info.wind_mode == RT_CW) ? WGPUFrontFace_CW : WGPUFrontFace_CCW;
    desc.primitive.cullMode = rt_to_wg_cull(info.cull_mode);
    desc.multisample.count = 1;
    desc.multisample.mask = 0xFFFFFFFF;
    if (native.fshader) desc.fragment = &fragment;
    if (depthEnabled || stencilEnabled) desc.depthStencil = &depth;
    native.renderPipeline = wgpuDeviceCreateRenderPipeline(webgpu.device, &desc);
    native.topology = desc.primitive.topology;
    if (!native.renderPipeline)
    {
        result.native = &native;
        wg_destroy_module_native(handle, result.native);
        return {};
    }
    webgpu.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    for (size_t i = 0; i < std::size(info.colors); ++i)
    {
        result.colors[i].format = info.colors[i].format;
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

rt_module_render_t wg_create_module_meshlet(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};
    if (!info.mshader.code || !info.mshader.size || !webgpu.device) return result;
    uint32_t handle = webgpu.moduleID + 1;
    auto& native = webgpu.modules[handle];
    if (info.tshader.code) native.tshader = wg_create_shader(info.tshader.code, info.tshader.size);
    native.mshader = wg_create_shader(info.mshader.code, info.mshader.size);
    if (info.fshader.code) native.fshader = wg_create_shader(info.fshader.code, info.fshader.size);
    if (!native.mshader || !wg_create_pipeline_layout(native, info.binding))
    {
        result.native = &native;
        wg_destroy_module_native(handle, result.native);
        return {};
    }
    webgpu.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    for (size_t i = 0; i < std::size(info.colors); ++i)
    {
        result.colors[i].format = info.colors[i].format;
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

void wg_destroy_module_render(rt_module_render_t& module)
{
    wg_destroy_module_native(module.handle, module.native);
    module.handle = 0;
}

void wg_destroy_module_compute(rt_module_compute_t& module)
{
    wg_destroy_module_native(module.handle, module.native);
    module.handle = 0;
}

void wg_push_constant(uint8_t const* buffer, size_t length)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (!buffer || length == 0) return;
    webgpu.pushLength = (uint32_t)std::min(length, sizeof(webgpu.pushData));
    std::memcpy(webgpu.pushData, buffer, webgpu.pushLength);
    if (webgpu.queue && webgpu.pushBuffer)
        wgpuQueueWriteBuffer(webgpu.queue, webgpu.pushBuffer, 0, webgpu.pushData, 256);
    wg_flush_descriptors();
}

void wg_push_const_int(const char*, int32_t) {}
void wg_push_const_uint(const char*, uint32_t) {}
void wg_push_const_float(const char*, float) {}
void wg_push_const_vec2(const char*, const float*) {}
void wg_push_const_vec3(const char*, const float*) {}
void wg_push_const_vec4(const char*, const float*) {}
void wg_push_const_mat3(const char*, const float*) {}
void wg_push_const_mat4(const char*, const float*) {}

void wg_begin_compute(rt_pass_compute_t& pass)
{
    if (pass.module.handle == 0 || !pass.module.native)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (webgpu.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    for (auto& binding : webgpu.currentBinding)
        binding = {};
    auto handle = webgpu.passID + 1;
    auto& native = webgpu.computePasses[handle];
    pass.handle = handle;
    pass.native = &native;
    webgpu.currentPassType = RT_MODULE_COMPUTE;
    webgpu.currentComputePass = &pass;
    wg_end_pass_encoders();
    wg_ensure_encoder();
    webgpu.computePass = wgpuCommandEncoderBeginComputePass(webgpu.encoder, nullptr);
    auto* mod = (wg_module_native_t*)pass.module.native;
    if (mod && mod->computePipeline)
        wgpuComputePassEncoderSetPipeline(webgpu.computePass, mod->computePipeline);
    webgpu.passID = handle;
}

void wg_end_compute(rt_pass_compute_t& pass)
{
    if (webgpu.currentComputePass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.computePass)
    {
        wgpuComputePassEncoderEnd(webgpu.computePass);
        wgpuComputePassEncoderRelease(webgpu.computePass);
        webgpu.computePass = nullptr;
    }
    webgpu.computePasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    webgpu.currentPassType = RT_MODULE_NONE;
    webgpu.currentPipeline = nullptr;
    for (auto& binding : webgpu.currentBinding)
        binding = {};
}

void wg_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_COMPUTE)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    wg_flush_descriptors();
    if (webgpu.computePass)
        wgpuComputePassEncoderDispatchWorkgroups(webgpu.computePass, std::max(1u, groupX), std::max(1u, groupY), std::max(1u, groupZ));
}

static void wg_bind_draw_vbos(rt_buffer_t vbo[], uint32_t vbo_num)
{
    rt_module_render_t const& module = webgpu.currentRenderPass->module;
    uint32_t count = vbo_num;
    if (count > (uint32_t)std::size(module.vertex))
        count = (uint32_t)std::size(module.vertex);
    for (uint32_t i = 0; i < count; ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.format == RT_VERTEX_NONE || !vbo || vbo[i].handle == 0)
            continue;
        auto* native = wg_buffer_native(vbo[i]);
        if (!native || !native->handle)
            continue;
        wg_transition_buffer(*native, WG_STATE_VERTEX);
        wgpuRenderPassEncoderSetVertexBuffer(webgpu.renderPass, layout.location, native->handle, 0, vbo[i].size);
    }
}

void wg_dispatch_compute_indirect(rt_buffer_t& indirect, size_t offset)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_COMPUTE)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* native = wg_buffer_native(indirect);
    if (!native || !native->handle)
        return;
    wg_flush_descriptors();
    wg_transition_buffer(*native, WG_STATE_SHADER_READ);
    if (webgpu.computePass)
        wgpuComputePassEncoderDispatchWorkgroupsIndirect(webgpu.computePass, native->handle, (uint64_t)offset);
}

void wg_begin_render(rt_pass_render_t& pass)
{
    if (pass.module.handle == 0 || !pass.module.native)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (webgpu.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    for (auto& binding : webgpu.currentBinding)
        binding = {};
    webgpu.currentPassType = RT_MODULE_RENDER;
    webgpu.currentRenderPass = &pass;

    WGPURenderPassColorAttachment colors[RT_MAX_COLOR_TEXTURE_NUM] = {};
    uint32_t colorCount = 0;
    uint32_t width = 0, height = 0;
    for (uint32_t i = 0; i < RT_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        if (pass.colors[i].texture_view.format == RT_TEXTURE_NONE)
            continue;
        auto* view = wg_texture_view_native(pass.colors[i].texture_view.handle);
        if (!view || !view->handle) continue;
        auto texIt = webgpu.textures.find(view->texture);
        if (texIt == webgpu.textures.end()) continue;
        auto* tex = &texIt->second;
        wg_transition_image(*tex, WG_STATE_COLOR);
        colors[i].view = view->handle;
        colors[i].depthSlice = tex->target == RT_TEXTURE_3D ? pass.colors[i].texture_view.base_layer : WGPU_DEPTH_SLICE_UNDEFINED;
        colors[i].loadOp = pass.colors[i].clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
        colors[i].storeOp = WGPUStoreOp_Store;
        colors[i].clearValue = {pass.colors[i].value.r, pass.colors[i].value.g, pass.colors[i].value.b, pass.colors[i].value.a};
        uint32_t mip = std::min(pass.colors[i].texture_view.base_level, 31u);
        width = std::max(width, std::max(1u, tex->width >> mip));
        height = std::max(height, std::max(1u, tex->height >> mip));
        colorCount = i + 1;
    }
    for (uint32_t i = 0; i < colorCount; ++i)
    {
        if (colors[i].view)
            continue;
        colors[i].depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        colors[i].loadOp = WGPULoadOp_Load;
        colors[i].storeOp = WGPUStoreOp_Store;
    }

    WGPURenderPassDepthStencilAttachment depth = {};
    bool hasDepth = false;
    if (auto* view = wg_texture_view_native(pass.depth.texture_view.handle))
    {
        auto texIt = webgpu.textures.find(view->texture);
        if (texIt != webgpu.textures.end() && view->handle)
        {
            auto* tex = &texIt->second;
            wg_transition_image(*tex, WG_STATE_DEPTH);
            depth.view = view->handle;
            depth.depthLoadOp = pass.depth.clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
            depth.depthStoreOp = WGPUStoreOp_Store;
            depth.depthClearValue = pass.depth.value;
            if (rt_texture_has_stencil(pass.depth.texture_view.format))
            {
                depth.stencilLoadOp = pass.stencil.clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
                depth.stencilStoreOp = WGPUStoreOp_Store;
                depth.stencilClearValue = (uint32_t)pass.stencil.value;
            }
            uint32_t mip = std::min(pass.depth.texture_view.base_level, 31u);
            width = std::max(width, std::max(1u, tex->width >> mip));
            height = std::max(height, std::max(1u, tex->height >> mip));
            hasDepth = true;
        }
    }
    WGPURenderPassDescriptor desc = {};
    desc.colorAttachmentCount = colorCount;
    desc.colorAttachments = colors;
    if (hasDepth) desc.depthStencilAttachment = &depth;
    wg_end_pass_encoders();
    wg_ensure_encoder();
    webgpu.renderPass = wgpuCommandEncoderBeginRenderPass(webgpu.encoder, &desc);
    auto* mod = (wg_module_native_t*)pass.module.native;
    if (mod && mod->renderPipeline)
        wgpuRenderPassEncoderSetPipeline(webgpu.renderPass, mod->renderPipeline);
    wg_set_viewport(0, 0, (int32_t)width, (int32_t)height);
    wg_set_scissor(0, 0, (int32_t)width, (int32_t)height);
}

void wg_end_render(rt_pass_render_t& pass)
{
    if (webgpu.currentRenderPass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.renderPass)
    {
        wgpuRenderPassEncoderEnd(webgpu.renderPass);
        wgpuRenderPassEncoderRelease(webgpu.renderPass);
        webgpu.renderPass = nullptr;
    }
    for (auto& color : pass.colors)
        if (auto* view = wg_texture_view_native(color.texture_view.handle))
            if (auto texIt = webgpu.textures.find(view->texture); texIt != webgpu.textures.end())
                wg_transition_image(texIt->second, WG_STATE_SHADER_READ);
    if (auto* view = wg_texture_view_native(pass.depth.texture_view.handle))
        if (auto texIt = webgpu.textures.find(view->texture); texIt != webgpu.textures.end())
            wg_transition_image(texIt->second, WG_STATE_SHADER_READ);
    pass.handle = 0;
    pass.native = nullptr;
    webgpu.currentPassType = RT_MODULE_NONE;
    webgpu.currentPipeline = nullptr;
    for (auto& binding : webgpu.currentBinding)
        binding = {};
}

void wg_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (!webgpu.renderPass) return;
    wgpuRenderPassEncoderSetViewport(webgpu.renderPass, (float)x, (float)y, (float)width, (float)height, 0.0f, 1.0f);
}

void wg_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (!webgpu.renderPass) return;
    wgpuRenderPassEncoderSetScissorRect(webgpu.renderPass, (uint32_t)std::max(0, x), (uint32_t)std::max(0, y),
        (uint32_t)std::max(0, width), (uint32_t)std::max(0, height));
}

void wg_draw_mesh_task(uint32_t, uint32_t, uint32_t)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    wg_flush_descriptors();
}

void wg_begin_transfer(rt_pass_transfer_t& pass)
{
    if (webgpu.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    auto handle = webgpu.passID + 1;
    auto& native = webgpu.transferPasses[handle];
    pass.handle = handle;
    pass.native = &native;
    webgpu.currentPassType = RT_MODULE_TRANSFER;
    webgpu.currentTransferPass = &pass;
    wg_end_pass_encoders();
    wg_ensure_encoder();
    webgpu.passID = handle;
}

void wg_end_transfer(rt_pass_transfer_t& pass)
{
    if (webgpu.currentTransferPass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    webgpu.transferPasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    webgpu.currentPassType = RT_MODULE_NONE;
    webgpu.currentPipeline = nullptr;
}

void wg_copy_buffer(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* src = wg_buffer_native(source.buffer);
    auto* dst = wg_buffer_native(destination.buffer);
    if (!src || !dst || copySize == 0) return;
    if (source.offset + copySize > source.buffer.size || destination.offset + copySize > destination.buffer.size)
        return;
    wg_transition_buffer(*src, WG_STATE_COPY_SRC);
    wg_transition_buffer(*dst, WG_STATE_COPY_DST);
    wgpuCommandEncoderCopyBufferToBuffer(webgpu.encoder, src->handle, source.offset, dst->handle, destination.offset, copySize);
}

void wg_copy_buffer_data(rt_buffer_data_t source, rt_buffer_copy_t destination, size_t copySize)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* dst = wg_buffer_native(destination.buffer);
    if (!source.data || !dst || copySize == 0) return;
    if (source.offset + copySize > source.size || destination.offset + copySize > destination.buffer.size)
        return;
    wgpuQueueWriteBuffer(webgpu.queue, dst->handle, destination.offset, source.data + source.offset, copySize);
    dst->state = WG_STATE_HOST;
}

void wg_copy_buffer_texture(rt_texture_copy_t source, rt_buffer_texel_t destination, rt_size_t copySize)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* src = wg_texture_native(source.texture);
    auto* dst = wg_buffer_native(destination.buffer);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    wg_transition_image(*src, WG_STATE_COPY_SRC);
    wg_transition_buffer(*dst, WG_STATE_COPY_DST);
    uint32_t bpp = wg_format_bytes(src->format);
    WGPUTexelCopyTextureInfo srcCopy = {};
    srcCopy.texture = src->handle;
    srcCopy.mipLevel = source.mipLevel;
    srcCopy.origin = {source.origin.x, source.origin.y, source.origin.z};
    WGPUTexelCopyBufferInfo dstCopy = {};
    dstCopy.buffer = dst->handle;
    dstCopy.layout.offset = destination.offset;
    dstCopy.layout.bytesPerRow = destination.bytesPerRow ? destination.bytesPerRow : copySize.x * bpp;
    dstCopy.layout.rowsPerImage = destination.rowsPerImage ? destination.rowsPerImage : copySize.y;
    WGPUExtent3D size = {copySize.x, copySize.y, copySize.z ? copySize.z : 1};
    wgpuCommandEncoderCopyTextureToBuffer(webgpu.encoder, &srcCopy, &dstCopy, &size);
}

void wg_copy_texture(rt_texture_copy_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* src = wg_texture_native(source.texture);
    auto* dst = wg_texture_native(destination.texture);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    wg_transition_image(*src, WG_STATE_COPY_SRC);
    wg_transition_image(*dst, WG_STATE_COPY_DST);
    WGPUTexelCopyTextureInfo srcCopy = {};
    srcCopy.texture = src->handle;
    srcCopy.mipLevel = source.mipLevel;
    srcCopy.origin = {source.origin.x, source.origin.y, source.origin.z};
    WGPUTexelCopyTextureInfo dstCopy = {};
    dstCopy.texture = dst->handle;
    dstCopy.mipLevel = destination.mipLevel;
    dstCopy.origin = {destination.origin.x, destination.origin.y, destination.origin.z};
    WGPUExtent3D size = {copySize.x, copySize.y, copySize.z ? copySize.z : 1};
    wgpuCommandEncoderCopyTextureToTexture(webgpu.encoder, &srcCopy, &dstCopy, &size);
}

void wg_copy_texture_data(rt_texture_data_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* dst = wg_texture_native(destination.texture);
    if (!source.data || !dst || copySize.x == 0 || copySize.y == 0) return;
    uint32_t bpp = wg_format_bytes(dst->format);
    size_t bytes = source.size ? source.size : (size_t)std::max(copySize.x * bpp, 1u) * copySize.y * std::max(1u, copySize.z);
    WGPUTexelCopyTextureInfo dstCopy = {};
    dstCopy.texture = dst->handle;
    dstCopy.mipLevel = destination.mipLevel;
    dstCopy.origin = {destination.origin.x, destination.origin.y, destination.origin.z};
    WGPUTexelCopyBufferLayout layout = {};
    layout.offset = 0;
    layout.bytesPerRow = source.bytesPerRow ? source.bytesPerRow : copySize.x * bpp;
    layout.rowsPerImage = source.rowsPerImage ? source.rowsPerImage : copySize.y;
    WGPUExtent3D size = {copySize.x, copySize.y, copySize.z ? copySize.z : 1};
    wgpuQueueWriteTexture(webgpu.queue, &dstCopy, source.data + source.offset, bytes, &layout, &size);
    wg_transition_image(*dst, WG_STATE_COPY_DST);
}

void wg_copy_texture_buffer(rt_buffer_texel_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* src = wg_buffer_native(source.buffer);
    auto* dst = wg_texture_native(destination.texture);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    wg_transition_image(*dst, WG_STATE_COPY_DST);
    wg_transition_buffer(*src, WG_STATE_COPY_SRC);
    uint32_t bpp = wg_format_bytes(dst->format);
    WGPUTexelCopyBufferInfo srcCopy = {};
    srcCopy.buffer = src->handle;
    srcCopy.layout.offset = source.offset;
    srcCopy.layout.bytesPerRow = source.bytesPerRow ? source.bytesPerRow : copySize.x * bpp;
    srcCopy.layout.rowsPerImage = source.rowsPerImage ? source.rowsPerImage : copySize.y;
    WGPUTexelCopyTextureInfo dstCopy = {};
    dstCopy.texture = dst->handle;
    dstCopy.mipLevel = destination.mipLevel;
    dstCopy.origin = {destination.origin.x, destination.origin.y, destination.origin.z};
    WGPUExtent3D size = {copySize.x, copySize.y, copySize.z ? copySize.z : 1};
    wgpuCommandEncoderCopyBufferToTexture(webgpu.encoder, &srcCopy, &dstCopy, &size);
}

rt_mesh_t wg_create_mesh(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    rt_mesh_t result = {};
    if (!vertices || vertex_count == 0) return result;
    uint32_t handle = webgpu.meshID + 1;
    auto& native = webgpu.meshes[handle];
    native.vertexCount = (uint32_t)vertex_count;
    native.indexCount = (uint32_t)index_count;
    if (vertices)
        result.vertex[0] = wg_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = wg_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = wg_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = wg_create_buffer({.size = index_count * sizeof(uint32_t), .usage = RT_BUFFER_USAGE_INDEX | RT_BUFFER_USAGE_COPY_DST, .data = indices});
    std::iota(result.location, result.location + std::size(result.location), 0);
    webgpu.meshID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void wg_destroy_mesh(rt_mesh_t& mesh)
{
    for (auto& vertex : mesh.vertex)
        wg_destroy_buffer(vertex);
    wg_destroy_buffer(mesh.index);
    webgpu.meshes.erase(mesh.handle);
    mesh.handle = 0;
    mesh.native = nullptr;
}

static void wg_draw_mesh_impl(rt_mesh_t& mesh, uint32_t instanceCount)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    wg_flush_descriptors();
    rt_module_render_t const& module = webgpu.currentRenderPass->module;
    uint32_t vertex_count = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.format == RT_VERTEX_NONE) continue;
        for (uint32_t k = 0; k < std::size(mesh.vertex); ++k)
        {
            if (mesh.vertex[k].handle == 0 || mesh.location[k] != layout.location) continue;
            auto* native = wg_buffer_native(mesh.vertex[k]);
            if (!native) break;
            wg_transition_buffer(*native, WG_STATE_VERTEX);
            wgpuRenderPassEncoderSetVertexBuffer(webgpu.renderPass, layout.location, native->handle, 0, mesh.vertex[k].size);
            uint32_t stride = rt_to_wg_vertex_size(layout.format);
            if (vertex_count == 0 && stride)
                vertex_count = (uint32_t)(mesh.vertex[k].size / stride);
            break;
        }
    }
    if (mesh.index.handle)
    {
        auto* native = wg_buffer_native(mesh.index);
        if (!native) return;
        uint32_t indexStride = rt_to_wg_index_size(module.index_type);
        wg_transition_buffer(*native, WG_STATE_INDEX);
        wgpuRenderPassEncoderSetIndexBuffer(webgpu.renderPass, native->handle, rt_to_wg_index_type(module.index_type), 0, mesh.index.size);
        wgpuRenderPassEncoderDrawIndexed(webgpu.renderPass, (uint32_t)(mesh.index.size / indexStride), instanceCount, 0, 0, 0);
    }
    else
        wgpuRenderPassEncoderDraw(webgpu.renderPass, vertex_count, instanceCount, 0, 0);
}

void wg_draw_array(rt_buffer_t vbo[], uint32_t vbo_num, uint32_t draw_num, uint32_t instance_num)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    wg_flush_descriptors();
    if (!webgpu.renderPass)
        return;
    wg_bind_draw_vbos(vbo, vbo_num);
    wgpuRenderPassEncoderDraw(webgpu.renderPass, draw_num, instance_num, 0, 0);
}

void wg_draw_index(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& ibo, uint32_t draw_num, uint32_t instance_num)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* indexNative = wg_buffer_native(ibo);
    if (!indexNative || !indexNative->handle || !webgpu.renderPass)
        return;
    wg_flush_descriptors();
    wg_bind_draw_vbos(vbo, vbo_num);
    wg_transition_buffer(*indexNative, WG_STATE_INDEX);
    wgpuRenderPassEncoderSetIndexBuffer(webgpu.renderPass, indexNative->handle, rt_to_wg_index_type(webgpu.currentRenderPass->module.index_type), 0, ibo.size);
    wgpuRenderPassEncoderDrawIndexed(webgpu.renderPass, draw_num, instance_num, 0, 0, 0);
}

void wg_draw_array_indirect(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& indirect, size_t offset)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* indirectNative = wg_buffer_native(indirect);
    if (!indirectNative || !indirectNative->handle || !webgpu.renderPass)
        return;
    wg_flush_descriptors();
    wg_bind_draw_vbos(vbo, vbo_num);
    wg_transition_buffer(*indirectNative, WG_STATE_SHADER_READ);
    wgpuRenderPassEncoderDrawIndirect(webgpu.renderPass, indirectNative->handle, (uint64_t)offset);
}

void wg_draw_index_indirect(rt_buffer_t vbo[], uint32_t vbo_num, rt_buffer_t& ibo, rt_buffer_t& indirect, size_t offset)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* indexNative = wg_buffer_native(ibo);
    auto* indirectNative = wg_buffer_native(indirect);
    if (!indexNative || !indexNative->handle || !indirectNative || !indirectNative->handle || !webgpu.renderPass)
        return;
    wg_flush_descriptors();
    wg_bind_draw_vbos(vbo, vbo_num);
    wg_transition_buffer(*indexNative, WG_STATE_INDEX);
    wgpuRenderPassEncoderSetIndexBuffer(webgpu.renderPass, indexNative->handle, rt_to_wg_index_type(webgpu.currentRenderPass->module.index_type), 0, ibo.size);
    wg_transition_buffer(*indirectNative, WG_STATE_SHADER_READ);
    wgpuRenderPassEncoderDrawIndexedIndirect(webgpu.renderPass, indirectNative->handle, (uint64_t)offset);
}

void wg_draw_mesh(rt_mesh_t& mesh)
{
    wg_draw_mesh_impl(mesh, 1);
}

void wg_draw_mesh_multi(rt_mesh_t& mesh, uint32_t count)
{
    wg_draw_mesh_impl(mesh, std::max(1u, count));
}

rt_meshlet_t wg_create_meshlet(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    rt_meshlet_t result = {};
    if (!vertices || vertex_count == 0) return result;
    uint32_t handle = webgpu.meshletID + 1;
    auto& native = webgpu.meshlets[handle];
    native.vertexCount = (uint32_t)vertex_count;
    native.indexCount = (uint32_t)index_count;
    if (vertices)
        result.vertex[0] = wg_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = wg_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = wg_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = wg_create_buffer({.size = index_count * sizeof(uint32_t), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = indices});
    std::iota(result.location, result.location + std::size(result.location), 0);
    webgpu.meshletID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void wg_destroy_meshlet(rt_meshlet_t& meshlet)
{
    for (auto& vertex : meshlet.vertex)
        wg_destroy_buffer(vertex);
    wg_destroy_buffer(meshlet.index);
    webgpu.meshlets.erase(meshlet.handle);
    meshlet.handle = 0;
    meshlet.native = nullptr;
}

void wg_draw_meshlet(rt_meshlet_t& meshlet)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    rt_module_render_t const& module = webgpu.currentRenderPass->module;
    uint32_t index_binding = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.format == RT_VERTEX_NONE) continue;
        for (uint32_t k = 0; k < std::size(meshlet.vertex); ++k)
        {
            if (meshlet.vertex[k].handle == 0 || meshlet.location[k] != layout.location) continue;
            wg_bind_buffer(meshlet.vertex[k], {.binding = layout.location});
            break;
        }
        if (layout.location + 1 > index_binding)
            index_binding = layout.location + 1;
    }
    if (meshlet.index.handle)
        wg_bind_buffer(meshlet.index, {.binding = index_binding});
    wg_flush_descriptors();
}

void wg_submit()
{
    if (!webgpu.encoder) return;
    wg_end_pass_encoders();
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(webgpu.encoder, nullptr);
    wgpuCommandEncoderRelease(webgpu.encoder);
    webgpu.encoder = nullptr;
    if (cmd && webgpu.queue)
        wgpuQueueSubmit(webgpu.queue, 1, &cmd);
    if (cmd) wgpuCommandBufferRelease(cmd);
    wg_ensure_encoder();
}

#endif


