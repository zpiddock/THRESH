struct VertexInput
{
    float2 in_position;
    float3 in_colour;
};

struct UniformBuffer
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};
ConstantBuffer<UniformBuffer> ubo;

struct VertexOutput
{
    float4 position : SV_Position;
    float3 colour;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.in_position, 0.0, 1.0))));
    output.colour = input.in_colour;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float3 colour = input.colour;
    return float4(colour, 1.0);
}