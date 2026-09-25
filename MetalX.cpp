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
#ifdef METAL_IMPLEMENTATION
#ifdef __APPLE__
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include "MetalX.h"
#include <dispatch/dispatch.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

enum rt_module_type_t : uint32_t
{
    GL_MODULE_RENDER = 1,
    GL_MODULE_COMPUTE = 2,
    GL_MODULE_MESHLET = 3,
    GL_MODULE_TRANSFER = 4,
};

enum mt_res_state_t : uint32_t
{
    MTL_STATE_UNKNOWN = 0,
    MTL_STATE_HOST,
    MTL_STATE_COPY_SRC,
    MTL_STATE_COPY_DST,
    MTL_STATE_SHADER_READ,
    MTL_STATE_SHADER_WRITE,
    MTL_STATE_COLOR,
    MTL_STATE_DEPTH,
    MTL_STATE_VERTEX,
    MTL_STATE_INDEX,
};

template<typename T>
static void mt_release(T*& object)
{
    if (object)
    {
        object->release();
        object = nullptr;
    }
}

static MTL::SamplerMinMagFilter gl_to_mt_filter(GLenum filter)
{
    return (filter == GL_NEAREST || filter == GL_NEAREST_MIPMAP_NEAREST || filter == GL_NEAREST_MIPMAP_LINEAR) ?
           MTL::SamplerMinMagFilterNearest : MTL::SamplerMinMagFilterLinear;
}

static MTL::SamplerMipFilter gl_to_mt_mip(GLenum minFilter)
{
    if (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST)
        return MTL::SamplerMipFilterNearest;
    if (minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR)
        return MTL::SamplerMipFilterLinear;
    return MTL::SamplerMipFilterNotMipmapped;
}

static MTL::SamplerAddressMode gl_to_mt_address(GLenum wrap)
{
    switch (wrap)
    {
        case GL_REPEAT: return MTL::SamplerAddressModeRepeat;
        case GL_MIRRORED_REPEAT: return MTL::SamplerAddressModeMirrorRepeat;
        case GL_CLAMP_TO_BORDER: return MTL::SamplerAddressModeClampToBorderColor;
        case GL_MIRROR_CLAMP_TO_EDGE: return MTL::SamplerAddressModeMirrorClampToEdge;
        case GL_CLAMP_TO_EDGE:
        default: return MTL::SamplerAddressModeClampToEdge;
    }
}

static bool gl_has_mipmap_filter(GLenum minFilter)
{
    return minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST ||
           minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR;
}

static MTL::PixelFormat gl_to_mt_format(GLenum glFormat, GLenum glType, GLenum internalFormat = 0)
{
    switch (internalFormat)
    {
        case GL_R8: return MTL::PixelFormatR8Unorm;
        case GL_RG8: return MTL::PixelFormatRG8Unorm;
        case GL_RGB8:
        case GL_RGBA8: return MTL::PixelFormatRGBA8Unorm;
        case GL_SRGB8_ALPHA8: return MTL::PixelFormatRGBA8Unorm_sRGB;
        case GL_R16F: return MTL::PixelFormatR16Float;
        case GL_RG16F: return MTL::PixelFormatRG16Float;
        case GL_RGBA16F: return MTL::PixelFormatRGBA16Float;
        case GL_R32F: return MTL::PixelFormatR32Float;
        case GL_RG32F: return MTL::PixelFormatRG32Float;
        case GL_RGBA32F: return MTL::PixelFormatRGBA32Float;
        case GL_DEPTH_COMPONENT16: return MTL::PixelFormatDepth16Unorm;
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32F: return MTL::PixelFormatDepth32Float;
        case GL_DEPTH24_STENCIL8: return MTL::PixelFormatDepth24Unorm_Stencil8;
        case GL_DEPTH32F_STENCIL8: return MTL::PixelFormatDepth32Float_Stencil8;
        default: break;
    }
    switch (glFormat)
    {
        case GL_RED: return (glType == GL_FLOAT) ? MTL::PixelFormatR32Float : MTL::PixelFormatR8Unorm;
        case GL_RG: return (glType == GL_FLOAT) ? MTL::PixelFormatRG32Float : MTL::PixelFormatRG8Unorm;
        case GL_RGB:
        case GL_RGBA: return (glType == GL_FLOAT) ? MTL::PixelFormatRGBA32Float : MTL::PixelFormatRGBA8Unorm;
        case GL_DEPTH_COMPONENT: return MTL::PixelFormatDepth32Float;
        case GL_DEPTH_STENCIL: return MTL::PixelFormatDepth32Float_Stencil8;
        default: return MTL::PixelFormatRGBA8Unorm;
    }
}

static MTL::VertexFormat gl_to_mt_vertex_format(GLenum type, GLenum count)
{
    if (type == GL_FLOAT)
    {
        switch (count)
        {
            case 1: return MTL::VertexFormatFloat;
            case 2: return MTL::VertexFormatFloat2;
            case 3: return MTL::VertexFormatFloat3;
            case 4: return MTL::VertexFormatFloat4;
            default: return MTL::VertexFormatFloat;
        }
    }
    if (type == GL_INT)
    {
        switch (count)
        {
            case 1: return MTL::VertexFormatInt;
            case 2: return MTL::VertexFormatInt2;
            case 3: return MTL::VertexFormatInt3;
            case 4: return MTL::VertexFormatInt4;
            default: return MTL::VertexFormatInt;
        }
    }
    if (type == GL_UNSIGNED_INT)
    {
        switch (count)
        {
            case 1: return MTL::VertexFormatUInt;
            case 2: return MTL::VertexFormatUInt2;
            case 3: return MTL::VertexFormatUInt3;
            case 4: return MTL::VertexFormatUInt4;
            default: return MTL::VertexFormatUInt;
        }
    }
    return MTL::VertexFormatFloat;
}

static MTL::CompareFunction gl_to_mt_compare(GLenum func)
{
    switch (func)
    {
        case GL_NEVER: return MTL::CompareFunctionNever;
        case GL_LESS: return MTL::CompareFunctionLess;
        case GL_EQUAL: return MTL::CompareFunctionEqual;
        case GL_LEQUAL: return MTL::CompareFunctionLessEqual;
        case GL_GREATER: return MTL::CompareFunctionGreater;
        case GL_NOTEQUAL: return MTL::CompareFunctionNotEqual;
        case GL_GEQUAL: return MTL::CompareFunctionGreaterEqual;
        case GL_ALWAYS:
        default: return MTL::CompareFunctionAlways;
    }
}

static MTL::BlendFactor gl_to_mt_blend(GLenum factor)
{
    switch (factor)
    {
        case GL_ZERO: return MTL::BlendFactorZero;
        case GL_ONE: return MTL::BlendFactorOne;
        case GL_SRC_COLOR: return MTL::BlendFactorSourceColor;
        case GL_ONE_MINUS_SRC_COLOR: return MTL::BlendFactorOneMinusSourceColor;
        case GL_DST_COLOR: return MTL::BlendFactorDestinationColor;
        case GL_ONE_MINUS_DST_COLOR: return MTL::BlendFactorOneMinusDestinationColor;
        case GL_SRC_ALPHA: return MTL::BlendFactorSourceAlpha;
        case GL_ONE_MINUS_SRC_ALPHA: return MTL::BlendFactorOneMinusSourceAlpha;
        case GL_DST_ALPHA: return MTL::BlendFactorDestinationAlpha;
        case GL_ONE_MINUS_DST_ALPHA: return MTL::BlendFactorOneMinusDestinationAlpha;
        case GL_CONSTANT_COLOR: return MTL::BlendFactorBlendColor;
        case GL_ONE_MINUS_CONSTANT_COLOR: return MTL::BlendFactorOneMinusBlendColor;
        case GL_SRC_ALPHA_SATURATE: return MTL::BlendFactorSourceAlphaSaturated;
        default: return MTL::BlendFactorOne;
    }
}

static MTL::BlendOperation gl_to_mt_blend_op(GLenum func)
{
    switch (func)
    {
        case GL_FUNC_SUBTRACT: return MTL::BlendOperationSubtract;
        case GL_FUNC_REVERSE_SUBTRACT: return MTL::BlendOperationReverseSubtract;
        case GL_MIN: return MTL::BlendOperationMin;
        case GL_MAX: return MTL::BlendOperationMax;
        case GL_FUNC_ADD:
        default: return MTL::BlendOperationAdd;
    }
}

static MTL::CullMode gl_to_mt_cull(GLenum mode)
{
    switch (mode)
    {
        case GL_FRONT: return MTL::CullModeFront;
        case GL_BACK: return MTL::CullModeBack;
        default: return MTL::CullModeNone;
    }
}

static MTL::PrimitiveType gl_to_mt_primitive(GLenum primitive)
{
    switch (primitive)
    {
        case GL_POINTS: return MTL::PrimitiveTypePoint;
        case GL_LINES: return MTL::PrimitiveTypeLine;
        case GL_LINE_STRIP:
        case GL_LINE_LOOP: return MTL::PrimitiveTypeLineStrip;
        case GL_TRIANGLE_STRIP: return MTL::PrimitiveTypeTriangleStrip;
        case GL_TRIANGLES:
        default: return MTL::PrimitiveTypeTriangle;
    }
}

static MTL::StencilOperation gl_to_mt_stencil_op(GLenum op)
{
    switch (op)
    {
        case GL_ZERO: return MTL::StencilOperationZero;
        case GL_REPLACE: return MTL::StencilOperationReplace;
        case GL_INCR: return MTL::StencilOperationIncrementClamp;
        case GL_INCR_WRAP: return MTL::StencilOperationIncrementWrap;
        case GL_DECR: return MTL::StencilOperationDecrementClamp;
        case GL_DECR_WRAP: return MTL::StencilOperationDecrementWrap;
        case GL_INVERT: return MTL::StencilOperationInvert;
        case GL_KEEP:
        default: return MTL::StencilOperationKeep;
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

static MTL::IndexType gl_to_mt_index_type(GLenum type)
{
    return (type == GL_UNSIGNED_SHORT) ? MTL::IndexTypeUInt16 : MTL::IndexTypeUInt32;
}

static uint32_t mt_format_bytes(MTL::PixelFormat format)
{
    switch (format)
    {
        case MTL::PixelFormatR8Unorm: return 1;
        case MTL::PixelFormatRG8Unorm:
        case MTL::PixelFormatR16Float:
        case MTL::PixelFormatDepth16Unorm: return 2;
        case MTL::PixelFormatRGBA8Unorm:
        case MTL::PixelFormatRGBA8Unorm_sRGB:
        case MTL::PixelFormatR32Float:
        case MTL::PixelFormatRG16Float:
        case MTL::PixelFormatDepth32Float:
        case MTL::PixelFormatDepth24Unorm_Stencil8: return 4;
        case MTL::PixelFormatRGBA16Float:
        case MTL::PixelFormatRG32Float:
        case MTL::PixelFormatDepth32Float_Stencil8: return 8;
        case MTL::PixelFormatRGBA32Float: return 16;
        default: return 4;
    }
}

static bool mt_is_depth(MTL::PixelFormat format)
{
    return format == MTL::PixelFormatDepth16Unorm || format == MTL::PixelFormatDepth32Float ||
           format == MTL::PixelFormatDepth24Unorm_Stencil8 || format == MTL::PixelFormatDepth32Float_Stencil8;
}

struct rt_buffer_native_t
{
    MTL::Buffer* handle = nullptr;
    mt_res_state_t state = MTL_STATE_UNKNOWN;
    void* mapped = nullptr;
    size_t mappedOffset = 0;
    size_t mappedSize = 0;

    rt_buffer_native_t() = default;
    rt_buffer_native_t(const rt_buffer_native_t&) = delete;
    rt_buffer_native_t& operator=(const rt_buffer_native_t&) = delete;
    rt_buffer_native_t(rt_buffer_native_t&& other) noexcept
        : handle(other.handle), state(other.state), mapped(other.mapped),
          mappedOffset(other.mappedOffset), mappedSize(other.mappedSize)
    {
        other.handle = nullptr;
        other.mapped = nullptr;
    }
    rt_buffer_native_t& operator=(rt_buffer_native_t&& other) noexcept
    {
        if (this != &other)
        {
            mt_release(handle);
            handle = other.handle;
            state = other.state;
            mapped = other.mapped;
            mappedOffset = other.mappedOffset;
            mappedSize = other.mappedSize;
            other.handle = nullptr;
            other.mapped = nullptr;
        }
        return *this;
    }
    ~rt_buffer_native_t() { mt_release(handle); }
};

struct rt_texture_native_t
{
    MTL::Texture* handle = nullptr;
    MTL::PixelFormat format = MTL::PixelFormatInvalid;
    mt_res_state_t state = MTL_STATE_UNKNOWN;
    uint32_t mipLevels = 1;
    uint32_t layers = 1;
    uint32_t width = 1, height = 1, depth = 1;
    GLenum target = GL_TEXTURE_2D;

    rt_texture_native_t() = default;
    rt_texture_native_t(const rt_texture_native_t&) = delete;
    rt_texture_native_t& operator=(const rt_texture_native_t&) = delete;
    rt_texture_native_t(rt_texture_native_t&& other) noexcept
        : handle(other.handle), format(other.format), state(other.state),
          mipLevels(other.mipLevels), layers(other.layers),
          width(other.width), height(other.height), depth(other.depth), target(other.target)
    {
        other.handle = nullptr;
    }
    rt_texture_native_t& operator=(rt_texture_native_t&& other) noexcept
    {
        if (this != &other)
        {
            mt_release(handle);
            handle = other.handle;
            format = other.format;
            state = other.state;
            mipLevels = other.mipLevels;
            layers = other.layers;
            width = other.width;
            height = other.height;
            depth = other.depth;
            target = other.target;
            other.handle = nullptr;
        }
        return *this;
    }
    ~rt_texture_native_t() { mt_release(handle); }
};

struct rt_sampler_native_t
{
    MTL::SamplerState* handle = nullptr;

    rt_sampler_native_t() = default;
    rt_sampler_native_t(const rt_sampler_native_t&) = delete;
    rt_sampler_native_t& operator=(const rt_sampler_native_t&) = delete;
    rt_sampler_native_t(rt_sampler_native_t&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
    rt_sampler_native_t& operator=(rt_sampler_native_t&& other) noexcept
    {
        if (this != &other)
        {
            mt_release(handle);
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }
    ~rt_sampler_native_t() { mt_release(handle); }
};

struct rt_module_native_t
{
    MTL::Library* vlib = nullptr;
    MTL::Library* tlib = nullptr;
    MTL::Library* mlib = nullptr;
    MTL::Library* flib = nullptr;
    MTL::Library* clib = nullptr;
    MTL::Function* vfn = nullptr;
    MTL::Function* tfn = nullptr;
    MTL::Function* mfn = nullptr;
    MTL::Function* ffn = nullptr;
    MTL::Function* cfn = nullptr;
    MTL::RenderPipelineState* renderPipeline = nullptr;
    MTL::ComputePipelineState* computePipeline = nullptr;
    MTL::DepthStencilState* depthStencil = nullptr;
    MTL::PrimitiveType primitive = MTL::PrimitiveTypeTriangle;
    MTL::CullMode cull = MTL::CullModeBack;
    MTL::Winding winding = MTL::WindingCounterClockwise;
    rt_binding_t bindings[GL_MAX_BINDING_HANDLE_NUM] = {};

    rt_module_native_t() = default;
    rt_module_native_t(const rt_module_native_t&) = delete;
    rt_module_native_t& operator=(const rt_module_native_t&) = delete;
    rt_module_native_t(rt_module_native_t&& other) noexcept { *this = std::move(other); }
    rt_module_native_t& operator=(rt_module_native_t&& other) noexcept
    {
        if (this != &other)
        {
            mt_release(vlib); mt_release(tlib); mt_release(mlib); mt_release(flib); mt_release(clib);
            mt_release(vfn); mt_release(tfn); mt_release(mfn); mt_release(ffn); mt_release(cfn);
            mt_release(renderPipeline); mt_release(computePipeline); mt_release(depthStencil);
            vlib = other.vlib; tlib = other.tlib; mlib = other.mlib; flib = other.flib; clib = other.clib;
            vfn = other.vfn; tfn = other.tfn; mfn = other.mfn; ffn = other.ffn; cfn = other.cfn;
            renderPipeline = other.renderPipeline; computePipeline = other.computePipeline; depthStencil = other.depthStencil;
            primitive = other.primitive; cull = other.cull; winding = other.winding;
            std::memcpy(bindings, other.bindings, sizeof(bindings));
            other.vlib = other.tlib = other.mlib = other.flib = other.clib = nullptr;
            other.vfn = other.tfn = other.mfn = other.ffn = other.cfn = nullptr;
            other.renderPipeline = nullptr; other.computePipeline = nullptr; other.depthStencil = nullptr;
        }
        return *this;
    }
    ~rt_module_native_t()
    {
        mt_release(vlib); mt_release(tlib); mt_release(mlib); mt_release(flib); mt_release(clib);
        mt_release(vfn); mt_release(tfn); mt_release(mfn); mt_release(ffn); mt_release(cfn);
        mt_release(renderPipeline); mt_release(computePipeline); mt_release(depthStencil);
    }
};

struct rt_mesh_native_t { uint32_t vertexCount = 0, indexCount = 0; };
struct rt_meshlet_native_t { uint32_t vertexCount = 0, indexCount = 0; };
struct rt_pass_compute_native_t { uint32_t dummy = 0; };
struct rt_pass_render_native_t { bool offscreen = false; uint32_t width = 0, height = 0; };
struct rt_pass_transfer_native_t { uint32_t dummy = 0; };

struct mt_staging_t
{
    MTL::Buffer* buffer = nullptr;

    mt_staging_t() = default;
    mt_staging_t(const mt_staging_t&) = delete;
    mt_staging_t& operator=(const mt_staging_t&) = delete;
    mt_staging_t(mt_staging_t&& other) noexcept : buffer(other.buffer) { other.buffer = nullptr; }
    mt_staging_t& operator=(mt_staging_t&& other) noexcept
    {
        if (this != &other)
        {
            mt_release(buffer);
            buffer = other.buffer;
            other.buffer = nullptr;
        }
        return *this;
    }
    ~mt_staging_t() { mt_release(buffer); }
};

struct mt_native_t
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

    MTL::Device* device = nullptr;
    MTL::CommandQueue* queue = nullptr;
    MTL::CommandBuffer* cmd = nullptr;
    MTL::RenderCommandEncoder* renderEncoder = nullptr;
    MTL::ComputeCommandEncoder* computeEncoder = nullptr;
    MTL::BlitCommandEncoder* blitEncoder = nullptr;
    MTL::SamplerState* defaultSampler = nullptr;
    bool ownsQueue = false;
    std::vector<mt_staging_t> pendingStaging;
    uint8_t pushData[128] = {};
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
} static metal;

static void mt_end_encoder()
{
    if (metal.renderEncoder) { metal.renderEncoder->endEncoding(); mt_release(metal.renderEncoder); }
    if (metal.computeEncoder) { metal.computeEncoder->endEncoding(); mt_release(metal.computeEncoder); }
    if (metal.blitEncoder) { metal.blitEncoder->endEncoding(); mt_release(metal.blitEncoder); }
}

static void mt_retain_cmd(MTL::CommandBuffer* cmd)
{
    metal.cmd = cmd;
    if (metal.cmd) metal.cmd->retain();
}

static void mt_ensure_blit()
{
    if (metal.blitEncoder) return;
    mt_end_encoder();
    metal.blitEncoder = metal.cmd->blitCommandEncoder();
    if (metal.blitEncoder) metal.blitEncoder->retain();
}

static void mt_flush_staging()
{
    metal.pendingStaging.clear();
}

static bool mt_create_staging(size_t size, mt_staging_t& staging, void** mapped)
{
    staging.buffer = metal.device->newBuffer(size, MTL::ResourceStorageModeShared);
    if (!staging.buffer) return false;
    if (mapped) *mapped = staging.buffer->contents();
    return true;
}

static void mt_transition_buffer(rt_buffer_native_t& buffer, mt_res_state_t dst)
{
    if (!buffer.handle || buffer.state == dst) return;
    buffer.state = dst;
}

static void mt_transition_image(rt_texture_native_t& image, mt_res_state_t dst)
{
    if (!image.handle || image.state == dst) return;
    image.state = dst;
}

static rt_buffer_native_t* mt_buffer_native(rt_buffer_t const& buffer)
{
    if (!buffer.native || buffer.handle == 0) return nullptr;
    auto it = metal.buffers.find(buffer.handle);
    return it == metal.buffers.end() ? nullptr : &it->second;
}

static rt_texture_native_t* mt_texture_native(rt_texture_t const& texture)
{
    if (!texture.native || texture.handle == 0) return nullptr;
    auto it = metal.textures.find(texture.handle);
    return it == metal.textures.end() ? nullptr : &it->second;
}

static rt_sampler_native_t* mt_sampler_native(rt_sampler_t const& sampler)
{
    if (!sampler.native || sampler.handle == 0) return nullptr;
    auto it = metal.samplers.find(sampler.handle);
    return it == metal.samplers.end() ? nullptr : &it->second;
}

static rt_module_native_t* mt_current_module_native()
{
    if (metal.currentPassType == GL_MODULE_COMPUTE && metal.currentComputePass)
        return (rt_module_native_t*)metal.currentComputePass->module.native;
    if (metal.currentPassType == GL_MODULE_RENDER && metal.currentRenderPass)
        return (rt_module_native_t*)metal.currentRenderPass->module.native;
    return nullptr;
}

static void mt_require_pass(GLenum type)
{
    if (metal.currentPipeline == nullptr || metal.currentPassType != type)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
}

static void mt_clear_bindings()
{
    for (auto& binding : metal.currentBinding)
        binding = {};
}

static MTL::Function* mt_function_from_binary(MTL::Library* library)
{
    if (!library) return nullptr;
    MTL::Function* fn = library->newFunction(MTLSTR("main"));
    if (fn) return fn;
    NS::Array* names = library->functionNames();
    if (names && names->count() > 0)
        return library->newFunction(static_cast<const NS::String*>(names->object(0)));
    return nullptr;
}

static MTL::Library* mt_create_library(const char* data, uint32_t length)
{
    if (!data || !length || !metal.device) return nullptr;
    NS::Error* error = nullptr;
    bool text = data[0] != 0 && (unsigned char)data[0] < 0x80;
    for (uint32_t i = 0; i < std::min(length, 8u) && text; ++i)
        if ((unsigned char)data[i] < 9 && data[i] != '\n' && data[i] != '\r' && data[i] != '\t')
            text = false;
    if (text)
    {
        NS::String* source = NS::String::alloc()->init(const_cast<char*>(data), length, NS::UTF8StringEncoding, false);
        MTL::Library* library = metal.device->newLibrary(source, nullptr, &error);
        mt_release(source);
        return library;
    }
    dispatch_data_t blob = dispatch_data_create(data, length, nullptr, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    MTL::Library* library = metal.device->newLibrary(blob, &error);
    if (blob) dispatch_release(blob);
    return library;
}

static void mt_fill_render_state(rt_module_render_t& result, rt_module_render_info_t const& info)
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

static void mt_fill_color_attachments(MTL::RenderPipelineColorAttachmentDescriptorArray* colors, rt_module_render_info_t const& info)
{
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        auto* attachment = colors->object(i);
        attachment->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
        bool blend =
            (info.colors[i].color.func != GL_FUNC_ADD || info.colors[i].color.src != GL_ONE ||
             info.colors[i].color.dst != GL_ZERO || info.colors[i].alpha.func != GL_FUNC_ADD ||
             info.colors[i].alpha.src != GL_ONE || info.colors[i].alpha.dst != GL_ZERO);
        attachment->setBlendingEnabled(blend);
        attachment->setSourceRGBBlendFactor(gl_to_mt_blend(info.colors[i].color.src));
        attachment->setDestinationRGBBlendFactor(gl_to_mt_blend(info.colors[i].color.dst));
        attachment->setRgbBlendOperation(gl_to_mt_blend_op(info.colors[i].color.func));
        attachment->setSourceAlphaBlendFactor(gl_to_mt_blend(info.colors[i].alpha.src));
        attachment->setDestinationAlphaBlendFactor(gl_to_mt_blend(info.colors[i].alpha.dst));
        attachment->setAlphaBlendOperation(gl_to_mt_blend_op(info.colors[i].alpha.func));
    }
}

static bool mt_create_depth_stencil(rt_module_native_t& native, rt_module_render_info_t const& info, bool stencilEnabled)
{
    MTL::DepthStencilDescriptor* ds = MTL::DepthStencilDescriptor::alloc()->init();
    ds->setDepthCompareFunction(gl_to_mt_compare(info.depth.func));
    ds->setDepthWriteEnabled(info.depth.write);
    if (stencilEnabled)
    {
        MTL::StencilDescriptor* front = MTL::StencilDescriptor::alloc()->init();
        front->setStencilCompareFunction(gl_to_mt_compare(info.stencil.front.func));
        front->setStencilFailureOperation(gl_to_mt_stencil_op(info.stencil.front.sfail));
        front->setDepthFailureOperation(gl_to_mt_stencil_op(info.stencil.front.zfail));
        front->setDepthStencilPassOperation(gl_to_mt_stencil_op(info.stencil.front.zpass));
        front->setReadMask(info.stencil.read);
        front->setWriteMask(info.stencil.write);
        MTL::StencilDescriptor* back = MTL::StencilDescriptor::alloc()->init();
        back->setStencilCompareFunction(gl_to_mt_compare(info.stencil.back.func));
        back->setStencilFailureOperation(gl_to_mt_stencil_op(info.stencil.back.sfail));
        back->setDepthFailureOperation(gl_to_mt_stencil_op(info.stencil.back.zfail));
        back->setDepthStencilPassOperation(gl_to_mt_stencil_op(info.stencil.back.zpass));
        back->setReadMask(info.stencil.read);
        back->setWriteMask(info.stencil.write);
        ds->setFrontFaceStencil(front);
        ds->setBackFaceStencil(back);
        mt_release(front);
        mt_release(back);
    }
    native.depthStencil = metal.device->newDepthStencilState(ds);
    mt_release(ds);
    native.primitive = gl_to_mt_primitive(info.primitive);
    native.cull = gl_to_mt_cull(info.cull_mode);
    native.winding = (info.front_face == GL_CW) ? MTL::WindingClockwise : MTL::WindingCounterClockwise;
    for (uint32_t i = 0; i < GL_MAX_BINDING_HANDLE_NUM; ++i)
        native.bindings[i] = info.binding[i];
    return native.depthStencil != nullptr;
}

static bool mt_create_graphics_pipeline(rt_module_native_t& native, rt_module_render_info_t const& info, bool meshlet)
{
    const bool depthEnabled = (info.depth.func != GL_ALWAYS || info.depth.write);
    const bool stencilEnabled =
        (info.stencil.back.func != GL_ALWAYS || info.stencil.back.sfail != GL_KEEP ||
         info.stencil.back.zfail != GL_KEEP || info.stencil.back.zpass != GL_KEEP ||
         info.stencil.front.func != GL_ALWAYS || info.stencil.front.sfail != GL_KEEP ||
         info.stencil.front.zfail != GL_KEEP || info.stencil.front.zpass != GL_KEEP);

    NS::Error* error = nullptr;
    if (meshlet)
    {
        MTL::MeshRenderPipelineDescriptor* desc = MTL::MeshRenderPipelineDescriptor::alloc()->init();
        desc->setObjectFunction(native.tfn);
        desc->setMeshFunction(native.mfn);
        desc->setFragmentFunction(native.ffn);
        mt_fill_color_attachments(desc->colorAttachments(), info);
        if (stencilEnabled) desc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
        else if (depthEnabled) desc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
        native.renderPipeline = metal.device->newRenderPipelineState(desc, MTL::PipelineOptionNone, nullptr, &error);
        mt_release(desc);
    }
    else
    {
        MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
        desc->setVertexFunction(native.vfn);
        desc->setFragmentFunction(native.ffn);
        MTL::VertexDescriptor* vd = MTL::VertexDescriptor::alloc()->init();
        for (uint32_t i = 0; i < GL_MAX_VERTEX_BUFFER_NUM; ++i)
        {
            if (info.vertex[i].type == GL_NONE || info.vertex[i].count == 0) continue;
            uint32_t loc = info.vertex[i].location;
            vd->attributes()->object(loc)->setFormat(gl_to_mt_vertex_format(info.vertex[i].type, info.vertex[i].count));
            vd->attributes()->object(loc)->setOffset(0);
            vd->attributes()->object(loc)->setBufferIndex(loc);
            vd->layouts()->object(loc)->setStride(gl_vertex_size(info.vertex[i].type, info.vertex[i].count));
            vd->layouts()->object(loc)->setStepFunction(info.vertex[i].instance ? MTL::VertexStepFunctionPerInstance : MTL::VertexStepFunctionPerVertex);
            vd->layouts()->object(loc)->setStepRate(1);
        }
        desc->setVertexDescriptor(vd);
        mt_release(vd);
        mt_fill_color_attachments(desc->colorAttachments(), info);
        if (stencilEnabled) desc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
        else if (depthEnabled) desc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
        native.renderPipeline = metal.device->newRenderPipelineState(desc, &error);
        mt_release(desc);
    }
    if (!native.renderPipeline) return false;
    return mt_create_depth_stencil(native, info, stencilEnabled);
}

static void mt_destroy_module_native(uint32_t handle, void*& native)
{
    metal.modules.erase(handle);
    native = nullptr;
}

static void mt_apply_push()
{
    if (metal.pushLength == 0) return;
    if (metal.renderEncoder)
    {
        metal.renderEncoder->setVertexBytes(metal.pushData, metal.pushLength, 16);
        metal.renderEncoder->setFragmentBytes(metal.pushData, metal.pushLength, 16);
    }
    if (metal.computeEncoder)
        metal.computeEncoder->setBytes(metal.pushData, metal.pushLength, 16);
}

static void mt_flush_descriptors()
{
    auto* mod = mt_current_module_native();
    if (!mod) return;
    mt_apply_push();
    for (uint32_t i = 0; i < GL_MAX_BINDING_HANDLE_NUM; ++i)
    {
        auto& slot = metal.currentBinding[i];
        if (slot.type == GL_NONE) continue;
        if (slot.type == GL_BINDING_BUFFER)
        {
            auto* buf = mt_buffer_native(slot.buffer);
            if (!buf) continue;
            bool storage = (slot.buffer_bind.target == GL_SHADER_STORAGE_BUFFER);
            mt_transition_buffer(*buf, storage ? MTL_STATE_SHADER_WRITE : MTL_STATE_SHADER_READ);
            if (metal.renderEncoder)
            {
                metal.renderEncoder->setVertexBuffer(buf->handle, 0, slot.buffer_bind.binding);
                metal.renderEncoder->setFragmentBuffer(buf->handle, 0, slot.buffer_bind.binding);
            }
            if (metal.computeEncoder)
                metal.computeEncoder->setBuffer(buf->handle, 0, slot.buffer_bind.binding);
        }
        else if (slot.type == GL_BINDING_TEXTURE)
        {
            auto* tex = mt_texture_native(slot.texture);
            if (!tex) continue;
            mt_transition_image(*tex, MTL_STATE_SHADER_READ);
            MTL::SamplerState* samp = metal.defaultSampler;
            if (auto* s = mt_sampler_native(slot.sampler))
                samp = s->handle;
            if (metal.renderEncoder)
            {
                metal.renderEncoder->setFragmentTexture(tex->handle, slot.texture_bind.binding);
                metal.renderEncoder->setFragmentSamplerState(samp, slot.texture_bind.binding);
            }
            if (metal.computeEncoder)
            {
                metal.computeEncoder->setTexture(tex->handle, slot.texture_bind.binding);
                metal.computeEncoder->setSamplerState(samp, slot.texture_bind.binding);
            }
        }
        else if (slot.type == GL_BINDING_STORAGE_TEXTURE)
        {
            auto* tex = mt_texture_native(slot.storage_texture);
            if (!tex) continue;
            mt_transition_image(*tex, MTL_STATE_SHADER_WRITE);
            if (metal.renderEncoder)
                metal.renderEncoder->setFragmentTexture(tex->handle, slot.storage_texture_bind.binding);
            if (metal.computeEncoder)
                metal.computeEncoder->setTexture(tex->handle, slot.storage_texture_bind.binding);
        }
        else if (slot.type == GL_BINDING_SAMPLER)
        {
            auto* samp = mt_sampler_native(slot.sampler);
            if (!samp) continue;
            if (metal.renderEncoder)
                metal.renderEncoder->setFragmentSamplerState(samp->handle, slot.sampler_bind.binding);
            if (metal.computeEncoder)
                metal.computeEncoder->setSamplerState(samp->handle, slot.sampler_bind.binding);
        }
    }
}

void mt_load_library(MTL::Device* device, MTL::CommandQueue* queue)
{
    metal.device = device;
    metal.queue = queue;
    metal.ownsQueue = false;
    if (!metal.device)
    {
        fprintf(stderr, "Metal: device is null\n");
        abort();
    }
    if (!metal.queue)
    {
        metal.queue = metal.device->newCommandQueue();
        metal.ownsQueue = true;
    }
    mt_retain_cmd(metal.queue->commandBuffer());
    MTL::SamplerDescriptor* samp = MTL::SamplerDescriptor::alloc()->init();
    samp->setMinFilter(MTL::SamplerMinMagFilterLinear);
    samp->setMagFilter(MTL::SamplerMinMagFilterLinear);
    samp->setSAddressMode(MTL::SamplerAddressModeRepeat);
    samp->setTAddressMode(MTL::SamplerAddressModeRepeat);
    samp->setRAddressMode(MTL::SamplerAddressModeRepeat);
    metal.defaultSampler = metal.device->newSamplerState(samp);
    mt_release(samp);

    rt_unload_library = mt_unload_library;
    rt_create_buffer = mt_create_buffer;
    rt_destroy_buffer = mt_destroy_buffer;
    rt_bind_buffer = mt_bind_buffer;
    rt_map_buffer = mt_map_buffer;
    rt_unmap_buffer = mt_unmap_buffer;
    rt_create_texture = mt_create_texture;
    rt_create_texture_color = mt_create_texture_color;
    rt_create_texture_color_float = mt_create_texture_color_float;
    rt_create_texture_depth = mt_create_texture_depth;
    rt_create_texture_depth_stencil = mt_create_texture_depth_stencil;
    rt_destroy_texture = mt_destroy_texture;
    rt_bind_texture = mt_bind_texture;
    rt_bind_texture_storage = mt_bind_texture_storage;
    rt_create_sampler = mt_create_sampler;
    rt_destroy_sampler = mt_destroy_sampler;
    rt_bind_sampler = mt_bind_sampler;
    rt_create_module_compute = mt_create_module_compute;
    rt_create_module_render = mt_create_module_render;
    rt_create_module_meshlet = mt_create_module_meshlet;
    rt_destroy_module_render = mt_destroy_module_render;
    rt_destroy_module_compute = mt_destroy_module_compute;
    rt_begin_compute = mt_begin_compute;
    rt_end_compute = mt_end_compute;
    rt_dispatch_compute = mt_dispatch_compute;
    rt_begin_render = mt_begin_render;
    rt_end_render = mt_end_render;
    rt_set_viewport = mt_set_viewport;
    rt_set_scissor = mt_set_scissor;
    rt_draw_mesh_task = mt_draw_mesh_task;
    rt_push_constant = mt_push_constant;
    rt_push_const_int = mt_push_const_int;
    rt_push_const_uint = mt_push_const_uint;
    rt_push_const_float = mt_push_const_float;
    rt_push_const_vec2 = mt_push_const_vec2;
    rt_push_const_vec3 = mt_push_const_vec3;
    rt_push_const_vec4 = mt_push_const_vec4;
    rt_push_const_mat3 = mt_push_const_mat3;
    rt_push_const_mat4 = mt_push_const_mat4;
    rt_begin_transfer = mt_begin_transfer;
    rt_end_transfer = mt_end_transfer;
    rt_copy_buffer = mt_copy_buffer;
    rt_copy_buffer_data = mt_copy_buffer_data;
    rt_copy_buffer_texture = mt_copy_buffer_texture;
    rt_copy_texture = mt_copy_texture;
    rt_copy_texture_data = mt_copy_texture_data;
    rt_copy_texture_buffer = mt_copy_texture_buffer;
    rt_create_mesh = mt_create_mesh;
    rt_destroy_mesh = mt_destroy_mesh;
    rt_draw_mesh = mt_draw_mesh;
    rt_draw_mesh_multi = mt_draw_mesh_multi;
    rt_create_meshlet = mt_create_meshlet;
    rt_destroy_meshlet = mt_destroy_meshlet;
    rt_draw_meshlet = mt_draw_meshlet;
    rt_create_mesh_screen = mt_create_mesh_screen;
    rt_draw_screen = mt_draw_screen;
    rt_submit = mt_submit;
}

void mt_unload_library()
{
    mt_end_encoder();
    mt_release(metal.cmd);
    mt_flush_staging();
    metal.buffers.clear();
    metal.textures.clear();
    metal.samplers.clear();
    metal.modules.clear();
    metal.meshes.clear();
    metal.meshlets.clear();
    metal.computePasses.clear();
    metal.renderPasses.clear();
    metal.transferPasses.clear();
    mt_release(metal.defaultSampler);
    if (metal.ownsQueue) mt_release(metal.queue);
    else metal.queue = nullptr;
    metal.device = nullptr;
    metal.ownsQueue = false;
    metal.bufferID = metal.textureID = metal.samplerID = metal.moduleID = 0;
    metal.meshID = metal.meshletID = metal.passID = 0;
    metal.currentPassType = GL_NONE;
    metal.currentPipeline = nullptr;

    if (rt_unload_library == mt_unload_library) rt_unload_library = nullptr;
    if (rt_create_buffer == mt_create_buffer) rt_create_buffer = nullptr;
    if (rt_destroy_buffer == mt_destroy_buffer) rt_destroy_buffer = nullptr;
    if (rt_bind_buffer == mt_bind_buffer) rt_bind_buffer = nullptr;
    if (rt_map_buffer == mt_map_buffer) rt_map_buffer = nullptr;
    if (rt_unmap_buffer == mt_unmap_buffer) rt_unmap_buffer = nullptr;
    if (rt_create_texture == mt_create_texture) rt_create_texture = nullptr;
    if (rt_create_texture_color == mt_create_texture_color) rt_create_texture_color = nullptr;
    if (rt_create_texture_color_float == mt_create_texture_color_float) rt_create_texture_color_float = nullptr;
    if (rt_create_texture_depth == mt_create_texture_depth) rt_create_texture_depth = nullptr;
    if (rt_create_texture_depth_stencil == mt_create_texture_depth_stencil) rt_create_texture_depth_stencil = nullptr;
    if (rt_destroy_texture == mt_destroy_texture) rt_destroy_texture = nullptr;
    if (rt_bind_texture == mt_bind_texture) rt_bind_texture = nullptr;
    if (rt_bind_texture_storage == mt_bind_texture_storage) rt_bind_texture_storage = nullptr;
    if (rt_create_sampler == mt_create_sampler) rt_create_sampler = nullptr;
    if (rt_destroy_sampler == mt_destroy_sampler) rt_destroy_sampler = nullptr;
    if (rt_bind_sampler == mt_bind_sampler) rt_bind_sampler = nullptr;
    if (rt_create_module_compute == mt_create_module_compute) rt_create_module_compute = nullptr;
    if (rt_create_module_render == mt_create_module_render) rt_create_module_render = nullptr;
    if (rt_create_module_meshlet == mt_create_module_meshlet) rt_create_module_meshlet = nullptr;
    if (rt_destroy_module_render == mt_destroy_module_render) rt_destroy_module_render = nullptr;
    if (rt_destroy_module_compute == mt_destroy_module_compute) rt_destroy_module_compute = nullptr;
    if (rt_begin_compute == mt_begin_compute) rt_begin_compute = nullptr;
    if (rt_end_compute == mt_end_compute) rt_end_compute = nullptr;
    if (rt_dispatch_compute == mt_dispatch_compute) rt_dispatch_compute = nullptr;
    if (rt_begin_render == mt_begin_render) rt_begin_render = nullptr;
    if (rt_end_render == mt_end_render) rt_end_render = nullptr;
    if (rt_set_viewport == mt_set_viewport) rt_set_viewport = nullptr;
    if (rt_set_scissor == mt_set_scissor) rt_set_scissor = nullptr;
    if (rt_draw_mesh_task == mt_draw_mesh_task) rt_draw_mesh_task = nullptr;
    if (rt_push_constant == mt_push_constant) rt_push_constant = nullptr;
    if (rt_push_const_int == mt_push_const_int) rt_push_const_int = nullptr;
    if (rt_push_const_uint == mt_push_const_uint) rt_push_const_uint = nullptr;
    if (rt_push_const_float == mt_push_const_float) rt_push_const_float = nullptr;
    if (rt_push_const_vec2 == mt_push_const_vec2) rt_push_const_vec2 = nullptr;
    if (rt_push_const_vec3 == mt_push_const_vec3) rt_push_const_vec3 = nullptr;
    if (rt_push_const_vec4 == mt_push_const_vec4) rt_push_const_vec4 = nullptr;
    if (rt_push_const_mat3 == mt_push_const_mat3) rt_push_const_mat3 = nullptr;
    if (rt_push_const_mat4 == mt_push_const_mat4) rt_push_const_mat4 = nullptr;
    if (rt_begin_transfer == mt_begin_transfer) rt_begin_transfer = nullptr;
    if (rt_end_transfer == mt_end_transfer) rt_end_transfer = nullptr;
    if (rt_copy_buffer == mt_copy_buffer) rt_copy_buffer = nullptr;
    if (rt_copy_buffer_data == mt_copy_buffer_data) rt_copy_buffer_data = nullptr;
    if (rt_copy_buffer_texture == mt_copy_buffer_texture) rt_copy_buffer_texture = nullptr;
    if (rt_copy_texture == mt_copy_texture) rt_copy_texture = nullptr;
    if (rt_copy_texture_data == mt_copy_texture_data) rt_copy_texture_data = nullptr;
    if (rt_copy_texture_buffer == mt_copy_texture_buffer) rt_copy_texture_buffer = nullptr;
    if (rt_create_mesh == mt_create_mesh) rt_create_mesh = nullptr;
    if (rt_destroy_mesh == mt_destroy_mesh) rt_destroy_mesh = nullptr;
    if (rt_draw_mesh == mt_draw_mesh) rt_draw_mesh = nullptr;
    if (rt_draw_mesh_multi == mt_draw_mesh_multi) rt_draw_mesh_multi = nullptr;
    if (rt_create_meshlet == mt_create_meshlet) rt_create_meshlet = nullptr;
    if (rt_destroy_meshlet == mt_destroy_meshlet) rt_destroy_meshlet = nullptr;
    if (rt_draw_meshlet == mt_draw_meshlet) rt_draw_meshlet = nullptr;
    if (rt_create_mesh_screen == mt_create_mesh_screen) rt_create_mesh_screen = nullptr;
    if (rt_draw_screen == mt_draw_screen) rt_draw_screen = nullptr;
    if (rt_submit == mt_submit) rt_submit = nullptr;
}

rt_buffer_t mt_create_buffer(rt_buffer_info_t const& info)
{
    if (info.usage == 0)
    {
        fprintf(stderr, "Buffer usage must not be 0");
        abort();
    }
    rt_buffer_t result = {};
    if (!metal.device || info.size == 0) return result;
    uint32_t handle = metal.bufferID + 1;
    auto& native = metal.buffers[handle];
    bool hostVisible = (info.usage & (GL_BUFFER_USAGE_MAP_READ | GL_BUFFER_USAGE_MAP_WRITE)) || info.data;
    MTL::ResourceOptions options = hostVisible ? MTL::ResourceStorageModeShared : MTL::ResourceStorageModePrivate;
    if (info.data && hostVisible)
        native.handle = metal.device->newBuffer(info.data, info.size, options);
    else
        native.handle = metal.device->newBuffer(info.size, options);
    if (!native.handle)
    {
        metal.buffers.erase(handle);
        return {};
    }
    if (info.data && !hostVisible)
    {
        mt_staging_t staging = {};
        void* ptr = nullptr;
        if (mt_create_staging(info.size, staging, &ptr))
        {
            std::memcpy(ptr, info.data, info.size);
            mt_ensure_blit();
            mt_transition_buffer(native, MTL_STATE_COPY_DST);
            metal.blitEncoder->copyFromBuffer(staging.buffer, 0, native.handle, 0, info.size);
            metal.pendingStaging.push_back(std::move(staging));
        }
    }
    else if (info.data)
        native.state = MTL_STATE_HOST;

    metal.bufferID = handle;
    result.handle = handle;
    result.size = info.size;
    result.usage = info.usage;
    result.native = &native;
    return result;
}

void mt_destroy_buffer(rt_buffer_t& buffer)
{
    if (buffer.native)
        metal.buffers.erase(buffer.handle);
    buffer = {};
}

void mt_bind_buffer(rt_buffer_t& buffer, rt_buffer_bind_t bind)
{
    if (metal.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    metal.currentBinding[bind.binding].type = GL_BINDING_BUFFER;
    metal.currentBinding[bind.binding].buffer = buffer;
    metal.currentBinding[bind.binding].buffer_bind = bind;
}

void* mt_map_buffer(rt_buffer_t& buffer, GLenum mode, size_t offset, size_t size)
{
    (void)mode;
    auto* native = mt_buffer_native(buffer);
    if (!native || !native->handle) return nullptr;
    if (offset > buffer.size) return nullptr;
    if (size == 0) size = buffer.size - offset;
    if (size == 0 || offset + size > buffer.size) return nullptr;
    void* base = native->handle->contents();
    if (!base) return nullptr;
    mt_transition_buffer(*native, MTL_STATE_HOST);
    native->mapped = (uint8_t*)base + offset;
    native->mappedOffset = offset;
    native->mappedSize = size;
    return native->mapped;
}

void mt_unmap_buffer(rt_buffer_t& buffer)
{
    auto* native = mt_buffer_native(buffer);
    if (!native) return;
    native->mapped = nullptr;
    native->mappedOffset = 0;
    native->mappedSize = 0;
}

rt_texture_t mt_create_texture(rt_texture_info_t const& info)
{
    rt_texture_t result = {};
    if (!metal.device || info.width == 0) return result;
    if (info.target != GL_TEXTURE_1D && info.height == 0) return result;

    uint32_t handle = metal.textureID + 1;
    auto& native = metal.textures[handle];
    native.format = gl_to_mt_format(info.format, info.type, info.internal_format);
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

    MTL::TextureDescriptor* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setPixelFormat(native.format);
    desc->setWidth(native.width);
    desc->setHeight(native.height);
    desc->setMipmapLevelCount(native.mipLevels);
    desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite | MTL::TextureUsageRenderTarget);
    desc->setStorageMode(MTL::StorageModePrivate);
    if (info.target == GL_TEXTURE_1D) desc->setTextureType(MTL::TextureType1D);
    else if (info.target == GL_TEXTURE_3D) { desc->setTextureType(MTL::TextureType3D); desc->setDepth(native.depth); }
    else if (info.target == GL_TEXTURE_2D_ARRAY) { desc->setTextureType(MTL::TextureType2DArray); desc->setArrayLength(native.layers); }
    else if (info.target == GL_TEXTURE_2D_MULTISAMPLE) { desc->setTextureType(MTL::TextureType2DMultisample); desc->setSampleCount(info.samples ? info.samples : 1); }
    else desc->setTextureType(MTL::TextureType2D);
    native.handle = metal.device->newTexture(desc);
    mt_release(desc);
    if (!native.handle)
    {
        metal.textures.erase(handle);
        return {};
    }

    if (info.data)
    {
        size_t bpp = mt_format_bytes(native.format);
        size_t bytes = (size_t)native.width * native.height * ((info.target == GL_TEXTURE_3D) ? native.depth : 1) * bpp;
        mt_staging_t staging = {};
        void* ptr = nullptr;
        if (mt_create_staging(bytes, staging, &ptr))
        {
            std::memcpy(ptr, info.data, bytes);
            mt_ensure_blit();
            mt_transition_image(native, MTL_STATE_COPY_DST);
            MTL::Size size = MTL::Size::Make(native.width, native.height, (info.target == GL_TEXTURE_3D) ? native.depth : 1);
            metal.blitEncoder->copyFromBuffer(staging.buffer, 0, native.width * bpp, (NS::UInteger)bytes, size,
                native.handle, 0, 0, MTL::Origin::Make(0, 0, 0));
            metal.pendingStaging.push_back(std::move(staging));
        }
        mt_transition_image(native, MTL_STATE_SHADER_READ);
    }
    else
        mt_transition_image(native, mt_is_depth(native.format) ? MTL_STATE_DEPTH : MTL_STATE_COLOR);

    metal.textureID = handle;
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

rt_texture_t mt_create_texture_color(uint32_t width, uint32_t height, const void* data)
{
    return mt_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_RGBA, .internal_format = GL_RGBA8, .type = GL_UNSIGNED_BYTE,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

rt_texture_t mt_create_texture_color_float(uint32_t width, uint32_t height, const void* data)
{
    return mt_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_RGBA, .internal_format = GL_RGBA32F, .type = GL_FLOAT,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

rt_texture_t mt_create_texture_depth(uint32_t width, uint32_t height, const void* data)
{
    return mt_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_DEPTH_COMPONENT, .internal_format = GL_DEPTH_COMPONENT32F, .type = GL_FLOAT,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

rt_texture_t mt_create_texture_depth_stencil(uint32_t width, uint32_t height, const void* data)
{
    return mt_create_texture({
        .width = width, .height = height, .target = GL_TEXTURE_2D,
        .format = GL_DEPTH_STENCIL, .internal_format = GL_DEPTH32F_STENCIL8, .type = GL_FLOAT,
        .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR,
        .wrap_s = GL_CLAMP_TO_EDGE, .wrap_t = GL_CLAMP_TO_EDGE, .wrap_r = GL_CLAMP_TO_EDGE,
        .data = data
    });
}

void mt_destroy_texture(rt_texture_t& texture)
{
    if (texture.native)
        metal.textures.erase(texture.handle);
    texture = {};
}

void mt_bind_texture(rt_texture_t& texture, rt_texture_bind_t bind)
{
    if (metal.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    metal.currentBinding[bind.binding].type = GL_BINDING_TEXTURE;
    metal.currentBinding[bind.binding].texture = texture;
    metal.currentBinding[bind.binding].texture_bind = bind;
}

void mt_bind_texture_storage(rt_texture_t& texture, rt_texture_storage_bind_t bind)
{
    if (metal.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    metal.currentBinding[bind.binding].type = GL_BINDING_STORAGE_TEXTURE;
    metal.currentBinding[bind.binding].storage_texture = texture;
    metal.currentBinding[bind.binding].storage_texture_bind = bind;
}

rt_sampler_t mt_create_sampler(rt_sampler_info_t const& info)
{
    rt_sampler_t result = {};
    uint32_t handle = metal.samplerID + 1;
    auto& native = metal.samplers[handle];
    MTL::SamplerDescriptor* desc = MTL::SamplerDescriptor::alloc()->init();
    desc->setMinFilter(gl_to_mt_filter(info.min_filter));
    desc->setMagFilter(gl_to_mt_filter(info.mag_filter));
    desc->setMipFilter(gl_to_mt_mip(info.min_filter));
    desc->setSAddressMode(gl_to_mt_address(info.wrap_s));
    desc->setTAddressMode(gl_to_mt_address(info.wrap_t));
    desc->setRAddressMode(gl_to_mt_address(info.wrap_r));
    native.handle = metal.device->newSamplerState(desc);
    mt_release(desc);
    metal.samplerID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void mt_destroy_sampler(rt_sampler_t& sampler)
{
    if (sampler.native)
        metal.samplers.erase(sampler.handle);
    sampler = {};
}

void mt_bind_sampler(rt_sampler_t& sampler, rt_sampler_bind_t bind)
{
    if (metal.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    metal.currentBinding[bind.binding].type = GL_BINDING_SAMPLER;
    metal.currentBinding[bind.binding].sampler = sampler;
    metal.currentBinding[bind.binding].sampler_bind = bind;
}

rt_module_compute_t mt_create_module_compute(rt_module_compute_info_t const& info)
{
    rt_module_compute_t result = {};
    if (!info.cshader || !info.clength || !metal.device) return result;
    uint32_t handle = metal.moduleID + 1;
    auto& native = metal.modules[handle];
    native.clib = mt_create_library(info.cshader, info.clength);
    native.cfn = mt_function_from_binary(native.clib);
    if (!native.cfn)
    {
        metal.modules.erase(handle);
        return {};
    }
    NS::Error* error = nullptr;
    native.computePipeline = metal.device->newComputePipelineState(native.cfn, &error);
    if (!native.computePipeline)
    {
        result.native = &native;
        mt_destroy_module_native(handle, result.native);
        return {};
    }
    metal.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

rt_module_render_t mt_create_module_render(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};
    if (!metal.device) return result;
    uint32_t handle = metal.moduleID + 1;
    auto& native = metal.modules[handle];
    if (info.vshader)
    {
        native.vlib = mt_create_library(info.vshader, info.vlength);
        native.vfn = mt_function_from_binary(native.vlib);
    }
    if (info.fshader)
    {
        native.flib = mt_create_library(info.fshader, info.flength);
        native.ffn = mt_function_from_binary(native.flib);
    }
    if (!mt_create_graphics_pipeline(native, info, false))
    {
        result.native = &native;
        mt_destroy_module_native(handle, result.native);
        return {};
    }
    metal.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    mt_fill_render_state(result, info);
    return result;
}

rt_module_render_t mt_create_module_meshlet(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};
    if (!info.mshader || !info.mlength || !metal.device) return result;
    uint32_t handle = metal.moduleID + 1;
    auto& native = metal.modules[handle];
    if (info.tshader)
    {
        native.tlib = mt_create_library(info.tshader, info.tlength);
        native.tfn = mt_function_from_binary(native.tlib);
    }
    native.mlib = mt_create_library(info.mshader, info.mlength);
    native.mfn = mt_function_from_binary(native.mlib);
    if (info.fshader)
    {
        native.flib = mt_create_library(info.fshader, info.flength);
        native.ffn = mt_function_from_binary(native.flib);
    }
    if (!native.mfn || !mt_create_graphics_pipeline(native, info, true))
    {
        result.native = &native;
        mt_destroy_module_native(handle, result.native);
        return {};
    }
    metal.moduleID = handle;
    result.handle = handle;
    result.native = &native;
    mt_fill_render_state(result, info);
    return result;
}

void mt_destroy_module_render(rt_module_render_t& module)
{
    mt_destroy_module_native(module.handle, module.native);
    module.handle = 0;
    module.vertex_vao = 0;
}

void mt_destroy_module_compute(rt_module_compute_t& module)
{
    mt_destroy_module_native(module.handle, module.native);
    module.handle = 0;
}

void mt_push_constant(uint8_t const* buffer, size_t length)
{
    mt_require_pass(metal.currentPassType);
    if (!buffer || length == 0) return;
    metal.pushLength = (uint32_t)std::min(length, sizeof(metal.pushData));
    std::memcpy(metal.pushData, buffer, metal.pushLength);
    mt_apply_push();
}

void mt_push_const_int(const char*, int32_t) {}
void mt_push_const_uint(const char*, uint32_t) {}
void mt_push_const_float(const char*, float) {}
void mt_push_const_vec2(const char*, const float*) {}
void mt_push_const_vec3(const char*, const float*) {}
void mt_push_const_vec4(const char*, const float*) {}
void mt_push_const_mat3(const char*, const float*) {}
void mt_push_const_mat4(const char*, const float*) {}

void mt_begin_compute(rt_pass_compute_t& pass)
{
    if (pass.module.handle == 0 || !pass.module.native)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (metal.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    mt_clear_bindings();
    pass.handle = ++metal.passID;
    auto& native = metal.computePasses[pass.handle];
    pass.native = &native;
    metal.currentPassType = GL_MODULE_COMPUTE;
    metal.currentComputePass = &pass;
    mt_end_encoder();
    metal.computeEncoder = metal.cmd->computeCommandEncoder();
    if (metal.computeEncoder) metal.computeEncoder->retain();
    auto* mod = (rt_module_native_t*)pass.module.native;
    if (mod && mod->computePipeline)
        metal.computeEncoder->setComputePipelineState(mod->computePipeline);
}

void mt_end_compute(rt_pass_compute_t& pass)
{
    if (metal.currentComputePass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (metal.computeEncoder)
    {
        metal.computeEncoder->endEncoding();
        mt_release(metal.computeEncoder);
    }
    metal.computePasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    metal.currentPassType = GL_NONE;
    metal.currentPipeline = nullptr;
    mt_clear_bindings();
}

void mt_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    mt_require_pass(GL_MODULE_COMPUTE);
    mt_flush_descriptors();
    MTL::Size groups = MTL::Size::Make(std::max(1u, groupX), std::max(1u, groupY), std::max(1u, groupZ));
    MTL::Size threads = MTL::Size::Make(1, 1, 1);
    auto* mod = mt_current_module_native();
    if (mod && mod->computePipeline)
        threads = MTL::Size::Make(mod->computePipeline->threadExecutionWidth(), 1, 1);
    metal.computeEncoder->dispatchThreadgroups(groups, threads);
}

void mt_begin_render(rt_pass_render_t& pass)
{
    if (pass.module.handle == 0 || !pass.module.native)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    if (metal.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    mt_clear_bindings();
    pass.handle = ++metal.passID;
    auto& native = metal.renderPasses[pass.handle];
    pass.native = &native;
    metal.currentPassType = GL_MODULE_RENDER;
    metal.currentRenderPass = &pass;

    native.offscreen = pass.depth.texture.handle != 0;
    for (auto& color : pass.colors)
        if (color.texture.handle) native.offscreen = true;

    MTL::RenderPassDescriptor* desc = MTL::RenderPassDescriptor::renderPassDescriptor();
    uint32_t width = 0, height = 0;
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        if (auto* tex = mt_texture_native(pass.colors[i].texture))
        {
            mt_transition_image(*tex, MTL_STATE_COLOR);
            auto* attachment = desc->colorAttachments()->object(i);
            attachment->setTexture(tex->handle);
            attachment->setLoadAction(pass.colors[i].clear ? MTL::LoadActionClear : MTL::LoadActionLoad);
            attachment->setStoreAction(MTL::StoreActionStore);
            attachment->setClearColor(MTL::ClearColor::Make(pass.colors[i].value.r, pass.colors[i].value.g, pass.colors[i].value.b, pass.colors[i].value.a));
            width = std::max(width, pass.colors[i].texture.width);
            height = std::max(height, pass.colors[i].texture.height);
        }
    }
    if (auto* tex = mt_texture_native(pass.depth.texture))
    {
        mt_transition_image(*tex, MTL_STATE_DEPTH);
        desc->depthAttachment()->setTexture(tex->handle);
        desc->depthAttachment()->setLoadAction(pass.depth.clear ? MTL::LoadActionClear : MTL::LoadActionLoad);
        desc->depthAttachment()->setStoreAction(MTL::StoreActionStore);
        desc->depthAttachment()->setClearDepth(pass.depth.value);
        if (pass.depth.texture.format == GL_DEPTH_STENCIL)
        {
            desc->stencilAttachment()->setTexture(tex->handle);
            desc->stencilAttachment()->setLoadAction(pass.stencil.clear ? MTL::LoadActionClear : MTL::LoadActionLoad);
            desc->stencilAttachment()->setStoreAction(MTL::StoreActionStore);
            desc->stencilAttachment()->setClearStencil((uint32_t)pass.stencil.value);
        }
        width = std::max(width, pass.depth.texture.width);
        height = std::max(height, pass.depth.texture.height);
    }
    native.width = width;
    native.height = height;
    mt_end_encoder();
    metal.renderEncoder = metal.cmd->renderCommandEncoder(desc);
    if (metal.renderEncoder) metal.renderEncoder->retain();
    auto* mod = (rt_module_native_t*)pass.module.native;
    if (mod && mod->renderPipeline)
        metal.renderEncoder->setRenderPipelineState(mod->renderPipeline);
    if (mod && mod->depthStencil)
        metal.renderEncoder->setDepthStencilState(mod->depthStencil);
    if (mod)
    {
        metal.renderEncoder->setCullMode(mod->cull);
        metal.renderEncoder->setFrontFacingWinding(mod->winding);
    }
    mt_set_viewport(0, 0, (int32_t)width, (int32_t)height);
    mt_set_scissor(0, 0, (int32_t)width, (int32_t)height);
}

void mt_end_render(rt_pass_render_t& pass)
{
    if (metal.currentRenderPass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (metal.renderEncoder)
    {
        metal.renderEncoder->endEncoding();
        mt_release(metal.renderEncoder);
    }
    for (auto& color : pass.colors)
        if (auto* tex = mt_texture_native(color.texture))
            mt_transition_image(*tex, MTL_STATE_SHADER_READ);
    if (auto* tex = mt_texture_native(pass.depth.texture))
        mt_transition_image(*tex, MTL_STATE_SHADER_READ);
    metal.renderPasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    metal.currentPassType = GL_NONE;
    metal.currentPipeline = nullptr;
    mt_clear_bindings();
}

void mt_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (metal.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (!metal.renderEncoder) return;
    MTL::Viewport vp = {(double)x, (double)y, (double)width, (double)height, 0.0, 1.0};
    metal.renderEncoder->setViewport(vp);
}

void mt_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (metal.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (!metal.renderEncoder) return;
    MTL::ScissorRect rect = {(NS::UInteger)std::max(0, x), (NS::UInteger)std::max(0, y), (NS::UInteger)std::max(0, width), (NS::UInteger)std::max(0, height)};
    metal.renderEncoder->setScissorRect(rect);
}

void mt_draw_mesh_task(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    mt_require_pass(GL_MODULE_RENDER);
    mt_flush_descriptors();
    MTL::Size groups = MTL::Size::Make(std::max(1u, groupX), std::max(1u, groupY), std::max(1u, groupZ));
    metal.renderEncoder->drawMeshThreadgroups(groups, MTL::Size::Make(1, 1, 1), MTL::Size::Make(1, 1, 1));
}

void mt_begin_transfer(rt_pass_transfer_t& pass)
{
    if (metal.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    pass.handle = ++metal.passID;
    auto& native = metal.transferPasses[pass.handle];
    pass.native = &native;
    metal.currentPassType = GL_MODULE_TRANSFER;
    metal.currentTransferPass = &pass;
    mt_ensure_blit();
}

void mt_end_transfer(rt_pass_transfer_t& pass)
{
    if (metal.currentTransferPass != &pass)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (metal.blitEncoder)
    {
        metal.blitEncoder->endEncoding();
        mt_release(metal.blitEncoder);
    }
    metal.transferPasses.erase(pass.handle);
    pass.handle = 0;
    pass.native = nullptr;
    metal.currentPassType = GL_NONE;
    metal.currentPipeline = nullptr;
}

void mt_copy_buffer(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize)
{
    mt_require_pass(GL_MODULE_TRANSFER);
    auto* src = mt_buffer_native(source.buffer);
    auto* dst = mt_buffer_native(destination.buffer);
    if (!src || !dst || copySize == 0) return;
    if (source.offset + copySize > source.buffer.size || destination.offset + copySize > destination.buffer.size)
        return;
    mt_transition_buffer(*src, MTL_STATE_COPY_SRC);
    mt_transition_buffer(*dst, MTL_STATE_COPY_DST);
    metal.blitEncoder->copyFromBuffer(src->handle, source.offset, dst->handle, destination.offset, copySize);
}

void mt_copy_buffer_data(rt_buffer_data_t source, rt_buffer_copy_t destination, size_t copySize)
{
    mt_require_pass(GL_MODULE_TRANSFER);
    auto* dst = mt_buffer_native(destination.buffer);
    if (!source.data || !dst || copySize == 0) return;
    if (source.offset + copySize > source.size || destination.offset + copySize > destination.buffer.size)
        return;
    if (void* mapped = dst->handle->contents())
    {
        std::memcpy((uint8_t*)mapped + destination.offset, source.data + source.offset, copySize);
        dst->state = MTL_STATE_HOST;
        return;
    }
    mt_staging_t staging = {};
    void* ptr = nullptr;
    if (!mt_create_staging(copySize, staging, &ptr)) return;
    std::memcpy(ptr, source.data + source.offset, copySize);
    mt_transition_buffer(*dst, MTL_STATE_COPY_DST);
    metal.blitEncoder->copyFromBuffer(staging.buffer, 0, dst->handle, destination.offset, copySize);
    metal.pendingStaging.push_back(std::move(staging));
}

void mt_copy_buffer_texture(rt_texture_copy_t source, rt_buffer_texel_t destination, rt_size_t copySize)
{
    mt_require_pass(GL_MODULE_TRANSFER);
    auto* src = mt_texture_native(source.texture);
    auto* dst = mt_buffer_native(destination.buffer);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    mt_transition_image(*src, MTL_STATE_COPY_SRC);
    mt_transition_buffer(*dst, MTL_STATE_COPY_DST);
    uint32_t bpp = mt_format_bytes(src->format);
    metal.blitEncoder->copyFromTexture(src->handle, 0, source.mipLevel,
        MTL::Origin::Make(source.origin.x, source.origin.y, source.origin.z),
        MTL::Size::Make(copySize.x, copySize.y, copySize.z ? copySize.z : 1),
        dst->handle, destination.offset,
        destination.bytesPerRow ? destination.bytesPerRow : copySize.x * bpp,
        destination.rowsPerImage ? destination.rowsPerImage * (destination.bytesPerRow ? destination.bytesPerRow : copySize.x * bpp) : 0);
}

void mt_copy_texture(rt_texture_copy_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    mt_require_pass(GL_MODULE_TRANSFER);
    auto* src = mt_texture_native(source.texture);
    auto* dst = mt_texture_native(destination.texture);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    mt_transition_image(*src, MTL_STATE_COPY_SRC);
    mt_transition_image(*dst, MTL_STATE_COPY_DST);
    metal.blitEncoder->copyFromTexture(src->handle, 0, source.mipLevel,
        MTL::Origin::Make(source.origin.x, source.origin.y, source.origin.z),
        MTL::Size::Make(copySize.x, copySize.y, copySize.z ? copySize.z : 1),
        dst->handle, 0, destination.mipLevel,
        MTL::Origin::Make(destination.origin.x, destination.origin.y, destination.origin.z));
}

void mt_copy_texture_data(rt_texture_data_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    mt_require_pass(GL_MODULE_TRANSFER);
    auto* dst = mt_texture_native(destination.texture);
    if (!source.data || !dst || copySize.x == 0 || copySize.y == 0) return;
    uint32_t bpp = mt_format_bytes(dst->format);
    size_t bytes = source.size ? source.size : (size_t)std::max(copySize.x * bpp, 1u) * copySize.y * std::max(1u, copySize.z);
    mt_staging_t staging = {};
    void* ptr = nullptr;
    if (!mt_create_staging(bytes, staging, &ptr)) return;
    std::memcpy(ptr, source.data + source.offset, std::min(bytes, source.size ? source.size - source.offset : bytes));
    mt_transition_image(*dst, MTL_STATE_COPY_DST);
    metal.blitEncoder->copyFromBuffer(staging.buffer, 0,
        source.bytesPerRow ? source.bytesPerRow : copySize.x * bpp,
        source.rowsPerImage ? source.rowsPerImage * (source.bytesPerRow ? source.bytesPerRow : copySize.x * bpp) : (NS::UInteger)bytes,
        MTL::Size::Make(copySize.x, copySize.y, copySize.z ? copySize.z : 1),
        dst->handle, 0, destination.mipLevel,
        MTL::Origin::Make(destination.origin.x, destination.origin.y, destination.origin.z));
    metal.pendingStaging.push_back(std::move(staging));
}

void mt_copy_texture_buffer(rt_buffer_texel_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    mt_require_pass(GL_MODULE_TRANSFER);
    auto* src = mt_buffer_native(source.buffer);
    auto* dst = mt_texture_native(destination.texture);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    mt_transition_image(*dst, MTL_STATE_COPY_DST);
    mt_transition_buffer(*src, MTL_STATE_COPY_SRC);
    uint32_t bpp = mt_format_bytes(dst->format);
    metal.blitEncoder->copyFromBuffer(src->handle, source.offset,
        source.bytesPerRow ? source.bytesPerRow : copySize.x * bpp,
        source.rowsPerImage ? source.rowsPerImage * (source.bytesPerRow ? source.bytesPerRow : copySize.x * bpp) : 0,
        MTL::Size::Make(copySize.x, copySize.y, copySize.z ? copySize.z : 1),
        dst->handle, 0, destination.mipLevel,
        MTL::Origin::Make(destination.origin.x, destination.origin.y, destination.origin.z));
}

rt_mesh_t mt_create_mesh(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    rt_mesh_t result = {};
    if (!vertices || vertex_count == 0) return result;
    uint32_t handle = metal.meshID + 1;
    auto& native = metal.meshes[handle];
    native.vertexCount = (uint32_t)vertex_count;
    native.indexCount = (uint32_t)index_count;
    if (vertices)
        result.vertex[0] = mt_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = mt_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = mt_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = GL_BUFFER_USAGE_VERTEX | GL_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = mt_create_buffer({.size = index_count * sizeof(uint32_t), .usage = GL_BUFFER_USAGE_INDEX | GL_BUFFER_USAGE_COPY_DST, .data = indices});
    std::iota(result.location, result.location + std::size(result.location), 0);
    metal.meshID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void mt_destroy_mesh(rt_mesh_t& mesh)
{
    for (auto& vertex : mesh.vertex)
        mt_destroy_buffer(vertex);
    mt_destroy_buffer(mesh.index);
    metal.meshes.erase(mesh.handle);
    mesh.handle = 0;
    mesh.native = nullptr;
}

static void mt_draw_mesh_impl(rt_mesh_t& mesh, uint32_t instanceCount)
{
    mt_require_pass(GL_MODULE_RENDER);
    mt_flush_descriptors();
    rt_module_render_t const& module = metal.currentRenderPass->module;
    auto* mod = (rt_module_native_t*)module.native;
    uint32_t vertex_count = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.type == GL_NONE || layout.count == 0) continue;
        for (uint32_t k = 0; k < std::size(mesh.vertex); ++k)
        {
            if (mesh.vertex[k].handle == 0 || mesh.location[k] != layout.location) continue;
            auto* native = mt_buffer_native(mesh.vertex[k]);
            if (!native) break;
            mt_transition_buffer(*native, MTL_STATE_VERTEX);
            metal.renderEncoder->setVertexBuffer(native->handle, 0, layout.location);
            uint32_t stride = gl_vertex_size(layout.type, layout.count);
            if (vertex_count == 0 && stride)
                vertex_count = (uint32_t)(mesh.vertex[k].size / stride);
            break;
        }
    }
    MTL::PrimitiveType primitive = mod ? mod->primitive : MTL::PrimitiveTypeTriangle;
    if (mesh.index.handle)
    {
        auto* native = mt_buffer_native(mesh.index);
        if (!native) return;
        uint32_t indexStride = gl_index_type_size(module.index_type);
        mt_transition_buffer(*native, MTL_STATE_INDEX);
        metal.renderEncoder->drawIndexedPrimitives(primitive, (NS::UInteger)(mesh.index.size / indexStride),
            gl_to_mt_index_type(module.index_type), native->handle, 0, instanceCount);
    }
    else
        metal.renderEncoder->drawPrimitives(primitive, 0, vertex_count, instanceCount);
}

void mt_draw_mesh(rt_mesh_t& mesh)
{
    mt_draw_mesh_impl(mesh, 1);
}

void mt_draw_mesh_multi(rt_mesh_t& mesh, uint32_t count)
{
    mt_draw_mesh_impl(mesh, std::max(1u, count));
}

rt_meshlet_t mt_create_meshlet(const float* vertices, const float* normals, const float* uvs, size_t vertex_count, const unsigned int* indices, size_t index_count)
{
    rt_meshlet_t result = {};
    if (!vertices || vertex_count == 0) return result;
    uint32_t handle = metal.meshletID + 1;
    auto& native = metal.meshlets[handle];
    native.vertexCount = (uint32_t)vertex_count;
    native.indexCount = (uint32_t)index_count;
    if (vertices)
        result.vertex[0] = mt_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = mt_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = mt_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = mt_create_buffer({.size = index_count * sizeof(uint32_t), .usage = GL_BUFFER_USAGE_STORAGE | GL_BUFFER_USAGE_COPY_DST, .data = indices});
    std::iota(result.location, result.location + std::size(result.location), 0);
    metal.meshletID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void mt_destroy_meshlet(rt_meshlet_t& meshlet)
{
    for (auto& vertex : meshlet.vertex)
        mt_destroy_buffer(vertex);
    mt_destroy_buffer(meshlet.index);
    metal.meshlets.erase(meshlet.handle);
    meshlet.handle = 0;
    meshlet.native = nullptr;
}

void mt_draw_meshlet(rt_meshlet_t& meshlet)
{
    mt_require_pass(GL_MODULE_RENDER);
    rt_module_render_t const& module = metal.currentRenderPass->module;
    uint32_t index_binding = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.type == GL_NONE || layout.count == 0) continue;
        for (uint32_t k = 0; k < std::size(meshlet.vertex); ++k)
        {
            if (meshlet.vertex[k].handle == 0 || meshlet.location[k] != layout.location) continue;
            mt_bind_buffer(meshlet.vertex[k], {.binding = layout.location, .target = GL_SHADER_STORAGE_BUFFER});
            break;
        }
        if (layout.location + 1 > index_binding)
            index_binding = layout.location + 1;
    }
    if (meshlet.index.handle)
        mt_bind_buffer(meshlet.index, {.binding = index_binding, .target = GL_SHADER_STORAGE_BUFFER});
    mt_flush_descriptors();
    auto* native = (rt_meshlet_native_t*)meshlet.native;
    uint32_t tasks = native && native->indexCount ? native->indexCount / 3 : 1;
    metal.renderEncoder->drawMeshThreadgroups(MTL::Size::Make(std::max(1u, tasks), 1, 1), MTL::Size::Make(1, 1, 1), MTL::Size::Make(1, 1, 1));
}

rt_mesh_t mt_create_mesh_screen()
{
    const float points[] = {-1.0f, -1.0f, 0.0f, +3.0f, -1.0f, 0.0f, -1.0f, +3.0f, 0.0f};
    const float uvs[] = {0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 2.0f};
    return mt_create_mesh(points, nullptr, uvs, 3, nullptr, 0);
}

void mt_draw_screen(int width, int height, rt_color_t clear, rt_texture_t& texture)
{
    static auto module = mt_create_module_render({
        .vertex = {rt_vertex_vertex, {}, rt_vertex_uv},
        .binding = {{.binding = 0, .type = GL_BINDING_TEXTURE}},
    });
    if (!module.handle) return;
    rt_pass_render_t pass = {.module = module, .screen = {.color = {.clear = true, .value = clear}}};
    mt_begin_render(pass);
    mt_set_viewport(0, 0, width, height);
    mt_bind_texture(texture, {.binding = 0});
    static auto mesh = mt_create_mesh_screen();
    mt_draw_mesh(mesh);
    mt_end_render(pass);
}

void mt_submit()
{
    if (!metal.cmd) return;
    mt_end_encoder();
    metal.cmd->commit();
    metal.cmd->waitUntilCompleted();
    mt_flush_staging();
    mt_release(metal.cmd);
    mt_retain_cmd(metal.queue->commandBuffer());
}

#endif
#endif

   