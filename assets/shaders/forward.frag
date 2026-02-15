#version 450

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

// TODO: Set 2 bindless textures will be added later
// For now, use material factors only (no texture sampling)

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

void main() {
    MaterialData mat = materials[frag_material_index];

    // Base material properties (factors only — texture sampling comes later)
    vec3 albedo = mat.base_color_factor.rgb;
    float metallic = mat.metallic_factor;
    float roughness = max(mat.roughness_factor, 0.04); // Avoid zero roughness
    vec3 emissive = mat.emissive_factor.rgb;

    // Use interpolated normal (normal mapping comes with textures)
    vec3 N = normalize(frag_normal);

    vec3 V = normalize(camera_pos - frag_world_pos);
    vec3 L = normalize(-sun_direction);
    vec3 H = normalize(V + L);

    // Dielectric/metallic F0
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = distribution_ggx(N, H, roughness);
    float G = geometry_smith(N, V, L, roughness);
    vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // Energy conservation
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * sun_color * sun_intensity * NdotL;

    // Ambient approximation (very simple — proper IBL comes later)
    vec3 ambient = vec3(0.03) * albedo;

    vec3 color = ambient + Lo + emissive;

    // Simple Reinhard tonemap
    color = color / (color + vec3(1.0));

    // Gamma correction (linear -> sRGB)
    color = pow(color, vec3(1.0 / 2.2));

    out_color = vec4(color, 1.0);
}
