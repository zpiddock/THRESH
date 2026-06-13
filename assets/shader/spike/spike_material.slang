// spike_material.slang — canary: MaterialData via BDA, uint -> DescriptorHandle
struct MaterialData {
    float4 base_color_factor;
    float3 emissive_factor;
    float  metallic_factor;
    float  roughness_factor;
    float  normal_scale;
    float  occlusion_strength;
    float  alpha_cutoff;
    uint   base_color_texture;
    uint   normal_texture;
    uint   metallic_roughness_texture;
    uint   emissive_texture;
    uint   occlusion_texture;
    uint   flags;
    uint2  _pad;
}

struct DrawPush {
    MaterialData*                  materials;
    DescriptorHandle<SamplerState> material_sampler;
    uint                           material_index;
}

[vk::push_constant] DrawPush draw;

[shader("fragment")]
float4 fragmentMain(float2 uv : TEXCOORD0) : SV_Target
{
    MaterialData mat = draw.materials[draw.material_index];
    let albedo = DescriptorHandle<Texture2D>(mat.base_color_texture);
    return albedo.Sample(*draw.material_sampler, uv) * mat.base_color_factor;
}