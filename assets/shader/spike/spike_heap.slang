// spike_heap.slang — canary: DescriptorHandle + BDA pointer through push constants
struct CameraData { float4x4 view; float4x4 projection; }

struct FrameConstants {
    CameraData*                    camera;   // 8 bytes, PhysicalStorageBuffer pointer
    DescriptorHandle<Texture2D>    albedo;   // 8 bytes (uint2)
    DescriptorHandle<SamplerState> samp;     // 8 bytes (uint2)
}

[vk::push_constant] FrameConstants frame;

[shader("vertex")]
float4 vertexMain(float3 pos : POSITION) : SV_Position
{
    return mul(frame.camera.projection, mul(frame.camera.view, float4(pos, 1.0)));
}

[shader("fragment")]
float4 fragmentMain(float2 uv : TEXCOORD0) : SV_Target
{
    return frame.albedo.Sample(*frame.samp, uv);
}