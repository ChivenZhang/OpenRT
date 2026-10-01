#pragma once
// Generated from Example/Slang/Default.slang
// WGSL from slangc -target wgsl
constexpr auto VS_ENTRY = "vertexMain";
constexpr auto FS_ENTRY = "fragmentMain";
constexpr auto VS = R"rt(
struct VSOut_0
{
    @builtin(position) position_0 : vec4<f32>,
    @location(0) vertex_0 : vec3<f32>,
    @location(1) normal_0 : vec3<f32>,
    @location(2) uv_0 : vec2<f32>,
    @location(3) color_0 : vec3<f32>,
};

struct vertexInput_0
{
    @location(0) in_vertex_0 : vec3<f32>,
    @location(1) in_normal_0 : vec3<f32>,
    @location(2) in_uv_0 : vec2<f32>,
};

@vertex
fn vertexMain( _S1 : vertexInput_0, @builtin(vertex_index) vertexID_0 : u32) -> VSOut_0
{
    var colors_0 : array<vec3<f32>, i32(3)> = array<vec3<f32>, i32(3)>( vec3<f32>(1.0f, 0.0f, 0.0f), vec3<f32>(0.0f, 1.0f, 0.0f), vec3<f32>(0.0f, 0.0f, 1.0f) );
    var output_0 : VSOut_0;
    output_0.vertex_0 = _S1.in_vertex_0;
    output_0.normal_0 = _S1.in_normal_0;
    output_0.uv_0 = _S1.in_uv_0;
    output_0.color_0 = colors_0[vertexID_0];
    output_0.position_0 = vec4<f32>(_S1.in_vertex_0, 1.0f);
    return output_0;
}

)rt";
constexpr auto FS = R"rt(
struct pixelOutput_0
{
    @location(0) output_0 : vec4<f32>,
};

struct pixelInput_0
{
    @location(0) vertex_0 : vec3<f32>,
    @location(1) normal_0 : vec3<f32>,
    @location(2) uv_0 : vec2<f32>,
    @location(3) color_0 : vec3<f32>,
};

@fragment
fn fragmentMain( _S1 : pixelInput_0, @builtin(position) position_0 : vec4<f32>) -> pixelOutput_0
{
    var _S2 : pixelOutput_0 = pixelOutput_0( vec4<f32>(_S1.color_0, 1.0f) );
    return _S2;
}

)rt";
