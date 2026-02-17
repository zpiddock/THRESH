#version 450

// --- Inputs (standard Vertex: position, normal, uv, tangent) ---
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_tangent;

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

struct ObjectData {
    mat4 model;
    mat4 normal_matrix;
    uint material_index;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

layout(set = 0, binding = 1) readonly buffer ObjectSSBO {
    ObjectData objects[];
};

// --- Push constant: object index ---
layout(push_constant) uniform PushConstants {
    uint object_index;
};

// --- Outputs to fragment shader ---
layout(location = 0) out vec3 frag_world_pos;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_uv;
layout(location = 3) out mat3 frag_TBN;
layout(location = 6) out flat uint frag_material_index;

void main() {
    ObjectData obj = objects[object_index];

    vec4 world_pos = obj.model * vec4(in_position, 1.0);
    gl_Position = proj * view * world_pos;

    frag_world_pos = world_pos.xyz;
    frag_uv = in_uv;
    frag_material_index = obj.material_index;

    // Transform normal to world space using normal matrix
    vec3 N = normalize(mat3(obj.normal_matrix) * in_normal);
    frag_normal = N;

    // Construct TBN matrix for normal mapping
    vec3 T = normalize(mat3(obj.model) * in_tangent.xyz);
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt re-orthogonalize
    vec3 B = cross(N, T) * in_tangent.w;
    frag_TBN = mat3(T, B, N);
}
