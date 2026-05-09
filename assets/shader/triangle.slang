struct VertexInput
{
    float3 in_position;
    float3 in_colour;
    float2 in_texcoord;
};

[vk::binding(0)]
struct UniformBuffer
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};
ConstantBuffer<UniformBuffer> ubo;

[vk::binding(2)]
struct CameraData {

    float4x4 view;
    float4x4 projection;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 colour;
    float2 texcoord;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.in_position, 1.0))));
    output.colour = input.in_colour;
    output.texcoord = input.in_texcoord;
    return output;
}

[vk::binding(1)]
Sampler2D textureSampler;

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return textureSampler.Sample(input.texcoord);
}