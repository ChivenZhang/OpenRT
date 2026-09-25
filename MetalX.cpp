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
#include "MetalX.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
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

static MTLSamplerMinMagFilter gl_to_mt_filter(GLenum filter)
{
    return (filter == GL_NEAREST || filter == GL_NEAREST_MIPMAP_NEAREST || filter == GL_NEAREST_MIPMAP_LINEAR) ?
           MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
}

static MTLSamplerMipFilter gl_to_mt_mip(GLenum minFilter)
{
    if (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST)
        return MTLSamplerMipFilterNearest;
    if (minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR)
        return MTLSamplerMipFilterLinear;
    return MTLSamplerMipFilterNotMipmapped;
}

static MTLSamplerAddressMode gl_to_mt_address(GLenum wrap)
{
    switch (wrap)
    {
        case GL_REPEAT: return MTLSamplerAddressModeRepeat;
        case GL_MIRRORED_REPEAT: return MTLSamplerAddressModeMirrorRepeat;
        case GL_CLAMP_TO_BORDER: return MTLSamplerAddressModeClampToBorderColor;
        case GL_MIRROR_CLAMP_TO_EDGE: return MTLSamplerAddressModeMirrorClampToEdge;
        case GL_CLAMP_TO_EDGE:
        default: return MTLSamplerAddressModeClampToEdge;
    }
}

static bool gl_has_mipmap_filter(GLenum minFilter)
{
    return minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST ||
           minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR;
}

static MTLPixelFormat gl_to_mt_format(GLenum glFormat, GLenum glType, GLenum internalFormat = 0)
{
    switch (internalFormat)
    {
        case GL_R8: return MTLPixelFormatR8Unorm;
        case GL_RG8: return MTLPixelFormatRG8Unorm;
        case GL_RGB8:
        case GL_RGBA8: return MTLPixelFormatRGBA8Unorm;
        case GL_SRGB8_ALPHA8: return MTLPixelFormatRGBA8Unorm_sRGB;
        case GL_R16F: return MTLPixelFormatR16Float;
        case GL_RG16F: return MTLPixelFormatRG16Float;
        case GL_RGBA16F: return MTLPixelFormatRGBA16Float;
        case GL_R32F: return MTLPixelFormatR32Float;
        case GL_RG32F: return MTLPixelFormatRG32Float;
        case GL_RGBA32F: return MTLPixelFormatRGBA32Float;
        case GL_DEPTH_COMPONENT16: return MTLPixelFormatDepth16Unorm;
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32F: return MTLPixelFormatDepth32Float;
        case GL_DEPTH24_STENCIL8: return MTLPixelFormatDepth24Unorm_Stencil8;
        case GL_DEPTH32F_STENCIL8: return MTLPixelFormatDepth32Float_Stencil8;
        default: break;
    }
    switch (glFormat)
    {
        case GL_RED: return (glType == GL_FLOAT) ? MTLPixelFormatR32Float : MTLPixelFormatR8Unorm;
        case GL_RG: return (glType == GL_FLOAT) ? MTLPixelFormatRG32Float : MTLPixelFormatRG8Unorm;
        case GL_RGB:
        case GL_RGBA: return (glType == GL_FLOAT) ? MTLPixelFormatRGBA32Float : MTLPixelFormatRGBA8Unorm;
        case GL_DEPTH_COMPONENT: return MTLPixelFormatDepth32Float;
        case GL_DEPTH_STENCIL: return MTLPixelFormatDepth32Float_Stencil8;
        default: return MTLPixelFormatRGBA8Unorm;
    }
}

static MTLVertexFormat gl_to_mt_vertex_format(GLenum type, GLenum count)
{
    if (type == GL_FLOAT)
    {
        switch (count)
        {
            case 1: return MTLVertexFormatFloat;
            case 2: return MTLVertexFormatFloat2;
            case 3: return MTLVertexFormatFloat3;
            case 4: return MTLVertexFormatFloat4;
            default: return MTLVertexFormatFloat;
        }
    }
    if (type == GL_INT)
    {
        switch (count)
        {
            case 1: return MTLVertexFormatInt;
            case 2: return MTLVertexFormatInt2;
            case 3: return MTLVertexFormatInt3;
            case 4: return MTLVertexFormatInt4;
            default: return MTLVertexFormatInt;
        }
    }
    if (type == GL_UNSIGNED_INT)
    {
        switch (count)
        {
            case 1: return MTLVertexFormatUInt;
            case 2: return MTLVertexFormatUInt2;
            case 3: return MTLVertexFormatUInt3;
            case 4: return MTLVertexFormatUInt4;
            default: return MTLVertexFormatUInt;
        }
    }
    return MTLVertexFormatFloat;
}

static MTLCompareFunction gl_to_mt_compare(GLenum func)
{
    switch (func)
    {
        case GL_NEVER: return MTLCompareFunctionNever;
        case GL_LESS: return MTLCompareFunctionLess;
        case GL_EQUAL: return MTLCompareFunctionEqual;
        case GL_LEQUAL: return MTLCompareFunctionLessEqual;
        case GL_GREATER: return MTLCompareFunctionGreater;
        case GL_NOTEQUAL: return MTLCompareFunctionNotEqual;
        case GL_GEQUAL: return MTLCompareFunctionGreaterEqual;
        case GL_ALWAYS:
        default: return MTLCompareFunctionAlways;
    }
}

static MTLBlendFactor gl_to_mt_blend(GLenum factor)
{
    switch (factor)
    {
        case GL_ZERO: return MTLBlendFactorZero;
        case GL_ONE: return MTLBlendFactorOne;
        case GL_SRC_COLOR: return MTLBlendFactorSourceColor;
        case GL_ONE_MINUS_SRC_COLOR: return MTLBlendFactorOneMinusSourceColor;
        case GL_DST_COLOR: return MTLBlendFactorDestinationColor;
        case GL_ONE_MINUS_DST_COLOR: return MTLBlendFactorOneMinusDestinationColor;
        case GL_SRC_ALPHA: return MTLBlendFactorSourceAlpha;
        case GL_ONE_MINUS_SRC_ALPHA: return MTLBlendFactorOneMinusSourceAlpha;
        case GL_DST_ALPHA: return MTLBlendFactorDestinationAlpha;
        case GL_ONE_MINUS_DST_ALPHA: return MTLBlendFactorOneMinusDestinationAlpha;
        case GL_CONSTANT_COLOR: return MTLBlendFactorBlendColor;
        case GL_ONE_MINUS_CONSTANT_COLOR: return MTLBlendFactorOneMinusBlendColor;
        case GL_SRC_ALPHA_SATURATE: return MTLBlendFactorSourceAlphaSaturated;
        default: return MTLBlendFactorOne;
    }
}

static MTLBlendOperation gl_to_mt_blend_op(GLenum func)
{
    switch (func)
    {
        case GL_FUNC_SUBTRACT: return MTLBlendOperationSubtract;
        case GL_FUNC_REVERSE_SUBTRACT: return MTLBlendOperationReverseSubtract;
        case GL_MIN: return MTLBlendOperationMin;
        case GL_MAX: return MTLBlendOperationMax;
        case GL_FUNC_ADD:
        default: return MTLBlendOperationAdd;
    }
}

static MTLCullMode gl_to_mt_cull(GLenum mode)
{
    switch (mode)
    {
        case GL_FRONT: return MTLCullModeFront;
        case GL_BACK: return MTLCullModeBack;
        default: return MTLCullModeNone;
    }
}

static MTLPrimitiveType gl_to_mt_primitive(GLenum primitive)
{
    switch (primitive)
    {
        case GL_POINTS: return MTLPrimitiveTypePoint;
        case GL_LINES: return MTLPrimitiveTypeLine;
        case GL_LINE_STRIP:
        case GL_LINE_LOOP: return MTLPrimitiveTypeLineStrip;
        case GL_TRIANGLE_STRIP: return MTLPrimitiveTypeTriangleStrip;
        case GL_TRIANGLES:
        default: return MTLPrimitiveTypeTriangle;
    }
}

static MTLStencilOperation gl_to_mt_stencil_op(GLenum op)
{
    switch (op)
    {
        case GL_ZERO: return MTLStencilOperationZero;
        case GL_REPLACE: return MTLStencilOperationReplace;
        case GL_INCR: return MTLStencilOperationIncrementClamp;
        case GL_INCR_WRAP: return MTLStencilOperationIncrementWrap;
        case GL_DECR: return MTLStencilOperationDecrementClamp;
        case GL_DECR_WRAP: return MTLStencilOperationDecrementWrap;
        case GL_INVERT: return MTLStencilOperationInvert;
        case GL_KEEP:
        default: return MTLStencilOperationKeep;
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

static MTLIndexType gl_to_mt_index_type(GLenum type)
{
    return (type == GL_UNSIGNED_SHORT) ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
}

static uint32_t mt_format_bytes(MTLPixelFormat format)
{
    switch (format)
    {
        case MTLPixelFormatR8Unorm: return 1;
        case MTLPixelFormatRG8Unorm:
        case MTLPixelFormatR16Float:
        case MTLPixelFormatDepth16Unorm: return 2;
        case MTLPixelFormatRGBA8Unorm:
        case MTLPixelFormatRGBA8Unorm_sRGB:
        case MTLPixelFormatR32Float:
        case MTLPixelFormatRG16Float:
        case MTLPixelFormatDepth32Float:
        case MTLPixelFormatDepth24Unorm_Stencil8: return 4;
        case MTLPixelFormatRGBA16Float:
        case MTLPixelFormatRG32Float:
        case MTLPixelFormatDepth32Float_Stencil8: return 8;
        case MTLPixelFormatRGBA32Float: return 16;
        default: return 4;
    }
}

static bool mt_is_depth(MTLPixelFormat format)
{
    return format == MTLPixelFormatDepth16Unorm || format == MTLPixelFormatDepth32Float ||
           format == MTLPixelFormatDepth24Unorm_Stencil8 || format == MTLPixelFormatDepth32Float_Stencil8;
}

struct rt_buffer_native_t
{
    id<MTLBuffer> handle = nil;
    mt_res_state_t state = MTL_STATE_UNKNOWN;
    void* mapped = nullptr;
    size_t mappedOffset = 0;
    size_t mappedSize = 0;
};

struct rt_texture_native_t
{
    id<MTLTexture> handle = nil;
    MTLPixelFormat format = MTLPixelFormatInvalid;
    mt_res_state_t state = MTL_STATE_UNKNOWN;
    uint32_t mipLevels = 1;
    uint32_t layers = 1;
    uint32_t width = 1, height = 1, depth = 1;
    GLenum target = GL_TEXTURE_2D;
};

struct rt_sampler_native_t
{
    id<MTLSamplerState> handle = nil;
};

struct rt_module_native_t
{
    id<MTLLibrary> vlib = nil, tlib = nil, mlib = nil, flib = nil, clib = nil;
    id<MTLFunction> vfn = nil, tfn = nil, mfn = nil, ffn = nil, cfn = nil;
    id<MTLRenderPipelineState> renderPipeline = nil;
    id<MTLComputePipelineState> computePipeline = nil;
    id<MTLDepthStencilState> depthStencil = nil;
    bool isMeshlet = false;
    bool isCompute = false;
    MTLPrimitiveType primitive = MTLPrimitiveTypeTriangle;
    MTLCullMode cull = MTLCullModeBack;
    MTLWinding winding = MTLWindingCounterClockwise;
    rt_binding_t bindings[GL_MAX_BINDING_HANDLE_NUM] = {};
};

struct rt_mesh_native_t { uint32_t vertexCount = 0, indexCount = 0; };
struct rt_meshlet_native_t { uint32_t vertexCount = 0, indexCount = 0; };
struct rt_pass_compute_native_t { uint32_t dummy = 0; };
struct rt_pass_render_native_t { bool offscreen = false; uint32_t width = 0, height = 0; };
struct rt_pass_transfer_native_t { uint32_t dummy = 0; };

struct mt_staging_t
{
    id<MTLBuffer> buffer = nil;
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

    id<MTLDevice> device = nil;
    id<MTLCommandQueue> queue = nil;
    id<MTLCommandBuffer> cmd = nil;
    id<MTLRenderCommandEncoder> renderEncoder = nil;
    id<MTLComputeCommandEncoder> computeEncoder = nil;
    id<MTLBlitCommandEncoder> blitEncoder = nil;
    id<MTLSamplerState> defaultSampler = nil;
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
    if (metal.renderEncoder) { [metal.renderEncoder endEncoding]; metal.renderEncoder = nil; }
    if (metal.computeEncoder) { [metal.computeEncoder endEncoding]; metal.computeEncoder = nil; }
    if (metal.blitEncoder) { [metal.blitEncoder endEncoding]; metal.blitEncoder = nil; }
}

static void mt_ensure_blit()
{
    if (metal.blitEncoder) return;
    mt_end_encoder();
    metal.blitEncoder = [metal.cmd blitCommandEncoder];
}

static void mt_flush_staging()
{
    metal.pendingStaging.clear();
}

static bool mt_create_staging(size_t size, mt_staging_t& staging, void** mapped)
{
    staging.buffer = [metal.device newBufferWithLength:size options:MTLResourceStorageModeShared];
    if (!staging.buffer) return false;
    if (mapped) *mapped = [staging.buffer contents];
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

static id<MTLFunction> mt_function_from_binary(id<MTLLibrary> library)
{
    if (!library) return nil;
    id<MTLFunction> fn = [library newFunctionWithName:@"main"];
    if (fn) return fn;
    NSArray<NSString*>* names = [library functionNames];
    if (names.count) return [library newFunctionWithName:names[0]];
    return nil;
}

static id<MTLLibrary> mt_create_library(const char* data, uint32_t length)
{
    if (!data || !length || !metal.device) return nil;
    NSError* error = nil;
    bool text = data[0] != 0 && (unsigned char)data[0] < 0x80 && data[0] != '\0';
    for (uint32_t i = 0; i < std::min(length, 8u) && text; ++i)
        if ((unsigned char)data[i] < 9 && data[i] != '\n' && data[i] != '\r' && data[i] != '\t')
            text = false;
    if (text)
    {
        NSString* source = [[NSString alloc] initWithBytes:data length:length encoding:NSUTF8StringEncoding];
        return [metal.device newLibraryWithSource:source options:nil error:&error];
    }
    dispatch_data_t blob = dispatch_data_create(data, length, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    return [metal.device newLibraryWithData:blob error:&error];
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

static bool mt_create_graphics_pipeline(rt_module_native_t& native, rt_module_render_info_t const& info, bool meshlet)
{
    MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = native.vfn;
    desc.fragmentFunction = native.ffn;
    if (!meshlet)
    {
        MTLVertexDescriptor* vd = [[MTLVertexDescriptor alloc] init];
        for (uint32_t i = 0; i < GL_MAX_VERTEX_BUFFER_NUM; ++i)
        {
            if (info.vertex[i].type == GL_NONE || info.vertex[i].count == 0) continue;
            uint32_t loc = info.vertex[i].location;
            vd.attributes[loc].format = gl_to_mt_vertex_format(info.vertex[i].type, info.vertex[i].count);
            vd.attributes[loc].offset = 0;
            vd.attributes[loc].bufferIndex = loc;
            vd.layouts[loc].stride = gl_vertex_size(info.vertex[i].type, info.vertex[i].count);
            vd.layouts[loc].stepFunction = info.vertex[i].instance ? MTLVertexStepFunctionPerInstance : MTLVertexStepFunctionPerVertex;
            vd.layouts[loc].stepRate = 1;
        }
        desc.vertexDescriptor = vd;
    }
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        desc.colorAttachments[i].pixelFormat = MTLPixelFormatRGBA8Unorm;
        bool blend =
            (info.colors[i].color.func != GL_FUNC_ADD || info.colors[i].color.src != GL_ONE ||
             info.colors[i].color.dst != GL_ZERO || info.colors[i].alpha.func != GL_FUNC_ADD ||
             info.colors[i].alpha.src != GL_ONE || info.colors[i].alpha.dst != GL_ZERO);
        desc.colorAttachments[i].blendingEnabled = blend;
        desc.colorAttachments[i].sourceRGBBlendFactor = gl_to_mt_blend(info.colors[i].color.src);
        desc.colorAttachments[i].destinationRGBBlendFactor = gl_to_mt_blend(info.colors[i].color.dst);
        desc.colorAttachments[i].rgbBlendOperation = gl_to_mt_blend_op(info.colors[i].color.func);
        desc.colorAttachments[i].sourceAlphaBlendFactor = gl_to_mt_blend(info.colors[i].alpha.src);
        desc.colorAttachments[i].destinationAlphaBlendFactor = gl_to_mt_blend(info.colors[i].alpha.dst);
        desc.colorAttachments[i].alphaBlendOperation = gl_to_mt_blend_op(info.colors[i].alpha.func);
    }
    const bool depthEnabled = (info.depth.func != GL_ALWAYS || info.depth.write);
    const bool stencilEnabled =
        (info.stencil.back.func != GL_ALWAYS || info.stencil.back.sfail != GL_KEEP ||
         info.stencil.back.zfail != GL_KEEP || info.stencil.back.zpass != GL_KEEP ||
         info.stencil.front.func != GL_ALWAYS || info.stencil.front.sfail != GL_KEEP ||
         info.stencil.front.zfail != GL_KEEP || info.stencil.front.zpass != GL_KEEP);
    if (stencilEnabled) desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    else if (depthEnabled) desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

    NSError* error = nil;
    native.renderPipeline = [metal.device newRenderPipelineStateWithDescriptor:desc error:&error];
    if (!native.renderPipeline) return false;

    MTLDepthStencilDescriptor* ds = [[MTLDepthStencilDescriptor alloc] init];
    ds.depthCompareFunction = gl_to_mt_compare(info.depth.func);
    ds.depthWriteEnabled = info.depth.write;
    if (stencilEnabled)
    {
        MTLStencilDescriptor* front = [[MTLStencilDescriptor alloc] init];
        front.stencilCompareFunction = gl_to_mt_compare(info.stencil.front.func);
        front.stencilFailureOperation = gl_to_mt_stencil_op(info.stencil.front.sfail);
        front.depthFailureOperation = gl_to_mt_stencil_op(info.stencil.front.zfail);
        front.depthStencilPassOperation = gl_to_mt_stencil_op(info.stencil.front.zpass);
        front.readMask = info.stencil.read;
        front.writeMask = info.stencil.write;
        MTLStencilDescriptor* back = [[MTLStencilDescriptor alloc] init];
        back.stencilCompareFunction = gl_to_mt_compare(info.stencil.back.func);
        back.stencilFailureOperation = gl_to_mt_stencil_op(info.stencil.back.sfail);
        back.depthFailureOperation = gl_to_mt_stencil_op(info.stencil.back.zfail);
        back.depthStencilPassOperation = gl_to_mt_stencil_op(info.stencil.back.zpass);
        back.readMask = info.stencil.read;
        back.writeMask = info.stencil.write;
        ds.frontFaceStencil = front;
        ds.backFaceStencil = back;
    }
    native.depthStencil = [metal.device newDepthStencilStateWithDescriptor:ds];
    native.primitive = gl_to_mt_primitive(info.primitive);
    native.cull = gl_to_mt_cull(info.cull_mode);
    native.winding = (info.front_face == GL_CW) ? MTLWindingClockwise : MTLWindingCounterClockwise;
    for (uint32_t i = 0; i < GL_MAX_BINDING_HANDLE_NUM; ++i)
        native.bindings[i] = info.binding[i];
    return true;
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
        [metal.renderEncoder setVertexBytes:metal.pushData length:metal.pushLength atIndex:16];
        [metal.renderEncoder setFragmentBytes:metal.pushData length:metal.pushLength atIndex:16];
    }
    if (metal.computeEncoder)
        [metal.computeEncoder setBytes:metal.pushData length:metal.pushLength atIndex:16];
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
                [metal.renderEncoder setVertexBuffer:buf->handle offset:0 atIndex:slot.buffer_bind.binding];
                [metal.renderEncoder setFragmentBuffer:buf->handle offset:0 atIndex:slot.buffer_bind.binding];
            }
            if (metal.computeEncoder)
                [metal.computeEncoder setBuffer:buf->handle offset:0 atIndex:slot.buffer_bind.binding];
        }
        else if (slot.type == GL_BINDING_TEXTURE)
        {
            auto* tex = mt_texture_native(slot.texture);
            if (!tex) continue;
            mt_transition_image(*tex, MTL_STATE_SHADER_READ);
            id<MTLSamplerState> samp = metal.defaultSampler;
            if (auto* s = mt_sampler_native(slot.sampler))
                samp = s->handle;
            if (metal.renderEncoder)
            {
                [metal.renderEncoder setFragmentTexture:tex->handle atIndex:slot.texture_bind.binding];
                [metal.renderEncoder setFragmentSamplerState:samp atIndex:slot.texture_bind.binding];
            }
            if (metal.computeEncoder)
            {
                [metal.computeEncoder setTexture:tex->handle atIndex:slot.texture_bind.binding];
                [metal.computeEncoder setSamplerState:samp atIndex:slot.texture_bind.binding];
            }
        }
        else if (slot.type == GL_BINDING_STORAGE_TEXTURE)
        {
            auto* tex = mt_texture_native(slot.storage_texture);
            if (!tex) continue;
            mt_transition_image(*tex, MTL_STATE_SHADER_WRITE);
            if (metal.renderEncoder)
                [metal.renderEncoder setFragmentTexture:tex->handle atIndex:slot.storage_texture_bind.binding];
            if (metal.computeEncoder)
                [metal.computeEncoder setTexture:tex->handle atIndex:slot.storage_texture_bind.binding];
        }
        else if (slot.type == GL_BINDING_SAMPLER)
        {
            auto* samp = mt_sampler_native(slot.sampler);
            if (!samp) continue;
            if (metal.renderEncoder)
                [metal.renderEncoder setFragmentSamplerState:samp->handle atIndex:slot.sampler_bind.binding];
            if (metal.computeEncoder)
                [metal.computeEncoder setSamplerState:samp->handle atIndex:slot.sampler_bind.binding];
        }
    }
}

static void mt_bind_api()
{
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

static void mt_unbind_api()
{
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

void mt_load_library(id<MTLDevice> device, id<MTLCommandQueue> queue)
{
    metal.device = device;
    metal.queue = queue;
    if (!metal.device)
    {
        fprintf(stderr, "Metal: device is null\n");
        abort();
    }
    if (!metal.queue)
        metal.queue = [metal.device newCommandQueue];
    metal.cmd = [metal.queue commandBuffer];
    MTLSamplerDescriptor* samp = [[MTLSamplerDescriptor alloc] init];
    samp.minFilter = MTLSamplerMinMagFilterLinear;
    samp.magFilter = MTLSamplerMinMagFilterLinear;
    samp.sAddressMode = samp.tAddressMode = samp.rAddressMode = MTLSamplerAddressModeRepeat;
    metal.defaultSampler = [metal.device newSamplerStateWithDescriptor:samp];
    mt_bind_api();
}

void mt_unload_library()
{
    mt_end_encoder();
    metal.cmd = nil;
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
    metal.defaultSampler = nil;
    metal.queue = nil;
    metal.device = nil;
    metal.bufferID = metal.textureID = metal.samplerID = metal.moduleID = 0;
    metal.meshID = metal.meshletID = metal.passID = 0;
    metal.currentPassType = GL_NONE;
    metal.currentPipeline = nullptr;
    mt_unbind_api();
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
    MTLResourceOptions options = hostVisible ? MTLResourceStorageModeShared : MTLResourceStorageModePrivate;
    if (info.data && hostVisible)
        native.handle = [metal.device newBufferWithBytes:info.data length:info.size options:options];
    else
        native.handle = [metal.device newBufferWithLength:info.size options:options];
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
            [metal.blitEncoder copyFromBuffer:staging.buffer sourceOffset:0 toBuffer:native.handle destinationOffset:0 size:info.size];
            metal.pendingStaging.push_back(staging);
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
    void* base = [native->handle contents];
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

    MTLTextureDescriptor* desc = [[MTLTextureDescriptor alloc] init];
    desc.pixelFormat = native.format;
    desc.width = native.width;
    desc.height = native.height;
    desc.mipmapLevelCount = native.mipLevels;
    desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite | MTLTextureUsageRenderTarget;
    desc.storageMode = MTLStorageModePrivate;
    if (info.target == GL_TEXTURE_1D) desc.textureType = MTLTextureType1D;
    else if (info.target == GL_TEXTURE_3D) { desc.textureType = MTLTextureType3D; desc.depth = native.depth; }
    else if (info.target == GL_TEXTURE_2D_ARRAY) { desc.textureType = MTLTextureType2DArray; desc.arrayLength = native.layers; }
    else if (info.target == GL_TEXTURE_2D_MULTISAMPLE) { desc.textureType = MTLTextureType2DMultisample; desc.sampleCount = info.samples ? info.samples : 1; }
    else desc.textureType = MTLTextureType2D;
    native.handle = [metal.device newTextureWithDescriptor:desc];
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
            MTLSize size = {native.width, native.height, (info.target == GL_TEXTURE_3D) ? native.depth : 1};
            [metal.blitEncoder copyFromBuffer:staging.buffer sourceOffset:0 sourceBytesPerRow:native.width * bpp
                sourceBytesPerImage:(NSUInteger)bytes sourceSize:size toTexture:native.handle destinationSlice:0
                destinationLevel:0 destinationOrigin:{}];
            metal.pendingStaging.push_back(staging);
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
    MTLSamplerDescriptor* desc = [[MTLSamplerDescriptor alloc] init];
    desc.minFilter = gl_to_mt_filter(info.min_filter);
    desc.magFilter = gl_to_mt_filter(info.mag_filter);
    desc.mipFilter = gl_to_mt_mip(info.min_filter);
    desc.sAddressMode = gl_to_mt_address(info.wrap_s);
    desc.tAddressMode = gl_to_mt_address(info.wrap_t);
    desc.rAddressMode = gl_to_mt_address(info.wrap_r);
    native.handle = [metal.device newSamplerStateWithDescriptor:desc];
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
    native.isCompute = true;
    native.clib = mt_create_library(info.cshader, info.clength);
    native.cfn = mt_function_from_binary(native.clib);
    if (!native.cfn)
    {
        metal.modules.erase(handle);
        return {};
    }
    NSError* error = nil;
    native.computePipeline = [metal.device newComputePipelineStateWithFunction:native.cfn error:&error];
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
    native.isMeshlet = true;
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
    metal.computeEncoder = [metal.cmd computeCommandEncoder];
    auto* mod = (rt_module_native_t*)pass.module.native;
    if (mod && mod->computePipeline)
        [metal.computeEncoder setComputePipelineState:mod->computePipeline];
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
        [metal.computeEncoder endEncoding];
        metal.computeEncoder = nil;
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
    MTLSize groups = {std::max(1u, groupX), std::max(1u, groupY), std::max(1u, groupZ)};
    MTLSize threads = {1, 1, 1};
    auto* mod = mt_current_module_native();
    if (mod && mod->computePipeline)
    {
        NSUInteger w = mod->computePipeline.threadExecutionWidth;
        threads = {w, 1, 1};
    }
    [metal.computeEncoder dispatchThreadgroups:groups threadsPerThreadgroup:threads];
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

    MTLRenderPassDescriptor* desc = [MTLRenderPassDescriptor renderPassDescriptor];
    uint32_t width = 0, height = 0;
    for (uint32_t i = 0; i < GL_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        if (auto* tex = mt_texture_native(pass.colors[i].texture))
        {
            mt_transition_image(*tex, MTL_STATE_COLOR);
            desc.colorAttachments[i].texture = tex->handle;
            desc.colorAttachments[i].loadAction = pass.colors[i].clear ? MTLLoadActionClear : MTLLoadActionLoad;
            desc.colorAttachments[i].storeAction = MTLStoreActionStore;
            desc.colorAttachments[i].clearColor = MTLClearColorMake(pass.colors[i].value.r, pass.colors[i].value.g, pass.colors[i].value.b, pass.colors[i].value.a);
            width = std::max(width, pass.colors[i].texture.width);
            height = std::max(height, pass.colors[i].texture.height);
        }
    }
    if (auto* tex = mt_texture_native(pass.depth.texture))
    {
        mt_transition_image(*tex, MTL_STATE_DEPTH);
        desc.depthAttachment.texture = tex->handle;
        desc.depthAttachment.loadAction = pass.depth.clear ? MTLLoadActionClear : MTLLoadActionLoad;
        desc.depthAttachment.storeAction = MTLStoreActionStore;
        desc.depthAttachment.clearDepth = pass.depth.value;
        if (pass.depth.texture.format == GL_DEPTH_STENCIL)
        {
            desc.stencilAttachment.texture = tex->handle;
            desc.stencilAttachment.loadAction = pass.stencil.clear ? MTLLoadActionClear : MTLLoadActionLoad;
            desc.stencilAttachment.storeAction = MTLStoreActionStore;
            desc.stencilAttachment.clearStencil = (uint32_t)pass.stencil.value;
        }
        width = std::max(width, pass.depth.texture.width);
        height = std::max(height, pass.depth.texture.height);
    }
    native.width = width;
    native.height = height;
    mt_end_encoder();
    metal.renderEncoder = [metal.cmd renderCommandEncoderWithDescriptor:desc];
    auto* mod = (rt_module_native_t*)pass.module.native;
    if (mod && mod->renderPipeline)
        [metal.renderEncoder setRenderPipelineState:mod->renderPipeline];
    if (mod && mod->depthStencil)
        [metal.renderEncoder setDepthStencilState:mod->depthStencil];
    if (mod)
    {
        [metal.renderEncoder setCullMode:mod->cull];
        [metal.renderEncoder setFrontFacingWinding:mod->winding];
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
        [metal.renderEncoder endEncoding];
        metal.renderEncoder = nil;
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
    MTLViewport vp = {(double)x, (double)y, (double)width, (double)height, 0.0, 1.0};
    [metal.renderEncoder setViewport:vp];
}

void mt_set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (metal.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (!metal.renderEncoder) return;
    MTLScissorRect rect = {(NSUInteger)std::max(0, x), (NSUInteger)std::max(0, y), (NSUInteger)std::max(0, width), (NSUInteger)std::max(0, height)};
    [metal.renderEncoder setScissorRect:rect];
}

void mt_draw_mesh_task(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    mt_require_pass(GL_MODULE_RENDER);
    mt_flush_descriptors();
    if ([metal.renderEncoder respondsToSelector:@selector(drawMeshThreadgroups:threadsPerObjectThreadgroup:threadsPerMeshThreadgroup:)])
    {
        MTLSize groups = {std::max(1u, groupX), std::max(1u, groupY), std::max(1u, groupZ)};
        [metal.renderEncoder drawMeshThreadgroups:groups threadsPerObjectThreadgroup:{1, 1, 1} threadsPerMeshThreadgroup:{1, 1, 1}];
    }
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
        [metal.blitEncoder endEncoding];
        metal.blitEncoder = nil;
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
    [metal.blitEncoder copyFromBuffer:src->handle sourceOffset:source.offset toBuffer:dst->handle destinationOffset:destination.offset size:copySize];
}

void mt_copy_buffer_data(rt_buffer_data_t source, rt_buffer_copy_t destination, size_t copySize)
{
    mt_require_pass(GL_MODULE_TRANSFER);
    auto* dst = mt_buffer_native(destination.buffer);
    if (!source.data || !dst || copySize == 0) return;
    if (source.offset + copySize > source.size || destination.offset + copySize > destination.buffer.size)
        return;
    if (void* mapped = [dst->handle contents])
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
    [metal.blitEncoder copyFromBuffer:staging.buffer sourceOffset:0 toBuffer:dst->handle destinationOffset:destination.offset size:copySize];
    metal.pendingStaging.push_back(staging);
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
    [metal.blitEncoder copyFromTexture:src->handle sourceSlice:0 sourceLevel:source.mipLevel
        sourceOrigin:{source.origin.x, source.origin.y, source.origin.z}
        sourceSize:{copySize.x, copySize.y, copySize.z ? copySize.z : 1}
        toBuffer:dst->handle destinationOffset:destination.offset
        destinationBytesPerRow:destination.bytesPerRow ? destination.bytesPerRow : copySize.x * bpp
        destinationBytesPerImage:destination.rowsPerImage ? destination.rowsPerImage * (destination.bytesPerRow ? destination.bytesPerRow : copySize.x * bpp) : 0];
}

void mt_copy_texture(rt_texture_copy_t source, rt_texture_copy_t destination, rt_size_t copySize)
{
    mt_require_pass(GL_MODULE_TRANSFER);
    auto* src = mt_texture_native(source.texture);
    auto* dst = mt_texture_native(destination.texture);
    if (!src || !dst || copySize.x == 0 || copySize.y == 0) return;
    mt_transition_image(*src, MTL_STATE_COPY_SRC);
    mt_transition_image(*dst, MTL_STATE_COPY_DST);
    [metal.blitEncoder copyFromTexture:src->handle sourceSlice:0 sourceLevel:source.mipLevel
        sourceOrigin:{source.origin.x, source.origin.y, source.origin.z}
        sourceSize:{copySize.x, copySize.y, copySize.z ? copySize.z : 1}
        toTexture:dst->handle destinationSlice:0 destinationLevel:destination.mipLevel
        destinationOrigin:{destination.origin.x, destination.origin.y, destination.origin.z}];
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
    [metal.blitEncoder copyFromBuffer:staging.buffer sourceOffset:0
        sourceBytesPerRow:source.bytesPerRow ? source.bytesPerRow : copySize.x * bpp
        sourceBytesPerImage:source.rowsPerImage ? source.rowsPerImage * (source.bytesPerRow ? source.bytesPerRow : copySize.x * bpp) : (NSUInteger)bytes
        sourceSize:{copySize.x, copySize.y, copySize.z ? copySize.z : 1}
        toTexture:dst->handle destinationSlice:0 destinationLevel:destination.mipLevel
        destinationOrigin:{destination.origin.x, destination.origin.y, destination.origin.z}];
    metal.pendingStaging.push_back(staging);
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
    [metal.blitEncoder copyFromBuffer:src->handle sourceOffset:source.offset
        sourceBytesPerRow:source.bytesPerRow ? source.bytesPerRow : copySize.x * bpp
        sourceBytesPerImage:source.rowsPerImage ? source.rowsPerImage * (source.bytesPerRow ? source.bytesPerRow : copySize.x * bpp) : 0
        sourceSize:{copySize.x, copySize.y, copySize.z ? copySize.z : 1}
        toTexture:dst->handle destinationSlice:0 destinationLevel:destination.mipLevel
        destinationOrigin:{destination.origin.x, destination.origin.y, destination.origin.z}];
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
            [metal.renderEncoder setVertexBuffer:native->handle offset:0 atIndex:layout.location];
            uint32_t stride = gl_vertex_size(layout.type, layout.count);
            if (vertex_count == 0 && stride)
                vertex_count = (uint32_t)(mesh.vertex[k].size / stride);
            break;
        }
    }
    MTLPrimitiveType primitive = mod ? mod->primitive : MTLPrimitiveTypeTriangle;
    if (mesh.index.handle)
    {
        auto* native = mt_buffer_native(mesh.index);
        if (!native) return;
        uint32_t indexStride = gl_index_type_size(module.index_type);
        mt_transition_buffer(*native, MTL_STATE_INDEX);
        [metal.renderEncoder drawIndexedPrimitives:primitive indexCount:(NSUInteger)(mesh.index.size / indexStride)
            indexType:gl_to_mt_index_type(module.index_type) indexBuffer:native->handle indexBufferOffset:0 instanceCount:instanceCount];
    }
    else
        [metal.renderEncoder drawPrimitives:primitive vertexStart:0 vertexCount:vertex_count instanceCount:instanceCount];
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
    if ([metal.renderEncoder respondsToSelector:@selector(drawMeshThreadgroups:threadsPerObjectThreadgroup:threadsPerMeshThreadgroup:)])
        [metal.renderEncoder drawMeshThreadgroups:{std::max(1u, tasks), 1, 1} threadsPerObjectThreadgroup:{1, 1, 1} threadsPerMeshThreadgroup:{1, 1, 1}];
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
    [metal.cmd commit];
    [metal.cmd waitUntilCompleted];
    mt_flush_staging();
    metal.cmd = [metal.queue commandBuffer];
}

#endif
#endif
