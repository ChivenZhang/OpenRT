#pragma once
// Generated from Example/Slang/Default.slang
// MSL from slangc -target metal
constexpr auto VS_ENTRY = "vertexMain";
constexpr auto FS_ENTRY = "fragmentMain";
constexpr auto VS = R"rt(
#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 1 "Example/Slang/Default.slang"
struct vertexMain_Result_0
{
    float4 position_0 [[position]];
    float3 vertex_0 [[user(TEXCOORD)]];
    float3 normal_0 [[user(TEXCOORD_1)]];
    float2 uv_0 [[user(TEXCOORD_2)]];
    float3 color_0 [[user(TEXCOORD_3)]];
};


#line 1
struct vertexInput_0
{
    float3 in_vertex_0 [[attribute(0)]];
    float3 in_normal_0 [[attribute(1)]];
    float2 in_uv_0 [[attribute(2)]];
};

struct VSOut_0
{
    float4 position_1;
    float3 vertex_1;
    float3 normal_1;
    float2 uv_1;
    float3 color_1;
};


#line 8
[[vertex]] vertexMain_Result_0 vertexMain(vertexInput_0 _S1 [[stage_in]], uint vertexID_0 [[vertex_id]])
{

#line 20
    array<float3, int(3)> colors_0 = { float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f), float3(0.0f, 0.0f, 1.0f) };
    thread VSOut_0 output_0;
    (&output_0)->vertex_1 = _S1.in_vertex_0;
    (&output_0)->normal_1 = _S1.in_normal_0;
    (&output_0)->uv_1 = _S1.in_uv_0;
    (&output_0)->color_1 = colors_0[vertexID_0];
    (&output_0)->position_1 = float4(_S1.in_vertex_0, 1.0f);

#line 26
    thread vertexMain_Result_0 _S2;

#line 26
    (&_S2)->position_0 = output_0.position_1;

#line 26
    (&_S2)->vertex_0 = output_0.vertex_1;

#line 26
    (&_S2)->normal_0 = output_0.normal_1;

#line 26
    (&_S2)->uv_0 = output_0.uv_1;

#line 26
    (&_S2)->color_0 = output_0.color_1;

#line 26
    return _S2;
}

)rt";
constexpr auto FS = R"rt(
#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 8 "Example/Slang/Default.slang"
struct pixelOutput_0
{
    float4 output_0 [[color(0)]];
};


#line 8
struct pixelInput_0
{
    float3 vertex_0 [[user(TEXCOORD)]];
    float3 normal_0 [[user(TEXCOORD_1)]];
    float2 uv_0 [[user(TEXCOORD_2)]];
    float3 color_0 [[user(TEXCOORD_3)]];
};


#line 31
[[fragment]] pixelOutput_0 fragmentMain(pixelInput_0 _S1 [[stage_in]], float4 position_0 [[position]])
{

#line 31
    pixelOutput_0 _S2 = { float4(_S1.color_0, 1.0f) };

    return _S2;
}

)rt";
