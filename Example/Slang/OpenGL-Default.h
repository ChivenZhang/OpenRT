#pragma once
// Generated from Example/Slang/Default.slang
// GLSL 460, checked with glslangValidator --client opengl100
constexpr auto VS = R"rt(
#version 460
layout(row_major) uniform;
layout(row_major) buffer;

#line 1 0
layout(location = 0)
out vec3 entryPointParam_vertexMain_vertex_0;


#line 1
layout(location = 1)
out vec3 entryPointParam_vertexMain_normal_0;


#line 1
layout(location = 2)
out vec2 entryPointParam_vertexMain_uv_0;


#line 1
layout(location = 3)
out vec3 entryPointParam_vertexMain_color_0;


#line 1
layout(location = 0)
in vec3 input_in_vertex_0;


#line 1
layout(location = 1)
in vec3 input_in_normal_0;


#line 1
layout(location = 2)
in vec2 input_in_uv_0;


#line 8
struct VSOut_0
{
    vec4 position_0;
    vec3 vertex_0;
    vec3 normal_0;
    vec2 uv_0;
    vec3 color_0;
};


void main()
{
    const vec3  colors_0[3] = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));
    VSOut_0 output_0;
    output_0.vertex_0 = input_in_vertex_0;
    output_0.normal_0 = input_in_normal_0;
    output_0.uv_0 = input_in_uv_0;
    output_0.color_0 = colors_0[uint(gl_VertexID)];
    output_0.position_0 = vec4(input_in_vertex_0, 1.0);
    VSOut_0 _S1 = output_0;

#line 27
    gl_Position = output_0.position_0;

#line 27
    entryPointParam_vertexMain_vertex_0 = _S1.vertex_0;

#line 27
    entryPointParam_vertexMain_normal_0 = _S1.normal_0;

#line 27
    entryPointParam_vertexMain_uv_0 = _S1.uv_0;

#line 27
    entryPointParam_vertexMain_color_0 = _S1.color_0;

#line 27
    return;
}

)rt";
constexpr auto FS = R"rt(
#version 460
layout(row_major) uniform;
layout(row_major) buffer;

#line 8 0
layout(location = 0)
out vec4 entryPointParam_fragmentMain_0;


#line 8
layout(location = 3)
in vec3 input_color_0;


#line 31
void main()
{

#line 31
    entryPointParam_fragmentMain_0 = vec4(input_color_0, 1.0);

#line 31
    return;
}

)rt";
