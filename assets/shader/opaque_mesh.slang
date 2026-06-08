#include "include/common_structs.slang"

struct VertexInput
{
    float3 in_position;
    float3 in_normal;
    float2 in_texcoord;
};

[vk::binding(0, 0)]
ConstantBuffer<CameraData> camera_data;
[vk::binding(1, 0)]
Sampler2D textureSampler;
[vk::binding(2, 0)]
ConstantBuffer<LightData> light_data;
[vk::push_constant]
ConstantBuffer<PushConstants> push_constants;

struct VertexOutput
{
    float4 position : SV_Position;
    float3 normal;
    float2 texcoord;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(camera_data.projection, mul(camera_data.view, mul(push_constants.model, float4(input.in_position, 1.0))));
    output.normal = input.in_normal;
    output.texcoord = input.in_texcoord;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    // sampled albedo
    float4 albedo = textureSampler.Sample(input.texcoord) * push_constants.colour_tint;

    float3 N = normalize(input.normal);

    float3 ambient = light_data.ambient.rgb * light_data.ambient.a;

    // point lights
    for(int i = 0; i < light_data.num_point_lights; i++)
    {
        float3 to_light = light_data.point_lights[i].position.xyz - input.position.xyz;
        float dist2 = dot(to_light, to_light);
        float3 L = to_light / sqrt(dist2);
        float attenuation = 1.0 / dist2;
        float ndotl = max(dot(N, L), 0.0);
        ambient += light_data.point_lights[i].colour.rgb * light_data.point_lights[i].colour.a * ndotl * attenuation;
    };

    return float4(albedo.rgb * ambient, albedo.a);
}