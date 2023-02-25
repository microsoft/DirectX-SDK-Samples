//
// Wood Shader
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// Note: This effect file works with EffectEdit.
// Note: This effect file is SAS 1.0.1 compliant and will work with DxViewer.
//

int sas : SasGlobal
<
	bool SasUiVisible = false;
	int3 SasVersion= {1,1,0};
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
float3 lightDir 
<  
	bool SasUiVisible = false;
	string SasBindAddress= "Sas.DirectionalLight[0].Direction";
> = {0.577, -0.577, 0.577};

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
> = { 1.0f, 1.0f, 1.0f, 1.0f };    // diffuse

float4 I_s
<
	string SasUiLabel = "light specular";
	string SasUiControl = "ColorPicker";
> = { 0.6f, 0.6f, 0.6f, 1.0f };    // specular

// material reflectivity
float4 k_a
<
	string SasUiLabel = "material ambient";
	string SasUiControl = "ColorPicker";
> = { 0.2f, 0.2f, 0.2f, 1.0f };    // ambient

float4 k_d
<
	string SasUiLabel = "material diffuse";
	string SasUiControl = "ColorPicker";
> = { 1.0f, 0.7f, 0.2f, 1.0f };    // diffuse

float4 k_s
<
	string SasUiLabel = "material specular";
	string SasUiControl = "ColorPicker";
> = { 1.0f, 1.0f, 1.0f, 1.0f };    // specular

float  k_n
<
    string SasUiLabel = "Material Specular Power";
    string SasUiControl = "Slider"; 
    float SasUiMin = 1.0f; 
    float SasUiMax = 32.0f; 
    int SasUiSteps = 31;
>   = 8.0f;                         // power

#define DARK    0.3f
#define LIGHT   1.0f

// model dependent wood parameters
#define RINGSCALE   10.0f
#define POINTSCALE  2.0f
#define TURBULENCE  1.0f
#define VOLUME_SIZE 32
// textures

texture LinearTex 
< 
	string SasUiLabel = "Linear Map";
	string SasUiControl= "FilePicker";
	string SasResourceAddress = "../misc/linear.dds";
>;

texture NoiseTex  
< 
	string SasUiLabel = "Noise Map";
	string SasUiControl= "FilePicker";
	string SasResourceAddress = "../misc/noise3d.dds";
>;

float4 Linear(float2 Pos : POSITION) : COLOR
{
    return float4(Pos, Pos);
}

// function used to fill the volume noise texture
float4 GenerateNoise(float3 Pos : POSITION) : COLOR
{
    float4 Noise = (float4)0;

    for (int i = 1; i < 256; i += i)
    {
        Noise.r += abs(noise(Pos * 500 * i)) / i;
        Noise.g += abs(noise((Pos + 1)* 500 * i)) / i;
        Noise.b += abs(noise((Pos + 2) * 500 * i)) / i;
        Noise.a += abs(noise((Pos + 3) * 500 * i)) / i;
    }

    return Noise;
}

struct VS_OUTPUT
{
    float4 Pos  : POSITION;
    float3 Diff : COLOR0;
    float3 Spec : COLOR1;
    float3 Tex0 : TEXCOORD0;               
    float3 Tex1 : TEXCOORD1;               
    float2 Tex2 : TEXCOORD2;               
};

VS_OUTPUT VS(    
    float3 Pos  : POSITION,
    float3 Norm : NORMAL)
{
    VS_OUTPUT Out = (VS_OUTPUT)0;

    float3 L = -lightDir;
    float3 P = mul(float4(Pos, 1), World);    // position (view space)
    P = mul(float4(P, 1), View);    // position (view space)
    
    
    float3 N = mul(Norm, (float3x3)World);   // normal (view space)
    N = normalize(mul(N, (float3x3)View));   // normal (view space)
   
    
    float3 R = normalize(2 * dot(N, L) * N - L);            // reflection vector (view space)
    float3 V = -normalize(P);                               // view direction (view space)

    Out.Pos  = mul(float4(P, 1), Projection);             // position (projected)
    Out.Diff = I_a * k_a + I_d * k_d * max(0, dot(N, L)); // diffuse + ambient
    Out.Spec = I_s * k_s * pow(max(0, dot(R, V)), k_n/4);   // specular
    Out.Tex1 = 0.5 * Pos * POINTSCALE; 
    Out.Tex0 = Out.Tex1 * TURBULENCE;        
    Out.Tex2 = RINGSCALE * length(Out.Tex1.xz);

    return Out;
}

// samplers
sampler NoiseSamp<bool SasUiVisible = false;> = sampler_state 
{
    texture = <NoiseTex>;
    AddressU  = WRAP;		
    AddressV  = WRAP;
    AddressW  = WRAP;
    MIPFILTER = LINEAR;
    MINFILTER = LINEAR;
    MAGFILTER = LINEAR;
};

sampler LinearSamp<bool SasUiVisible = false;> = sampler_state 
{
    texture = <LinearTex>;
    AddressU  = WRAP;        
    AddressV  = WRAP;
    AddressW  = WRAP;
    MIPFILTER = LINEAR;
    MINFILTER = LINEAR;
    MAGFILTER = LINEAR;
};

// PS11 version is a gross approximation
float4 PS11(VS_OUTPUT In) : COLOR
{   
    float4 Color;
    float r;

    // note the use of a linear texture with wrapped texcoords to emulate 'frac'
    r = tex2D(LinearSamp, In.Tex2) +  0.1 * (tex3D(NoiseSamp, In.Tex1) - 0.5); 

    Color.rgb = In.Diff * lerp(DARK, LIGHT, saturate(0.8 - 0.6 * r))
              + In.Spec;
    Color.w   = 1;

    return Color;
}  

// PS20 is much better than PS11 because of dependent texture reads
float4 PS20(VS_OUTPUT In) : COLOR
{   
    float4 Color;
    float3 PP = In.Tex1 + 0.1 * (tex3D(NoiseSamp, frac(In.Tex1)) - 0.5);
    float r;

    r = RINGSCALE * length(PP.xz);
    r += 0.1 * tex3D(NoiseSamp, r);
    r = frac(r);

    Color.rgb = In.Diff * lerp(DARK, LIGHT, saturate(0.8 - 0.6 * r))
              + In.Spec;
    Color.w   = 1;

    return Color;
}  

technique TWood_PS_2_0
{
    pass P0
    {
        VertexShader = compile vs_1_1 VS();
        PixelShader  = compile ps_2_0 PS20();
    }
}

technique TWood_PS_1_1
{
    pass P0
    {
        VertexShader = compile vs_1_1 VS();
        PixelShader  = compile ps_1_1 PS11();
    }
}
