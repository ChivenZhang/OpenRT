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

static D3D12_FILTER rt_to_dx_filter(rt_filter_t minFilter, rt_filter_t magFilter)
{
    switch (minFilter)
    {
        case RT_NEAREST:
            switch (magFilter)
            {
                case RT_NEAREST: return D3D12_FILTER_MIN_MAG_MIP_POINT;
                default: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            }
        case RT_LINEAR:
            switch (magFilter)
            {
                case RT_NEAREST: return D3D12_FILTER_MIN_MAG_MIP_POINT;
                default: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            }
        case RT_NEAREST_MIPMAP_NEAREST:
            switch (magFilter)
            {
                case RT_NEAREST: return D3D12_FILTER_MIN_MAG_MIP_POINT;
                default: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            }
        case RT_LINEAR_MIPMAP_NEAREST: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        case RT_NEAREST_MIPMAP_LINEAR: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        case RT_LINEAR_MIPMAP_LINEAR: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        default: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    }
}

static D3D12_TEXTURE_ADDRESS_MODE rt_to_dx_address(rt_address_t address)
{
    switch (address)
    {
        case RT_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case RT_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case RT_CLAMP_TO_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case RT_MIRRORED_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case RT_MIRROR_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        default: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
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

static DXGI_FORMAT rt_to_dx_texture_format(rt_texture_format_t format)
{
    switch (format)
    {
        case RT_TEXTURE_NONE: return DXGI_FORMAT_UNKNOWN;
        case RT_TEXTURE_R8UNORM: return DXGI_FORMAT_R8_UNORM;
        case RT_TEXTURE_R8SNORM: return DXGI_FORMAT_R8_SNORM;
        case RT_TEXTURE_R8UINT: return DXGI_FORMAT_R8_UINT;
        case RT_TEXTURE_R8SINT: return DXGI_FORMAT_R8_SINT;
        case RT_TEXTURE_R16UNORM: return DXGI_FORMAT_R16_UNORM;
        case RT_TEXTURE_R16SNORM: return DXGI_FORMAT_R16_SNORM;
        case RT_TEXTURE_R16UINT: return DXGI_FORMAT_R16_UINT;
        case RT_TEXTURE_R16SINT: return DXGI_FORMAT_R16_SINT;
        case RT_TEXTURE_R16FLOAT: return DXGI_FORMAT_R16_FLOAT;
        case RT_TEXTURE_RG8UNORM: return DXGI_FORMAT_R8G8_UNORM;
        case RT_TEXTURE_RG8SNORM: return DXGI_FORMAT_R8G8_SNORM;
        case RT_TEXTURE_RG8UINT: return DXGI_FORMAT_R8G8_UINT;
        case RT_TEXTURE_RG8SINT: return DXGI_FORMAT_R8G8_SINT;
        case RT_TEXTURE_R32UINT: return DXGI_FORMAT_R32_UINT;
        case RT_TEXTURE_R32SINT: return DXGI_FORMAT_R32_SINT;
        case RT_TEXTURE_R32FLOAT: return DXGI_FORMAT_R32_FLOAT;
        case RT_TEXTURE_RG16UNORM: return DXGI_FORMAT_R16G16_UNORM;
        case RT_TEXTURE_RG16SNORM: return DXGI_FORMAT_R16G16_SNORM;
        case RT_TEXTURE_RG16UINT: return DXGI_FORMAT_R16G16_UINT;
        case RT_TEXTURE_RG16SINT: return DXGI_FORMAT_R16G16_SINT;
        case RT_TEXTURE_RG16FLOAT: return DXGI_FORMAT_R16G16_FLOAT;
        case RT_TEXTURE_RGBA8UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case RT_TEXTURE_RGBA8UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case RT_TEXTURE_RGBA8SNORM: return DXGI_FORMAT_R8G8B8A8_SNORM;
        case RT_TEXTURE_RGBA8UINT: return DXGI_FORMAT_R8G8B8A8_UINT;
        case RT_TEXTURE_RGBA8SINT: return DXGI_FORMAT_R8G8B8A8_SINT;
        case RT_TEXTURE_BGRA8UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM;
        case RT_TEXTURE_BGRA8UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case RT_TEXTURE_RGB10A2UINT: return DXGI_FORMAT_R10G10B10A2_UINT;
        case RT_TEXTURE_RGB10A2UNORM: return DXGI_FORMAT_R10G10B10A2_UNORM;
        case RT_TEXTURE_RG11B10UFLOAT: return DXGI_FORMAT_R11G11B10_FLOAT;
        case RT_TEXTURE_RGB9E5UFLOAT: return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        case RT_TEXTURE_RG32UINT: return DXGI_FORMAT_R32G32_UINT;
        case RT_TEXTURE_RG32SINT: return DXGI_FORMAT_R32G32_SINT;
        case RT_TEXTURE_RG32FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
        case RT_TEXTURE_RGBA16UNORM: return DXGI_FORMAT_R16G16B16A16_UNORM;
        case RT_TEXTURE_RGBA16SNORM: return DXGI_FORMAT_R16G16B16A16_SNORM;
        case RT_TEXTURE_RGBA16UINT: return DXGI_FORMAT_R16G16B16A16_UINT;
        case RT_TEXTURE_RGBA16SINT: return DXGI_FORMAT_R16G16B16A16_SINT;
        case RT_TEXTURE_RGBA16FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case RT_TEXTURE_RGBA32UINT: return DXGI_FORMAT_R32G32B32A32_UINT;
        case RT_TEXTURE_RGBA32SINT: return DXGI_FORMAT_R32G32B32A32_SINT;
        case RT_TEXTURE_RGBA32FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case RT_TEXTURE_STENCIL8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case RT_TEXTURE_DEPTH16UNORM: return DXGI_FORMAT_D16_UNORM;
        case RT_TEXTURE_DEPTH24PLUS: return DXGI_FORMAT_D32_FLOAT;
        case RT_TEXTURE_DEPTH24PLUS_STENCIL8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case RT_TEXTURE_DEPTH32FLOAT: return DXGI_FORMAT_D32_FLOAT;
        case RT_TEXTURE_DEPTH32FLOAT_STENCIL8: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        default: return DXGI_FORMAT_UNKNOWN;
    }
}

static UINT rt_to_dx_sample_count(rt_texture_sample_t samples)
{
    switch (samples)
    {
        case RT_TEXTURE_SAMPLE_1X: return 1;
        case RT_TEXTURE_SAMPLE_4X: return 4;
        default: return 1;
    }
}

static DXGI_FORMAT rt_to_dx_vertex_format(rt_vertex_format_t format)
{
    switch (format)
    {
        case RT_VERTEX_NONE: return DXGI_FORMAT_UNKNOWN;
        case RT_VERTEX_UINT8: return DXGI_FORMAT_R8_UINT;
        case RT_VERTEX_UINT8X2: return DXGI_FORMAT_R8G8_UINT;
        case RT_VERTEX_UINT8X4: return DXGI_FORMAT_R8G8B8A8_UINT;
        case RT_VERTEX_SINT8: return DXGI_FORMAT_R8_SINT;
        case RT_VERTEX_SINT8X2: return DXGI_FORMAT_R8G8_SINT;
        case RT_VERTEX_SINT8X4: return DXGI_FORMAT_R8G8B8A8_SINT;
        case RT_VERTEX_UNORM8: return DXGI_FORMAT_R8_UNORM;
        case RT_VERTEX_UNORM8X2: return DXGI_FORMAT_R8G8_UNORM;
        case RT_VERTEX_UNORM8X4: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case RT_VERTEX_SNORM8: return DXGI_FORMAT_R8_SNORM;
        case RT_VERTEX_SNORM8X2: return DXGI_FORMAT_R8G8_SNORM;
        case RT_VERTEX_SNORM8X4: return DXGI_FORMAT_R8G8B8A8_SNORM;
        case RT_VERTEX_UINT16: return DXGI_FORMAT_R16_UINT;
        case RT_VERTEX_UINT16X2: return DXGI_FORMAT_R16G16_UINT;
        case RT_VERTEX_UINT16X4: return DXGI_FORMAT_R16G16B16A16_UINT;
        case RT_VERTEX_SINT16: return DXGI_FORMAT_R16_SINT;
        case RT_VERTEX_SINT16X2: return DXGI_FORMAT_R16G16_SINT;
        case RT_VERTEX_SINT16X4: return DXGI_FORMAT_R16G16B16A16_SINT;
        case RT_VERTEX_UNORM16: return DXGI_FORMAT_R16_UNORM;
        case RT_VERTEX_UNORM16X2: return DXGI_FORMAT_R16G16_UNORM;
        case RT_VERTEX_UNORM16X4: return DXGI_FORMAT_R16G16B16A16_UNORM;
        case RT_VERTEX_SNORM16: return DXGI_FORMAT_R16_SNORM;
        case RT_VERTEX_SNORM16X2: return DXGI_FORMAT_R16G16_SNORM;
        case RT_VERTEX_SNORM16X4: return DXGI_FORMAT_R16G16B16A16_SNORM;
        case RT_VERTEX_FLOAT16: return DXGI_FORMAT_R16_FLOAT;
        case RT_VERTEX_FLOAT16X2: return DXGI_FORMAT_R16G16_FLOAT;
        case RT_VERTEX_FLOAT16X4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case RT_VERTEX_FLOAT32: return DXGI_FORMAT_R32_FLOAT;
        case RT_VERTEX_FLOAT32X2: return DXGI_FORMAT_R32G32_FLOAT;
        case RT_VERTEX_FLOAT32X3: return DXGI_FORMAT_R32G32B32_FLOAT;
        case RT_VERTEX_FLOAT32X4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case RT_VERTEX_UINT32: return DXGI_FORMAT_R32_UINT;
        case RT_VERTEX_UINT32X2: return DXGI_FORMAT_R32G32_UINT;
        case RT_VERTEX_UINT32X3: return DXGI_FORMAT_R32G32B32_UINT;
        case RT_VERTEX_UINT32X4: return DXGI_FORMAT_R32G32B32A32_UINT;
        case RT_VERTEX_SINT32: return DXGI_FORMAT_R32_SINT;
        case RT_VERTEX_SINT32X2: return DXGI_FORMAT_R32G32_SINT;
        case RT_VERTEX_SINT32X3: return DXGI_FORMAT_R32G32B32_SINT;
        case RT_VERTEX_SINT32X4: return DXGI_FORMAT_R32G32B32A32_SINT;
        default: return DXGI_FORMAT_UNKNOWN;
    }
}

static D3D12_COMPARISON_FUNC rt_to_dx_compare(rt_compare_op_t func)
{
    switch (func)
    {
        case RT_NEVER: return D3D12_COMPARISON_FUNC_NEVER;
        case RT_LESS: return D3D12_COMPARISON_FUNC_LESS;
        case RT_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
        case RT_LEQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case RT_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
        case RT_NOTEQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case RT_GEQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case RT_ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
        default: return D3D12_COMPARISON_FUNC_ALWAYS;
    }
}

static D3D12_BLEND rt_to_dx_blend(rt_blend_factor_t factor)
{
    switch (factor)
    {
        case RT_BLEND_ZERO: return D3D12_BLEND_ZERO;
        case RT_BLEND_ONE: return D3D12_BLEND_ONE;
        case RT_BLEND_SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
        case RT_BLEND_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
        case RT_BLEND_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
        case RT_BLEND_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
        case RT_BLEND_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
        case RT_BLEND_ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
        case RT_BLEND_DST_COLOR: return D3D12_BLEND_DEST_COLOR;
        case RT_BLEND_ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
        case RT_BLEND_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
        case RT_BLEND_CONSTANT_COLOR: return D3D12_BLEND_BLEND_FACTOR;
        case RT_BLEND_ONE_MINUS_CONSTANT_COLOR: return D3D12_BLEND_INV_BLEND_FACTOR;
        case RT_BLEND_CONSTANT_ALPHA: return D3D12_BLEND_ONE;
        case RT_BLEND_ONE_MINUS_CONSTANT_ALPHA: return D3D12_BLEND_ONE;
        default: return D3D12_BLEND_ONE;
    }
}

static D3D12_BLEND_OP rt_to_dx_blend_op(rt_blend_op_t func)
{
    switch (func)
    {
        case RT_FUNC_ADD: return D3D12_BLEND_OP_ADD;
        case RT_MIN: return D3D12_BLEND_OP_MIN;
        case RT_MAX: return D3D12_BLEND_OP_MAX;
        case RT_FUNC_SUBTRACT: return D3D12_BLEND_OP_SUBTRACT;
        case RT_FUNC_REVERSE_SUBTRACT: return D3D12_BLEND_OP_REV_SUBTRACT;
        default: return D3D12_BLEND_OP_ADD;
    }
}

static D3D12_CULL_MODE rt_to_dx_cull(rt_cull_mode_t mode)
{
    switch (mode)
    {
        case RT_CULL_NONE: return D3D12_CULL_MODE_NONE;
        case RT_CULL_FRONT: return D3D12_CULL_MODE_FRONT;
        case RT_CULL_BACK: return D3D12_CULL_MODE_BACK;
        case RT_CULL_FRONT_AND_BACK: return D3D12_CULL_MODE_NONE;
        default: return D3D12_CULL_MODE_NONE;
    }
}

static D3D12_FILL_MODE rt_to_dx_fill(rt_fill_mode_t fill)
{
    switch (fill)
    {
        case RT_POINT: return D3D12_FILL_MODE_WIREFRAME;
        case RT_LINE: return D3D12_FILL_MODE_WIREFRAME;
        case RT_FILL: return D3D12_FILL_MODE_SOLID;
        default: return D3D12_FILL_MODE_SOLID;
    }
}

static D3D12_PRIMITIVE_TOPOLOGY rt_to_dx_topology(rt_primitive_t primitive)
{
    switch (primitive)
    {
        case RT_POINTS: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case RT_LINES: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case RT_LINE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case RT_TRIANGLES: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case RT_TRIANGLE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

static D3D12_PRIMITIVE_TOPOLOGY_TYPE rt_to_dx_topology_type(rt_primitive_t primitive)
{
    switch (primitive)
    {
        case RT_POINTS: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case RT_LINES:
        case RT_LINE_STRIP: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case RT_TRIANGLES:
        case RT_TRIANGLE_STRIP: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        default: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }
}

static D3D12_STENCIL_OP rt_to_dx_stencil_op(rt_stencil_op_t op)
{
    switch (op)
    {
        case RT_STENCIL_ZERO: return D3D12_STENCIL_OP_ZERO;
        case RT_STENCIL_INVERT: return D3D12_STENCIL_OP_INVERT;
        case RT_STENCIL_KEEP: return D3D12_STENCIL_OP_KEEP;
        case RT_STENCIL_REPLACE: return D3D12_STENCIL_OP_REPLACE;
        case RT_STENCIL_INCR: return D3D12_STENCIL_OP_INCR_SAT;
        case RT_STENCIL_DECR: return D3D12_STENCIL_OP_DECR_SAT;
        case RT_STENCIL_INCR_WRAP: return D3D12_STENCIL_OP_INCR;
        case RT_STENCIL_DECR_WRAP: return D3D12_STENCIL_OP_DECR;
        default: return D3D12_STENCIL_OP_KEEP;
    }
}

static uint32_t rt_to_dx_vertex_size(rt_vertex_format_t format)
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

static uint32_t rt_to_dx_index_size(rt_index_type_t type)
{
    switch (type)
    {
        case RT_INDEX_UINT16: return 2;
        case RT_INDEX_UINT32: return 4;
        default: return 4;
    }
}

static DXGI_FORMAT rt_to_dx_index_type(rt_index_type_t type)
{
    switch (type)
    {
        case RT_INDEX_UINT16: return DXGI_FORMAT_R16_UINT;
        case RT_INDEX_UINT32: return DXGI_FORMAT_R32_UINT;
        default: return DXGI_FORMAT_R32_UINT;
    }
}

static uint32_t dx_format_bytes(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 16;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return 8;
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
            return 8;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return 8;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return 4;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return 4;
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
            return 4;
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
            return 4;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return 4;
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
            return 2;
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
            return 2;
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
            return 1;
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            return 4;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return 4;
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

struct dx_buffer_native_t
{
    ComPtr<ID3D12Resource> handle;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    D3D12_HEAP_TYPE heap = D3D12_HEAP_TYPE_DEFAULT;
    void* mapped = nullptr;
    size_t mappedOffset = 0;
    size_t mappedSize = 0;
};

struct dx_texture_native_t
{
    ComPtr<ID3D12Resource> handle;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    uint32_t levels = 1;
    uint32_t layers = 1;
    uint32_t width = 1, height = 1, depth = 1;
    rt_texture_target_t target = RT_TEXTURE_2D;
    rt_texture_format_t rtFormat = RT_TEXTURE_NONE;
    rt_texture_usages_t usage = 0;
    uint32_t samples = 1;
};

struct dx_texture_view_native_t
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    uint32_t texture = 0;
};

struct dx_sampler_native_t
{
    D3D12_SAMPLER_DESC desc = {};
};

struct dx_module_native_t
{
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipeline;
    D3D12_SHADER_BYTECODE vshader = {}, tshader = {}, mshader = {}, fshader = {}, cshader = {};
    std::vector<uint8_t> vcode, tcode, mcode, fcode, ccode;
    D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    uint32_t colorCount = 0;

    dx_binding_kind_t kinds[RT_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t descriptorBindings[RT_MAX_BINDING_HANDLE_NUM] = {};
    uint32_t descriptorCount = 0;
};

struct dx_mesh_native_t
{
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

struct dx_meshlet_native_t
{
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

struct dx_pass_compute_native_t { uint32_t dummy = 0; };
struct dx_pass_transfer_native_t { uint32_t dummy = 0; };

struct dx_staging_t
{
    ComPtr<ID3D12Resource> buffer;
};

struct dx_native_t
{
    uint32_t bufferID = 0, textureID = 0, textureViewID = 0, samplerID = 0, moduleID = 0;
    uint32_t meshID = 0, meshletID = 0, passID = 0;

    std::map<uint32_t, dx_buffer_native_t> buffers;
    std::map<uint32_t, dx_texture_native_t> textures;
    std::map<uint32_t, dx_texture_view_native_t> textureViews;
    std::map<uint32_t, dx_sampler_native_t> samplers;
    std::map<uint32_t, dx_module_native_t> modules;
    std::map<uint32_t, dx_mesh_native_t> meshes;
    std::map<uint32_t, dx_meshlet_native_t> meshlets;
    std::map<uint32_t, dx_pass_compute_native_t> computePasses;
    std::map<uint32_t, dx_pass_transfer_native_t> transferPasses;

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
    std::vector<dx_staging_t> pendingStaging;
    uint8_t pushData[128] = {};
    uint32_t pushCount = 0;

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

static void dx_transition_buffer(dx_buffer_native_t& buffer, D3D12_RESOURCE_STATES dst)
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

static void dx_transition_image(dx_texture_native_t& image, D3D12_RESOURCE_STATES dst)
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

static dx_buffer_native_t* dx_buffer_native(rt_buffer_t const& buffer)
{
    if (!buffer.native || buffer.handle == 0) return nullptr;
    auto it = direct.buffers.find(buffer.handle);
    return it == direct.buffers.end() ? nullptr : &it->second;
}

static dx_texture_native_t* dx_texture_native(rt_texture_t const& texture)
{
    if (!texture.native || texture.handle == 0) return nullptr;
    auto it = direct.textures.find(texture.handle);
    return it == direct.textures.end() ? nullptr : &it->second;
}

static dx_texture_view_native_t* dx_texture_view_native(uint32_t handle)
{
    if (handle == 0) return nullptr;
    auto it = direct.textureViews.find(handle);
    return it == direct.textureViews.end() ? nullptr : &it->second;
}

static dx_sampler_native_t* dx_sampler_native(rt_sampler_t const& sampler)
{
    if (!sampler.native || sampler.handle == 0) return nullptr;
    auto it = direct.samplers.find(sampler.handle);
    return it == direct.samplers.end() ? nullptr : &it->second;
}

static dx_module_native_t* dx_current_module_native()
{
    if (direct.currentPassType == RT_MODULE_COMPUTE && direct.currentComputePass)
        return (dx_module_native_t*)direct.currentComputePass->module.native;
    if (direct.currentPassType == RT_MODULE_RENDER && direct.currentRenderPass)
        return (dx_module_native_t*)direct.currentRenderPass->module.native;
    return nullptr;
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
    if (direct.samplerCursor >= 256) direct.samplerCursor = 0;
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
    if (mod->cshader.BytecodeLength)
        direct.cmd->SetComputeRootSignature(mod->rootSignature.Get());
    else
        direct.cmd->SetGraphicsRootSignature(mod->rootSignature.Get());

    for (uint32_t i = 0; i < mod->descriptorCount; ++i)
    {
        uint32_t binding = mod->descriptorBindings[i];
        auto& slot = direct.currentBinding[binding];
        uint32_t root = i + 1;
        if (mod->kinds[i] == DX_KIND_CBV || (mod->kinds[i] == DX_KIND_SRV && slot.type == RT_BINDING_BUFFER) ||
            (slot.buffer_bind.target == RT_SHADER_STORAGE_BUFFER && slot.buffer.handle))
        {
            auto* buf = dx_buffer_native(slot.buffer);
            if (!buf) continue;
            bool storage = (slot.buffer_bind.target == RT_SHADER_STORAGE_BUFFER) || (mod->kinds[i] == DX_KIND_UAV);
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
            if (mod->cshader.BytecodeLength) direct.cmd->SetComputeRootDescriptorTable(root, gpu);
            else direct.cmd->SetGraphicsRootDescriptorTable(root, gpu);
        }
        else if (mod->kinds[i] == DX_KIND_SAMPLER)
        {
            auto* samp = dx_sampler_native(slot.sampler);
            if (!samp) continue;
            auto gpu = dx_alloc_sampler();
            direct.device->CreateSampler(&samp->desc, dx_sampler_cpu(gpu));
            if (mod->cshader.BytecodeLength) direct.cmd->SetComputeRootDescriptorTable(root, gpu);
            else direct.cmd->SetGraphicsRootDescriptorTable(root, gpu);
        }
        else if (mod->kinds[i] == DX_KIND_SRV)
        {
            auto* view = dx_texture_view_native(slot.texture_view.handle);
            if (!view) continue;
            auto texIt = direct.textures.find(view->texture);
            if (texIt == direct.textures.end() || !texIt->second.handle) continue;
            auto* tex = &texIt->second;
            dx_transition_image(*tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            auto gpu = dx_alloc_srv();
            direct.device->CreateShaderResourceView(tex->handle.Get(), &view->srv, dx_srv_cpu(gpu));
            if (mod->cshader.BytecodeLength) direct.cmd->SetComputeRootDescriptorTable(root, gpu);
            else direct.cmd->SetGraphicsRootDescriptorTable(root, gpu);
        }
        else if (mod->kinds[i] == DX_KIND_UAV)
        {
            auto* view = dx_texture_view_native(slot.storage_view.handle);
            if (!view) continue;
            auto texIt = direct.textures.find(view->texture);
            if (texIt == direct.textures.end() || !texIt->second.handle) continue;
            auto* tex = &texIt->second;
            dx_transition_image(*tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            auto gpu = dx_alloc_srv();
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};
            uav.Format = dx_srv_format(tex->format);
            uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uav.Texture2D.MipSlice = slot.storage_texture_bind.base_level;
            direct.device->CreateUnorderedAccessView(tex->handle.Get(), nullptr, &uav, dx_srv_cpu(gpu));
            if (mod->cshader.BytecodeLength) direct.cmd->SetComputeRootDescriptorTable(root, gpu);
            else direct.cmd->SetGraphicsRootDescriptorTable(root, gpu);
        }
    }
    if (direct.pushCount)
    {
        if (mod->cshader.BytecodeLength)
            direct.cmd->SetComputeRoot32BitConstants(0, direct.pushCount, direct.pushData, 0);
        else
            direct.cmd->SetGraphicsRoot32BitConstants(0, direct.pushCount, direct.pushData, 0);
    }
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
    if (!direct.queue)
    {
        fprintf(stderr, "DirectX: queue is null\n");
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
    direct.samplerCursor = 0;

    direct.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&direct.fence));
    direct.fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    rt_unload_library = dx_unload_library;
    rt_create_buffer = dx_create_buffer;
    rt_destroy_buffer = dx_destroy_buffer;
    rt_bind_buffer = dx_bind_buffer;
    rt_map_buffer = dx_map_buffer;
    rt_unmap_buffer = dx_unmap_buffer;
    rt_create_texture = dx_create_texture;
    rt_destroy_texture = dx_destroy_texture;
    rt_bind_texture = dx_bind_texture;
    rt_bind_texture_storage = dx_bind_texture_storage;
    rt_create_texture_view = dx_create_texture_view;
    rt_destroy_texture_view = dx_destroy_texture_view;
    rt_bind_texture_view = dx_bind_texture_view;
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
    rt_submit = dx_submit;
}

void dx_unload_library()
{
    if (!direct.device) return;
    if (direct.cmd) direct.cmd->Close();
    dx_wait_gpu();
    if (direct.fenceEvent)
    {
        CloseHandle(direct.fenceEvent);
        direct.fenceEvent = nullptr;
    }
    for (auto& staging : direct.pendingStaging)
        staging.buffer.Reset();
    dx_flush_staging();
    for (auto& item : direct.modules)
    {
        item.second.pipeline.Reset();
        item.second.rootSignature.Reset();
    }
    direct.modules.clear();
    for (auto& item : direct.buffers)
    {
        if (item.second.mapped && item.second.handle)
            item.second.handle->Unmap(0, nullptr);
        item.second.mapped = nullptr;
        item.second.handle.Reset();
    }
    direct.buffers.clear();
    direct.textureViews.clear();
    for (auto& item : direct.textures)
        item.second.handle.Reset();
    direct.textures.clear();
    direct.samplers.clear();
    direct.meshes.clear();
    direct.meshlets.clear();
    direct.computePasses.clear();
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
    direct.bufferID = direct.textureID = direct.textureViewID = direct.samplerID = direct.moduleID = 0;
    direct.meshID = direct.meshletID = direct.passID = 0;
    direct.currentPassType = RT_MODULE_NONE;
    direct.currentPipeline = nullptr;

    if (rt_unload_library == dx_unload_library) rt_unload_library = nullptr;
    if (rt_create_buffer == dx_create_buffer) rt_create_buffer = nullptr;
    if (rt_destroy_buffer == dx_destroy_buffer) rt_destroy_buffer = nullptr;
    if (rt_bind_buffer == dx_bind_buffer) rt_bind_buffer = nullptr;
    if (rt_map_buffer == dx_map_buffer) rt_map_buffer = nullptr;
    if (rt_unmap_buffer == dx_unmap_buffer) rt_unmap_buffer = nullptr;
    if (rt_create_texture == dx_create_texture) rt_create_texture = nullptr;
    if (rt_destroy_texture == dx_destroy_texture) rt_destroy_texture = nullptr;
    if (rt_bind_texture == dx_bind_texture) rt_bind_texture = nullptr;
    if (rt_bind_texture_storage == dx_bind_texture_storage) rt_bind_texture_storage = nullptr;
    if (rt_create_texture_view == dx_create_texture_view) rt_create_texture_view = nullptr;
    if (rt_destroy_texture_view == dx_destroy_texture_view) rt_destroy_texture_view = nullptr;
    if (rt_bind_texture_view == dx_bind_texture_view) rt_bind_texture_view = nullptr;
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
    if (rt_submit == dx_submit) rt_submit = nullptr;
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
    bool hostVisible = (info.usage & (RT_BUFFER_USAGE_MAP_READ | RT_BUFFER_USAGE_MAP_WRITE)) || info.data;
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
    direct.currentBinding[bind.binding].type = RT_BINDING_BUFFER;
    direct.currentBinding[bind.binding].buffer = buffer;
    direct.currentBinding[bind.binding].buffer_bind = bind;
}

void* dx_map_buffer(rt_buffer_t& buffer, rt_access_t mode, size_t offset, size_t size)
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
    if (info.usage == 0)
    {
        fprintf(stderr, "Texture usage must not be 0");
        abort();
    }
    rt_texture_t result = {};
    if (!direct.device || info.width == 0) return result;
    if (info.target != RT_TEXTURE_1D && info.height == 0) return result;

    uint32_t handle = direct.textureID + 1;
    auto& native = direct.textures[handle];
    native.format = rt_to_dx_texture_format(info.format);
    native.rtFormat = info.format;
    native.usage = info.usage;
    native.width = info.width;
    native.height = info.target == RT_TEXTURE_1D ? 1 : info.height;
    native.depth = info.depth ? info.depth : 1;
    native.target = info.target;
    native.layers = (info.target == RT_TEXTURE_2D_ARRAY) ? native.depth : 1;
    native.samples = rt_to_dx_sample_count(info.samples);
    if (info.target == RT_TEXTURE_1D || info.target == RT_TEXTURE_3D)
        native.samples = 1;
    native.levels = 1;
    if (native.samples == 1 && info.mipmaps == 0 && rt_has_mipmap_filter(info.min_filter))
    {
        uint32_t maxDim = max(native.width, native.height);
        while (maxDim >>= 1) native.levels++;
    }
    else if (native.samples == 1 && info.mipmaps > 1)
        native.levels = info.mipmaps;

    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = (info.target == RT_TEXTURE_3D) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D :
                     (info.target == RT_TEXTURE_1D) ? D3D12_RESOURCE_DIMENSION_TEXTURE1D :
                     D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = native.width;
    desc.Height = native.height;
    desc.DepthOrArraySize = (UINT16)((info.target == RT_TEXTURE_3D) ? native.depth : native.layers);
    desc.MipLevels = (UINT16)native.levels;
    desc.Format = dx_typeless(native.format);
    desc.SampleDesc.Count = native.samples;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    if (dx_is_depth(native.format))
    {
        if (info.usage & RT_TEXTURE_USAGE_RENDER_ATTACHMENT)
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    else
    {
        if (info.usage & RT_TEXTURE_USAGE_STORAGE_BINDING)
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        if (info.usage & RT_TEXTURE_USAGE_RENDER_ATTACHMENT)
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if ((info.usage & (RT_TEXTURE_USAGE_TEXTURE_BINDING | RT_TEXTURE_USAGE_STORAGE_BINDING)) == 0)
        desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

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

    if (info.data && native.samples == 1)
    {
        size_t bytes = (size_t)native.width * native.height * ((info.target == RT_TEXTURE_3D) ? native.depth : 1) * dx_format_bytes(native.format);
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
            src.PlacedFootprint.Footprint.Depth = (info.target == RT_TEXTURE_3D) ? native.depth : 1;
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

    if (native.samples == 4 && (native.target == RT_TEXTURE_2D || native.target == RT_TEXTURE_2D_MULTISAMPLE))
        native.target = RT_TEXTURE_2D_MULTISAMPLE;
    direct.textureID = handle;
    result.handle = handle;
    result.width = native.width;
    result.height = native.height;
    result.depth = native.depth;
    result.target = native.target;
    result.format = native.rtFormat;
    result.samples = native.samples > 1 ? RT_TEXTURE_SAMPLE_4X : RT_TEXTURE_SAMPLE_1X;
    result.usage = native.usage;
    result.mipmaps = native.levels;
    result.native = &native;
    result.default_view = dx_create_texture_view(result, {
        .target = native.target,
        .format = native.rtFormat,
        .aspect = RT_TEXTURE_ASPECT_ALL,
        .usage = native.usage,
        .layer_count = native.layers,
        .level_count = native.levels,
    });
    return result;
}

void dx_destroy_texture(rt_texture_t& texture)
{
    if (texture.handle)
    {
        for (auto it = direct.textureViews.begin(); it != direct.textureViews.end(); )
        {
            if (it->second.texture == texture.handle)
                it = direct.textureViews.erase(it);
            else
                ++it;
        }
    }
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
    direct.currentBinding[bind.binding].type = RT_BINDING_TEXTURE;
    direct.currentBinding[bind.binding].texture_view = texture.default_view;
    direct.currentBinding[bind.binding].texture_bind = bind;
}

void dx_bind_texture_storage(rt_texture_t& texture, rt_texture_storage_bind_t bind)
{
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    direct.currentBinding[bind.binding].type = RT_BINDING_STORAGE_TEXTURE;
    direct.currentBinding[bind.binding].storage_view = texture.default_view;
    direct.currentBinding[bind.binding].storage_texture_bind = bind;
}

rt_texture_view_t dx_create_texture_view(rt_texture_t& texture, rt_texture_view_info_t const& info)
{
    auto* tex = dx_texture_native(texture);
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

    DXGI_FORMAT dxFormat = result.format != RT_TEXTURE_NONE ? rt_to_dx_texture_format(result.format) : tex->format;
    UINT plane = 0;
    DXGI_FORMAT srvFormat = dx_srv_format(dxFormat);
    if (info.aspect == RT_TEXTURE_ASPECT_STENCIL)
    {
        if (tex->format == DXGI_FORMAT_D24_UNORM_S8_UINT)
            srvFormat = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
        else if (tex->format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
            srvFormat = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
        plane = 1;
    }
    else if (dx_is_depth(tex->format) || dx_is_depth(dxFormat))
        srvFormat = dx_srv_format(tex->format);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.Format = srvFormat;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (info.target == RT_TEXTURE_3D)
    {
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srv.Texture3D.MostDetailedMip = info.base_level;
        srv.Texture3D.MipLevels = levelCount;
    }
    else if (info.target == RT_TEXTURE_1D)
    {
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        srv.Texture1D.MostDetailedMip = info.base_level;
        srv.Texture1D.MipLevels = levelCount;
    }
    else if (tex->samples > 1)
    {
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    }
    else if (info.target == RT_TEXTURE_2D_ARRAY || layerCount > 1 || baseLayer != 0)
    {
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srv.Texture2DArray.MostDetailedMip = info.base_level;
        srv.Texture2DArray.MipLevels = levelCount;
        srv.Texture2DArray.FirstArraySlice = baseLayer;
        srv.Texture2DArray.ArraySize = layerCount;
        srv.Texture2DArray.PlaneSlice = plane;
    }
    else
    {
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Texture2D.MostDetailedMip = info.base_level;
        srv.Texture2D.MipLevels = levelCount;
        srv.Texture2D.PlaneSlice = plane;
    }

    uint32_t handle = direct.textureViewID + 1;
    auto& native = direct.textureViews[handle];
    native.texture = texture.handle;
    native.srv = srv;
    direct.textureViewID = handle;
    result.handle = handle;
    result.native = &native;
    return result;
}

void dx_destroy_texture_view(rt_texture_view_t& view)
{
    direct.textureViews.erase(view.handle);
    view.handle = 0;
    view.native = nullptr;
}

void dx_bind_texture_view(rt_texture_view_t& view, rt_texture_view_bind_t bind)
{
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    direct.currentBinding[bind.binding].type = RT_BINDING_TEXTURE;
    direct.currentBinding[bind.binding].texture_view = view;
    direct.currentBinding[bind.binding].texture_bind = {};
    direct.currentBinding[bind.binding].texture_bind.binding = bind.binding;
}

rt_sampler_t dx_create_sampler(rt_sampler_info_t const& info)
{
    rt_sampler_t result = {};
    uint32_t handle = direct.samplerID + 1;
    auto& native = direct.samplers[handle];
    native.desc.Filter = rt_to_dx_filter(info.min_filter, info.mag_filter);
    native.desc.AddressU = rt_to_dx_address(info.address_u);
    native.desc.AddressV = rt_to_dx_address(info.address_v);
    native.desc.AddressW = rt_to_dx_address(info.address_w);
    native.desc.MaxLOD = rt_has_mipmap_filter(info.min_filter) ? D3D12_FLOAT32_MAX : 0.0f;
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
    direct.currentBinding[bind.binding].type = RT_BINDING_SAMPLER;
    direct.currentBinding[bind.binding].sampler = sampler;
    direct.currentBinding[bind.binding].sampler_bind = bind;
}

rt_module_compute_t dx_create_module_compute(rt_module_compute_info_t const& info)
{
    rt_module_compute_t result = {};
    if (!info.cshader.code || !info.cshader.size || !direct.device) return result;
    uint32_t handle = direct.moduleID + 1;
    auto& native = direct.modules[handle];
    native.ccode.assign((const uint8_t*)info.cshader.code, (const uint8_t*)info.cshader.code + info.cshader.size);
    native.cshader.pShaderBytecode = native.ccode.data();
    native.cshader.BytecodeLength = native.ccode.size();
    native.descriptorCount = 0;
    D3D12_ROOT_PARAMETER params[1] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[0].Constants.ShaderRegister = 16;
    params[0].Constants.RegisterSpace = 0;
    params[0].Constants.Num32BitValues = 32;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 1;
    desc.pParameters = params;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    ComPtr<ID3DBlob> blob, error;
    if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error)) ||
        FAILED(direct.device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
            IID_PPV_ARGS(&native.rootSignature))))
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
    if (info.vshader.code && info.vshader.size)
    {
        native.vcode.assign((const uint8_t*)info.vshader.code, (const uint8_t*)info.vshader.code + info.vshader.size);
        native.vshader.pShaderBytecode = native.vcode.data();
        native.vshader.BytecodeLength = native.vcode.size();
    }
    if (info.fshader.code && info.fshader.size)
    {
        native.fcode.assign((const uint8_t*)info.fshader.code, (const uint8_t*)info.fshader.code + info.fshader.size);
        native.fshader.pShaderBytecode = native.fcode.data();
        native.fshader.BytecodeLength = native.fcode.size();
    }
    native.descriptorCount = 0;
    D3D12_DESCRIPTOR_RANGE ranges[RT_MAX_BINDING_HANDLE_NUM] = {};
    D3D12_ROOT_PARAMETER params[1 + RT_MAX_BINDING_HANDLE_NUM] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[0].Constants.ShaderRegister = 16;
    params[0].Constants.RegisterSpace = 0;
    params[0].Constants.Num32BitValues = 32;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    uint32_t paramCount = 1;
    for (uint32_t i = 0; i < RT_MAX_BINDING_HANDLE_NUM; ++i)
    {
        dx_binding_kind_t kind = DX_KIND_NONE;
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        if (info.binding[i].type == RT_BINDING_BUFFER)
        {
            kind = DX_KIND_CBV;
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        }
        else if (info.binding[i].type == RT_BINDING_TEXTURE)
        {
            kind = DX_KIND_SRV;
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        }
        else if (info.binding[i].type == RT_BINDING_STORAGE_TEXTURE)
        {
            kind = DX_KIND_UAV;
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        }
        else if (info.binding[i].type == RT_BINDING_SAMPLER)
        {
            kind = DX_KIND_SAMPLER;
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        }
        else continue;
        native.kinds[native.descriptorCount] = kind;
        native.descriptorBindings[native.descriptorCount] = info.binding[i].binding;
        ranges[native.descriptorCount].RangeType = rangeType;
        ranges[native.descriptorCount].NumDescriptors = 1;
        ranges[native.descriptorCount].BaseShaderRegister = info.binding[i].binding;
        ranges[native.descriptorCount].RegisterSpace = 0;
        ranges[native.descriptorCount].OffsetInDescriptorsFromTableStart = 0;
        params[paramCount].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[paramCount].DescriptorTable.NumDescriptorRanges = 1;
        params[paramCount].DescriptorTable.pDescriptorRanges = &ranges[native.descriptorCount];
        params[paramCount].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        native.descriptorCount++;
        paramCount++;
    }
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = paramCount;
    desc.pParameters = params;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> blob, error;
    if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error)) ||
        FAILED(direct.device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
            IID_PPV_ARGS(&native.rootSignature))))
    {
        result.native = &native;
        dx_destroy_module_native(handle, result.native);
        return {};
    }
    D3D12_INPUT_ELEMENT_DESC elements[RT_MAX_VERTEX_BUFFER_NUM] = {};
    uint32_t attrCount = 0;
    for (uint32_t i = 0; i < RT_MAX_VERTEX_BUFFER_NUM; ++i)
    {
        if (info.vertex[i].format == RT_VERTEX_NONE) continue;
        elements[attrCount].SemanticName = "TEXCOORD";
        elements[attrCount].SemanticIndex = info.vertex[i].location;
        elements[attrCount].Format = rt_to_dx_vertex_format(info.vertex[i].format);
        elements[attrCount].InputSlot = info.vertex[i].location;
        elements[attrCount].AlignedByteOffset = 0;
        elements[attrCount].InputSlotClass = info.vertex[i].instance ?
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        elements[attrCount].InstanceDataStepRate = info.vertex[i].instance ? 1 : 0;
        attrCount++;
    }
    const bool depthEnabled = (info.depth.func != RT_ALWAYS || info.depth.write);
    const bool stencilEnabled =
        (info.stencil.back.func != RT_ALWAYS || info.stencil.back.sfail != RT_STENCIL_KEEP ||
         info.stencil.back.zfail != RT_STENCIL_KEEP || info.stencil.back.zpass != RT_STENCIL_KEEP ||
         info.stencil.front.func != RT_ALWAYS || info.stencil.front.sfail != RT_STENCIL_KEEP ||
         info.stencil.front.zfail != RT_STENCIL_KEEP || info.stencil.front.zpass != RT_STENCIL_KEEP);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = native.rootSignature.Get();
    pso.VS = native.vshader;
    pso.PS = native.fshader;
    pso.InputLayout = {elements, attrCount};
    pso.PrimitiveTopologyType = rt_to_dx_topology_type(info.primitive);
    pso.RasterizerState.FillMode = rt_to_dx_fill(info.fill_mode);
    pso.RasterizerState.CullMode = rt_to_dx_cull(info.cull_mode);
    pso.RasterizerState.FrontCounterClockwise = (info.wind_mode != RT_CW);
    pso.RasterizerState.DepthBias = (INT)info.depth.bias;
    pso.RasterizerState.DepthBiasClamp = info.depth.biasClamp;
    pso.RasterizerState.SlopeScaledDepthBias = info.depth.biasSlope;
    pso.RasterizerState.DepthClipEnable = TRUE;
    pso.BlendState.AlphaToCoverageEnable = FALSE;
    pso.BlendState.IndependentBlendEnable = TRUE;
    uint32_t colorCount = 0;
    for (uint32_t i = 0; i < RT_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        if (info.colors[i].format == RT_TEXTURE_NONE)
        {
            pso.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
            continue;
        }
        colorCount = i + 1;
        auto& rt = pso.BlendState.RenderTarget[i];
        bool blend =
            (info.colors[i].color.func != RT_FUNC_ADD || info.colors[i].color.src != RT_BLEND_ONE ||
             info.colors[i].color.dst != RT_BLEND_ZERO || info.colors[i].alpha.func != RT_FUNC_ADD ||
             info.colors[i].alpha.src != RT_BLEND_ONE || info.colors[i].alpha.dst != RT_BLEND_ZERO);
        rt.BlendEnable = blend;
        rt.SrcBlend = rt_to_dx_blend(info.colors[i].color.src);
        rt.DestBlend = rt_to_dx_blend(info.colors[i].color.dst);
        rt.BlendOp = rt_to_dx_blend_op(info.colors[i].color.func);
        rt.SrcBlendAlpha = rt_to_dx_blend(info.colors[i].alpha.src);
        rt.DestBlendAlpha = rt_to_dx_blend(info.colors[i].alpha.dst);
        rt.BlendOpAlpha = rt_to_dx_blend_op(info.colors[i].alpha.func);
        rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        pso.RTVFormats[i] = rt_to_dx_texture_format(info.colors[i].format);
    }
    native.colorCount = colorCount;
    pso.NumRenderTargets = colorCount;
    pso.SampleMask = UINT_MAX;
    pso.SampleDesc.Count = 1;
    pso.DepthStencilState.DepthEnable = depthEnabled;
    pso.DepthStencilState.DepthWriteMask = info.depth.write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    pso.DepthStencilState.DepthFunc = rt_to_dx_compare(info.depth.func);
    pso.DepthStencilState.StencilEnable = stencilEnabled;
    pso.DepthStencilState.StencilReadMask = (UINT8)info.stencil.read;
    pso.DepthStencilState.StencilWriteMask = (UINT8)info.stencil.write;
    pso.DepthStencilState.FrontFace.StencilFailOp = rt_to_dx_stencil_op(info.stencil.front.sfail);
    pso.DepthStencilState.FrontFace.StencilDepthFailOp = rt_to_dx_stencil_op(info.stencil.front.zfail);
    pso.DepthStencilState.FrontFace.StencilPassOp = rt_to_dx_stencil_op(info.stencil.front.zpass);
    pso.DepthStencilState.FrontFace.StencilFunc = rt_to_dx_compare(info.stencil.front.func);
    pso.DepthStencilState.BackFace.StencilFailOp = rt_to_dx_stencil_op(info.stencil.back.sfail);
    pso.DepthStencilState.BackFace.StencilDepthFailOp = rt_to_dx_stencil_op(info.stencil.back.zfail);
    pso.DepthStencilState.BackFace.StencilPassOp = rt_to_dx_stencil_op(info.stencil.back.zpass);
    pso.DepthStencilState.BackFace.StencilFunc = rt_to_dx_compare(info.stencil.back.func);
    if (stencilEnabled) pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    else if (depthEnabled) pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    native.topology = rt_to_dx_topology(info.primitive);
    if (FAILED(direct.device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&native.pipeline))))
    {
        result.native = &native;
        dx_destroy_module_native(handle, result.native);
        return {};
    }
    direct.moduleID = handle;
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

rt_module_render_t dx_create_module_meshlet(rt_module_render_info_t const& info)
{
    rt_module_render_t result = {};
    if (!info.mshader.code || !info.mshader.size || !direct.device) return result;
    uint32_t handle = direct.moduleID + 1;
    auto& native = direct.modules[handle];
    if (info.tshader.code && info.tshader.size)
    {
        native.tcode.assign((const uint8_t*)info.tshader.code, (const uint8_t*)info.tshader.code + info.tshader.size);
        native.tshader.pShaderBytecode = native.tcode.data();
        native.tshader.BytecodeLength = native.tcode.size();
    }
    native.mcode.assign((const uint8_t*)info.mshader.code, (const uint8_t*)info.mshader.code + info.mshader.size);
    native.mshader.pShaderBytecode = native.mcode.data();
    native.mshader.BytecodeLength = native.mcode.size();
    if (info.fshader.code && info.fshader.size)
    {
        native.fcode.assign((const uint8_t*)info.fshader.code, (const uint8_t*)info.fshader.code + info.fshader.size);
        native.fshader.pShaderBytecode = native.fcode.data();
        native.fshader.BytecodeLength = native.fcode.size();
    }
    native.descriptorCount = 0;
    D3D12_DESCRIPTOR_RANGE ranges[RT_MAX_BINDING_HANDLE_NUM] = {};
    D3D12_ROOT_PARAMETER params[1 + RT_MAX_BINDING_HANDLE_NUM] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[0].Constants.ShaderRegister = 16;
    params[0].Constants.RegisterSpace = 0;
    params[0].Constants.Num32BitValues = 32;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    uint32_t paramCount = 1;
    for (uint32_t i = 0; i < RT_MAX_BINDING_HANDLE_NUM; ++i)
    {
        dx_binding_kind_t kind = DX_KIND_NONE;
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        if (info.binding[i].type == RT_BINDING_BUFFER)
        {
            kind = DX_KIND_CBV;
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        }
        else if (info.binding[i].type == RT_BINDING_TEXTURE)
        {
            kind = DX_KIND_SRV;
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        }
        else if (info.binding[i].type == RT_BINDING_STORAGE_TEXTURE)
        {
            kind = DX_KIND_UAV;
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        }
        else if (info.binding[i].type == RT_BINDING_SAMPLER)
        {
            kind = DX_KIND_SAMPLER;
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        }
        else continue;
        native.kinds[native.descriptorCount] = kind;
        native.descriptorBindings[native.descriptorCount] = info.binding[i].binding;
        ranges[native.descriptorCount].RangeType = rangeType;
        ranges[native.descriptorCount].NumDescriptors = 1;
        ranges[native.descriptorCount].BaseShaderRegister = info.binding[i].binding;
        ranges[native.descriptorCount].RegisterSpace = 0;
        ranges[native.descriptorCount].OffsetInDescriptorsFromTableStart = 0;
        params[paramCount].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[paramCount].DescriptorTable.NumDescriptorRanges = 1;
        params[paramCount].DescriptorTable.pDescriptorRanges = &ranges[native.descriptorCount];
        params[paramCount].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        native.descriptorCount++;
        paramCount++;
    }
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = paramCount;
    desc.pParameters = params;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> blob, error;
    if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error)) ||
        FAILED(direct.device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
            IID_PPV_ARGS(&native.rootSignature))))
    {
        result.native = &native;
        dx_destroy_module_native(handle, result.native);
        return {};
    }
    const bool depthEnabled = (info.depth.func != RT_ALWAYS || info.depth.write);
    const bool stencilEnabled =
        (info.stencil.back.func != RT_ALWAYS || info.stencil.back.sfail != RT_STENCIL_KEEP ||
         info.stencil.back.zfail != RT_STENCIL_KEEP || info.stencil.back.zpass != RT_STENCIL_KEEP ||
         info.stencil.front.func != RT_ALWAYS || info.stencil.front.sfail != RT_STENCIL_KEEP ||
         info.stencil.front.zfail != RT_STENCIL_KEEP || info.stencil.front.zpass != RT_STENCIL_KEEP);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = native.rootSignature.Get();
    pso.VS = native.vshader;
    pso.PS = native.fshader;
    pso.InputLayout = {nullptr, 0};
    pso.PrimitiveTopologyType = rt_to_dx_topology_type(info.primitive);
    pso.RasterizerState.FillMode = rt_to_dx_fill(info.fill_mode);
    pso.RasterizerState.CullMode = rt_to_dx_cull(info.cull_mode);
    pso.RasterizerState.FrontCounterClockwise = (info.wind_mode != RT_CW);
    pso.RasterizerState.DepthBias = (INT)info.depth.bias;
    pso.RasterizerState.DepthBiasClamp = info.depth.biasClamp;
    pso.RasterizerState.SlopeScaledDepthBias = info.depth.biasSlope;
    pso.RasterizerState.DepthClipEnable = TRUE;
    pso.BlendState.AlphaToCoverageEnable = FALSE;
    pso.BlendState.IndependentBlendEnable = TRUE;
    uint32_t colorCount = 0;
    for (uint32_t i = 0; i < RT_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        if (info.colors[i].format == RT_TEXTURE_NONE)
        {
            pso.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
            continue;
        }
        colorCount = i + 1;
        auto& rt = pso.BlendState.RenderTarget[i];
        bool blend =
            (info.colors[i].color.func != RT_FUNC_ADD || info.colors[i].color.src != RT_BLEND_ONE ||
             info.colors[i].color.dst != RT_BLEND_ZERO || info.colors[i].alpha.func != RT_FUNC_ADD ||
             info.colors[i].alpha.src != RT_BLEND_ONE || info.colors[i].alpha.dst != RT_BLEND_ZERO);
        rt.BlendEnable = blend;
        rt.SrcBlend = rt_to_dx_blend(info.colors[i].color.src);
        rt.DestBlend = rt_to_dx_blend(info.colors[i].color.dst);
        rt.BlendOp = rt_to_dx_blend_op(info.colors[i].color.func);
        rt.SrcBlendAlpha = rt_to_dx_blend(info.colors[i].alpha.src);
        rt.DestBlendAlpha = rt_to_dx_blend(info.colors[i].alpha.dst);
        rt.BlendOpAlpha = rt_to_dx_blend_op(info.colors[i].alpha.func);
        rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        pso.RTVFormats[i] = rt_to_dx_texture_format(info.colors[i].format);
    }
    native.colorCount = colorCount;
    pso.NumRenderTargets = colorCount;
    pso.SampleMask = UINT_MAX;
    pso.SampleDesc.Count = 1;
    pso.DepthStencilState.DepthEnable = depthEnabled;
    pso.DepthStencilState.DepthWriteMask = info.depth.write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    pso.DepthStencilState.DepthFunc = rt_to_dx_compare(info.depth.func);
    pso.DepthStencilState.StencilEnable = stencilEnabled;
    pso.DepthStencilState.StencilReadMask = (UINT8)info.stencil.read;
    pso.DepthStencilState.StencilWriteMask = (UINT8)info.stencil.write;
    pso.DepthStencilState.FrontFace.StencilFailOp = rt_to_dx_stencil_op(info.stencil.front.sfail);
    pso.DepthStencilState.FrontFace.StencilDepthFailOp = rt_to_dx_stencil_op(info.stencil.front.zfail);
    pso.DepthStencilState.FrontFace.StencilPassOp = rt_to_dx_stencil_op(info.stencil.front.zpass);
    pso.DepthStencilState.FrontFace.StencilFunc = rt_to_dx_compare(info.stencil.front.func);
    pso.DepthStencilState.BackFace.StencilFailOp = rt_to_dx_stencil_op(info.stencil.back.sfail);
    pso.DepthStencilState.BackFace.StencilDepthFailOp = rt_to_dx_stencil_op(info.stencil.back.zfail);
    pso.DepthStencilState.BackFace.StencilPassOp = rt_to_dx_stencil_op(info.stencil.back.zpass);
    pso.DepthStencilState.BackFace.StencilFunc = rt_to_dx_compare(info.stencil.back.func);
    if (stencilEnabled) pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    else if (depthEnabled) pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    native.topology = rt_to_dx_topology(info.primitive);
    if (FAILED(direct.device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&native.pipeline))))
    {
        result.native = &native;
        dx_destroy_module_native(handle, result.native);
        return {};
    }
    direct.moduleID = handle;
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

void dx_destroy_module_render(rt_module_render_t& module)
{
    dx_destroy_module_native(module.handle, module.native);
    module.handle = 0;
}

void dx_destroy_module_compute(rt_module_compute_t& module)
{
    dx_destroy_module_native(module.handle, module.native);
    module.handle = 0;
}

void dx_push_constant(uint8_t const* buffer, size_t length)
{
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    auto* mod = dx_current_module_native();
    if (!buffer || length == 0 || !mod) return;
    uint32_t count = (uint32_t)((length + 3) / 4);
    if (count > 32) count = 32;
    std::memcpy(direct.pushData, buffer, (size_t)count * 4);
    direct.pushCount = count;
    if (mod->cshader.BytecodeLength)
        direct.cmd->SetComputeRoot32BitConstants(0, count, direct.pushData, 0);
    else
        direct.cmd->SetGraphicsRoot32BitConstants(0, count, direct.pushData, 0);
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
    if (direct.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    if (pass.module.handle == 0 || !pass.module.native)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    for (auto& binding : direct.currentBinding)
        binding = {};
    direct.pushCount = 0;
    auto handle = direct.passID + 1;
    auto& native = direct.computePasses[handle];
    pass.handle = handle;
    pass.native = &native;
    direct.currentPassType = RT_MODULE_COMPUTE;
    direct.currentComputePass = &pass;
    auto* mod = (dx_module_native_t*)pass.module.native;
    if (mod && mod->pipeline)
        direct.cmd->SetPipelineState(mod->pipeline.Get());
    if (mod && mod->rootSignature)
        direct.cmd->SetComputeRootSignature(mod->rootSignature.Get());
    direct.passID = handle;
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
    direct.currentPassType = RT_MODULE_NONE;
    direct.currentPipeline = nullptr;
    for (auto& binding : direct.currentBinding)
        binding = {};
    direct.pushCount = 0;
}

void dx_dispatch_compute(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (direct.currentPassType != RT_MODULE_COMPUTE)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    dx_flush_descriptors();
    direct.cmd->Dispatch(max(1u, groupX), max(1u, groupY), max(1u, groupZ));
}

void dx_begin_render(rt_pass_render_t& pass)
{
    if (direct.currentPipeline)
    {
        fprintf(stderr, "Pipeline not end");
        abort();
    }
    if (pass.module.handle == 0 || !pass.module.native)
    {
        fprintf(stderr, "Pipeline module is not created\n");
        abort();
    }
    for (auto& binding : direct.currentBinding)
        binding = {};
    direct.pushCount = 0;
    direct.currentPassType = RT_MODULE_RENDER;
    direct.currentRenderPass = &pass;

    auto* mod = (dx_module_native_t*)pass.module.native;
    if (mod && mod->pipeline)
        direct.cmd->SetPipelineState(mod->pipeline.Get());
    if (mod && mod->rootSignature)
        direct.cmd->SetGraphicsRootSignature(mod->rootSignature.Get());
    if (mod)
        direct.cmd->IASetPrimitiveTopology(mod->topology);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[RT_MAX_COLOR_TEXTURE_NUM] = {};
    uint32_t colorCount = 0;
    uint32_t width = 0, height = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvStart = direct.rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (uint32_t i = 0; i < RT_MAX_COLOR_TEXTURE_NUM; ++i)
    {
        if (pass.colors[i].texture_view.format == RT_TEXTURE_NONE)
            continue;
        auto* view = dx_texture_view_native(pass.colors[i].texture_view.handle);
        if (!view) continue;
        auto texIt = direct.textures.find(view->texture);
        if (texIt == direct.textures.end() || !texIt->second.handle) continue;
        auto* tex = &texIt->second;
        dx_transition_image(*tex, D3D12_RESOURCE_STATE_RENDER_TARGET);
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvStart;
        rtv.ptr += (SIZE_T)i * direct.rtvSize;
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = tex->format;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = pass.colors[i].texture_view.base_level;
        direct.device->CreateRenderTargetView(tex->handle.Get(), &rtvDesc, rtv);
        rtvs[i] = rtv;
        if (pass.colors[i].clear)
        {
            float clear[4] = {pass.colors[i].value.r, pass.colors[i].value.g, pass.colors[i].value.b, pass.colors[i].value.a};
            direct.cmd->ClearRenderTargetView(rtv, clear, 0, nullptr);
        }
        uint32_t mip = min(pass.colors[i].texture_view.base_level, 31u);
        width = max(width, max(1u, tex->width >> mip));
        height = max(height, max(1u, tex->height >> mip));
        colorCount = i + 1;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = {};
    bool hasDepth = false;
    if (auto* view = dx_texture_view_native(pass.depth.texture_view.handle))
    {
        auto texIt = direct.textures.find(view->texture);
        if (texIt != direct.textures.end() && texIt->second.handle)
        {
            auto* tex = &texIt->second;
            dx_transition_image(*tex, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            dsv = direct.dsvHeap->GetCPUDescriptorHandleForHeapStart();
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = tex->format;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = pass.depth.texture_view.base_level;
            direct.device->CreateDepthStencilView(tex->handle.Get(), &dsvDesc, dsv);
            if (pass.depth.clear || pass.stencil.clear)
            {
                D3D12_CLEAR_FLAGS flags = {};
                if (pass.depth.clear) flags |= D3D12_CLEAR_FLAG_DEPTH;
                if (pass.stencil.clear) flags |= D3D12_CLEAR_FLAG_STENCIL;
                direct.cmd->ClearDepthStencilView(dsv, flags, pass.depth.value, (UINT8)pass.stencil.value, 0, nullptr);
            }
            uint32_t mip = min(pass.depth.texture_view.base_level, 31u);
            width = max(width, max(1u, tex->width >> mip));
            height = max(height, max(1u, tex->height >> mip));
            hasDepth = true;
        }
    }

    uint32_t rtvCount = mod ? mod->colorCount : colorCount;
    direct.cmd->OMSetRenderTargets(rtvCount, rtvCount ? rtvs : nullptr, FALSE, hasDepth ? &dsv : nullptr);
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
        if (auto* view = dx_texture_view_native(color.texture_view.handle))
            if (auto texIt = direct.textures.find(view->texture); texIt != direct.textures.end())
                dx_transition_image(texIt->second, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    if (auto* view = dx_texture_view_native(pass.depth.texture_view.handle))
        if (auto texIt = direct.textures.find(view->texture); texIt != direct.textures.end())
            dx_transition_image(texIt->second, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    pass.handle = 0;
    pass.native = nullptr;
    direct.currentPassType = RT_MODULE_NONE;
    direct.currentPipeline = nullptr;
    for (auto& binding : direct.currentBinding)
        binding = {};
    direct.pushCount = 0;
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
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (direct.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
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
    auto handle = direct.passID + 1;
    auto& native = direct.transferPasses[handle];
    pass.handle = handle;
    pass.native = &native;
    direct.currentPassType = RT_MODULE_TRANSFER;
    direct.currentTransferPass = &pass;
    direct.passID = handle;
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
    direct.currentPassType = RT_MODULE_NONE;
    direct.currentPipeline = nullptr;
}

void dx_copy_buffer(rt_buffer_copy_t source, rt_buffer_copy_t destination, size_t copySize)
{
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (direct.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
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
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (direct.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
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

static void dx_fill_placed(D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint, dx_texture_native_t& tex, uint32_t bytesPerRow, rt_size_t copySize)
{
    footprint.Footprint.Format = tex.format;
    footprint.Footprint.Width = copySize.x;
    footprint.Footprint.Height = copySize.y;
    footprint.Footprint.Depth = copySize.z ? copySize.z : 1;
    footprint.Footprint.RowPitch = bytesPerRow ? bytesPerRow : (copySize.x * dx_format_bytes(tex.format));
}

void dx_copy_buffer_texture(rt_texture_copy_t source, rt_buffer_texel_t destination, rt_size_t copySize)
{
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (direct.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
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
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (direct.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
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
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (direct.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
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
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (direct.currentPassType != RT_MODULE_TRANSFER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
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
        result.vertex[0] = dx_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = dx_create_buffer({.size = vertex_count * 3 * sizeof(float), .usage = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = dx_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = dx_create_buffer({.size = index_count * sizeof(uint32_t), .usage = RT_BUFFER_USAGE_INDEX | RT_BUFFER_USAGE_COPY_DST, .data = indices});
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
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (direct.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    dx_flush_descriptors();
    rt_module_render_t const& module = direct.currentRenderPass->module;
    uint32_t vertex_count = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.format == RT_VERTEX_NONE) continue;
        for (uint32_t k = 0; k < std::size(mesh.vertex); ++k)
        {
            if (mesh.vertex[k].handle == 0 || mesh.location[k] != layout.location) continue;
            auto* native = dx_buffer_native(mesh.vertex[k]);
            if (!native) break;
            dx_transition_buffer(*native, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            uint32_t stride = rt_to_dx_vertex_size(layout.format);
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
        uint32_t indexStride = rt_to_dx_index_size(module.index_type);
        dx_transition_buffer(*native, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        D3D12_INDEX_BUFFER_VIEW view = {};
        view.BufferLocation = native->handle->GetGPUVirtualAddress();
        view.SizeInBytes = (UINT)mesh.index.size;
        view.Format = rt_to_dx_index_type(module.index_type);
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
        result.vertex[0] = dx_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = vertices});
    if (normals)
        result.vertex[1] = dx_create_buffer({.size = vertex_count * 4 * sizeof(float), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = normals});
    if (uvs)
        result.vertex[2] = dx_create_buffer({.size = vertex_count * 2 * sizeof(float), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = uvs});
    if (indices)
        result.index = dx_create_buffer({.size = index_count * sizeof(uint32_t), .usage = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_COPY_DST, .data = indices});
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
    if (direct.currentPipeline == nullptr)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    if (direct.currentPassType != RT_MODULE_RENDER)
    {
        fprintf(stderr, "Pipeline not begin");
        abort();
    }
    rt_module_render_t const& module = direct.currentRenderPass->module;
    uint32_t index_binding = 0;
    for (uint32_t i = 0; i < std::size(module.vertex); ++i)
    {
        rt_vertex_t const& layout = module.vertex[i];
        if (layout.format == RT_VERTEX_NONE) continue;
        for (uint32_t k = 0; k < std::size(meshlet.vertex); ++k)
        {
            if (meshlet.vertex[k].handle == 0 || meshlet.location[k] != layout.location) continue;
            dx_bind_buffer(meshlet.vertex[k], {.binding = layout.location, .target = RT_SHADER_STORAGE_BUFFER});
            break;
        }
        if (layout.location + 1 > index_binding)
            index_binding = layout.location + 1;
    }
    if (meshlet.index.handle)
        dx_bind_buffer(meshlet.index, {.binding = index_binding, .target = RT_SHADER_STORAGE_BUFFER});
    dx_flush_descriptors();
    auto* native = (dx_meshlet_native_t*)meshlet.native;
    uint32_t tasks = native && native->indexCount ? native->indexCount / 3 : 1;
    if (direct.cmdMesh)
        direct.cmdMesh->DispatchMesh(max(1u, tasks), 1, 1);
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
