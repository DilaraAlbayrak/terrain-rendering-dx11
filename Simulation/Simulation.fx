cbuffer ConstantBufferCamera : register(b0)
{
    matrix Model;
    matrix View;
    matrix Projection;
    float4 eyePos;
}

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    float4 worldPos = float4(input.position, 1.0f);
    worldPos = mul(float4(input.position, 1.0f), Model);
    output.position = mul(worldPos, View);
    output.position = mul(output.position, Projection);
    output.normal = input.normal; 
    output.texcoord = input.texcoord;
    return output;
}

Texture2D diffuseMap : register(t0);
SamplerState samplerLinear : register(s0);

float4 PS(VS_OUTPUT input) : SV_Target
{
    float3 baseColor = diffuseMap.Sample(samplerLinear, input.texcoord).rgb;

    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(float3(0.5f, 1.0f, -0.2f)); 
    float diffuse = max(dot(normal, lightDir), 0.8f); 

    return float4(baseColor * diffuse, 1.0f);
    //return float4(0.5f, 0.5f, 0.5f, 1.0f);
}
