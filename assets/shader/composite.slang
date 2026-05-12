import fullscreen;

[vk::binding(0, 0)]
Texture2D<float4> colour_target;


[shader("vertex")]
FullscreenVertex vertexMain(uint vert_id : SV_VertexID) {

    return fullscreen_vertex(vert_id);
}

[shader("fragment")]
float4 fragmentMain(FullscreenVertex fullscreen_vertex) : SV_Target {

    return colour_target.Load(int3(fullscreen_vertex.position.xy, 0));
}