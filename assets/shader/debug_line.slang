struct VertexIn {

    float3 position : POSITION;
    float3 colour : COLOR0;
}

struct VertexOut {
    float4 position : SV_Position;
    float3 colour : COLOR0;
}

struct PushConstants {
    float4x4 worldViewProjection;
}

[[vk::push_constant]] PushConstants push;

[shader("vertex")]
VertexOut vertexMain(VertexIn in_vertex) {
    VertexOut out_vertex;
    out_vertex.position = mul(push.worldViewProjection, float4(in_vertex.position, 1.0));
    out_vertex.colour = in_vertex.colour;
    return out_vertex;
}

[shader("fragment")]
float4 fragmentMain(VertexOut in_vertex) : SV_Target {

    return float4(in_vertex.colour, 1.0);
}