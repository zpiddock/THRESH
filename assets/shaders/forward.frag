#version 450
#extension GL_EXT_nonuniform_qualifier : require

// --- Inputs from vertex shader ---
layout(location = 0) in vec3 frag_world_pos;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec2 frag_uv;
layout(location = 3) in mat3 frag_TBN;
layout(location = 6) in flat uint frag_material_index;

// --- Set 0: Per-frame global data ---
layout(set = 0, binding = 0) uniform FrameUBO {
    mat4 view;
    mat4 proj;
    vec3 camera_pos;
    float _pad0;
    vec3 sun_direction;
    float sun_intensity;
    vec3 sun_color;
    float _pad1;
    vec3 ambient_color;
    float ambient_intensity;
    uint point_light_count;
    float _pad2[3];
};

struct MaterialData {
    vec4 base_color_factor;
    float metallic_factor;
    float roughness_factor;
    float normal_scale;
    float occlusion_strength;
    vec4 emissive_factor;
    int albedo_tex_index;
    int normal_tex_index;
    int metallic_roughness_tex_index;
    int emissive_tex_index;
};

layout(set = 0, binding = 2) readonly buffer MaterialSSBO {
    MaterialData materials[];
};

struct PointLightData {
    vec3 position;
    float radius;
    vec3 color;
    float intensity;
};

layout(set = 0, binding = 3) readonly buffer PointLightSSBO {
    PointLightData point_lights[];
};

// --- Set 1: Bindless textures ---
layout(set = 1, binding = 0) uniform sampler tex_sampler;
layout(set = 1, binding = 1) uniform texture2D textures[];

// --- Output ---
layout(location = 0) out vec4 out_color;

// --- PBR Constants ---
const float PI = 3.14159265359;

// --- PBR Functions ---

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float distribution_ggx(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0001);
}

// Geometry function (Smith's method with Schlick-GGX)
float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = geometry_schlick_ggx(NdotV, roughness);
    float ggx2 = geometry_schlick_ggx(NdotL, roughness);
    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
vec3 fresnel_schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

// Compute direct lighting contribution for a single light direction.
// Returns the outgoing radiance (diffuse + specular) multiplied by NdotL.
vec3 compute_direct_lighting(
    vec3 N, vec3 V, vec3 L,
    vec3 albedo, float metallic, float roughness, vec3 F0
) {
    vec3 H = normalize(V + L);

    float NDF = distribution_ggx(N, H, roughness);
    float G = geometry_smith(N, V, L, roughness);
    vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * NdotL;
}

void main() {
    MaterialData mat = materials[frag_material_index];

    // --- Albedo ---
    vec4 albedo4 = mat.base_color_factor;
    if (mat.albedo_tex_index >= 0) {
        albedo4 *= texture(sampler2D(textures[nonuniformEXT(mat.albedo_tex_index)], tex_sampler), frag_uv);
    }
    vec3 albedo = albedo4.rgb;

    // --- Metallic / Roughness ---
    float metallic = mat.metallic_factor;
    float roughness = mat.roughness_factor;
    if (mat.metallic_roughness_tex_index >= 0) {
        vec4 mr = texture(sampler2D(textures[nonuniformEXT(mat.metallic_roughness_tex_index)], tex_sampler), frag_uv);
        // glTF spec: G = roughness, B = metallic
        metallic *= mr.b;
        roughness *= mr.g;
    }
    roughness = max(roughness, 0.04);

    // --- Normal mapping ---
    vec3 N;
    if (mat.normal_tex_index >= 0) {
        vec3 tangent_normal = texture(sampler2D(textures[nonuniformEXT(mat.normal_tex_index)], tex_sampler), frag_uv).rgb;
        tangent_normal = tangent_normal * 2.0 - 1.0;
        tangent_normal.xy *= mat.normal_scale;
        N = normalize(frag_TBN * tangent_normal);
    } else {
        N = normalize(frag_normal);
    }

    vec3 V = normalize(camera_pos - frag_world_pos);

    // --- Emissive ---
    vec3 emissive = mat.emissive_factor.rgb;
    if (mat.emissive_tex_index >= 0) {
        emissive *= texture(sampler2D(textures[nonuniformEXT(mat.emissive_tex_index)], tex_sampler), frag_uv).rgb;
    }

    // Dielectric/metallic F0
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Accumulate lighting
    vec3 Lo = vec3(0.0);

    // --- Directional light (sun) ---
    {
        vec3 L = normalize(-sun_direction);
        Lo += compute_direct_lighting(N, V, L, albedo, metallic, roughness, F0)
              * sun_color * sun_intensity;
    }

    // --- Point lights ---
    for (uint i = 0; i < point_light_count; i++) {
        PointLightData pl = point_lights[i];

        vec3 light_vec = pl.position - frag_world_pos;
        float dist = length(light_vec);

        // Skip lights beyond their radius
        if (dist > pl.radius) continue;

        vec3 L = normalize(light_vec);

        // Inverse-square attenuation with +1 to avoid singularity at dist=0
        float attenuation = 1.0 / (dist * dist + 1.0);

        // Smooth distance falloff at radius boundary (UE4-style)
        float ratio = dist / pl.radius;
        float falloff = clamp(1.0 - ratio * ratio * ratio * ratio, 0.0, 1.0);
        falloff = falloff * falloff;
        attenuation *= falloff;

        Lo += compute_direct_lighting(N, V, L, albedo, metallic, roughness, F0)
              * pl.color * pl.intensity * attenuation;
    }

    // Configurable ambient
    vec3 ambient = ambient_color * ambient_intensity * albedo;

    vec3 color = ambient + Lo + emissive;

    // Simple Reinhard tonemap
    color = color / (color + vec3(1.0));

    // Gamma correction (linear -> sRGB)
    color = pow(color, vec3(1.0 / 2.2));

    out_color = vec4(color, 1.0);
}
