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

enum rt_module_type_t : uint32_t
{
    GL_MODULE_RENDER = 1,
    GL_MODULE_COMPUTE = 2,
    GL_MODULE_MESHLET = 3,
    GL_MODULE_TRANSFER = 4,
};

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

static bool gl_has_mipmap_filter(GLenum minFilter)
{
    return minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST ||
           minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR;
}

static WGPUFilterMode gl_to_wg_filter(GLenum filter)
{
    return (filter == GL_NEAREST || filter == GL_NEAREST_MIPMAP_NEAREST || filter == GL_NEAREST_MIPMAP_LINEAR) ?
           WGPUFilterMode_Nearest : WGPUFilterMode_Linear;
}

static WGPUMipmapFilterMode gl_to_wg_mip(GLenum minFilter)
{
    if (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST)
        return WGPUMipmapFilterMode_Nearest;
    if (minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR)
        return WGPUMipmapFilterMode_Linear;
    return WGPUMipmapFilterMode_Nearest;
}

static WGPUAddressMode gl_to_wg_address(GLenum wrap)
{
    switch (wrap)
    {
        case GL_REPEAT: return WGPUAddressMode_Repeat;
        case GL_MIRRORED_REPEAT: return WGPUAddressMode_MirrorRepeat;
        case GL_CLAMP_TO_EDGE:
        case GL_CLAMP_TO_BORDER:
        case GL_MIRROR_CLAMP_TO_EDGE:
        default: return WGPUAddressMode_ClampToEdge;
    }
}

static WGPUTextureFormat gl_to_wg_format(GLenum glFormat, GLenum glType, GLenum internalFormat = 0)
{
    switch (internalFormat)
    {
        case GL_R8: return WGPUTextureFormat_R8Unorm;
        case GL_RG8: return WGPUTextureFormat_RG8Unorm;
        case GL_RGB8:
        case GL_RGBA8: return WGPUTextureFormat_RGBA8Unorm;
        case GL_SRGB8_ALPHA8: return WGPUTextureFormat_RGBA8UnormSrgb;
        case GL_R16F: return WGPUTextureFormat_R16Float;
        case GL_RG16F: return WGPUTextureFormat_RG16Float;
        case GL_RGBA16F: return WGPUTextureFormat_RGBA16Float;
        case GL_R32F: return WGPUTextureFormat_R32Float;
        case GL_RG32F: return WGPUTextureFormat_RG32Float;
        case GL_RGBA32F: return WGPUTextureFormat_RGBA32Float;
        case GL_DEPTH_COMPONENT16: return WGPUTextureFormat_Depth16Unorm;
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32F: return WGPUTextureFormat_Depth32Float;
        case GL_DEPTH24_STENCIL8: return WGPUTextureFormat_Depth24PlusStencil8;
        case GL_DEPTH32F_STENCIL8: return WGPUTextureFormat_Depth32FloatStencil8;
        default: break;
    }
    switch (glFormat)
    {
        case GL_RED: return (glType == GL_FLOAT) ? WGPUTextureFormat_R32Float : WGPUTextureFormat_R8Unorm;
        case GL_RG: return (glType == GL_FLOAT) ? WGPUTextureFormat_RG32Float : WGPUTextureFormat_RG8Unorm;
        case GL_RGB:
        case GL_RGBA: return (glType == GL_FLOAT) ? WGPUTextureFormat_RGBA32Float : WGPUTextureFormat_RGBA8Unorm;
        case GL_DEPTH_COMPONENT: return WGPUTextureFormat_Depth32Float;
        case GL_DEPTH_STENCIL: return WGPUTextureFormat_Depth32FloatStencil8;
        default: return WGPUTextureFormat_RGBA8Unorm;
    }
}

static WGPUVertexFormat gl_to_wg_vertex_format(GLenum type, GLenum count)
{
    if (type == GL_FLOAT)
    {
        switch (count)
        {
            case 1: return WGPUVertexFormat_Float32;
            case 2: return WGPUVertexFormat_Float32x2;
            case 3: return WGPUVertexFormat_Float32x3;
            case 4: return WGPUVertexFormat_Float32x4;
            default: return WGPUVertexFormat_Float32;
        }
    }
    if (type == GL_INT)
    {
        switch (count)
        {
            case 1: return WGPUVertexFormat_Sint32;
            case 2: return WGPUVertexFormat_Sint32x2;
            case 3: return WGPUVertexFormat_Sint32x3;
            case 4: return WGPUVertexFormat_Sint32x4;
            default: return WGPUVertexFormat_Sint32;
        }
    }
    if (type == GL_UNSIGNED_INT)
    {
        switch (count)
        {
            case 1: return WGPUVertexFormat_Uint32;
            case 2: return WGPUVertexFormat_Uint32x2;
            case 3: return WGPUVertexFormat_Uint32x3;
            case 4: return WGPUVertexFormat_Uint32x4;
            default: return WGPUVertexFormat_Uint32;
        }
    }
    return WGPUVertexFormat_Float32;
}

static WGPUCompareFunction gl_to_wg_compare(GLenum func)
{
    switch (func)
    {
        case GL_NEVER: return WGPUCompareFunction_Never;
        case GL_LESS: return WGPUCompareFunction_Less;
        case GL_EQUAL: return WGPUCompareFunction_Equal;
        case GL_LEQUAL: return WGPUCompareFunction_LessEqual;
        case GL_GREATER: return WGPUCompareFunction_Greater;
        case GL_NOTEQUAL: return WGPUCompareFunction_NotEqual;
        case GL_GEQUAL: return WGPUCompareFunction_GreaterEqual;
        case GL_ALWAYS:
        default: return WGPUCompareFunction_Always;
    }
}

static WGPUBlendFactor gl_to_wg_blend(GLenum factor)
{
    switch (factor)
    {
        case GL_ZERO: return WGPUBlendFactor_Zero;
        case GL_ONE: return WGPUBlendFactor_One;
        case GL_SRC_COLOR: return WGPUBlendFactor_Src;
        case GL_ONE_MINUS_SRC_COLOR: return WGPUBlendFactor_OneMinusSrc;
        case GL_DST_COLOR: return WGPUBlendFactor_Dst;
        case GL_ONE_MINUS_DST_COLOR: return WGPUBlendFactor_OneMinusDst;
        case GL_SRC_ALPHA: return WGPUBlendFactor_SrcAlpha;
        case GL_ONE_MINUS_SRC_ALPHA: return WGPUBlendFactor_OneMinusSrcAlpha;
        case GL_DST_ALPHA: return WGPUBlendFactor_DstAlpha;
        case GL_ONE_MINUS_DST_ALPHA: return WGPUBlendFactor_OneMinusDstAlpha;
        case GL_CONSTANT_COLOR: return WGPUBlendFactor_Constant;
        case GL_ONE_MINUS_CONSTANT_COLOR: return WGPUBlendFactor_OneMinusConstant;
        case GL_SRC_ALPHA_SATURATE: return WGPUBlendFactor_SrcAlphaSaturated;
        default: return WGPUBlendFactor_One;
    }
}

static WGPUBlendOperation gl_to_wg_blend_op(GLenum func)
{
    switch (func)
    {
        case GL_FUNC_SUBTRACT: return WGPUBlendOperation_Subtract;
        case GL_FUNC_REVERSE_SUBTRACT: return WGPUBlendOperation_ReverseSubtract;
        case GL_MIN: return WGPUBlendOperation_Min;
        case GL_MAX: return WGPUBlendOperation_Max;
        case GL_FUNC_ADD:
        default: return WGPUBlendOperation_Add;
    }
}

static WGPUCullMode gl_to_wg_cull(GLenum mode)
{
    switch (mode)
    {
        case GL_FRONT: return WGPUCullMode_Front;
        case GL_BACK: return WGPUCullMode_Back;
        default: return WGPUCullMode_None;
    }
}

static WGPUPrimitiveTopology gl_to_wg_primitive(GLenum primitive)
{
    switch (primitive)
    {
        case GL_POINTS: return WGPUPrimitiveTopology_PointList;
        case GL_LINES: return WGPUPrimitiveTopology_LineList;
        case GL_LINE_STRIP:
        case GL_LINE_LOOP: return WGPUPrimitiveTopology_LineStrip;
        case GL_TRIANGLE_STRIP: return WGPUPrimitiveTopology_TriangleStrip;
        case GL_TRIANGLES:
        default: return WGPUPrimitiveTopology_TriangleList;
    }
}

static uint32_t gl_vertex_size(GLenum type, GLenum count)
{
    if (type == GL_FLOAT || type == GL_INT || type == GL_UNSIGNED_INT) return count * 4;
    if (type == GL_SHORT || type == GL_UNSIGNED_SHORT || type == GL_HALF_FLOAT) return count * 2;
    if (type == GL_BYTE || type == GL_UNSIGNED_BYTE) return count;
    if (type == GL_DOUBLE) return count * 8;
    return count * 4;
}

static uint32_t gl_index_type_size(GLenum type)
{
    return (type == GL_UNSIGNED_SHORT) ? 2 : 4;
}

static WGPUIndexFormat gl_to_wg_index_type(GLenum type)
{
    return (type == GL_UNSIGNED_SHORT) ? WGPUIndexFormat_Uint16 : WGPUIndexFormat_Uint32;
}

static uint32_t wg_format_bytes(WGPUTextureFormat format)
{
    switch (format)
    {
        case WGPUTextureFormat_R8Unorm: return 1;
        case WGPUTextureFormat_RG8Unorm:
        case WGPUTextureFormat_R16Float:
        case WGPUTextureFormat_Depth16Unorm: return 2;
        case WGPUTextureFormat_RGBA8Unorm:
        case WGPUTextureFormat_RGBA8UnormSrgb:
        case WGPUTextureFormat_R32Float:
        case WGPUTextureFormat_RG16Float:
        case WGPUTextureFormat_Depth32Float:
        case WGPUTextureFormat_Depth24PlusStencil8: return 4;
        case WGPUTextureFormat_RGBA16Float:
        case WGPUTextureFormat_RG32Float:
        case WGPUTextureFormat_Depth32FloatStencil8: return 8;
        case WGPUTextureFormat_RGBA32Float: return 16;
        default: return 4;
    }
}

static bool wg_is_depth(WGPUTextureFormat format)
{
    return format == WGPUTextureFormat_Depth16Unorm || format == WGPUTextureFormat_Depth32Float ||
           format == WGPUTextureFormat_Depth24PlusStencil8 || format == WGPUTextureFormat_Depth32FloatStencil8;
}

static const char* wg_entry(const char* entry)
{
    return (entry && entry[0]) ? entry : "main";
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
    WGPUTextureView view = nullptr;
    WGPUTextureFormat format = WGPUTextureFormat_Undefined;
    wg_res_state_t state = WG_STATE_UNKNOWN;
    uint32_t mipLevels = 1;
    uint32_t layers = 1;
    uint32_t width = 1, height = 1, depth = 1;
    GLenum target = GL_TEXTURE_2D;
    ~wg_texture_native_t()
    {
        if (view) wgpuTextureViewRelease(view);
        if (handle) wgpuTextureRelease(handle);
    }
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
    wg_binding_kind_t kinds[GL_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t descriptorBindings[GL_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t descriptorCount = 0;
};

struct wg_mesh_native_t { uint32_t vertexCount = 0, indexCount = 0; };
struct wg_meshlet_native_t { uint32_t vertexCount = 0, indexCount = 0; };
struct wg_pass_compute_native_t { uint32_t dummy = 0; };
struct wg_pass_render_native_t { bool offscreen = false; uint32_t width = 0, height = 0; };
struct wg_pass_transfer_native_t { uint32_t dummy = 0; };

struct wg_native_t
{
    uint32_t bufferID = 0, textureID = 0, samplerID = 0, moduleID = 0;
    uint32_t meshID = 0, meshletID = 0, passID = 0;

    std::map<uint32_t, wg_buffer_native_t> buffers;
    std::map<uint32_t, wg_texture_native_t> textures;
    std::map<uint32_t, wg_sampler_native_t> samplers;
    std::map<uint32_t, wg_module_native_t> modules;
    std::map<uint32_t, wg_mesh_native_t> meshes;
    std::map<uint32_t, wg_meshlet_native_t> meshlets;
    std::map<uint32_t, wg_pass_compute_native_t> computePasses;
    std::map<uint32_t, wg_pass_render_native_t> renderPasses;
    std::map<uint32_t, wg_pass_transfer_native_t> transferPasses;

    WGPUDevice device = nullptr;
    WGPUQueue queue = nullptr;
    WGPUCommandEncoder encoder = nullptr;
    WGPURenderPassEncoder renderPass = nullptr;
    WGPUComputePassEncoder computePass = nullptr;
    WGPUSampler defaultSampler = nullptr;
    WGPUBuffer pushBuffer = nullptr;
    uint8_t pushData[256] = {};
    uint32_t pushLength = 0;

    struct
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
} static webgpu;

static WGPUShaderModule wg_create_shader(const char* data, uint32_t length)
{
    if (!data || !length || !webgpu.device) return nullptr;
    std::string source(data, length);
    WGPUShaderModuleWGSLDescriptor wgsl = {};
    wgsl.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    wgsl.code = source.c_str();
    WGPUShaderModuleDescriptor desc = {};
    desc.nextInChain = &wgsl.chain;
    return wgpuDeviceCreateShaderModule(webgpu.device, &desc);
}

static WGPUBufferUsage wg_buffer_usage(rt_buffer_usages_t usage)
{
    WGPUBufferUsage flags = WGPUBufferUsage_None;
    if (usage & GL_BUFFER_USAGE_MAP_READ) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst);
    if (usage & GL_BUFFER_USAGE_MAP_WRITE) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc);
    if (usage & GL_BUFFER_USAGE_COPY_SRC) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_CopySrc);
    if (usage & GL_BUFFER_USAGE_COPY_DST) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_CopyDst);
    if (usage & GL_BUFFER_USAGE_INDEX) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_Index);
    if (usage & GL_BUFFER_USAGE_VERTEX) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_Vertex);
    if (usage & GL_BUFFER_USAGE_UNIFORM) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_Uniform);
    if (usage & GL_BUFFER_USAGE_STORAGE) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_Storage);
    if (usage & GL_BUFFER_USAGE_INDIRECT) flags = (WGPUBufferUsage)(flags | WGPUBufferUsage_Indirect);
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

static wg_sampler_native_t* wg_sampler_native(rt_sampler_t const& sampler)
{
    if (!sampler.native || sampler.handle == 0) return nullptr;
    auto it = webgpu.samplers.find(sampler.handle);
    return it == webgpu.samplers.end() ? nullptr : &it->second;
}

static wg_module_native_t* wg_current_module_native()
{
    if (webgpu.currentPassType == GL_MODULE_COMPUTE && webgpu.currentComputePass)
        return (wg_module_native_t*)webgpu.currentComputePass->module.native;
    if (webgpu.currentPassType == GL_MODULE_RENDER && webgpu.currentRenderPass)
        return (wg_module_native_t*)webgpu.currentRenderPass->module.native;
    return nullptr;
}

static void wg_clear_bindings()
{
    for (auto& binding : webgpu.currentBinding)
        binding = {};
}

static bool wg_create_pipeline_layout(wg_module_native_t& native, rt_binding_t const* bindings)
{
    WGPUBindGroupLayoutEntry entries[GL_MAX_BINDING_HANDLE_NUM + 1] = {};
    native.descriptorCount = 0;
    WGPUShaderStage visibility = (WGPUShaderStage)(WGPUShaderStage_Vertex | WGPUShaderStage_Fragment | WGPUShaderStage_Compute);
    if (bindings)
    {
        for (uint32_t i = 0; i < GL_MAX_BINDING_HANDLE_NUM; ++i)
        {
            if (bindings[i].type == GL_NONE) continue;
            auto& entry = entries[native.descriptorCount];
            entry.binding = bindings[i].binding;
            entry.visibility = visibility;
            if (bindings[i].type == GL_BINDING_BUFFER)
            {
                native.kinds[native.descriptorCount] = WG_KIND_UNIFORM;
                entry.buffer.type = WGPUBufferBindingType_Uniform;
            }
            else if (bindings[i].type == GL_BINDING_TEXTURE)
            {
                native.kinds[native.descriptorCount] = WG_KIND_TEXTURE;
                entry.texture.sampleType = WGPUTextureSampleType_Float;
                entry.texture.viewDimension = WGPUTextureViewDimension_2D;
            }
            else if (bindings[i].type == GL_BINDING_STORAGE_TEXTURE)
            {
                native.kinds[native.descriptorCount] = WG_KIND_STORAGE_TEXTURE;
                entry.storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
                entry.storageTexture.format = WGPUTextureFormat_RGBA8Unorm;
                entry.storageTexture.viewDimension = WGPUTextureViewDimension_2D;
            }
            else if (bindings[i].type == GL_BINDING_SAMPLER)
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

static bool wg_create_graphics_pipeline(wg_module_native_t& native, rt_module_render_info_t const& info)
{
    if (!native.vshader) return false;
    WGPUVertexAttribute attributes[GL_MAX_VERTEX_BUFFER_NUM] = {};
    WGPUVertexBufferLayout layouts[GL_MAX_VERTEX_BUFFER_NUM] = {};
    uint32_t attrCount = 0;
    for (uint32_t i = 0; i < GL_MAX_VERTEX_BUFFER_NUM; ++i)
    {
        if (info.vertex[i].type == GL_NONE || info.vertex[i].count == 0) continue;
        attributes[attrCount].format = gl_to_wg_vertex_format(info.vertex[i].type, info.vertex[i].count);
        attributes[attrCount].offset = 0;
        attributes[attrCount].shaderLocation = info.vertex[i].location;
        layouts[attrCount].arrayStride = gl_vertex_size(info.vertex[i].type, info.vertex[i].count);
        layouts[attrCount].stepMode = info.vertex[i].instance ? WGPUVertexStepMode_Instance : WGPUVertexStepMode_Vertex;
        layouts[attrCount].attributeCount = 1;
        layouts[attrCount].attributes = &attributes[attrCount];
        attrCount++;
    }

    WGPUColorTargetState targets[GL_MAX_COLOR_TEXTURE_NUM] = {};
    WGPUBlendState blends[GL_MAX_COLOR_TEXTURE_NUM] = {};
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        targets[i].format = WGPUTextureFormat_RGBA8Unorm;
        targets[i].writeMask = WGPUColorWriteMask_All;
        bool blend =
            (info.colors[i].color.func != GL_FUNC_ADD || info.colors[i].color.src != GL_ONE ||
             info.colors[i].color.dst != GL_ZERO || info.colors[i].alpha.func != GL_FUNC_ADD ||
             info.colors[i].alpha.src != GL_ONE || info.colors[i].alpha.dst != GL_ZERO);
        if (blend)
        {
            blends[i].color.srcFactor = gl_to_wg_blend(info.colors[i].color.src);
            blends[i].color.dstFactor = gl_to_wg_blend(info.colors[i].color.dst);
            blends[i].color.operation = gl_to_wg_blend_op(info.colors[i].color.func);
            blends[i].alpha.srcFactor = gl_to_wg_blend(info.colors[i].alpha.src);
            blends[i].alpha.dstFactor = gl_to_wg_blend(info.colors[i].alpha.dst);
            blends[i].alpha.operation = gl_to_wg_blend_op(info.colors[i].alpha.func);
            targets[i].blend = &blends[i];
        }
    }

    WGPUFragmentState fragment = {};
    fragment.module = native.fshader;
    fragment.entryPoint = wg_entry(info.fentry);
    fragment.targetCount = GL_MAX_COLOR_TEXTURE_NUM;
    fragment.targets = targets;

    const bool depthEnabled = (info.depth.func != GL_ALWAYS || info.depth.write);
    const bool stencilEnabled =
        (info.stencil.back.func != GL_ALWAYS || info.stencil.back.sfail != GL_KEEP ||
         info.stencil.back.zfail != GL_KEEP || info.stencil.back.zpass != GL_KEEP ||
         info.stencil.front.func != GL_ALWAYS || info.stencil.front.sfail != GL_KEEP ||
         info.stencil.front.zfail != GL_KEEP || info.stencil.front.zpass != GL_KEEP);
    WGPUDepthStencilState depth = {};
    depth.format = stencilEnabled ? WGPUTextureFormat_Depth32FloatStencil8 : WGPUTextureFormat_Depth32Float;
    depth.depthWriteEnabled = info.depth.write;
    depth.depthCompare = gl_to_wg_compare(info.depth.func);
    depth.stencilFront.compare = gl_to_wg_compare(info.stencil.front.func);
    depth.stencilBack.compare = gl_to_wg_compare(info.stencil.back.func);
    depth.stencilReadMask = info.stencil.read;
    depth.stencilWriteMask = info.stencil.write;

    WGPURenderPipelineDescriptor desc = {};
    desc.layout = native.pipelineLayout;
    desc.vertex.module = native.vshader;
    desc.vertex.entryPoint = wg_entry(info.ventry);
    desc.vertex.bufferCount = attrCount;
    desc.vertex.buffers = layouts;
    desc.primitive.topology = gl_to_wg_primitive(info.primitive);
    desc.primitive.frontFace = (info.front_face == GL_CW) ? WGPUFrontFace_CW : WGPUFrontFace_CCW;
    desc.primitive.cullMode = gl_to_wg_cull(info.cull_mode);
    desc.multisample.count = 1;
    desc.multisample.mask = 0xFFFFFFFF;
    if (native.fshader) desc.fragment = &fragment;
    if (depthEnabled || stencilEnabled) desc.depthStencil = &depth;
    native.renderPipeline = wgpuDeviceCreateRenderPipeline(webgpu.device, &desc);
    native.topology = desc.primitive.topology;
    return native.renderPipeline != nullptr;
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

    WGPUBindGroupEntry entries[GL_MAX_BINDING_HANDLE_NUM + 1] = {};
    uint32_t count = 0;
    for (uint32_t i = 0; i < mod->descriptorCount; ++i)
    {
        uint32_t binding = mod->descriptorBindings[i];
        auto& slot = webgpu.currentBinding[binding];
        entries[count].binding = binding;
        if (mod->kinds[i] == WG_KIND_UNIFORM || slot.type == GL_BINDING_BUFFER)
        {
            auto* buf = wg_buffer_native(slot.buffer);
            if (!buf) continue;
            bool storage = (slot.buffer_bind.target == GL_SHADER_STORAGE_BUFFER);
            wg_transition_buffer(*buf, storage ? WG_STATE_SHADER_WRITE : WG_STATE_SHADER_READ);
            entries[count].buffer = buf->handle;
            entries[count].offset = 0;
            entries[count].size = slot.buffer.size;
        }
        else if (mod->kinds[i] == WG_KIND_TEXTURE)
        {
            auto* tex = wg_texture_native(slot.texture);
            if (!tex || !tex->view) continue;
            wg_transition_image(*tex, WG_STATE_SHADER_READ);
            entries[count].textureView = tex->view;
        }
        else if (mod->kinds[i] == WG_KIND_STORAGE_TEXTURE)
        {
            auto* tex = wg_texture_native(slot.storage_texture);
            if (!tex || !tex->view) continue;
            wg_transition_image(*tex, WG_STATE_SHADER_WRITE);
            entries[count].textureView = tex->view;
        }
        else if (mod->kinds[i] == WG_KIND_SAMPLER)
        {
            WGPUSampler samp = webgpu.defaultSampler;
            if (auto* s = wg_sampler_native(slot.sampler))
                samp = s->handle;
            entries[count].sampler = samp;
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
        webgpu.queue = wgpuDeviceGetQueue(webgpu.device);
    wg_ensure_encoder();

    WGPUSamplerDescriptor samp = {};
    samp.minFilter = WGPUFilterMode_Linear;
    samp.magFilter = WGPUFilterMode_Linear;
    samp.mipmapFilter = WGPUMipmapFilterMode_Linear;
    samp.addressModeU = WGPUAddressMode_Repeat;
    samp.addressModeV = WGPUAddressMode_Repeat;
    samp.addressModeW = WGPUAddressMode_Repeat;
    samp.maxAnisotropy = 1;
    webgpu.defaultSampler = wgpuDeviceCreateSampler(webgpu.device, &samp);

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
    rt_create_texture_color = wg_create_texture_color;
    rt_create_texture_color_float = wg_create_texture_color_float;
    rt_create_texture_depth = wg_create_texture_depth;
    rt_create_texture_depth_stencil = wg_create_texture_depth_stencil;
    rt_destroy_texture = wg_destroy_texture;
    rt_bind_texture = wg_bind_texture;
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
    rt_draw_mesh_multi = wg_draw_mesh_multi;
    rt_create_meshlet = wg_create_meshlet;
    rt_destroy_meshlet = wg_destroy_meshlet;
    rt_draw_meshlet = wg_draw_meshlet;
    rt_create_mesh_screen = wg_create_mesh_screen;
    rt_draw_screen = wg_draw_screen;
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
    webgpu.buffers.clear();
    webgpu.textures.clear();
    webgpu.samplers.clear();
    while (!webgpu.modules.empty())
    {
        uint32_t handle = webgpu.modules.begin()->first;
        void* dummy = nullptr;
        wg_destroy_module_native(handle, dummy);
    }
    webgpu.meshes.clear();
    webgpu.meshlets.clear();
    webgpu.computePasses.clear();
    webgpu.renderPasses.clear();
    webgpu.transferPasses.clear();
    if (webgpu.pushBuffer) { wgpuBufferRelease(webgpu.pushBuffer); webgpu.pushBuffer = nullptr; }
    if (webgpu.defaultSampler) { wgpuSamplerRelease(webgpu.defaultSampler); webgpu.defaultSampler = nullptr; }
    webgpu.queue = nullptr;
    webgpu.device = nullptr;
    webgpu.bufferID = webgpu.textureID = webgpu.samplerID = webgpu.moduleID = 0;
    webgpu.meshID = webgpu.meshletID = webgpu.passID = 0;
    webgpu.currentPassType = GL_NONE;
    webgpu.currentPipeline = nullptr;

    if (rt_unload_library == wg_unload_library) rt_unload_library = nullptr;
    if (rt_create_buffer == wg_create_buffer) rt_create_buffer = nullptr;
    if (rt_destroy_buffer == wg_destroy_buffer) rt_destroy_buffer = nullptr;
    if (rt_bind_buffer == wg_bind_buffer) rt_bind_buffer = nullptr;
    if (rt_map_buffer == wg_map_buffer) rt_map_buffer = nullptr;
    if (rt_unmap_buffer == wg_unmap_buffer) rt_unmap_buffer = nullptr;
    if (rt_create_texture == wg_create_texture) rt_create_texture = nullptr;
    if (rt_create_texture_color == wg_create_texture_color) rt_create_texture_color = nullptr;
    if (rt_create_texture_color_float == wg_create_texture_color_float) rt_create_texture_color_float = nullptr;
    if (rt_create_texture_depth == wg_create_texture_depth) rt_create_texture_depth = nullptr;
    if (rt_create_texture_depth_stencil == wg_create_texture_depth_stencil) rt_create_texture_depth_stencil = nullptr;
    if (rt_destroy_texture == wg_destroy_texture) rt_destroy_texture = nullptr;
    if (rt_bind_texture == wg_bind_texture) rt_bind_texture = nullptr;
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
    if (rt_draw_mesh_multi == wg_draw_mesh_multi) rt_draw_mesh_multi = nullptr;
    if (rt_create_meshlet == wg_create_meshlet) rt_create_meshlet = nullptr;
    if (rt_destroy_meshlet == wg_destroy_meshlet) rt_destroy_meshlet = nullptr;
    if (rt_draw_meshlet == wg_draw_meshlet) rt_draw_meshlet = nullptr;
    if (rt_create_mesh_screen == wg_create_mesh_screen) rt_create_mesh_screen = nullptr;
    if (rt_draw_screen == wg_draw_screen) rt_draw_screen = nullptr;
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
    if (info.usage & (GL_BUFFER_USAGE_MAP_READ | GL_BUFFER_USAGE_MAP_WRITE))
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
    webgpu.currentBinding[bind.binding].type = GL_BINDING_BUFFER;
    webgpu.currentBinding[bind.binding].buffer = buffer;
    webgpu.currentBinding[bind.binding].buffer_bind = bind;
}

void* wg_map_buffer(rt_buffer_t& buffer, GLenum mode, size_t offset, size_t size)
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
    rt_texture_t result = {};
    if (!webgpu.device || info.width == 0) return result;
    if (info.target != GL_TEXTURE_1D && info.height == 0) return result;

    uint32_t handle = webgpu.textureID + 1;
    auto& native = webgpu.textures[handle];
    native.format = gl_to_wg_format(info.format, info.type, info.internal_format);
    native.width = info.width;
    native.height = info.target == GL_TEXTURE_1D ? 1 : info.height;
    native.depth = info.depth ? info.depth : 1;
    native.target = info.target;
    native.layers = (info.target == GL_TEXTURE_2D_ARRAY) ? native.depth : 1;
    native.mipLevels = 1;
    if (info.mipmaps == 0 && gl_has_mipmap_filter(info.min_filter))
    {
        uint32_t maxDim = std::max(native.width, native.height);
        while (maxDim >>= 1) native.mipLevels++;
    }
    else if (info.mipmaps > 1)
        native.mipLevels = info.mipmaps;

    WGPUTextureDescriptor desc = {};
    desc.size.width = native.width;
    desc.size.height = native.height;
    desc.size.depthOrArrayLayers = (info.target == GL_TEXTURE_3D) ? native.depth : native.layers;
    desc.mipLevelCount = native.mipLevels;
    desc.sampleCount = (info.target == GL_TEXTURE_2D_MULTISAMPLE && info.samples) ? info.samples : 1;
    desc.dimension = (info.target == GL_TEXTURE_3D) ? WGPUTextureDimension_3D :
                     (info.target == GL_TEXTURE_1D) ? WGPUTextureDimension_1D :
                     WGPUTextureDimension_2D;
    desc.format = native.format;
    desc.usage = (WGPUTextureUsage)(WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst |
                                    WGPUTextureUsage_CopySrc | WGPUTextureUsage_RenderAttachment |
                                    WGPUTextureUsage_StorageBinding);
    native.handle = wgpuDeviceCreateTexture(webgpu.device, &desc);
    if (!native.handle)
    {
        webgpu.textures.erase(handle);
        return {};
    }
    native.view = wgpuTextureCreateView(native.handle, nullptr);

    if (info.data)
    {
        size_t bpp = wg_format_bytes(native.format);
        size_t bytes = (size_t)native.width * native.height * ((info.target == GL_TEXTURE_3D) ? native.depth : 1) * bpp;
        WGPUImageCopyTexture dst = {};
        dst.texture = native.handle;
        WGPUTextureDataLayout layout = {};
        layout.bytesPerRow = (uint32_t)(native.width * bpp);
        layout.rowsPerImage = native.height;
        WGPUExtent3D size = {native.width, native.height, (info.target == GL_TEXTURE_3D) ? native.depth : 1};
        wgpuQueueWriteTexture(webgpu.queue, &dst, info.data, bytes, &layout, &size);
        wg_transition_image(native, WG_STATE_SHADER_READ);
    }
    else
        wg_transition_image(native, wg_is_depth(native.format) ? WG_STATE_DEPTH : WG_STATE_COLOR);

    webgpu.textureID = handle;
    result.handle = handle;
    result.width = info.width;
    result.height = native.height;
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

rt_texture_t wg_create_texture_color(uint32_t width, uint32_t height, const void* data)
{
    return wg_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_RGBA, .internal_format = GL_RGBA8, .type = GL_UNSIGNED_BYTE,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

rt_texture_t wg_create_texture_color_float(uint32_t width, uint32_t height, const void* data)
{
    return wg_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_RGBA, .internal_format = GL_RGBA32F, .type = GL_FLOAT,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

rt_texture_t wg_create_texture_depth(uint32_t width, uint32_t height, const void* data)
{
    return wg_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_DEPTH_COMPONENT, .internal_format = GL_DEPTH_COMPONENT32F, .type = GL_FLOAT,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

rt_texture_t wg_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data)
{
    return wg_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_DEPTH_STENCIL, .internal_format = GL_DEPTH32F_STENCIL8, .type = GL_FLOAT,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

void wg_destroy_texture(rt_texture_t& texture)
{
    if (texture.native)
    {
        auto it = webgpu.textures.find(texture.handle);
        if (it != webgpu.textures.end())
        {
            webgpu.textures.erase(it);
        }
    }
    texture = {};
}

void wg_bind_texture(rt_texture_t& texture, rt_texture_bind_t bind)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    webgpu.currentBinding[bind.binding].type = GL_BINDING_TEXTURE;
    webgpu.currentBinding[bind.binding].texture = texture;
    webgpu.currentBinding[bind.binding].texture_bind = bind;
}

void wg_bind_texture_storage(rt_texture_t& texture, rt_texture_storage_bind_t bind)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    webgpu.currentBinding[bind.binding].type = GL_BINDING_STORAGE_TEXTURE;
    webgpu.currentBinding[bind.binding].storage_texture = texture;
    webgpu.currentBinding[bind.binding].storage_texture_bind = bind;
}

rt_sampler_t wg_create_sampler(rt_sampler_info_t const& info)
{
    rt_sampler_t result = {};
    uint32_t handle = webgpu.samplerID + 1;
    auto& native = webgpu.samplers[handle];
    WGPUSamplerDescriptor desc = {};
    desc.minFilter = gl_to_wg_filter(info.min_filter);
    desc.magFilter = gl_to_wg_filter(info.mag_filter);
    desc.mipmapFilter = gl_to_wg_mip(info.min_filter);
    desc.addressModeU = gl_to_wg_address(info.wrap_s);
    desc.addressModeV = gl_to_wg_address(info.wrap_t);
    desc.addressModeW = gl_to_wg_address(info.wrap_r);
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
    webgpu.currentBinding[bind.binding].type = GL_BINDING_SAMPLER;
    webgpu.currentBinding[bind.binding].sampler = sampler;
    webgpu.currentBinding[bind.binding].sampler_bind = bind;
}

rt_module_compute_t wg_create_module_compute(rt_module_compute_info_t const& info)
{
    rt_module_compute_t result = {};
    if (!info.cshader || !info.clength || !webgpu.device) return result;
    uint32_t handle = webgpu.moduleID + 1;
    auto& native = webgpu.modules[handle];
    native.cshader = wg_create_shader(info.cshader, info.clength);
    if (!native.cshader || !wg_create_pipeline_layout(native, nullptr))
    {
        result.native = &native;
        wg_destroy_module_native(handle, result.native);
        return {};
    }
    WGPUComputePipelineDescriptor desc = {};
    desc.layout = native.pipelineLayout;
    desc.compute.module = native.cshader;
    desc.compute.entryPoint = wg_entry(info.centry);
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
    return result;
}

rt_module_render_t wg_create_module_render(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};
    if (!webgpu.device) return result;
    uint32_t handle = webgpu.moduleID + 1;
    auto& native = webgpu.modules[handle];
    if (info.vshader) native.vshader = wg_create_shader(info.vshader, info.vlength);
    if (info.fshader) native.fshader = wg_create_shader(info.fshader, info.flength);
    if (!wg_create_pipeline_layout(native, info.binding) || !wg_create_graphics_pipeline(native, info))
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

rt_module_render_t wg_create_module_meshlet(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};
    if (!info.mshader || !info.mlength || !webgpu.device) return result;
    uint32_t handle = webgpu.moduleID + 1;
    auto& native = webgpu.modules[handle];
    if (info.tshader) native.tshader = wg_create_shader(info.tshader, info.tlength);
    native.mshader = wg_create_shader(info.mshader, info.mlength);
    if (info.fshader) native.fshader = wg_create_shader(info.fshader, info.flength);
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

void wg_destroy_module_render(rt_module_render_t& module)
{
    wg_destroy_module_native(module.handle, module.native);
    module.handle = 0;
    module.vertex_vao = 0;
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
    wg_clear_bindings();
    pass.handle = ++webgpu.passID;
    auto& native = webgpu.computePasses[pass.handle];
    pass.native = &native;
    webgpu.currentPassType = GL_MODULE_COMPUTE;
    webgpu.currentComputePass = &pass;
    wg_end_pass_encoders();
    wg_ensure_encoder();
    webgpu.computePass = wgpuCommandEncoderBeginComputePass(webgpu.encoder, nullptr);
    auto* mod = (wg_module_native_t*)pass.module.native;
    if (mod && mod->computePipeline)
        wgpuComputePassEncoderSetPipeline(webgpu.computePass, mod->computePipeline);
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
    webgpu.currentPassType = GL_NONE;
    webgpu.currentPipeline = nullptr;
    wg_clear_bindings();
}

void wg_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != GL_MODULE_COMPUTE)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    wg_flush_descriptors();
    if (webgpu.computePass)
        wgpuComputePassEncoderDispatchWorkgroups(webgpu.computePass, std::max(1u, groupX), std::max(1u, groupY), std::max(1u, groupZ));
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
    wg_clear_bindings();
    pass.handle = ++webgpu.passID;
    auto& native = webgpu.renderPasses[pass.handle];
    pass.native = &native;
    webgpu.currentPassType = GL_MODULE_RENDER;
    webgpu.currentRenderPass = &pass;

    native.offscreen = pass.depth.texture.handle != 0;
    for (auto& color : pass.colors)
        if (color.texture.handle) native.offscreen = true;

    WGPURenderPassColorAttachment colors[GL_MAX_COLOR_TEXTURE_NUM] = {};
    uint32_t colorCount = 0;
    uint32_t width = 0, height = 0;
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        if (auto* tex = wg_texture_native(pass.colors[i].texture))
        {
            wg_transition_image(*tex, WG_STATE_COLOR);
            colors[colorCount].view = tex->view;
            colors[colorCount].loadOp = pass.colors[i].clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
            colors[colorCount].storeOp = WGPUStoreOp_Store;
            colors[colorCount].clearValue = {pass.colors[i].value.r, pass.colors[i].value.g, pass.colors[i].value.b, pass.colors[i].value.a};
            width = std::max(width, pass.colors[i].texture.width);
            height = std::max(height, pass.colors[i].texture.height);
            colorCount++;
        }
    }

    WGPURenderPassDepthStencilAttachment depth = {};
    bool hasDepth = false;
    if (auto* tex = wg_texture_native(pass.depth.texture))
    {
        wg_transition_image(*tex, WG_STATE_DEPTH);
        depth.view = tex->view;
        depth.depthLoadOp = pass.depth.clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
        depth.depthStoreOp = WGPUStoreOp_Store;
        depth.depthClearValue = pass.depth.value;
        if (pass.depth.texture.format == GL_DEPTH_STENCIL)
        {
            depth.stencilLoadOp = pass.stencil.clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
            depth.stencilStoreOp = WGPUStoreOp_Store;
            depth.stencilClearValue = (uint32_t)pass.stencil.value;
        }
        width = std::max(width, pass.depth.texture.width);
        height = std::max(height, pass.depth.texture.height);
        hasDepth = true;
    }
    native.width = width;
    native.height = height;

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
        if (auto* tex = wg_texture_native(color.texture))
            wg_transition_image(*tex, WG_STATE_SHADER_READ);
    if (auto* tex = wg_texture_native(pass.depth.texture))
        wg_transition_image(*tex, WG_STATE_SHADER_READ);
    webgpu.renderPasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    webgpu.currentPassType = GL_NONE;
    webgpu.currentPipeline = nullptr;
    wg_clear_bindings();
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
    if (webgpu.currentPassType != GL_MODULE_RENDER)
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
    pass.handle = ++webgpu.passID;
    auto& native = webgpu.transferPasses[pass.handle];
    pass.native = &native;
    webgpu.currentPassType = GL_MODULE_TRANSFER;
    webgpu.currentTransferPass = &pass;
    wg_end_pass_encoders();
    wg_ensure_encoder();
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
    webgpu.currentPassType = GL_NONE;
    webgpu.currentPipeline = nullptr;
}

void wg_copy_buffer(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize)
{
    if (webgpu.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (webgpu.currentPassType != GL_MODULE_TRANSFER)
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
    if (webgpu.currentPassType != GL_MODULE_TRANSFER)
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
    if (webgpu.currentPassType != GL_MODULE_TRANSFER)
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
    WGPUImageCopyTexture srcCopy = {};
    srcCopy.texture = src->handle;
    srcCopy.mipLevel = source.mipLevel;
    srcCopy.origin = {source.origin.x, source.origin.y, source.origin.z};
    WGPUImageCopyBuffer dstCopy = {};
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
    if (webgpu.currentPassType != GL_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* src = wg_texture_native(source.texture);
    auto* dst = wg_texture_native(destination.texture);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    wg_transition_image(*src, WG_STATE_COPY_SRC);
    wg_transition_image(*dst, WG_STATE_COPY_DST);
    WGPUImageCopyTexture srcCopy = {};
    srcCopy.texture = src->handle;
    srcCopy.mipLevel = source.mipLevel;
    srcCopy.origin = {source.origin.x, source.origin.y, source.origin.z};
    WGPUImageCopyTexture dstCopy = {};
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
    if (webgpu.currentPassType != GL_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* dst = wg_texture_native(destination.texture);
    if (!source.data || !dst || copySize.x == 0 || copySize.y == 0) return;
    uint32_t bpp = wg_format_bytes(dst->format);
    size_t bytes = source.size ? source.size : (size_t)std::max(copySize.x * bpp, 1u) * copySize.y * std::max(1u, copySize.z);
    WGPUImageCopyTexture dstCopy = {};
    dstCopy.texture = dst->handle;
    dstCopy.mipLevel = destination.mipLevel;
    dstCopy.origin = {destination.origin.x, destination.origin.y, destination.origin.z};
    WGPUTextureDataLayout layout = {};
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
    if (webgpu.currentPassType != GL_MODULE_TRANSFER)
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
    WGPUImageCopyBuffer srcCopy = {};
    srcCopy.buffer = src->handle;
    srcCopy.layout.offset = source.offset;
    srcCopy.layout.bytesPerRow = source.bytesPerRow ? source.bytesPerRow : copySize.x * bpp;
    srcCopy.layout.rowsPerImage = source.rowsPerImage ? source.rowsPerImage : copySize.y;
    WGPUImageCopyTexture dstCopy = {};
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
        result.vertex[0] = wg_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = wg_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = wg_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = wg_create_buffer({.size = index_count * sizeof(uint32_t), .usage = GL_BUFFER_USAGE_INDEX | GL_BUFFER_USAGE_COPY_DST, .data = indices});
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
    if (webgpu.currentPassType != GL_MODULE_RENDER)
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
        if (layout.type == GL_NONE || layout.count == 0) continue;
        for (uint32_t k = 0; k < std::size(mesh.vertex); ++k)
        {
            if (mesh.vertex[k].handle == 0 || mesh.location[k] != layout.location) continue;
            auto* native = wg_buffer_native(mesh.vertex[k]);
            if (!native) break;
            wg_transition_buffer(*native, WG_STATE_VERTEX);
            wgpuRenderPassEncoderSetVertexBuffer(webgpu.renderPass, layout.location, native->handle, 0, mesh.vertex[k].size);
            uint32_t stride = gl_vertex_size(layout.type, layout.count);
            if (vertex_count == 0 && stride)
                vertex_count = (uint32_t)(mesh.vertex[k].size / stride);
            break;
        }
    }
    if (mesh.index.handle)
    {
        auto* native = wg_buffer_native(mesh.index);
        if (!native) return;
        uint32_t indexStride = gl_index_type_size(module.index_type);
        wg_transition_buffer(*native, WG_STATE_INDEX);
        wgpuRenderPassEncoderSetIndexBuffer(webgpu.renderPass, native->handle, gl_to_wg_index_type(module.index_type), 0, mesh.index.size);
        wgpuRenderPassEncoderDrawIndexed(webgpu.renderPass, (uint32_t)(mesh.index.size / indexStride), instanceCount, 0, 0, 0);
    }
    else
        wgpuRenderPassEncoderDraw(webgpu.renderPass, vertex_count, instanceCount, 0, 0);
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
        result.vertex[0] = wg_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = wg_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = wg_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = wg_create_buffer({.size = index_count * sizeof(uint32_t), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = indices});
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
    if (webgpu.currentPassType != GL_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    rt_module_render_t const& module = webgpu.currentRenderPass->module;
    uint32_t index_binding = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.type == GL_NONE || layout.count == 0) continue;
        for (uint32_t k = 0; k < std::size(meshlet.vertex); ++k)
        {
            if (meshlet.vertex[k].handle == 0 || meshlet.location[k] != layout.location) continue;
            wg_bind_buffer(meshlet.vertex[k], {.binding = layout.location, .target = GL_SHADER_STORAGE_BUFFER});
            break;
        }
        if (layout.location + 1 > index_binding)
            index_binding = layout.location + 1;
    }
    if (meshlet.index.handle)
        wg_bind_buffer(meshlet.index, {.binding = index_binding, .target = GL_SHADER_STORAGE_BUFFER});
    wg_flush_descriptors();
}

rt_mesh_t wg_create_mesh_screen()
{
    const float points[] = {-1.0f, -1.0f, 0.0f, +3.0f, -1.0f, 0.0f, -1.0f, +3.0f, 0.0f};
    const float uvs[] = {0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 2.0f};
    return wg_create_mesh(points, nullptr, uvs, 3, nullptr, 0);
}

void wg_draw_screen(int width, int height, rt_color_t clear, rt_texture_t& texture)
{
    static auto module = wg_create_module_render({
        .vertex = {rt_vertex_vertex, {}, rt_vertex_uv},
        .binding = {{.binding = 0, .type = GL_BINDING_TEXTURE}},
    });
    if (!module.handle) return;
    rt_pass_render_t pass = {.module = module, .screen = {.color = {.clear = true, .value = clear}}};
    wg_begin_render(pass);
    wg_set_viewport(0, 0, width, height);
    wg_bind_texture(texture, {.binding = 0});
    static auto mesh = wg_create_mesh_screen();
    wg_draw_mesh(mesh);
    wg_end_render(pass);
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


