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


float Time <string SasBindAddress = "Sas.Time.Now"; bool SasUiVisible = false;>;
float4x4 World <string SasBindAddress = "Sas.Skeleton.MeshToJointToWorld[0]"; bool SasUiVisible = false;>;         
float4x4 View <string SasBindAddress = "Sas.Camera.WorldToView"; bool SasUiVisible = false;>;
float4x4 Projection <string SasBindAddress = "Sas.Camera.Projection"; bool SasUiVisible = false;>;
SasDirectionalLight DirectionalLight <string SasBindAddress = "Sas.DirectionalLight[0]"; bool SasUiVisible = false;>;


float ReflectionRatio < string SasUiControl = "Slider"; float SasUiMin = 0.0f; float SasUiMax = 1.0f; int SasUiSteps = 100;> = 0.05f;
float SpecularRatio < string SasUiControl = "Slider"; float SasUiMin = 0.0f; float SasUiMax = 1.0f; int SasUiSteps = 100;> = 0.15f;
float SpecularStyleLerp < string SasUiControl = "Slider"; float SasUiMin = 0.0f; float SasUiMax = 1.0f; int SasUiSteps = 100;> = 0.15f;
int SpecularPower < string SasUiControl = "Slider"; float SasUiMin = 1.0f; float SasUiMax = 32.0f; int SasUiSteps = 31; > = 8;


//-----------------------------------------------------------------------------------------
// Textures
//-----------------------------------------------------------------------------------------
texture DiffuseTexture <string SasResourceAddress = "../Misc/Earth_Diffuse.dds"; bool SasUiVisible = false;>;
texture NightTexture <string SasResourceAddress = "../Misc/Earth_Night.dds"; bool SasUiVisible = false;>;
texture NormalMapTexture <string SasResourceAddress = "../Misc/Earth_NormalMap.dds"; bool SasUiVisible = false;>;
texture ReflectionMask <string SasResourceAddress = "../Misc/Earth_ReflectionMask.dds"; bool SasUiVisible = false;>;

textureCUBE EnvironmentMap 
< 
	string SasBindAddress= "Sas.EnvironmentMap";
	string SasUiControl = "FilePicker"; 
>;

sampler DiffuseSampler< bool SasUiVisible = false; > = sampler_state
{ 
    Texture = (DiffuseTexture);
    MinFilter = Anisotropic;
    MagFilter = Linear;
    
    MaxAnisotropy = 4;
};

sampler NightSampler< bool SasUiVisible = false; > = sampler_state
{ 
    Texture = (NightTexture);
    MinFilter = Anisotropic;
    MagFilter = Linear;
    
    MaxAnisotropy = 4;
};

sampler NormalMapSampler< bool SasUiVisible = false; > = sampler_state
{ 
    Texture = (NormalMapTexture);
    MinFilter = Linear;
    MagFilter = Linear;
};

sampler EnvironmentMapSampler< bool SasUiVisible = false; > = sampler_state
{ 
    Texture = (EnvironmentMap);
    MinFilter = Linear;
    MagFilter = Linear;
};

sampler ReflectionMaskSampler< bool SasUiVisible = false; > = sampler_state
{ 
    Texture = (ReflectionMask);
    MinFilter = Linear;
    MagFilter = Linear;
};


//-----------------------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------------------
float4x4 RotationY( float angle )
{
    return float4x4( cos(angle), 0, -sin(angle), 0,
                              0, 1,           0, 0,
                     sin(angle), 0,  cos(angle), 0,
                              0, 0,           0, 1 );
}


//-----------------------------------------------------------------------------------------
// VertexShader I/O
//-----------------------------------------------------------------------------------------
struct VSInput
{
    float4 Position : POSITION; 
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 TexCoords : TEXCOORD0;
};

struct VSOutput
{
    float4 PositionVS : POSITION;
    float2 TexCoords : TEXCOORD0; 
    float3 Normal : TEXCOORD1;
    float3 Tangent : TEXCOORD2;
    float3 Binormal : TEXCOORD3;
    float3 PositionPS : TEXCOORD4;
};

struct PSInput
{
    float2 TexCoords : TEXCOORD0; 
    float3 Normal : TEXCOORD1;
    float3 Tangent : TEXCOORD2;
    float3 Binormal : TEXCOORD3;
    float3 Position : TEXCOORD4;
};

//-----------------------------------------------------------------------------------------
// Vertex Shader
//-----------------------------------------------------------------------------------------
VSOutput VS( VSInput input )
{
    VSOutput output;
   
    // Transform to clip space by multiplying by the basic transform matrices.
    // An additional rotation is performed to illustrate vertex animation.
    World = mul(RotationY(Time/10), World);
    float4 worldPosition = mul(input.Position, World);
    output.PositionVS = mul(worldPosition, mul(View, Projection));
    
    // Move the incoming normal and tangent into world space and compute the binormal.
    // These three axes will be used by the pixel shader to move the normal map from 
    // tangent space to world space. 
    output.Normal = mul(input.Normal, World);
    output.Tangent = mul(input.Tangent, World);
    output.Binormal = cross(output.Normal, output.Tangent);
    output.PositionPS = worldPosition.xyz;
     
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

    float3 EyeVector = normalize( input.Position - CameraPosition );

    
    // Look up the normal from the NormalMap texture, and unbias the result
    float3 Normal = tex2D(NormalMapSampler, input.TexCoords);
    Normal = (Normal * 2) - 1;
    
    // Move the normal from tangent space to world space
    float3x3 tangentFrame = {input.Tangent, input.Binormal, input.Normal};
    Normal = normalize(mul(Normal, tangentFrame));
    
    // Start with N dot L lighting
    float light = saturate( dot( Normal, -DirectionalLight.Direction ) );
    float3 color = DirectionalLight.Color * light;
    
    // Modulate against the diffuse texture color
    float4 diffuse = tex2D(DiffuseSampler, input.TexCoords);
    color *= diffuse.rgb;
    
    // Add ground lights if the area is not in sunlight
    float sunlitRatio = saturate(2*light);
    float4 nightColor = tex2D(NightSampler, input.TexCoords);
    color = lerp( nightColor.xyz, color, float3( sunlitRatio, sunlitRatio, sunlitRatio) );
    
    // Add the environment map
    float reflectionMask = tex2D(ReflectionMaskSampler, input.TexCoords);
    float3 reflection = normalize( reflect(EyeVector, Normal) );
    float3 environment = texCUBE(EnvironmentMapSampler, reflection);
    color += ReflectionRatio * reflectionMask * environment;
    
   // Add a specular highlight
    float3 vHalf = normalize( -EyeVector + -DirectionalLight.Direction );
    float PhongSpecular = saturate(dot(vHalf, Normal));
    float BlinnSpecular = saturate(dot(reflection ,-DirectionalLight.Direction ));
    float specular= lerp(PhongSpecular, BlinnSpecular, SpecularStyleLerp );

    color += DirectionalLight.Color * ( pow(specular, SpecularPower) * SpecularRatio * reflectionMask);
    
    
    // Add atmosphere
    float atmosphereRatio = 1 - saturate( dot(-EyeVector, input.Normal) );
    color += 0.30f * float3(.3, .5, 1) * pow(atmosphereRatio, 2);
    
    // Set alpha to 1.0 and return
    return float4(color, 1.0);
}


//-----------------------------------------------------------------------------------------
// Techniques
//-----------------------------------------------------------------------------------------
technique Earth
{
    pass P0
    {   
        VertexShader = compile vs_2_0 VS();
        PixelShader  = compile ps_2_0 PS(); 
    }
}



