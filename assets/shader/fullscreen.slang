public struct FullscreenVertex {

    public float4 position : SV_Position;
    public float2 uv : TEXCOORD0;
};

public FullscreenVertex fullscreen_vertex(uint vertexID : SV_VertexID) {

    FullscreenVertex output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}