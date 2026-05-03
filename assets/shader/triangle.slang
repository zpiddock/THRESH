struct VertexInput
{
    float2 in_position;
    float3 in_colour;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 colour;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.in_position, 0.0, 1.0);
    output.colour = input.in_colour;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float3 colour = input.colour;
    return float4(colour, 1.0);
}