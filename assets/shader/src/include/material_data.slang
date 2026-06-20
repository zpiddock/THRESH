struct MaterialData {

        float4  base_colour_factor;
        float3  emissive_factor;
        float   metallic_factor;
        float   roughness_factor;
        float   normal_scale;
        float   occlusion_strength;
        float   alpha_cutoff;
        uint    base_colour_texture_handle;
        uint    normal_texture_handle;
        uint    emissive_texture_handle;
        uint    metallic_roughness_texture_handle; // G Chan: Roughness, B Chan: Metallic
        uint    occlusion_texture_handle;
        uint    flags;
        uint2   _pad; // pad to 80 bytes
    };