//
// Metallic Flakes Shader
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// Note: This effect file is SAS 1.0.1 compliant and will work with DxViewer.
//
// NOTE:
// The metallic surface consists of 2 layers - 
// 1. a polished layer of wax on top (contributes a smooth specular reflection and an environment mapped reflection with a Fresnel term), and 
// 2. a blue metallic layer with a sprinkling of gold metallic flakes underneath


// sparkle parameters
#define SPRINKLE    1.0
#define SCATTER     0.5

#define VOLUME_NOISE_SCALE  10

int sas : SasGlobal
<
    bool SasUiVisible = false;
    int3 SasVersion= {1,1,0};
>;

// textures
texture EnvironmentMap 
< 
    string SasUiLabel = "Environment Map";
    string SasUiControl= "FilePicker";
>;

// procedural texture that contains a normal map used for the metal sparkles
texture3D NoiseMap 
< 
    string SasUiLabel = "Noise Map";
    string SasUiControl= "FilePicker";
    string SasResourceAddress = "../Misc/smallNoise3D.dds";
>;

// transformations
float4x4 World
<
    bool SasUiVisible = false;
    string SasBindAddress= "Sas.Skeleton.MeshToJointToWorld[0]";
>;

float4x4 View
<
    bool SasUiVisible = false;
    string SasBindAddress= "Sas.Camera.WorldToView";
>;

float4x4 Projection
<
    bool SasUiVisible = false;
    string SasBindAddress= "Sas.Camera.Projection";
>;

// light direction (view space)
float3 L 
< 
    bool SasUiVisible = false;
    string SasBindAddress= "Sas.DirectionalLight[0].Direction";
> = normalize(float3(-0.397f, -0.397f, 0.827f));

// light intensity
float4 I_a
<
    bool SasUiVisible = false;
    string SasBindAddress= "Sas.AmbientLight[0].Color";
> = { 0.3f, 0.3f, 0.3f, 1.0f };    // ambient

float4 I_d 
<
    bool SasUiVisible = false;
    string SasBindAddress= "Sas.DirectionalLight[0].Color";
> = { 0.0f, 0.0f, 0.0f, 1.0f };    // diffuse

float4 I_s
<
    string SasUiLabel = "light specular";
    string SasUiControl = "ColorPicker";
> = { 0.7f, 0.7f, 0.7f, 1.0f };    // specular

// material reflectivity
float4 k_a
<
    string SasUiLabel = "material ambient(metal)";
    string SasUiControl = "ColorPicker";
> = { 0.2f, 0.2f, 0.2f, 1.0f };    // ambient  (metal)

float4 k_d
<
    string SasUiLabel = "material diffuse(metal)";
    string SasUiControl = "ColorPicker";
> = { 0.1f, 0.1f, 0.9f, 1.0f };    // diffuse  (metal)

float4 k_s
<
    string SasUiLabel = "material specular(metal)";
    string SasUiControl = "ColorPicker";
> = { 0.6f, 0.5f, 0.1f, 1.0f };    // specular (metal)

float4 k_r
<
    string SasUiLabel = "material specular(wax)";
    string SasUiControl = "ColorPicker";
> = { 0.7f, 0.7f, 0.7f, 1.0f };    // specular (wax)

// function used to fill the volume noise texture
float4 GenerateSparkle(float3 Pos : POSITION) : COLOR
{
    float4 Noise = (float4)0;

    // set the normal with a probability based on SPRINKLE
    if (SPRINKLE >= abs(noise(Pos * 600)))
    {
        // scatter the normal (in vertex space) based on SCATTER
        Noise.rgb = float3(1 - SCATTER * abs(noise(Pos * 500)), SCATTER * noise((Pos + 1) * 500), SCATTER * noise((Pos + 2) * 500));
        Noise.rgb = normalize(Noise.rgb);
    }

    // bias the normal
    Noise.rgb = (Noise.rgb + 1)/2;

    // diffuse noise
    Noise.w = abs(noise(Pos * 500)) * 0.0 + 1.0;

    return Noise;
}

struct VS_OUTPUT
{
    float4 Position   : POSITION;
    float3 Diffuse    : COLOR0;
    float3 Specular   : COLOR1;               
    float3 Reflection : TEXCOORD0;               
    float3 NoiseCoord : TEXCOORD1;               
    float3 Glossiness : TEXCOORD2;               
    float3 HalfVector : TEXCOORD3;
};

// vertex shader
VS_OUTPUT VS(    
    float3 Position : POSITION,
    float3 Normal   : NORMAL, 
    float3 Tangent  : TANGENT)
{
    VS_OUTPUT Out = (VS_OUTPUT)0;

    L = -L;

    float3 P = mul(float4(Position, 1), (float4x3)World);   // position (view space)
    P = mul(float4(P, 1), (float4x3)View);   // position (view space)
    
    float3 N = mul(Normal, (float3x3)World);     // normal (view space)
    N = normalize(mul(N, (float3x3)View));     // normal (view space)
    
    float3 T = mul(Tangent, (float3x3)World);    // tangent (view space)
    T = normalize(mul(Tangent, (float3x3)View));    // tangent (view space)
    
    float3 B = cross(N, T);                                     // binormal (view space)
    float3 R = normalize(2 * dot(N, L) * N - L);                // reflection vector (view space)
    float3 V = -normalize(P);                                   // view direction (view space)
    float3 G = normalize(2 * dot(N, V) * N - V);                // glance vector (view space)
    float3 H = normalize(L + V);                                // half vector (view space)
    float  f = 0.5 - dot(V, N); f = 1 - 4 * f * f;              // fresnel term

    // position (projected)
    Out.Position = mul(float4(P, 1), Projection);             

    // diffuse + ambient (metal)
    Out.Diffuse = I_a * k_a + I_d * k_d * max(0, dot(N, L)); 

    // specular (wax)
    Out.Specular  = saturate(dot(H, N));
    Out.Specular *= Out.Specular;
    Out.Specular *= Out.Specular;
    Out.Specular *= Out.Specular;
    Out.Specular *= Out.Specular;                              
    Out.Specular *= Out.Specular;                        
    Out.Specular *= k_r;                                       

     // glossiness (wax)
    Out.Glossiness = f * k_r;

    // transform half vector into vertex space
    Out.HalfVector = float3(dot(H, N), dot(H, B), dot(H, T));   
    Out.HalfVector = (1 + Out.HalfVector) / 2;  // bias

    // environment cube map coordinates
    Out.Reflection = float3(-G.x, G.y, -G.z);                   

    // volume noise coordinates
    Out.NoiseCoord = Position * VOLUME_NOISE_SCALE;             

    return Out;
}

// samplers
sampler Environment<bool SasUiVisible = false;> = sampler_state
{ 
    Texture = (EnvironmentMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

sampler SparkleNoise<bool SasUiVisible = false;> = sampler_state
{ 
    Texture = (NoiseMap);
    MipFilter = LINEAR; 
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

// pixel shader
float4 PS(VS_OUTPUT In) : COLOR
{   
    float4 Color = (float4)0;
    float3 Diffuse, Specular, Gloss, Sparkle;

    // volume noise
    float4 Noise = tex3D(SparkleNoise, In.NoiseCoord);

    // noisy diffuse of metal
    Diffuse = In.Diffuse * Noise.a;
    
    // glossy specular of wax
    Specular  = In.Specular;
    Specular *= Specular;
    Specular *= Specular;
    
    // glossy reflection of wax 
    Gloss = texCUBE(Environment, In.Reflection) * saturate(In.Glossiness);              

    // specular sparkle of flakes
    Sparkle  = saturate(dot((saturate(In.HalfVector) - 0.5) * 2, (Noise.rgb - 0.5) * 2));
    Sparkle *= Sparkle;
    Sparkle *= Sparkle;
    Sparkle *= Sparkle;                                                                    
    Sparkle *= Sparkle;                                                                    
    Sparkle *= k_s;      

    // combine the contributions
    Color.rgb = Diffuse + Specular + Gloss + Sparkle;
    Color.w   = 1;

    return Color;
}  

// technique
technique TMetallicFlakes
{
    pass P0
    {
        VertexShader = compile vs_1_1 VS();
        PixelShader  = compile ps_1_1 PS();

        AlphaBlendEnable = FALSE;
        CullMode         = NONE;
    }
}

