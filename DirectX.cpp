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
#ifdef DIRECTX_IMPLEMENTATION
#ifdef _WIN32
#include "DirectX.h"
#include <algorithm>
#include <cstdio>
#include <map>
#include <numeric>
#include <vector>
#include <windows.h>
#include <wrl/client.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

enum rt_module_type_t : uint32_t
{
    GL_MODULE_RENDER = 1,
    GL_MODULE_COMPUTE = 2,
    GL_MODULE_MESHLET = 3,
    GL_MODULE_TRANSFER = 4,
};

static D3D12_FILTER gl_to_dx_filter(GLenum minFilter, GLenum magFilter)
{
    bool magLinear = (magFilter != GL_NEAREST);
    bool minLinear = !(minFilter == GL_NEAREST || minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR);
    bool mipLinear = (minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR);
    bool hasMip = (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST ||
                   minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR);
    if (!hasMip)
        return magLinear && minLinear ? D3D12_FILTER_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_MIP_POINT;
    if (minLinear && magLinear && mipLinear) return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    if (!minLinear && !magLinear && !mipLinear) return D3D12_FILTER_MIN_MAG_MIP_POINT;
    return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
}

static D3D12_TEXTURE_ADDRESS_MODE gl_to_dx_address(GLenum wrap)
{
    switch (wrap)
    {
        case GL_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case GL_MIRRORED_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case GL_CLAMP_TO_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case GL_MIRROR_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        case GL_CLAMP_TO_EDGE:
        default: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    }
}

static bool gl_has_mipmap_filter(GLenum minFilter)
{
    return minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST ||
           minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR;
}

static DXGI_FORMAT gl_to_dx_format(GLenum glFormat, GLenum glType, GLenum internalFormat = 0)
{
    switch (internalFormat)
    {
        case GL_R8: return DXGI_FORMAT_R8_UNORM;
        case GL_RG8: return DXGI_FORMAT_R8G8_UNORM;
        case GL_RGB8:
        case GL_RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case GL_SRGB8_ALPHA8: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case GL_R16F: return DXGI_FORMAT_R16_FLOAT;
        case GL_RG16F: return DXGI_FORMAT_R16G16_FLOAT;
        case GL_RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case GL_R32F: return DXGI_FORMAT_R32_FLOAT;
        case GL_RG32F: return DXGI_FORMAT_R32G32_FLOAT;
        case GL_RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case GL_DEPTH_COMPONENT16: return DXGI_FORMAT_D16_UNORM;
        case GL_DEPTH_COMPONENT24: return DXGI_FORMAT_D32_FLOAT;
        case GL_DEPTH_COMPONENT32F: return DXGI_FORMAT_D32_FLOAT;
        case GL_DEPTH24_STENCIL8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case GL_DEPTH32F_STENCIL8: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        default: break;
    }
    switch (glFormat)
    {
        case GL_RED: return (glType == GL_FLOAT) ? DXGI_FORMAT_R32_FLOAT : DXGI_FORMAT_R8_UNORM;
        case GL_RG: return (glType == GL_FLOAT) ? DXGI_FORMAT_R32G32_FLOAT : DXGI_FORMAT_R8G8_UNORM;
        case GL_RGB:
        case GL_RGBA: return (glType == GL_FLOAT) ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
        case GL_DEPTH_COMPONENT: return DXGI_FORMAT_D32_FLOAT;
        case GL_DEPTH_STENCIL: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        default: return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
}

static DXGI_FORMAT gl_to_dx_vertex_format(GLenum type, GLenum count)
{
    if (type == GL_FLOAT)
    {
        switch (count)
        {
            case 1: return DXGI_FORMAT_R32_FLOAT;
            case 2: return DXGI_FORMAT_R32G32_FLOAT;
            case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
            case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            default: return DXGI_FORMAT_R32_FLOAT;
        }
    }
    if (type == GL_INT)
    {
        switch (count)
        {
            case 1: return DXGI_FORMAT_R32_SINT;
            case 2: return DXGI_FORMAT_R32G32_SINT;
            case 3: return DXGI_FORMAT_R32G32B32_SINT;
            case 4: return DXGI_FORMAT_R32G32B32A32_SINT;
            default: return DXGI_FORMAT_R32_SINT;
        }
    }
    if (type == GL_UNSIGNED_INT)
    {
        switch (count)
        {
            case 1: return DXGI_FORMAT_R32_UINT;
            case 2: return DXGI_FORMAT_R32G32_UINT;
            case 3: return DXGI_FORMAT_R32G32B32_UINT;
            case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
            default: return DXGI_FORMAT_R32_UINT;
        }
    }
    return DXGI_FORMAT_R32_FLOAT;
}

static D3D12_COMPARISON_FUNC gl_to_dx_compare(GLenum func)
{
    switch (func)
    {
        case GL_NEVER: return D3D12_COMPARISON_FUNC_NEVER;
        case GL_LESS: return D3D12_COMPARISON_FUNC_LESS;
        case GL_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
        case GL_LEQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case GL_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
        case GL_NOTEQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case GL_GEQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case GL_ALWAYS:
        default: return D3D12_COMPARISON_FUNC_ALWAYS;
    }
}

static D3D12_BLEND gl_to_dx_blend(GLenum factor)
{
    switch (factor)
    {
        case GL_ZERO: return D3D12_BLEND_ZERO;
        case GL_ONE: return D3D12_BLEND_ONE;
        case GL_SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
        case GL_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
        case GL_DST_COLOR: return D3D12_BLEND_DEST_COLOR;
        case GL_ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
        case GL_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
        case GL_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
        case GL_ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
        case GL_CONSTANT_COLOR: return D3D12_BLEND_BLEND_FACTOR;
        case GL_ONE_MINUS_CONSTANT_COLOR: return D3D12_BLEND_INV_BLEND_FACTOR;
        case GL_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
        default: return D3D12_BLEND_ONE;
    }
}

static D3D12_BLEND_OP gl_to_dx_blend_op(GLenum func)
{
    switch (func)
    {
        case GL_FUNC_SUBTRACT: return D3D12_BLEND_OP_SUBTRACT;
        case GL_FUNC_REVERSE_SUBTRACT: return D3D12_BLEND_OP_REV_SUBTRACT;
        case GL_MIN: return D3D12_BLEND_OP_MIN;
        case GL_MAX: return D3D12_BLEND_OP_MAX;
        case GL_FUNC_ADD:
        default: return D3D12_BLEND_OP_ADD;
    }
}

static D3D12_CULL_MODE gl_to_dx_cull(GLenum mode)
{
    switch (mode)
    {
        case GL_FRONT: return D3D12_CULL_MODE_FRONT;
        case GL_BACK: return D3D12_CULL_MODE_BACK;
        default: return D3D12_CULL_MODE_NONE;
    }
}

static D3D12_FILL_MODE gl_to_dx_fill(GLenum fill)
{
    return (fill == GL_LINE || fill == GL_POINT) ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
}

static D3D12_PRIMITIVE_TOPOLOGY gl_to_dx_topology(GLenum primitive)
{
    switch (primitive)
    {
        case GL_POINTS: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case GL_LINES: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case GL_LINE_STRIP:
        case GL_LINE_LOOP: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case GL_TRIANGLE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case GL_TRIANGLES:
        default: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

static D3D12_PRIMITIVE_TOPOLOGY_TYPE gl_to_dx_topology_type(GLenum primitive)
{
    switch (primitive)
    {
        case GL_POINTS: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case GL_LINES:
        case GL_LINE_STRIP:
        case GL_LINE_LOOP: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        default: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }
}

static D3D12_STENCIL_OP gl_to_dx_stencil_op(GLenum op)
{
    switch (op)
    {
        case GL_ZERO: return D3D12_STENCIL_OP_ZERO;
        case GL_REPLACE: return D3D12_STENCIL_OP_REPLACE;
        case GL_INCR: return D3D12_STENCIL_OP_INCR_SAT;
        case GL_INCR_WRAP: return D3D12_STENCIL_OP_INCR;
        case GL_DECR: return D3D12_STENCIL_OP_DECR_SAT;
        case GL_DECR_WRAP: return D3D12_STENCIL_OP_DECR;
        case GL_INVERT: return D3D12_STENCIL_OP_INVERT;
        case GL_KEEP:
        default: return D3D12_STENCIL_OP_KEEP;
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

static DXGI_FORMAT gl_to_dx_index_type(GLenum type)
{
    return (type == GL_UNSIGNED_SHORT) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}

static uint32_t dx_format_bytes(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R8_UNORM: return 1;
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_FLOAT: return 2;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R16G16_FLOAT: return 4;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return 8;
        case DXGI_FORMAT_R32G32B32_FLOAT: return 12;
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return 16;
        default: return 4;
    }
}

static bool dx_is_depth(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_D16_UNORM || format == DXGI_FORMAT_D32_FLOAT ||
           format == DXGI_FORMAT_D24_UNORM_S8_UINT || format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
}

static DXGI_FORMAT dx_typeless(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_D16_UNORM: return DXGI_FORMAT_R16_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_TYPELESS;
        case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24G8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32G8X24_TYPELESS;
        default: return format;
    }
}

static DXGI_FORMAT dx_srv_format(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_D16_UNORM: return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        default: return format;
    }
}

enum dx_binding_kind_t : uint32_t
{
    DX_KIND_NONE = 0,
    DX_KIND_CBV,
    DX_KIND_SRV,
    DX_KIND_UAV,
    DX_KIND_SAMPLER,
};

// ====================================================================

struct rt_buffer_native_t
{
    ComPtr<ID3D12Resource> handle;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    D3D12_HEAP_TYPE heap = D3D12_HEAP_TYPE_DEFAULT;
    void* mapped = nullptr;
    size_t mappedOffset = 0;
    size_t mappedSize = 0;
};

struct rt_texture_native_t
{
    ComPtr<ID3D12Resource> handle;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    uint32_t mipLevels = 1;
    uint32_t layers = 1;
    uint32_t width = 1, height = 1, depth = 1;
    GLenum target = GL_TEXTURE_2D;
    uint32_t samples = 1;
};

struct rt_sampler_native_t
{
    D3D12_SAMPLER_DESC desc = {};
};

struct rt_module_native_t
{
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipeline;
    D3D12_SHADER_BYTECODE vshader = {}, tshader = {}, mshader = {}, fshader = {}, cshader = {};
    std::vector<uint8_t> vcode, tcode, mcode, fcode, ccode;
    bool isMeshlet = false;
    bool isCompute = false;
    D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    dx_binding_kind_t kinds[GL_MAX_BINDING_HANDLE_NUM] = {};
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

struct rt_pass_compute_native_t { uint32_t dummy = 0; };
struct rt_pass_render_native_t { bool offscreen = false; uint32_t width = 0, height = 0; };
struct rt_pass_transfer_native_t { uint32_t dummy = 0; };

struct dx_staging_t
{
    ComPtr<ID3D12Resource> buffer;
};

struct dx_native_t
{
    uint32_t bufferID = 0, textureID = 0, samplerID = 0, moduleID = 0;
    uint32_t meshID = 0, meshletID = 0, passID = 0;

    std::map<uint32_t, rt_buffer_native_t> buffers;
    std::map<uint32_t, rt_texture_native_t> textures;
    std::map<uint32_t, rt_sampler_native_t> samplers;
    std::map<uint32_t, rt_module_native_t> modules;
    std::map<uint32_t, rt_mesh_native_t> meshes;
    std::map<uint32_t, rt_meshlet_native_t> meshlets;
    std::map<uint32_t, rt_pass_compute_native_t> computePasses;
    std::map<uint32_t, rt_pass_render_native_t> renderPasses;
    std::map<uint32_t, rt_pass_transfer_native_t> transferPasses;

    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> cmd;
    ComPtr<ID3D12GraphicsCommandList6> cmdMesh;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    ComPtr<ID3D12DescriptorHeap> samplerHeap;
    ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent = nullptr;
    uint64_t fenceValue = 0;
    uint32_t rtvSize = 0, dsvSize = 0, srvSize = 0, samplerSize = 0;
    uint32_t srvCursor = 0, samplerCursor = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE defaultSamplerCPU = {};
    std::vector<dx_staging_t> pendingStaging;

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
} static direct;

static void dx_wait_gpu()
{
    if (!direct.queue || !direct.fence) return;
    uint64_t value = ++direct.fenceValue;
    direct.queue->Signal(direct.fence.Get(), value);
    if (direct.fence->GetCompletedValue() < value)
    {
        direct.fence->SetEventOnCompletion(value, direct.fenceEvent);
        WaitForSingleObject(direct.fenceEvent, INFINITE);
    }
}

static void dx_flush_staging()
{
    direct.pendingStaging.clear();
}

static bool dx_create_staging(size_t size, dx_staging_t& staging, void** mapped)
{
    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(direct.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&staging.buffer))))
        return false;
    if (mapped)
    {
        D3D12_RANGE range = {0, 0};
        if (FAILED(staging.buffer->Map(0, &range, mapped)))
            return false;
    }
    return true;
}

static void dx_transition_buffer(rt_buffer_native_t& buffer, D3D12_RESOURCE_STATES dst)
{
    if (!buffer.handle || buffer.state == dst) return;
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = buffer.handle.Get();
    barrier.Transition.StateBefore = buffer.state;
    barrier.Transition.StateAfter = dst;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    direct.cmd->ResourceBarrier(1, &barrier);
    buffer.state = dst;
}

static void dx_transition_image(rt_texture_native_t& image, D3D12_RESOURCE_STATES dst)
{
    if (!image.handle || image.state == dst) return;
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = image.handle.Get();
    barrier.Transition.StateBefore = image.state;
    barrier.Transition.StateAfter = dst;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    direct.cmd->ResourceBarrier(1, &barrier);
    image.state = dst;
}

static rt_buffer_native_t* dx_buffer_native(rt_buffer_t const& buffer)
{
    if (!buffer.native || buffer.handle == 0) return nullptr;
    auto it = direct.buffers.find(buffer.handle);
    return it == direct.buffers.end() ? nullptr : &it->second;
}

static rt_texture_native_t* dx_texture_native(rt_texture_t const& texture)
{
    if (!texture.native || texture.handle == 0) return nullptr;
    auto it = direct.textures.find(texture.handle);
    return it == direct.textures.end() ? nullptr : &it->second;
}

static rt_sampler_native_t* dx_sampler_native(rt_sampler_t const& sampler)
{
    if (!sampler.native || sampler.handle == 0) return nullptr;
    auto it = direct.samplers.find(sampler.handle);
    return it == direct.samplers.end() ? nullptr : &it->second;
}

static rt_module_native_t* dx_current_module_native()
{
    if (direct.currentPassType == GL_MODULE_COMPUTE && direct.currentComputePass)
        return (rt_module_native_t*)direct.currentComputePass->module.native;
    if (direct.currentPassType == GL_MODULE_RENDER && direct.currentRenderPass)
        return (rt_module_native_t*)direct.currentRenderPass->module.native;
    return nullptr;
}

static void dx_require_pass(GLenum type)
{
    if (direct.currentPipeline == nullptr || direct.currentPassType != type)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
}

static void dx_clear_bindings()
{
    for (auto& binding : direct.currentBinding)
        binding = {};
}

static void dx_copy_shader(D3D12_SHADER_BYTECODE& dest, std::vector<uint8_t>& store, const char* data, uint32_t length)
{
    dest = {};
    store.clear();
    if (!data || !length) return;
    store.assign((const uint8_t*)data, (const uint8_t*)data + length);
    dest.pShaderBytecode = store.data();
    dest.BytecodeLength = store.size();
}

static bool dx_setup_root(rt_module_native_t& native, rt_binding_t const* bindings)
{
    native.descriptorCount = 0;
    D3D12_DESCRIPTOR_RANGE ranges[GL_MAX_BINDING_HANDLE_NUM] = {};
    D3D12_ROOT_PARAMETER params[1 + GL_MAX_BINDING_HANDLE_NUM] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[0].Constants.ShaderRegister = 16;
    params[0].Constants.RegisterSpace = 0;
    params[0].Constants.Num32BitValues = 32;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    uint32_t paramCount = 1;
    if (bindings)
    {
        for (uint32_t i = 0; i < GL_MAX_BINDING_HANDLE_NUM; ++i)
        {
            if (bindings[i].type == GL_NONE) continue;
            dx_binding_kind_t kind = DX_KIND_CBV;
            D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            if (bindings[i].type == GL_BINDING_TEXTURE)
            {
                kind = DX_KIND_SRV;
                rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            }
            else if (bindings[i].type == GL_BINDING_STORAGE_TEXTURE)
            {
                kind = DX_KIND_UAV;
                rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            }
            else if (bindings[i].type == GL_BINDING_SAMPLER)
            {
                kind = DX_KIND_SAMPLER;
                rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            }
            native.kinds[native.descriptorCount] = kind;
            native.descriptorBindings[native.descriptorCount] = bindings[i].binding;
            ranges[native.descriptorCount].RangeType = rangeType;
            ranges[native.descriptorCount].NumDescriptors = 1;
            ranges[native.descriptorCount].BaseShaderRegister = bindings[i].binding;
            ranges[native.descriptorCount].RegisterSpace = 0;
            ranges[native.descriptorCount].OffsetInDescriptorsFromTableStart = 0;
            params[paramCount].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            params[paramCount].DescriptorTable.NumDescriptorRanges = 1;
            params[paramCount].DescriptorTable.pDescriptorRanges = &ranges[native.descriptorCount];
            params[paramCount].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            native.descriptorCount++;
            paramCount++;
        }
    }

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = paramCount;
    desc.pParameters = params;
    desc.Flags = native.isCompute ? D3D12_ROOT_SIGNATURE_FLAG_NONE :
                 D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> blob, error;
    if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error)))
        return false;
    return SUCCEEDED(direct.device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_PPV_ARGS(&native.rootSignature)));
}

static void dx_fill_render_state(rt_module_render_t& result, rt_module_render_info_t const& info)
{
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
}

static bool dx_create_graphics_pipeline(rt_module_native_t& native, rt_module_render_info_t const& info, bool meshlet)
{
    D3D12_INPUT_ELEMENT_DESC elements[GL_MAX_VERTEX_BUFFER_NUM] = {};
    uint32_t attrCount = 0;
    if (!meshlet)
    {
        for (uint32_t i = 0; i < GL_MAX_VERTEX_BUFFER_NUM; ++i)
        {
            if (info.vertex[i].type == GL_NONE || info.vertex[i].count == 0) continue;
            elements[attrCount].SemanticName = "TEXCOORD";
            elements[attrCount].SemanticIndex = info.vertex[i].location;
            elements[attrCount].Format = gl_to_dx_vertex_format(info.vertex[i].type, info.vertex[i].count);
            elements[attrCount].InputSlot = info.vertex[i].location;
            elements[attrCount].AlignedByteOffset = 0;
            elements[attrCount].InputSlotClass = info.vertex[i].instance ?
                D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            elements[attrCount].InstanceDataStepRate = info.vertex[i].instance ? 1 : 0;
            attrCount++;
        }
    }

    const bool depthEnabled = (info.depth.func != GL_ALWAYS || info.depth.write);
    const bool stencilEnabled =
        (info.stencil.back.func != GL_ALWAYS || info.stencil.back.sfail != GL_KEEP ||
         info.stencil.back.zfail != GL_KEEP || info.stencil.back.zpass != GL_KEEP ||
         info.stencil.front.func != GL_ALWAYS || info.stencil.front.sfail != GL_KEEP ||
         info.stencil.front.zfail != GL_KEEP || info.stencil.front.zpass != GL_KEEP);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = native.rootSignature.Get();
    pso.VS = native.vshader;
    pso.PS = native.fshader;
    pso.InputLayout = {meshlet ? nullptr : elements, meshlet ? 0u : attrCount};
    pso.PrimitiveTopologyType = gl_to_dx_topology_type(info.primitive);
    pso.RasterizerState.FillMode = gl_to_dx_fill(info.fill_mode);
    pso.RasterizerState.CullMode = gl_to_dx_cull(info.cull_mode);
    pso.RasterizerState.FrontCounterClockwise = (info.front_face != GL_CW);
    pso.RasterizerState.DepthBias = (INT)info.depth.bias;
    pso.RasterizerState.DepthBiasClamp = info.depth.biasClamp;
    pso.RasterizerState.SlopeScaledDepthBias = info.depth.biasSlope;
    pso.RasterizerState.DepthClipEnable = TRUE;
    pso.BlendState.AlphaToCoverageEnable = FALSE;
    pso.BlendState.IndependentBlendEnable = TRUE;
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        auto& rt = pso.BlendState.RenderTarget[i];
        bool blend =
            (info.colors[i].color.func != GL_FUNC_ADD || info.colors[i].color.src != GL_ONE ||
             info.colors[i].color.dst != GL_ZERO || info.colors[i].alpha.func != GL_FUNC_ADD ||
             info.colors[i].alpha.src != GL_ONE || info.colors[i].alpha.dst != GL_ZERO);
        rt.BlendEnable = blend;
        rt.SrcBlend = gl_to_dx_blend(info.colors[i].color.src);
        rt.DestBlend = gl_to_dx_blend(info.colors[i].color.dst);
        rt.BlendOp = gl_to_dx_blend_op(info.colors[i].color.func);
        rt.SrcBlendAlpha = gl_to_dx_blend(info.colors[i].alpha.src);
        rt.DestBlendAlpha = gl_to_dx_blend(info.colors[i].alpha.dst);
        rt.BlendOpAlpha = gl_to_dx_blend_op(info.colors[i].alpha.func);
        rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        pso.RTVFormats[i] = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    pso.NumRenderTargets = GL_MAX_COLOR_TEXTURE_NUM;
    pso.SampleMask = UINT_MAX;
    pso.SampleDesc.Count = 1;
    pso.DepthStencilState.DepthEnable = depthEnabled;
    pso.DepthStencilState.DepthWriteMask = info.depth.write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    pso.DepthStencilState.DepthFunc = gl_to_dx_compare(info.depth.func);
    pso.DepthStencilState.StencilEnable = stencilEnabled;
    pso.DepthStencilState.StencilReadMask = (UINT8)info.stencil.read;
    pso.DepthStencilState.StencilWriteMask = (UINT8)info.stencil.write;
    pso.DepthStencilState.FrontFace.StencilFailOp = gl_to_dx_stencil_op(info.stencil.front.sfail);
    pso.DepthStencilState.FrontFace.StencilDepthFailOp = gl_to_dx_stencil_op(info.stencil.front.zfail);
    pso.DepthStencilState.FrontFace.StencilPassOp = gl_to_dx_stencil_op(info.stencil.front.zpass);
    pso.DepthStencilState.FrontFace.StencilFunc = gl_to_dx_compare(info.stencil.front.func);
    pso.DepthStencilState.BackFace.StencilFailOp = gl_to_dx_stencil_op(info.stencil.back.sfail);
    pso.DepthStencilState.BackFace.StencilDepthFailOp = gl_to_dx_stencil_op(info.stencil.back.zfail);
    pso.DepthStencilState.BackFace.StencilPassOp = gl_to_dx_stencil_op(info.stencil.back.zpass);
    pso.DepthStencilState.BackFace.StencilFunc = gl_to_dx_compare(info.stencil.back.func);
    if (stencilEnabled) pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    else if (depthEnabled) pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    native.topology = gl_to_dx_topology(info.primitive);
    return SUCCEEDED(direct.device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&native.pipeline)));
}

static void dx_destroy_module_native(uint32_t handle, void*& native)
{
    direct.modules.erase(handle);
    native = nullptr;
}

static D3D12_GPU_DESCRIPTOR_HANDLE dx_alloc_srv()
{
    if (direct.srvCursor >= 2047) direct.srvCursor = 1;
    uint32_t index = direct.srvCursor++;
    D3D12_GPU_DESCRIPTOR_HANDLE handle = direct.srvHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += (SIZE_T)index * direct.srvSize;
    return handle;
}

static D3D12_CPU_DESCRIPTOR_HANDLE dx_srv_cpu(D3D12_GPU_DESCRIPTOR_HANDLE gpu)
{
    SIZE_T offset = gpu.ptr - direct.srvHeap->GetGPUDescriptorHandleForHeapStart().ptr;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu = direct.srvHeap->GetCPUDescriptorHandleForHeapStart();
    cpu.ptr += offset;
    return cpu;
}

static D3D12_GPU_DESCRIPTOR_HANDLE dx_alloc_sampler()
{
    if (direct.samplerCursor >= 255) direct.samplerCursor = 1;
    uint32_t index = direct.samplerCursor++;
    D3D12_GPU_DESCRIPTOR_HANDLE handle = direct.samplerHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += (SIZE_T)index * direct.samplerSize;
    return handle;
}

static D3D12_CPU_DESCRIPTOR_HANDLE dx_sampler_cpu(D3D12_GPU_DESCRIPTOR_HANDLE gpu)
{
    SIZE_T offset = gpu.ptr - direct.samplerHeap->GetGPUDescriptorHandleForHeapStart().ptr;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu = direct.samplerHeap->GetCPUDescriptorHandleForHeapStart();
    cpu.ptr += offset;
    return cpu;
}

static void dx_flush_descriptors()
{
    auto* mod = dx_current_module_native();
    if (!mod || !mod->rootSignature) return;

    ID3D12DescriptorHeap* heaps[] = {direct.srvHeap.Get(), direct.samplerHeap.Get()};
    direct.cmd->SetDescriptorHeaps(2, heaps);
    if (mod->isCompute)
        direct.cmd->SetComputeRootSignature(mod->rootSignature.Get());
    else
        direct.cmd->SetGraphicsRootSignature(mod->rootSignature.Get());

    for (uint32_t i = 0; i < mod->descriptorCount; ++i)
    {
        uint32_t binding = mod->descriptorBindings[i];
        auto& slot = direct.currentBinding[binding];
        uint32_t root = i + 1;
        if (mod->kinds[i] == DX_KIND_CBV || (mod->kinds[i] == DX_KIND_SRV && slot.type == GL_BINDING_BUFFER) ||
            (slot.buffer_bind.target == GL_SHADER_STORAGE_BUFFER && slot.buffer.handle))
        {
            auto* buf = dx_buffer_native(slot.buffer);
            if (!buf) continue;
            bool storage = (slot.buffer_bind.target == GL_SHADER_STORAGE_BUFFER) || (mod->kinds[i] == DX_KIND_UAV);
            dx_transition_buffer(*buf, storage ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            auto gpu = dx_alloc_srv();
            if (storage)
            {
                D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};
                uav.Format = DXGI_FORMAT_R32_TYPELESS;
                uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                uav.Buffer.NumElements = (UINT)(slot.buffer.size / 4);
                uav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
                direct.device->CreateUnorderedAccessView(buf->handle.Get(), nullptr, &uav, dx_srv_cpu(gpu));
            }
            else if (mod->kinds[i] == DX_KIND_CBV)
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = {};
                cbv.BufferLocation = buf->handle->GetGPUVirtualAddress();
                cbv.SizeInBytes = (UINT)((slot.buffer.size + 255) & ~255ull);
                direct.device->CreateConstantBufferView(&cbv, dx_srv_cpu(gpu));
            }
            else
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
                srv.Format = DXGI_FORMAT_R32_TYPELESS;
                srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srv.Buffer.NumElements = (UINT)(slot.buffer.size / 4);
                srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
                direct.device->CreateShaderResourceView(buf->handle.Get(), &srv, dx_srv_cpu(gpu));
            }
            if (mod->isCompute) direct.cmd->SetComputeRootDescriptorTable(root, gpu);
            else direct.cmd->SetGraphicsRootDescriptorTable(root, gpu);
        }
        else if (mod->kinds[i] == DX_KIND_SAMPLER)
        {
            auto gpu = dx_alloc_sampler();
            D3D12_SAMPLER_DESC desc = {};
            desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            desc.MaxLOD = D3D12_FLOAT32_MAX;
            if (auto* samp = dx_sampler_native(slot.sampler))
                desc = samp->desc;
            direct.device->CreateSampler(&desc, dx_sampler_cpu(gpu));
            if (mod->isCompute) direct.cmd->SetComputeRootDescriptorTable(root, gpu);
            else direct.cmd->SetGraphicsRootDescriptorTable(root, gpu);
        }
        else if (mod->kinds[i] == DX_KIND_SRV)
        {
            auto* tex = dx_texture_native(slot.texture);
            if (!tex || !tex->handle) continue;
            dx_transition_image(*tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            auto gpu = dx_alloc_srv();
            D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
            srv.Format = dx_srv_format(tex->format);
            srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            if (tex->target == GL_TEXTURE_3D)
            {
                srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                srv.Texture3D.MipLevels = tex->mipLevels;
            }
            else if (tex->target == GL_TEXTURE_2D_ARRAY)
            {
                srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srv.Texture2DArray.MipLevels = tex->mipLevels;
                srv.Texture2DArray.ArraySize = tex->layers;
            }
            else if (tex->target == GL_TEXTURE_1D)
            {
                srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                srv.Texture1D.MipLevels = tex->mipLevels;
            }
            else
            {
                srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srv.Texture2D.MipLevels = tex->mipLevels;
            }
            direct.device->CreateShaderResourceView(tex->handle.Get(), &srv, dx_srv_cpu(gpu));
            if (mod->isCompute) direct.cmd->SetComputeRootDescriptorTable(root, gpu);
            else direct.cmd->SetGraphicsRootDescriptorTable(root, gpu);
        }
        else if (mod->kinds[i] == DX_KIND_UAV)
        {
            auto* tex = dx_texture_native(slot.storage_texture);
            if (!tex || !tex->handle) continue;
            dx_transition_image(*tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            auto gpu = dx_alloc_srv();
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};
            uav.Format = dx_srv_format(tex->format);
            uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uav.Texture2D.MipSlice = slot.storage_texture_bind.base_level;
            direct.device->CreateUnorderedAccessView(tex->handle.Get(), nullptr, &uav, dx_srv_cpu(gpu));
            if (mod->isCompute) direct.cmd->SetComputeRootDescriptorTable(root, gpu);
            else direct.cmd->SetGraphicsRootDescriptorTable(root, gpu);
        }
    }
}

static void dx_bind_api()
{
    rt_unload_library = dx_unload_library;
    rt_create_buffer = dx_create_buffer;
    rt_destroy_buffer = dx_destroy_buffer;
    rt_bind_buffer = dx_bind_buffer;
    rt_map_buffer = dx_map_buffer;
    rt_unmap_buffer = dx_unmap_buffer;
    rt_create_texture = dx_create_texture;
    rt_create_texture_color = dx_create_texture_color;
    rt_create_texture_color_float = dx_create_texture_color_float;
    rt_create_texture_depth = dx_create_texture_depth;
    rt_create_texture_depth_stencil = dx_create_texture_depth_stencil;
    rt_destroy_texture = dx_destroy_texture;
    rt_bind_texture = dx_bind_texture;
    rt_bind_texture_storage = dx_bind_texture_storage;
    rt_create_sampler = dx_create_sampler;
    rt_destroy_sampler = dx_destroy_sampler;
    rt_bind_sampler = dx_bind_sampler;
    rt_create_module_compute = dx_create_module_compute;
    rt_create_module_render = dx_create_module_render;
    rt_create_module_meshlet = dx_create_module_meshlet;
    rt_destroy_module_render = dx_destroy_module_render;
    rt_destroy_module_compute = dx_destroy_module_compute;
    rt_begin_compute = dx_begin_compute;
    rt_end_compute = dx_end_compute;
    rt_dispatch_compute = dx_dispatch_compute;
    rt_begin_render = dx_begin_render;
    rt_end_render = dx_end_render;
    rt_set_viewport = dx_set_viewport;
    rt_set_scissor = dx_set_scissor;
    rt_draw_mesh_task = dx_draw_mesh_task;
    rt_push_constant = dx_push_constant;
    rt_push_const_int = dx_push_const_int;
    rt_push_const_uint = dx_push_const_uint;
    rt_push_const_float = dx_push_const_float;
    rt_push_const_vec2 = dx_push_const_vec2;
    rt_push_const_vec3 = dx_push_const_vec3;
    rt_push_const_vec4 = dx_push_const_vec4;
    rt_push_const_mat3 = dx_push_const_mat3;
    rt_push_const_mat4 = dx_push_const_mat4;
    rt_begin_transfer = dx_begin_transfer;
    rt_end_transfer = dx_end_transfer;
    rt_copy_buffer = dx_copy_buffer;
    rt_copy_buffer_data = dx_copy_buffer_data;
    rt_copy_buffer_texture = dx_copy_buffer_texture;
    rt_copy_texture = dx_copy_texture;
    rt_copy_texture_data = dx_copy_texture_data;
    rt_copy_texture_buffer = dx_copy_texture_buffer;
    rt_create_mesh = dx_create_mesh;
    rt_destroy_mesh = dx_destroy_mesh;
    rt_draw_mesh = dx_draw_mesh;
    rt_draw_mesh_multi = dx_draw_mesh_multi;
    rt_create_meshlet = dx_create_meshlet;
    rt_destroy_meshlet = dx_destroy_meshlet;
    rt_draw_meshlet = dx_draw_meshlet;
    rt_create_mesh_screen = dx_create_mesh_screen;
    rt_draw_screen = dx_draw_screen;
    rt_submit = dx_submit;
}

static void dx_unbind_api()
{
    if (rt_unload_library == dx_unload_library) rt_unload_library = nullptr;
    if (rt_create_buffer == dx_create_buffer) rt_create_buffer = nullptr;
    if (rt_destroy_buffer == dx_destroy_buffer) rt_destroy_buffer = nullptr;
    if (rt_bind_buffer == dx_bind_buffer) rt_bind_buffer = nullptr;
    if (rt_map_buffer == dx_map_buffer) rt_map_buffer = nullptr;
    if (rt_unmap_buffer == dx_unmap_buffer) rt_unmap_buffer = nullptr;
    if (rt_create_texture == dx_create_texture) rt_create_texture = nullptr;
    if (rt_create_texture_color == dx_create_texture_color) rt_create_texture_color = nullptr;
    if (rt_create_texture_color_float == dx_create_texture_color_float) rt_create_texture_color_float = nullptr;
    if (rt_create_texture_depth == dx_create_texture_depth) rt_create_texture_depth = nullptr;
    if (rt_create_texture_depth_stencil == dx_create_texture_depth_stencil) rt_create_texture_depth_stencil = nullptr;
    if (rt_destroy_texture == dx_destroy_texture) rt_destroy_texture = nullptr;
    if (rt_bind_texture == dx_bind_texture) rt_bind_texture = nullptr;
    if (rt_bind_texture_storage == dx_bind_texture_storage) rt_bind_texture_storage = nullptr;
    if (rt_create_sampler == dx_create_sampler) rt_create_sampler = nullptr;
    if (rt_destroy_sampler == dx_destroy_sampler) rt_destroy_sampler = nullptr;
    if (rt_bind_sampler == dx_bind_sampler) rt_bind_sampler = nullptr;
    if (rt_create_module_compute == dx_create_module_compute) rt_create_module_compute = nullptr;
    if (rt_create_module_render == dx_create_module_render) rt_create_module_render = nullptr;
    if (rt_create_module_meshlet == dx_create_module_meshlet) rt_create_module_meshlet = nullptr;
    if (rt_destroy_module_render == dx_destroy_module_render) rt_destroy_module_render = nullptr;
    if (rt_destroy_module_compute == dx_destroy_module_compute) rt_destroy_module_compute = nullptr;
    if (rt_begin_compute == dx_begin_compute) rt_begin_compute = nullptr;
    if (rt_end_compute == dx_end_compute) rt_end_compute = nullptr;
    if (rt_dispatch_compute == dx_dispatch_compute) rt_dispatch_compute = nullptr;
    if (rt_begin_render == dx_begin_render) rt_begin_render = nullptr;
    if (rt_end_render == dx_end_render) rt_end_render = nullptr;
    if (rt_set_viewport == dx_set_viewport) rt_set_viewport = nullptr;
    if (rt_set_scissor == dx_set_scissor) rt_set_scissor = nullptr;
    if (rt_draw_mesh_task == dx_draw_mesh_task) rt_draw_mesh_task = nullptr;
    if (rt_push_constant == dx_push_constant) rt_push_constant = nullptr;
    if (rt_push_const_int == dx_push_const_int) rt_push_const_int = nullptr;
    if (rt_push_const_uint == dx_push_const_uint) rt_push_const_uint = nullptr;
    if (rt_push_const_float == dx_push_const_float) rt_push_const_float = nullptr;
    if (rt_push_const_vec2 == dx_push_const_vec2) rt_push_const_vec2 = nullptr;
    if (rt_push_const_vec3 == dx_push_const_vec3) rt_push_const_vec3 = nullptr;
    if (rt_push_const_vec4 == dx_push_const_vec4) rt_push_const_vec4 = nullptr;
    if (rt_push_const_mat3 == dx_push_const_mat3) rt_push_const_mat3 = nullptr;
    if (rt_push_const_mat4 == dx_push_const_mat4) rt_push_const_mat4 = nullptr;
    if (rt_begin_transfer == dx_begin_transfer) rt_begin_transfer = nullptr;
    if (rt_end_transfer == dx_end_transfer) rt_end_transfer = nullptr;
    if (rt_copy_buffer == dx_copy_buffer) rt_copy_buffer = nullptr;
    if (rt_copy_buffer_data == dx_copy_buffer_data) rt_copy_buffer_data = nullptr;
    if (rt_copy_buffer_texture == dx_copy_buffer_texture) rt_copy_buffer_texture = nullptr;
    if (rt_copy_texture == dx_copy_texture) rt_copy_texture = nullptr;
    if (rt_copy_texture_data == dx_copy_texture_data) rt_copy_texture_data = nullptr;
    if (rt_copy_texture_buffer == dx_copy_texture_buffer) rt_copy_texture_buffer = nullptr;
    if (rt_create_mesh == dx_create_mesh) rt_create_mesh = nullptr;
    if (rt_destroy_mesh == dx_destroy_mesh) rt_destroy_mesh = nullptr;
    if (rt_draw_mesh == dx_draw_mesh) rt_draw_mesh = nullptr;
    if (rt_draw_mesh_multi == dx_draw_mesh_multi) rt_draw_mesh_multi = nullptr;
    if (rt_create_meshlet == dx_create_meshlet) rt_create_meshlet = nullptr;
    if (rt_destroy_meshlet == dx_destroy_meshlet) rt_destroy_meshlet = nullptr;
    if (rt_draw_meshlet == dx_draw_meshlet) rt_draw_meshlet = nullptr;
    if (rt_create_mesh_screen == dx_create_mesh_screen) rt_create_mesh_screen = nullptr;
    if (rt_draw_screen == dx_draw_screen) rt_draw_screen = nullptr;
    if (rt_submit == dx_submit) rt_submit = nullptr;
}

void dx_load_library(ID3D12Device* device, ID3D12CommandQueue* queue)
{
    direct.device = device;
    direct.queue = queue;
    if (!direct.device)
    {
        fprintf(stderr, "DirectX: device is null\n");
        abort();
    }

    if (FAILED(direct.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&direct.allocator))) ||
        FAILED(direct.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, direct.allocator.Get(), nullptr, IID_PPV_ARGS(&direct.cmd))))
    {
        fprintf(stderr, "DirectX: failed to create command list\n");
        abort();
    }
    direct.cmd.As(&direct.cmdMesh);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = 64;
    direct.device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&direct.rtvHeap));
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    direct.device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&direct.dsvHeap));
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 2048;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    direct.device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&direct.srvHeap));
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    heapDesc.NumDescriptors = 256;
    direct.device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&direct.samplerHeap));

    direct.rtvSize = direct.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    direct.dsvSize = direct.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    direct.srvSize = direct.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    direct.samplerSize = direct.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    direct.srvCursor = 1;
    direct.samplerCursor = 1;

    D3D12_SAMPLER_DESC samp = {};
    samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samp.MaxLOD = D3D12_FLOAT32_MAX;
    direct.defaultSamplerCPU = direct.samplerHeap->GetCPUDescriptorHandleForHeapStart();
    direct.device->CreateSampler(&samp, direct.defaultSamplerCPU);

    direct.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&direct.fence));
    direct.fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    dx_bind_api();
}

void dx_unload_library()
{
    if (!direct.device) return;
    if (direct.cmd) direct.cmd->Close();
    dx_wait_gpu();
    dx_flush_staging();
    if (direct.fenceEvent)
    {
        CloseHandle(direct.fenceEvent);
        direct.fenceEvent = nullptr;
    }
    direct.buffers.clear();
    direct.textures.clear();
    direct.samplers.clear();
    direct.modules.clear();
    direct.meshes.clear();
    direct.meshlets.clear();
    direct.computePasses.clear();
    direct.renderPasses.clear();
    direct.transferPasses.clear();
    direct.cmdMesh.Reset();
    direct.cmd.Reset();
    direct.allocator.Reset();
    direct.rtvHeap.Reset();
    direct.dsvHeap.Reset();
    direct.srvHeap.Reset();
    direct.samplerHeap.Reset();
    direct.fence.Reset();
    direct.queue.Reset();
    direct.device.Reset();
    direct.bufferID = direct.textureID = direct.samplerID = direct.moduleID = 0;
    direct.meshID = direct.meshletID = direct.passID = 0;
    direct.currentPassType = GL_NONE;
    direct.currentPipeline = nullptr;
    dx_unbind_api();
}

rt_buffer_t dx_create_buffer(rt_buffer_info_t const& info)
{
    if (info.usage == 0)
    {
        fprintf(stderr, "Buffer usage must not be 0");
        abort();
    }
    rt_buffer_t result = {};
    if (!direct.device || info.size == 0) return result;

    uint32_t handle = direct.bufferID + 1;
    auto& native = direct.buffers[handle];
    bool hostVisible = (info.usage & (GL_BUFFER_USAGE_MAP_READ | GL_BUFFER_USAGE_MAP_WRITE)) || info.data;
    native.heap = hostVisible ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;

    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type = native.heap;
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = (info.size + 255) & ~255ull;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_RESOURCE_STATES init = (native.heap == D3D12_HEAP_TYPE_UPLOAD) ?
        D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;
    if (FAILED(direct.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, init, nullptr, IID_PPV_ARGS(&native.handle))))
    {
        direct.buffers.erase(handle);
        return {};
    }
    native.state = init;

    if (info.data)
    {
        if (native.heap == D3D12_HEAP_TYPE_UPLOAD)
        {
            void* mapped = nullptr;
            D3D12_RANGE range = {0, 0};
            native.handle->Map(0, &range, &mapped);
            std::memcpy(mapped, info.data, info.size);
            native.handle->Unmap(0, nullptr);
        }
        else
        {
            dx_staging_t staging = {};
            void* ptr = nullptr;
            if (dx_create_staging(info.size, staging, &ptr))
            {
                std::memcpy(ptr, info.data, info.size);
                staging.buffer->Unmap(0, nullptr);
                dx_transition_buffer(native, D3D12_RESOURCE_STATE_COPY_DEST);
                direct.cmd->CopyBufferRegion(native.handle.Get(), 0, staging.buffer.Get(), 0, info.size);
                direct.pendingStaging.push_back(std::move(staging));
            }
        }
    }

    direct.bufferID = handle;
    result.handle = handle;
    result.size = info.size;
    result.usage = info.usage;
    result.native = &native;
    return result;
}

void dx_destroy_buffer(rt_buffer_t& buffer)
{
    if (buffer.native)
    {
        auto it = direct.buffers.find(buffer.handle);
        if (it != direct.buffers.end())
        {
            if (it->second.mapped && it->second.handle)
                it->second.handle->Unmap(0, nullptr);
            direct.buffers.erase(it);
        }
    }
    buffer = {};
}

void dx_bind_buffer(rt_buffer_t& buffer, rt_buffer_bind_t bind)
{
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    direct.currentBinding[bind.binding].type = GL_BINDING_BUFFER;
    direct.currentBinding[bind.binding].buffer = buffer;
    direct.currentBinding[bind.binding].buffer_bind = bind;
}

void* dx_map_buffer(rt_buffer_t& buffer, GLenum mode, size_t offset, size_t size)
{
    (void)mode;
    auto* native = dx_buffer_native(buffer);
    if (!native || !native->handle) return nullptr;
    if (offset > buffer.size) return nullptr;
    if (size == 0) size = buffer.size - offset;
    if (size == 0 || offset + size > buffer.size) return nullptr;
    if (native->heap != D3D12_HEAP_TYPE_UPLOAD && native->heap != D3D12_HEAP_TYPE_READBACK)
        return nullptr;
    if (native->mapped)
        native->handle->Unmap(0, nullptr);
    D3D12_RANGE range = {offset, offset + size};
    void* ptr = nullptr;
    if (FAILED(native->handle->Map(0, &range, &ptr)))
        return nullptr;
    native->mapped = (uint8_t*)ptr + offset;
    native->mappedOffset = offset;
    native->mappedSize = size;
    return native->mapped;
}

void dx_unmap_buffer(rt_buffer_t& buffer)
{
    auto* native = dx_buffer_native(buffer);
    if (!native || !native->mapped || !native->handle) return;
    native->handle->Unmap(0, nullptr);
    native->mapped = nullptr;
    native->mappedOffset = 0;
    native->mappedSize = 0;
}

rt_texture_t dx_create_texture(rt_texture_info_t const& info)
{
    rt_texture_t result = {};
    if (!direct.device || info.width == 0) return result;
    if (info.target != GL_TEXTURE_1D && info.height == 0) return result;

    uint32_t handle = direct.textureID + 1;
    auto& native = direct.textures[handle];
    native.format = gl_to_dx_format(info.format, info.type, info.internal_format);
    native.width = info.width;
    native.height = info.target == GL_TEXTURE_1D ? 1 : info.height;
    native.depth = info.depth ? info.depth : 1;
    native.target = info.target;
    native.layers = (info.target == GL_TEXTURE_2D_ARRAY) ? native.depth : 1;
    native.samples = info.samples ? info.samples : 1;
    native.mipLevels = 1;
    if (info.mipmaps == 0 && gl_has_mipmap_filter(info.min_filter))
    {
        uint32_t maxDim = max(native.width, native.height);
        while (maxDim >>= 1) native.mipLevels++;
    }
    else if (info.mipmaps > 1)
        native.mipLevels = info.mipmaps;

    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = (info.target == GL_TEXTURE_3D) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D :
                     (info.target == GL_TEXTURE_1D) ? D3D12_RESOURCE_DIMENSION_TEXTURE1D :
                     D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = native.width;
    desc.Height = native.height;
    desc.DepthOrArraySize = (UINT16)((info.target == GL_TEXTURE_3D) ? native.depth : native.layers);
    desc.MipLevels = (UINT16)native.mipLevels;
    desc.Format = dx_typeless(native.format);
    desc.SampleDesc.Count = native.samples;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    if (dx_is_depth(native.format))
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    else
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = native.format;
    if (dx_is_depth(native.format))
        clear.DepthStencil.Depth = 1.0f;
    D3D12_CLEAR_VALUE* pClear = (desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) ? &clear : nullptr;
    D3D12_RESOURCE_STATES init = D3D12_RESOURCE_STATE_COMMON;
    if (FAILED(direct.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, init, pClear, IID_PPV_ARGS(&native.handle))))
    {
        direct.textures.erase(handle);
        return {};
    }
    native.state = init;

    if (info.data)
    {
        size_t bytes = (size_t)native.width * native.height * ((info.target == GL_TEXTURE_3D) ? native.depth : 1) * dx_format_bytes(native.format);
        dx_staging_t staging = {};
        void* ptr = nullptr;
        if (dx_create_staging(bytes, staging, &ptr))
        {
            std::memcpy(ptr, info.data, bytes);
            staging.buffer->Unmap(0, nullptr);
            dx_transition_image(native, D3D12_RESOURCE_STATE_COPY_DEST);
            D3D12_TEXTURE_COPY_LOCATION dst = {};
            dst.pResource = native.handle.Get();
            dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            D3D12_TEXTURE_COPY_LOCATION src = {};
            src.pResource = staging.buffer.Get();
            src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            src.PlacedFootprint.Footprint.Format = native.format;
            src.PlacedFootprint.Footprint.Width = native.width;
            src.PlacedFootprint.Footprint.Height = native.height;
            src.PlacedFootprint.Footprint.Depth = (info.target == GL_TEXTURE_3D) ? native.depth : 1;
            src.PlacedFootprint.Footprint.RowPitch = native.width * dx_format_bytes(native.format);
            direct.cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
            direct.pendingStaging.push_back(std::move(staging));
        }
        dx_transition_image(native, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    else
    {
        dx_transition_image(native, dx_is_depth(native.format) ? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    direct.textureID = handle;
    result.handle = handle;
    result.width = info.width;
    result.height = native.height;
    result.depth = info.depth;
    result.format = info.format;
    result.internal_format = info.internal_format;
    result.type = info.type;
    result.target = info.target;
    result.mipmaps = native.mipLevels;
    result.samples = native.samples;
    result.native = &native;
    return result;
}

rt_texture_t dx_create_texture_color(uint32_t width, uint32_t height, const void* data)
{
    return dx_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_RGBA, .internal_format = GL_RGBA8, .type = GL_UNSIGNED_BYTE,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

rt_texture_t dx_create_texture_color_float(uint32_t width, uint32_t height, const void* data)
{
    return dx_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_RGBA, .internal_format = GL_RGBA32F, .type = GL_FLOAT,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

rt_texture_t dx_create_texture_depth(uint32_t width, uint32_t height, const void* data)
{
    return dx_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_DEPTH_COMPONENT, .internal_format = GL_DEPTH_COMPONENT32F, .type = GL_FLOAT,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

rt_texture_t dx_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data)
{
    return dx_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_DEPTH_STENCIL, .internal_format = GL_DEPTH24_STENCIL8, .type = GL_UNSIGNED_INT_24_8,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

void dx_destroy_texture(rt_texture_t& texture)
{
    if (texture.native)
        direct.textures.erase(texture.handle);
    texture = {};
}

void dx_bind_texture(rt_texture_t& texture, rt_texture_bind_t bind)
{
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    direct.currentBinding[bind.binding].type = GL_BINDING_TEXTURE;
    direct.currentBinding[bind.binding].texture = texture;
    direct.currentBinding[bind.binding].texture_bind = bind;
}

void dx_bind_texture_storage(rt_texture_t& texture, rt_texture_storage_bind_t bind)
{
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    direct.currentBinding[bind.binding].type = GL_BINDING_STORAGE_TEXTURE;
    direct.currentBinding[bind.binding].storage_texture = texture;
    direct.currentBinding[bind.binding].storage_texture_bind = bind;
}

rt_sampler_t dx_create_sampler(rt_sampler_info_t const& info)
{
    rt_sampler_t result = {};
    uint32_t handle = direct.samplerID + 1;
    auto& native = direct.samplers[handle];
    native.desc.Filter = gl_to_dx_filter(info.min_filter, info.mag_filter);
    native.desc.AddressU = gl_to_dx_address(info.wrap_s);
    native.desc.AddressV = gl_to_dx_address(info.wrap_t);
    native.desc.AddressW = gl_to_dx_address(info.wrap_r);
    native.desc.MaxLOD = gl_has_mipmap_filter(info.min_filter) ? D3D12_FLOAT32_MAX : 0.0f;
    native.desc.MinLOD = 0.0f;
    direct.samplerID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void dx_destroy_sampler(rt_sampler_t& sampler)
{
    if (sampler.native)
        direct.samplers.erase(sampler.handle);
    sampler = {};
}

void dx_bind_sampler(rt_sampler_t& sampler, rt_sampler_bind_t bind)
{
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    direct.currentBinding[bind.binding].type = GL_BINDING_SAMPLER;
    direct.currentBinding[bind.binding].sampler = sampler;
    direct.currentBinding[bind.binding].sampler_bind = bind;
}

rt_module_compute_t dx_create_module_compute(rt_module_compute_info_t const& info)
{
    rt_module_compute_t result = {};
    if (!info.cshader || !info.clength || !direct.device) return result;
    uint32_t handle = direct.moduleID + 1;
    auto& native = direct.modules[handle];
    native.isCompute = true;
    dx_copy_shader(native.cshader, native.ccode, info.cshader, info.clength);
    if (!dx_setup_root(native, nullptr))
    {
        direct.modules.erase(handle);
        return {};
    }
    D3D12_COMPUTE_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = native.rootSignature.Get();
    pso.CS = native.cshader;
    if (FAILED(direct.device->CreateComputePipelineState(&pso, IID_PPV_ARGS(&native.pipeline))))
    {
        result.native = &native;
        dx_destroy_module_native(handle, result.native);
        return {};
    }
    direct.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

rt_module_render_t dx_create_module_render(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};
    if (!direct.device) return result;
    uint32_t handle = direct.moduleID + 1;
    auto& native = direct.modules[handle];
    if (info.vshader) dx_copy_shader(native.vshader, native.vcode, info.vshader, info.vlength);
    if (info.fshader) dx_copy_shader(native.fshader, native.fcode, info.fshader, info.flength);
    if (!dx_setup_root(native, info.binding) || !dx_create_graphics_pipeline(native, info, false))
    {
        result.native = &native;
        dx_destroy_module_native(handle, result.native);
        return {};
    }
    direct.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    dx_fill_render_state(result, info);
    return result;
}

rt_module_render_t dx_create_module_meshlet(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};
    if (!info.mshader || !info.mlength || !direct.device) return result;
    uint32_t handle = direct.moduleID + 1;
    auto& native = direct.modules[handle];
    native.isMeshlet = true;
    if (info.tshader) dx_copy_shader(native.tshader, native.tcode, info.tshader, info.tlength);
    dx_copy_shader(native.mshader, native.mcode, info.mshader, info.mlength);
    if (info.fshader) dx_copy_shader(native.fshader, native.fcode, info.fshader, info.flength);
    if (!dx_setup_root(native, info.binding) || !dx_create_graphics_pipeline(native, info, true))
    {
        result.native = &native;
        dx_destroy_module_native(handle, result.native);
        return {};
    }
    direct.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    dx_fill_render_state(result, info);
    return result;
}

void dx_destroy_module_render(rt_module_render_t& module)
{
    dx_destroy_module_native(module.handle, module.native);
    module.handle = 0;
    module.vertex_vao = 0;
}

void dx_destroy_module_compute(rt_module_compute_t& module)
{
    dx_destroy_module_native(module.handle, module.native);
    module.handle = 0;
}

void dx_push_constant(uint8_t const* buffer, size_t length)
{
    dx_require_pass(direct.currentPassType);
    auto* mod = dx_current_module_native();
    if (!buffer || length == 0 || !mod) return;
    uint32_t count = (uint32_t)((length + 3) / 4);
    if (mod->isCompute)
        direct.cmd->SetComputeRoot32BitConstants(0, count, buffer, 0);
    else
        direct.cmd->SetGraphicsRoot32BitConstants(0, count, buffer, 0);
}

void dx_push_const_int(const char*, int32_t) {}
void dx_push_const_uint(const char*, uint32_t) {}
void dx_push_const_float(const char*, float) {}
void dx_push_const_vec2(const char*, const float*) {}
void dx_push_const_vec3(const char*, const float*) {}
void dx_push_const_vec4(const char*, const float*) {}
void dx_push_const_mat3(const char*, const float*) {}
void dx_push_const_mat4(const char*, const float*) {}

void dx_begin_compute(rt_pass_compute_t& pass)
{
    if (pass.module.handle == 0 || !pass.module.native)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (direct.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    dx_clear_bindings();
    pass.handle = ++direct.passID;
    auto& native = direct.computePasses[pass.handle];
    pass.native = &native;
    direct.currentPassType = GL_MODULE_COMPUTE;
    direct.currentComputePass = &pass;
    auto* mod = (rt_module_native_t*)pass.module.native;
    if (mod && mod->pipeline)
        direct.cmd->SetPipelineState(mod->pipeline.Get());
    if (mod && mod->rootSignature)
        direct.cmd->SetComputeRootSignature(mod->rootSignature.Get());
}

void dx_end_compute(rt_pass_compute_t& pass)
{
    if (direct.currentComputePass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    direct.computePasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    direct.currentPassType = GL_NONE;
    direct.currentPipeline = nullptr;
    dx_clear_bindings();
}

void dx_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    dx_require_pass(GL_MODULE_COMPUTE);
    dx_flush_descriptors();
    direct.cmd->Dispatch(max(1u, groupX), max(1u, groupY), max(1u, groupZ));
}

void dx_begin_render(rt_pass_render_t& pass)
{
    if (pass.module.handle == 0 || !pass.module.native)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (direct.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    dx_clear_bindings();
    pass.handle = ++direct.passID;
    auto& native = direct.renderPasses[pass.handle];
    pass.native = &native;
    direct.currentPassType = GL_MODULE_RENDER;
    direct.currentRenderPass = &pass;

    auto* mod = (rt_module_native_t*)pass.module.native;
    if (mod && mod->pipeline)
        direct.cmd->SetPipelineState(mod->pipeline.Get());
    if (mod && mod->rootSignature)
        direct.cmd->SetGraphicsRootSignature(mod->rootSignature.Get());
    if (mod)
        direct.cmd->IASetPrimitiveTopology(mod->topology);

    native.offscreen = pass.depth.texture.handle != 0;
    for (auto& color : pass.colors)
        if (color.texture.handle) native.offscreen = true;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[GL_MAX_COLOR_TEXTURE_NUM] = {};
    uint32_t colorCount = 0;
    uint32_t width = 0, height = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvStart = direct.rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        if (auto* tex = dx_texture_native(pass.colors[i].texture))
        {
            dx_transition_image(*tex, D3D12_RESOURCE_STATE_RENDER_TARGET);
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvStart;
            rtv.ptr += (SIZE_T)i * direct.rtvSize;
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = tex->format;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            direct.device->CreateRenderTargetView(tex->handle.Get(), &rtvDesc, rtv);
            rtvs[i] = rtv;
            if (pass.colors[i].clear)
            {
                float clear[4] = {pass.colors[i].value.r, pass.colors[i].value.g, pass.colors[i].value.b, pass.colors[i].value.a};
                direct.cmd->ClearRenderTargetView(rtv, clear, 0, nullptr);
            }
            width = max(width, pass.colors[i].texture.width);
            height = max(height, pass.colors[i].texture.height);
            colorCount = i + 1;
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = {};
    bool hasDepth = false;
    if (auto* tex = dx_texture_native(pass.depth.texture))
    {
        dx_transition_image(*tex, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        dsv = direct.dsvHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = tex->format;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        direct.device->CreateDepthStencilView(tex->handle.Get(), &dsvDesc, dsv);
        if (pass.depth.clear || pass.stencil.clear)
        {
            D3D12_CLEAR_FLAGS flags = {};
            if (pass.depth.clear) flags |= D3D12_CLEAR_FLAG_DEPTH;
            if (pass.stencil.clear) flags |= D3D12_CLEAR_FLAG_STENCIL;
            direct.cmd->ClearDepthStencilView(dsv, flags, pass.depth.value, (UINT8)pass.stencil.value, 0, nullptr);
        }
        width = max(width, pass.depth.texture.width);
        height = max(height, pass.depth.texture.height);
        hasDepth = true;
    }

    native.width = width;
    native.height = height;
    direct.cmd->OMSetRenderTargets(max(colorCount, (uint32_t)GL_MAX_COLOR_TEXTURE_NUM), rtvs, FALSE, hasDepth ? &dsv : nullptr);
    dx_set_viewport(0, 0, (int32_t)width, (int32_t)height);
    dx_set_scissor(0, 0, (int32_t)width, (int32_t)height);
}

void dx_end_render(rt_pass_render_t& pass)
{
    if (direct.currentRenderPass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    for (auto& color : pass.colors)
        if (auto* tex = dx_texture_native(color.texture))
            dx_transition_image(*tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    if (auto* tex = dx_texture_native(pass.depth.texture))
        dx_transition_image(*tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    direct.renderPasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    direct.currentPassType = GL_NONE;
    direct.currentPipeline = nullptr;
    dx_clear_bindings();
}

void dx_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    D3D12_VIEWPORT viewport = {(float)x, (float)y, (float)width, (float)height, 0.0f, 1.0f};
    direct.cmd->RSSetViewports(1, &viewport);
}

void dx_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    D3D12_RECT scissor = {x, y, x + max(0, width), y + max(0, height)};
    direct.cmd->RSSetScissorRects(1, &scissor);
}

void dx_draw_mesh_task(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    dx_require_pass(GL_MODULE_RENDER);
    dx_flush_descriptors();
    if (direct.cmdMesh)
        direct.cmdMesh->DispatchMesh(max(1u, groupX), max(1u, groupY), max(1u, groupZ));
}

void dx_begin_transfer(rt_pass_transfer_t& pass)
{
    if (direct.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    pass.handle = ++direct.passID;
    auto& native = direct.transferPasses[pass.handle];
    pass.native = &native;
    direct.currentPassType = GL_MODULE_TRANSFER;
    direct.currentTransferPass = &pass;
}

void dx_end_transfer(rt_pass_transfer_t& pass)
{
    if (direct.currentTransferPass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    direct.transferPasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    direct.currentPassType = GL_NONE;
    direct.currentPipeline = nullptr;
}

void dx_copy_buffer(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize)
{
    dx_require_pass(GL_MODULE_TRANSFER);
    auto* src = dx_buffer_native(source.buffer);
    auto* dst = dx_buffer_native(destination.buffer);
    if (!src || !dst || copySize == 0) return;
    if (source.offset + copySize > source.buffer.size || destination.offset + copySize > destination.buffer.size)
        return;
    dx_transition_buffer(*src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    dx_transition_buffer(*dst, D3D12_RESOURCE_STATE_COPY_DEST);
    direct.cmd->CopyBufferRegion(dst->handle.Get(), destination.offset, src->handle.Get(), source.offset, copySize);
}

void dx_copy_buffer_data(rt_buffer_data_t source, rt_buffer_copy_t destination, size_t copySize)
{
    dx_require_pass(GL_MODULE_TRANSFER);
    auto* dst = dx_buffer_native(destination.buffer);
    if (!source.data || !dst || copySize == 0) return;
    if (source.offset + copySize > source.size || destination.offset + copySize > destination.buffer.size)
        return;
    if (dst->heap == D3D12_HEAP_TYPE_UPLOAD)
    {
        void* mapped = nullptr;
        D3D12_RANGE range = {destination.offset, destination.offset + copySize};
        if (SUCCEEDED(dst->handle->Map(0, &range, &mapped)))
        {
            std::memcpy((uint8_t*)mapped + destination.offset, source.data + source.offset, copySize);
            dst->handle->Unmap(0, nullptr);
            return;
        }
    }
    dx_staging_t staging = {};
    void* ptr = nullptr;
    if (!dx_create_staging(copySize, staging, &ptr)) return;
    std::memcpy(ptr, source.data + source.offset, copySize);
    staging.buffer->Unmap(0, nullptr);
    dx_transition_buffer(*dst, D3D12_RESOURCE_STATE_COPY_DEST);
    direct.cmd->CopyBufferRegion(dst->handle.Get(), destination.offset, staging.buffer.Get(), 0, copySize);
    direct.pendingStaging.push_back(std::move(staging));
}

static void dx_fill_placed(D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint, rt_texture_native_t& tex, uint32_t bytesPerRow, rt_size_t copySize)
{
    footprint.Footprint.Format = tex.format;
    footprint.Footprint.Width = copySize.x;
    footprint.Footprint.Height = copySize.y;
    footprint.Footprint.Depth = copySize.z ? copySize.z : 1;
    footprint.Footprint.RowPitch = bytesPerRow ? bytesPerRow : (copySize.x * dx_format_bytes(tex.format));
}

void dx_copy_buffer_texture(rt_texture_copy_t source, rt_buffer_texel_t destination, rt_size_t copySize)
{
    dx_require_pass(GL_MODULE_TRANSFER);
    auto* src = dx_texture_native(source.texture);
    auto* dst = dx_buffer_native(destination.buffer);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    dx_transition_image(*src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    dx_transition_buffer(*dst, D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = src->handle.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLoc.SubresourceIndex = source.mipLevel;
    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = dst->handle.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLoc.PlacedFootprint.Offset = destination.offset;
    dx_fill_placed(dstLoc.PlacedFootprint, *src, destination.bytesPerRow, copySize);
    D3D12_BOX box = {source.origin.x, source.origin.y, source.origin.z,
                     (source.origin.x + copySize.x), (source.origin.y + copySize.y),
                     (source.origin.z + (copySize.z ? copySize.z : 1))};
    direct.cmd->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, &box);
}

void dx_copy_texture(rt_texture_copy_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    dx_require_pass(GL_MODULE_TRANSFER);
    auto* src = dx_texture_native(source.texture);
    auto* dst = dx_texture_native(destination.texture);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    dx_transition_image(*src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    dx_transition_image(*dst, D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = src->handle.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLoc.SubresourceIndex = source.mipLevel;
    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = dst->handle.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = destination.mipLevel;
    D3D12_BOX box = {source.origin.x, source.origin.y, source.origin.z,
                     (source.origin.x + copySize.x), (source.origin.y + copySize.y),
                     (source.origin.z + (copySize.z ? copySize.z : 1))};
    direct.cmd->CopyTextureRegion(&dstLoc, destination.origin.x, destination.origin.y, destination.origin.z, &srcLoc, &box);
}

void dx_copy_texture_data(rt_texture_data_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    dx_require_pass(GL_MODULE_TRANSFER);
    auto* dst = dx_texture_native(destination.texture);
    if (!source.data || !dst || copySize.x == 0 || copySize.y == 0) return;
    uint32_t bpp = dx_format_bytes(dst->format);
    size_t bytes = source.size ? source.size : (size_t)max(copySize.x * bpp, 1u) * copySize.y * max(1u, copySize.z);
    dx_staging_t staging = {};
    void* ptr = nullptr;
    if (!dx_create_staging(bytes, staging, &ptr)) return;
    std::memcpy(ptr, source.data + source.offset, min(bytes, source.size ? source.size - source.offset : bytes));
    staging.buffer->Unmap(0, nullptr);
    dx_transition_image(*dst, D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = dst->handle.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = destination.mipLevel;
    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = staging.buffer.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dx_fill_placed(srcLoc.PlacedFootprint, *dst, source.bytesPerRow, copySize);
    direct.cmd->CopyTextureRegion(&dstLoc, destination.origin.x, destination.origin.y, destination.origin.z, &srcLoc, nullptr);
    direct.pendingStaging.push_back(std::move(staging));
}

void dx_copy_texture_buffer(rt_buffer_texel_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    dx_require_pass(GL_MODULE_TRANSFER);
    auto* src = dx_buffer_native(source.buffer);
    auto* dst = dx_texture_native(destination.texture);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    dx_transition_image(*dst, D3D12_RESOURCE_STATE_COPY_DEST);
    dx_transition_buffer(*src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = dst->handle.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = destination.mipLevel;
    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = src->handle.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.PlacedFootprint.Offset = source.offset;
    dx_fill_placed(srcLoc.PlacedFootprint, *dst, source.bytesPerRow, copySize);
    direct.cmd->CopyTextureRegion(&dstLoc, destination.origin.x, destination.origin.y, destination.origin.z, &srcLoc, nullptr);
}

rt_mesh_t dx_create_mesh(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    rt_mesh_t result = {};
    if (!vertices || vertex_count == 0) return result;
    uint32_t handle = direct.meshID + 1;
    auto& native = direct.meshes[handle];
    native.vertexCount = (uint32_t)vertex_count;
    native.indexCount = (uint32_t)index_count;
    if (vertices)
        result.vertex[0] = dx_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = dx_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = dx_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = dx_create_buffer({.size = index_count * sizeof(uint32_t), .usage = GL_BUFFER_USAGE_INDEX | GL_BUFFER_USAGE_COPY_DST, .data = indices});
    std::iota(result.location, result.location + std::size(result.location), 0);
    direct.meshID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void dx_destroy_mesh(rt_mesh_t& mesh)
{
    for (auto& vertex : mesh.vertex)
        dx_destroy_buffer(vertex);
    dx_destroy_buffer(mesh.index);
    direct.meshes.erase(mesh.handle);
    mesh.handle = 0;
    mesh.native = nullptr;
}

static void dx_draw_mesh_impl(rt_mesh_t& mesh, uint32_t instanceCount)
{
    dx_require_pass(GL_MODULE_RENDER);
    dx_flush_descriptors();
    rt_module_render_t const& module = direct.currentRenderPass->module;
    uint32_t vertex_count = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.type == GL_NONE || layout.count == 0) continue;
        for (uint32_t k = 0; k < std::size(mesh.vertex); ++k)
        {
            if (mesh.vertex[k].handle == 0 || mesh.location[k] != layout.location) continue;
            auto* native = dx_buffer_native(mesh.vertex[k]);
            if (!native) break;
            dx_transition_buffer(*native, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            uint32_t stride = gl_vertex_size(layout.type, layout.count);
            D3D12_VERTEX_BUFFER_VIEW view = {};
            view.BufferLocation = native->handle->GetGPUVirtualAddress();
            view.SizeInBytes = (UINT)mesh.vertex[k].size;
            view.StrideInBytes = stride;
            direct.cmd->IASetVertexBuffers(layout.location, 1, &view);
            if (vertex_count == 0 && stride)
                vertex_count = (uint32_t)(mesh.vertex[k].size / stride);
            break;
        }
    }
    if (mesh.index.handle)
    {
        auto* native = dx_buffer_native(mesh.index);
        if (!native) return;
        uint32_t indexStride = gl_index_type_size(module.index_type);
        dx_transition_buffer(*native, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        D3D12_INDEX_BUFFER_VIEW view = {};
        view.BufferLocation = native->handle->GetGPUVirtualAddress();
        view.SizeInBytes = (UINT)mesh.index.size;
        view.Format = gl_to_dx_index_type(module.index_type);
        direct.cmd->IASetIndexBuffer(&view);
        direct.cmd->DrawIndexedInstanced((UINT)(mesh.index.size / indexStride), instanceCount, 0, 0, 0);
    }
    else
        direct.cmd->DrawInstanced(vertex_count, instanceCount, 0, 0);
}

void dx_draw_mesh(rt_mesh_t& mesh)
{
    dx_draw_mesh_impl(mesh, 1);
}

void dx_draw_mesh_multi(rt_mesh_t& mesh, uint32_t count)
{
    dx_draw_mesh_impl(mesh, max(1u, count));
}

rt_meshlet_t dx_create_meshlet(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    rt_meshlet_t result = {};
    if (!vertices || vertex_count == 0) return result;
    uint32_t handle = direct.meshletID + 1;
    auto& native = direct.meshlets[handle];
    native.vertexCount = (uint32_t)vertex_count;
    native.indexCount = (uint32_t)index_count;
    if (vertices)
        result.vertex[0] = dx_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = dx_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = dx_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = dx_create_buffer({.size = index_count * sizeof(uint32_t), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = indices});
    std::iota(result.location, result.location + std::size(result.location), 0);
    direct.meshletID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void dx_destroy_meshlet(rt_meshlet_t& meshlet)
{
    for (auto& vertex : meshlet.vertex)
        dx_destroy_buffer(vertex);
    dx_destroy_buffer(meshlet.index);
    direct.meshlets.erase(meshlet.handle);
    meshlet.handle = 0;
    meshlet.native = nullptr;
}

void dx_draw_meshlet(rt_meshlet_t& meshlet)
{
    dx_require_pass(GL_MODULE_RENDER);
    rt_module_render_t const& module = direct.currentRenderPass->module;
    uint32_t index_binding = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.type == GL_NONE || layout.count == 0) continue;
        for (uint32_t k = 0; k < std::size(meshlet.vertex); ++k)
        {
            if (meshlet.vertex[k].handle == 0 || meshlet.location[k] != layout.location) continue;
            dx_bind_buffer(meshlet.vertex[k], {.binding = layout.location, .target = GL_SHADER_STORAGE_BUFFER});
            break;
        }
        if (layout.location + 1 > index_binding)
            index_binding = layout.location + 1;
    }
    if (meshlet.index.handle)
        dx_bind_buffer(meshlet.index, {.binding = index_binding, .target = GL_SHADER_STORAGE_BUFFER});
    dx_flush_descriptors();
    auto* native = (rt_meshlet_native_t*)meshlet.native;
    uint32_t tasks = native && native->indexCount ? native->indexCount / 3 : 1;
    if (direct.cmdMesh)
        direct.cmdMesh->DispatchMesh(max(1u, tasks), 1, 1);
}

rt_mesh_t dx_create_mesh_screen()
{
    const float points[] = {-1.0f, -1.0f, 0.0f, +3.0f, -1.0f, 0.0f, -1.0f, +3.0f, 0.0f};
    const float uvs[] = {0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 2.0f};
    return dx_create_mesh(points, nullptr, uvs, 3, nullptr, 0);
}

void dx_draw_screen(int width, int height, rt_color_t clear, rt_texture_t& texture)
{
    static auto module = dx_create_module_render({
        .vertex = {rt_vertex_vertex, {}, rt_vertex_uv},
        .binding = {{.binding = 0, .type = GL_BINDING_TEXTURE}},
    });
    if (!module.handle) return;
    rt_pass_render_t pass = {.module = module, .screen = {.color = {.clear = true, .value = clear}}};
    dx_begin_render(pass);
    dx_set_viewport(0, 0, width, height);
    dx_bind_texture(texture, {.binding = 0});
    static auto mesh = dx_create_mesh_screen();
    dx_draw_mesh(mesh);
    dx_end_render(pass);
}

void dx_submit()
{
    if (!direct.cmd) return;
    if (FAILED(direct.cmd->Close()))
    {
        fprintf(stderr, "DirectX: failed to close command list\n");
        abort();
    }
    ID3D12CommandList* lists[] = {direct.cmd.Get()};
    if (direct.queue)
        direct.queue->ExecuteCommandLists(1, lists);
    dx_wait_gpu();
    dx_flush_staging();
    direct.allocator->Reset();
    if (FAILED(direct.cmd->Reset(direct.allocator.Get(), nullptr)))
    {
        fprintf(stderr, "DirectX: failed to reset command list\n");
        abort();
    }
}

#endif
#endif
