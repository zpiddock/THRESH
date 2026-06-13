struct CameraData {

    float4x4 view;
    float4x4 projection;
};

static const int MAX_LIGHTS = 4;

struct PointLight {

    float4 position;
    float4 colour;
}

struct LightData {

    float4 ambient;
    PointLight point_lights[MAX_LIGHTS];
    int num_point_lights;
}

struct PushConstants {

    float4x4 model;
    float4 colour_tint;
};