struct CameraData {

    float4x4 view;
    float4x4 projection;
}
static const int MAX_POINT_LIGHTS = 4; // MUST MATCH gpu_data.hpp

struct PointLight {

    float4 position;
    float4 colour;
}

struct LightData {

    float4     ambient;
    PointLight point_lights[MAX_POINT_LIGHTS];
    int        num_point_lights;
}