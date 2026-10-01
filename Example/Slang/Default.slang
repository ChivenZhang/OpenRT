struct VSIn
{
    float3 in_vertex : TEXCOORD0;
    float3 in_normal : TEXCOORD1;
    float2 in_uv : TEXCOORD2;
};

struct VSOut
{
    float4 position : SV_Position;
    float3 vertex : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float3 color : TEXCOORD3;
};

[shader("vertex")]
VSOut vertexMain(VSIn input, uint vertexID : SV_VertexID)
{
    float3 colors[3] = {float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1)};
    VSOut output;
    output.vertex = input.in_vertex;
    output.normal = input.in_normal;
    output.uv = input.in_uv;
    output.color = colors[vertexID];
    output.position = float4(input.in_vertex, 1.0);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOut input) : SV_Target0
{
    return float4(input.color, 1);
}
