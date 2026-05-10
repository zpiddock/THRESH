struct VertexInput
{
    float3 in_position;
    float3 in_colour;
    float2 in_texcoord;
};

struct CameraData {

    float4x4 view;
    float4x4 projection;
};

struct PushConstants
{
    float4x4 model;
    float4 base_colour;
};

[vk::binding(0, 0)]
ConstantBuffer<CameraData> camera_data;
[vk::push_constant]
ConstantBuffer<PushConstants> push_constants;

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
    output.position = mul(camera_data.projection, mul(camera_data.view, mul(push_constants.model, float4(input.in_position, 1.0))));
    output.colour = input.in_colour;
    output.texcoord = input.in_texcoord;
    return output;
}

[vk::binding(1, 0)]
Sampler2D textureSampler;

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return textureSampler.Sample(input.texcoord) * push_constants.base_colour;
}