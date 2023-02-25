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

SasAmbientLight AmbientLight 
<
	string SasBindAddress = "Sas.AmbientLight[0]";
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

float3 MaterialAmbientColor 
<
	string SasUiControl = "ColorPicker"; 
> = { 0.3f, 0.3f, 0.3f };

float3 MaterialDiffuseColor
<
	string SasUiControl = "ColorPicker"; 
> = { 0.6f, 0.6f, 0.6f };

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
> = 16;

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


texture2D MaterialTexture;

bool IsMaterialTextured = false;

sampler2D MaterialTextureSampler
< bool SasUiVisible = false; > = sampler_state
{ 
    Texture = (MaterialTexture);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

textureCUBE EnvironmentMap 
< 
	string SasBindAddress = "Sas.EnvironmentMap";
>;

samplerCUBE EnvironmentSampler = sampler_state
{ 
    Texture = (EnvironmentMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR; 
    MagFilter = LINEAR; 
};

float3 CalcAmbient()
{
    return AmbientLight.Color * MaterialAmbientColor; 
}

float3 CalcDiffuse(float3 Normal)
{
    return MaterialDiffuseColor * DirectionalLight.Color * max(0, dot(Normal, -DirectionalLight.Direction));
    
}

float3 CalcSpecular(float3 eyeToVertex, float3 Normal)
{	
    float3 R = normalize(reflect(DirectionalLight.Direction, Normal));
	
	float specularTerm = pow( dot(Normal, R), MaterialSpecularPower);
	
	return MaterialSpecularColor * DirectionalLight.Color * specularTerm;    
}

float3 CalcReflectionVector(float3 ViewToPos, float3 Normal)
{
    return normalize(reflect(ViewToPos, Normal));
}


float3 CalcFresnel(float3 eyeToVertex, float3 Normal)
{
    float  fresnelFalloff = 1.0f-dot(normalize(Normal), -eyeToVertex);
    fresnelFalloff = pow( fresnelFalloff, 2);  
    
	float3 reflection = CalcReflectionVector( eyeToVertex, Normal);
    
    return texCUBE(EnvironmentSampler, reflection)* fresnelFalloff;
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
    Out.Diffuse = CalcAmbient() + CalcDiffuse(worldNormal);  
 
    return Out;
}



float4 PS(VS_OUTPUT input) : COLOR
{
	float3 texColor = {1,1,1};
	if(IsMaterialTextured)
		texColor= tex2D(MaterialTextureSampler, input.Texcoord); 

	input.Normal = normalize(input.Normal);

    float3 eyeToVertex = normalize(input.Position - CameraPos);
	
    float3 specular = CalcSpecular( eyeToVertex, input.Normal); 
    float3 fresnel = CalcFresnel( eyeToVertex, input.Normal);

	float3 color = {0,0,0};
	color += input.Diffuse*texColor;
	color += specular;
	color += fresnel;
	
    return float4( color, 1);

}

technique Hemisphere
{
    pass P0
    {
        VertexShader = compile vs_2_0 VS();
        PixelShader = compile ps_2_0 PS();
    }
}
