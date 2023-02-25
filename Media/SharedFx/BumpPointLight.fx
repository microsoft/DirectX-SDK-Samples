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
float4x4 World <string SasBindAddress = "Sas.Skeleton.MeshToJointToWorld[0]"; bool SasUiVisible = false;>;         
float4x4 View <string SasBindAddress = "Sas.Camera.WorldToView"; bool SasUiVisible = false;>;
float4x4 Projection <string SasBindAddress = "Sas.Camera.Projection"; bool SasUiVisible = false;>;
SasPointLight PointLight <string SasBindAddress = "Sas.PointLight[0]"; bool SasUiVisible = false;>;

bool UseNormalMaps<bool SasUiVisible = false;> = true;
float reflectionRatio < string SasUiControl = "Slider"; float SasUiMin = 0.0f; float SasUiMax = 1.0f; int SasUiSteps = 100;> = 0.1f;

float SpecularRatio < string SasUiControl = "Slider"; float SasUiMin = 0.0f; float SasUiMax = 1.0f; int SasUiSteps = 100;> = 0.15f;
float SpecularStyleLerp < string SasUiControl = "Slider"; float SasUiMin = 0.0f; float SasUiMax = 1.0f; int SasUiSteps = 100;> = 0.15f;

int SpecularPower < string SasUiControl = "Slider"; float SasUiMin = 1.0f; float SasUiMax = 32.0f; int SasUiSteps = 31; > = 8;

//-----------------------------------------------------------------------------------------
// Textures
//-----------------------------------------------------------------------------------------
texture DiffuseTexture < string SasUiControl = "FilePicker"; >;
texture NormalMapTexture < string SasUiControl = "FilePicker"; >;

textureCUBE EnvironmentMap 
< 
	string SasBindAddress= "Sas.EnvironmentMap";
	string SasUiControl = "FilePicker"; 
>;

sampler DiffuseSampler = sampler_state
{ 
    Texture = (DiffuseTexture);
    MinFilter = Anisotropic;
    MagFilter = Linear;
    
    MaxAnisotropy = 4;
};

sampler NormalMapSampler = sampler_state
{ 
    Texture = (NormalMapTexture);
    MinFilter = Linear;
    MagFilter = Linear;
};

sampler EnvironmentMapSampler = sampler_state
{ 
    Texture = (EnvironmentMap);
    MinFilter = Linear;
    MagFilter = Linear;
};

//-----------------------------------------------------------------------------------------
// VertexShader I/O
//-----------------------------------------------------------------------------------------
struct VSInput
{
    float4 Position : POSITION; 
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Binormal : BINORMAL;
    float2 TexCoords : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : POSITION; 
    float2 TexCoords : TEXCOORD0; 
    float3 Normal : TEXCOORD2;
    float3 Tangent : TEXCOORD3;
    float3 Binormal : TEXCOORD4;
    float3 PositionForPixelShader : TEXCOORD5;
};


struct PSInput
{
    float2 TexCoords : TEXCOORD0; 
    float3 Normal : TEXCOORD2;
    float3 Tangent : TEXCOORD3;
    float3 Binormal : TEXCOORD4;
    float3 Position : TEXCOORD5;
};

//-----------------------------------------------------------------------------------------
// Vertex Shader
//-----------------------------------------------------------------------------------------
VSOutput VS( VSInput input )
{
    VSOutput output;
   
    // Transform to clip space by multiplying by the basic transform matrices.
    // An additional rotation is performed to illustrate vertex animation.
    float4 worldPosition = mul(input.Position, World);
    output.PositionForPixelShader = worldPosition.xyz;
    output.Position = mul(worldPosition, mul(View, Projection));
   
    // Move the incoming normal and tangent into world space and compute the binormal.
    // These three axes will be used by the pixel shader to move the normal map from 
    // tangent space to world space. 
    output.Normal = mul(input.Normal, World);
    output.Tangent = mul(input.Tangent, World);
    output.Binormal = mul(input.Binormal, World);// cross(output.Normal, output.Tangent);
     
    // Pass texture coordinates on to the pixel shader
    output.TexCoords = input.TexCoords;
    return output;    
}


//-----------------------------------------------------------------------------------------
// Pixel Shader
//-----------------------------------------------------------------------------------------
float4 PS( PSInput input ) : COLOR
{ 
	float4x4 ViewInv = inverse(View);
	float3 CameraPosition = mul(float4(0,0,0,1), ViewInv);

	
    float3 LightDir = normalize(input.Position - PointLight.Position); //DirectionalLight.Direction
    float3 EyeVector = normalize( input.Position - CameraPosition );//vector from eye to vertex
 

    // Look up the normal from the NormalMap texture, and unbias the result
    float3 Normal;

    if(UseNormalMaps)
    {
        Normal = tex2D(NormalMapSampler, input.TexCoords);
        Normal = (Normal * 2) - 1;
    
        // Move the normal from tangent space to world space
        float3x3 tangentFrame = {input.Tangent, input.Binormal, input.Normal};
    
        Normal = normalize(mul(Normal, tangentFrame)); //input.Normal; //
    }
    else
    {
        Normal = normalize( input.Normal ); 
    }
    
    // Start with N dot L lighting
    float light = saturate( dot( Normal, -LightDir) );
    float3 color = PointLight.Color * light;
    
    // Modulate against the diffuse texture color
    float4 diffuse = tex2D(DiffuseSampler, input.TexCoords);
    color *= diffuse.rgb;
   
 
    float3 reflection = normalize( reflect(EyeVector, Normal) );
    float3 environment = texCUBE(EnvironmentMapSampler, reflection);
    color += reflectionRatio * environment;

  
    // Add a specular highlight
    float3 vHalf = normalize( -EyeVector + -LightDir );
    float PhongSpecular = saturate(dot(vHalf, Normal));
    float BlinnSpecular = saturate(dot(reflection ,-LightDir ));
    float specular= lerp(PhongSpecular, BlinnSpecular, SpecularStyleLerp );

    color += PointLight.Color * ( pow(specular, SpecularPower) * SpecularRatio);
     
    // Set alpha to 1.0 and return
    return float4(color, 1.0);
}


//-----------------------------------------------------------------------------------------
// Techniques
//-----------------------------------------------------------------------------------------
technique Bump
{
    pass P0
    {   
        VertexShader = compile vs_2_0 VS();
        PixelShader  = compile ps_2_0 PS(); 
    }
}



