//-----------------------------------------------------------------------------------------
// Completed solution
// HLSL Hands-On Workshop, GDC 2005
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------------------
#include <sas\sas.fxh>

int GlobalParameter : SasGlobal                                                             
<
    int3 SasVersion = {1, 1, 0};
    
    string SasEffectDescription = "HLSL Hands-On Workshop: Completed solution";
    string SasEffectCompany = "Microsoft Corporation";
    bool SasUiVisible = false;
>;


//-----------------------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------------------
float4x4 World 
<
	string SasBindAddress = "Sas.Skeleton.MeshToJointToWorld[0]"; 
	bool SasUiVisible = false;
>;         

float4x4 View 
<
	string SasBindAddress = "Sas.Camera.WorldToView"; 
	bool SasUiVisible = false;
>;

float4x4 Projection 
<
	string SasBindAddress = "Sas.Camera.Projection"; 
	bool SasUiVisible = false;
>;

SasDirectionalLight DirectionalLight 
<
	string SasBindAddress = "Sas.DirectionalLight[0]"; 
	bool SasUiVisible = false;
>;

float3 CameraPos
<
	string SasBindAddress = "Sas.Camera.Position"; 
	bool SasUiVisible = false;
>;

float3 MaterialDiffuseColor
<
	string SasUiControl = "ColorPicker"; 
> = { 0.4f, 0.4f, 0.4f };

float3 MaterialSpecularColor
<
	string SasUiControl = "ColorPicker"; 
> = { 0.1f, 0.1f, 0.1f }; 

int    MaterialSpecularPower
< 
	string SasUiControl = "Slider"; 
	float SasUiMin = 1.0f; 
	float SasUiMax = 32.0f; 
	int SasUiSteps = 31; 
> = 8;

float3 DirFromSky 
< 
	string SasUiLabel = "Direction from Sky"; 
	string SasUiControl = "Direction"; 
> = { 0.0f, -1.0f, 0.0f };            

float3 GroundColor
< 
	string SasUiLabel = "Ground Color"; 
	string SasUiControl = "ColorPicker"; 
> = { 0.0f, 1.0f, 0.0f }; 

float3 SkyColor 
< 
	string SasUiLabel = "Ground Color"; 
	string SasUiControl = "ColorPicker"; 
> = { 0.0f, 0.0f, 1.0f };


float HemisphereContribution
< 
	string SasUiLabel = "Hemisphere Contribution Multiplier"; 
	string SasUiControl = "Slider"; 
> = 0.3;


texture MaterialTexture;
bool IsMaterialTextured = true;

sampler MaterialTextureSampler
< bool SasUiVisible = false; > = sampler_state
{ 
    Texture = (MaterialTexture);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};


//-----

float3 CalcHemisphere(float3 Normal, float Occ)
{

    //   What is a hemisphere light:
    //      A "hemisphere" light is an ambient term that is made up of
    //      two colors, sky and ground, that are interpolated between
    //      based on the normal.  i.e. areas with normals pointing towards
    //      the sky are "sky" colored, and areas pointed towards the 
    //      "ground" are more "ground" colored.
        
    // lerp between the ground and sky color based on the LerpFactor which 
    //   is calculated by the angle between the normal and the sky direction
    
    float LerpFactor = (dot(Normal, DirFromSky) + 1.0f) / 2.0f; 
    
    float3 Hemi = lerp(SkyColor, GroundColor, LerpFactor);
 
    //  What is the occlusion term:
    //    The occlusion term is a weight that was calculated in a offline
    //      pass of the model that is the percentage of the hemisphere that 
    //      is blocked by other parts of the model.  In the term provided
    //      a value of 0 means that the vertex is not occluded by any
    //      geometry (all light will be received) versus a value of 1
    //      which means that the vertex is completely blocked from light

    Hemi *= (1 - Occ);

    return Hemi;
}

float3 CalcDiffuse(float3 Normal)
{
    return MaterialDiffuseColor * DirectionalLight.Color * max(0, dot(Normal, -DirectionalLight.Direction));
    
}

float3 CalcSpecular(float3 Position, float3 Normal)
{
    float3 EyeToVertex = normalize(Position - CameraPos);
    float3 R = normalize(reflect(DirectionalLight.Direction, Normal));
    return MaterialSpecularColor * DirectionalLight.Color * pow(max(0, dot(R, -EyeToVertex)), MaterialSpecularPower);
}


struct VS_INPUT
{
    float3 Position  : POSITION; 
    float3 Normal : NORMAL; 
    float  Occlusion  : PSIZE;
    float2 Texcoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 PositionProjected  : POSITION;
    float2 Texcoord  : TEXCOORD0;     
    float3 Position  : TEXCOORD1;
    float3 Normal  : TEXCOORD2; 
    float3 Diffuse : COLOR0;
    float3 Hemi : COLOR1;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT Out = (VS_OUTPUT)0;

    // transform the position and normal
    float3 worldPosition = mul(float4(input.Position, 1), World);         // position (view space)
    float3 worldNormal = normalize(mul(input.Normal, (float3x3)World));   // normal (view space)
        
    Out.PositionProjected  = mul(float4(worldPosition, 1), mul( View, Projection) ); 
    Out.Texcoord = input.Texcoord;
    Out.Position = worldPosition;
    Out.Normal = worldNormal;

    // calculate all the possible lighting terms
    Out.Diffuse = CalcDiffuse(worldNormal); 
    Out.Hemi = CalcHemisphere(worldNormal, input.Occlusion); 
   
 
    return Out;
}

float4 PS(VS_OUTPUT input) : COLOR
{
	float3 texColor = {1,1,1};
	if(IsMaterialTextured)
		texColor= tex2D(MaterialTextureSampler, input.Texcoord); 

    float3 color =  (HemisphereContribution*input.Hemi) + (input.Diffuse*texColor*(1.0f-HemisphereContribution));

    return float4( ( color + CalcSpecular(input.Position, input.Normal) ), 1);

}

technique Hemisphere
{
    pass P0
    {
        VertexShader = compile vs_2_0 VS();
        PixelShader = compile ps_2_0 PS();
    }
}
