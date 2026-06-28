#include "include/frame_data.slang"
#include "include/material_data.slang"

struct VertexInput
{
    float3 in_position;
    float3 in_normal;
    float4 in_tangent;
    float2 in_texcoord;
};

struct PushConstants {
    // Frame data, offset 0
    CameraData*                    camera_data;
    LightData*                     light_data;
    MaterialData*                  materials;
    DescriptorHandle<SamplerState> sampler;
    // Draw Data, offset 32
    float4x4 model;
    float4   colour_tint;
    uint     material_index;
}

[vk::push_constant]
PushConstants push_constants;

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
    output.position = mul(push_constants.camera_data.projection, mul(push_constants.camera_data.view, mul(push_constants.model, float4(input.in_position, 1.0))));
    output.normal = input.in_normal;
    output.texcoord = input.in_texcoord;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    MaterialData material = push_constants.materials[push_constants.material_index];

    // sampled albedo
    let albedo_texture = DescriptorHandle<Texture2D<float4>>(material.base_colour_texture_handle);
    float4 albedo = albedo_texture.Sample(*push_constants.sampler, input.texcoord) * material.base_colour_factor;

    float3 N = normalize(input.normal);

    float3 ambient = push_constants.light_data.ambient.rgb * push_constants.light_data.ambient.a;

    // point lights
    for(int i = 0; i < push_constants.light_data.num_point_lights; i++)
    {
        float3 to_light = push_constants.light_data.point_lights[i].position.xyz - input.position.xyz;
        float dist2 = dot(to_light, to_light);
        float3 L = to_light / sqrt(dist2);
        float attenuation = 1.0 / dist2;
        float ndotl = max(dot(N, L), 0.0);
        ambient += push_constants.light_data.point_lights[i].colour.rgb * push_constants.light_data.point_lights[i].colour.a * ndotl * attenuation;
    };

    return float4(albedo.rgb * ambient, albedo.a);
}