import fullscreen;

struct CompositePushData {
    DescriptorHandle<Texture2D<float4>> colour_target;
    DescriptorHandle<SamplerState> sampler;
};

[vk::push_constant]
CompositePushData push_data;

[shader("vertex")]
FullscreenVertex vertexMain(uint vert_id : SV_VertexID) {

    return fullscreen_vertex(vert_id);
}

[shader("fragment")]
float4 fragmentMain(FullscreenVertex fullscreen_vertex) : SV_Target {

    return push_data.colour_target.Sample(*push_data.sampler, fullscreen_vertex.uv);
}