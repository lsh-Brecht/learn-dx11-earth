cbuffer constants : register(b0)
{
    matrix model_matrix;
    matrix view_projection_matrix;

    float4 light_position;
    float4 Ia;
    float4 Id;
    float4 Is;
    float4 Ka;
    float4 Kd;
    float4 Ks;
    
    float shininess;
    int mode;
    float2 pad1;
    float3 camera_position;
    float pad2;
}

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 world_pos : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    float4 worldPos = mul(model_matrix, float4(input.position, 1.0f));
    output.world_pos = worldPos.xyz;
    
    output.position = mul(view_projection_matrix, worldPos);
    
    output.normal = normalize(mul((float3x3) model_matrix, input.normal));
    
    output.texcoord = input.texcoord;
    
    return output;
}

Texture2D myTexture : register(t0);
SamplerState mySampler : register(s0);

float4 phong(float3 l, float3 n, float3 h, float4 current_Kd)
{
    float4 Ira = Ka * Ia;
    float4 Ird = max(current_Kd * dot(l, n) * Id, 0.0f);
    
    float4 Irs = max(Ks * pow(max(dot(h, n), 0.0f), shininess) * Is, 0.0f);
    
    return Ira + Ird + Irs;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float3 n = normalize(input.normal);
    float3 p = input.world_pos;
    
    float3 l;
    if (light_position.w == 0.0f) 
        l = normalize(light_position.xyz);
    else
        l = normalize(light_position.xyz - p);
        
    float3 v = normalize(camera_position - p);
    
    float3 h = normalize(l + v);
    
    float4 iKd = myTexture.Sample(mySampler, input.texcoord);
    
    float4 finalColor = float4(0, 0, 0, 1);
    
    if (mode == 0)
        finalColor = phong(l, n, h, iKd);
    else if (mode == 1)
        finalColor = phong(l, n, h, Kd);
    else if (mode == 2)
        finalColor = iKd;
    else
        finalColor = float4(input.texcoord, 0, 1);
    
    finalColor.a = 1.0f;
    return finalColor;
}