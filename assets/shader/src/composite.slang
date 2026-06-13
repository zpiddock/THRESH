import fullscreen;

[vk::binding(0, 0)]
Sampler2D<float4> colour_target;


[shader("vertex")]
FullscreenVertex vertexMain(uint vert_id : SV_VertexID) {

    return fullscreen_vertex(vert_id);
}

[shader("fragment")]
float4 fragmentMain(FullscreenVertex fullscreen_vertex) : SV_Target {

    return colour_target.Sample(fullscreen_vertex.uv);
}