//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

int global : SasGlobal
<
	bool SasUiVisible = false;
	int3 SasVersion = {1,0,0};
	string SasEffectDescription = "Anisotropic lighting using the Ashikhmin-Shirley reflectance model";
>;

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
float  PI = 3.141592654f;    // constant for PI

float4x4 World  : WORLD
<
	bool SasUiVisible = false;
	string SasBindAddress = "Sas.Skeleton.MeshToJointToWorld[0]";
>;

float4x4 View  : VIEW
<
	bool SasUiVisible = false;
	string SasBindAddress = "Sas.Camera.WorldToView";
>;

float4x4 Projection : PROJECTION
<
	bool SasUiVisible = false;
	string SasBindAddress = "Sas.Camera.Projection";
>; 


//-----------------------------------------------------------------------------
// UI variables
//-----------------------------------------------------------------------------
float3 LightDir
<
	bool SasUiVisible = false;
	string SasBindAddress = "Sas.DirectionalLight[0].Direction";
> = {0.577, -0.577, 0.577};

// UI slider for the PowerU
float Nu 
< 
	string SasUiLabel = "PowerU";
	string SasUiControl = "Slider";
	float SasUiMin = 1;
	float SasUiMax = 1000;
	int SasUiSteps = 100;
> = 800;

// UI slider for the PowerV
float Nv 
< 
	string SasUiLabel = "PowerV";
	string SasUiControl = "Slider";
	float SasUiMin = 1;
	float SasUiMax = 1000;
	int SasUiSteps = 100;
> = 40;


//-----------------------------------------------------------------------------
// Name: NormalTexture
// Type: texture
// Desc: Texture containing normals for model
//-----------------------------------------------------------------------------
texture2D NormalTexture
<
	string SasUiLabel = "Normal Map";
	string SasUiControl = "FilePicker";
>;
bool UseNormalMap = false;

//-----------------------------------------------------------------------------
// Name: NormalsSampler
// Type: sampler
// Desc: Trilinear sampler for "NormalTexture" textures
//-----------------------------------------------------------------------------
sampler2D NormalsSampler
< bool SasUiVisible = false; > = sampler_state
{ 
    Texture = (NormalTexture);
    MipFilter = LINEAR; 
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};




//-----------------------------------------------------------------------------
// Name: DiffuseTexture
// Type: texture
// Desc: Texture containing normals for model
//-----------------------------------------------------------------------------

texture2D MaterialTexture 
<
	string SasUiLabel = "Diffuse Map";
	string SasUiControl = "FilePicker";
>;

bool IsMaterialTextured = false;


float3 MaterialDiffuseColor
<
	string SasUiControl = "ColorPicker"; 
> = { 0.6f, 0.6f, 0.6f };


//-----------------------------------------------------------------------------
// Name: DiffuseSampler
// Type: sampler
// Desc: Trilinear sampler for "DiffuseTexture" textures
//-----------------------------------------------------------------------------
sampler2D DiffuseSampler
< bool SasUiVisible = false; > = sampler_state
{ 
    Texture = (MaterialTexture);
    MipFilter = LINEAR; 
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};



//-----------------------------------------------------------------------------
// Name: GlossTexture
// Type: texture
// Desc: Texture containing normals for model
//-----------------------------------------------------------------------------
texture2D GlossTexture
<
	string SasUiLabel = "Gloss Map";
	string SasUiControl = "FilePicker";
>;
bool UseGlossMap = false;

float3 MaterialSpecularColor
<
	string SasUiControl = "ColorPicker"; 
> = { 0.1f, 0.1f, 0.1f }; 

//-----------------------------------------------------------------------------
// Name: GlossSampler
// Type: sampler
// Desc: Trilinear sampler for "GlossTexture" textures
//-----------------------------------------------------------------------------
sampler2D GlossSampler
< bool SasUiVisible = false; > = sampler_state
{ 
    Texture = (GlossTexture);
    MipFilter = LINEAR; 
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};



//-----------------------------------------------------------------------------
// Vertex shader input structure
//-----------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float3 Tangent  : TANGENT0;
};


//-----------------------------------------------------------------------------
// Vertex shader output structure
//-----------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Position  : POSITION;
    float2 TexCoord  : TEXCOORD0;
    float3 LightVec  : TEXCOORD1;
    float3 ViewVec   : TEXCOORD2;
};


//-----------------------------------------------------------------------------
// Name: AnisoVS
// Type: Vertex shader
// Desc: Transforms the vertex position into screen space and 
//       transforms the light and view vector into tangent space
//-----------------------------------------------------------------------------
VS_OUTPUT AnisoVS( VS_INPUT Input )
{
    VS_OUTPUT Output;

	float4x4 WorldView = mul( World, View);
	float4x4 WorldViewProjection = mul( WorldView, Projection);


    // Transform the vertex position into screen space
    Output.Position = mul(Input.Position, WorldViewProjection);
    
    // Copy through the texture coordinates
    Output.TexCoord = Input.TexCoord;

    // Tranform the position into camera space
    float3 Pos = mul( Input.Position, WorldView );

    // In camera space the camera is at <0,0,0>, therefore
    // then the direction towards the camera is <0,0,0> - Pos, normalized.
    float3 ViewDir = -normalize(Pos);
    
    // Setup the inverse transform matrix
    float3 ViewSpaceNormal  = mul( Input.Normal, (float3x3)WorldView );
    float3 ViewSpaceTangent = mul( Input.Tangent, (float3x3)WorldView );
    float3 Binorm = cross( ViewSpaceNormal, ViewSpaceTangent );
    float3x3 InvTrans = float3x3( ViewSpaceTangent, Binorm, ViewSpaceNormal );
    
    // Traspose the matrix
    InvTrans = transpose(InvTrans);

    // Transform the light dir and view dir by the matrix
    Output.LightVec = mul(-LightDir,InvTrans);
    Output.ViewVec  = mul(ViewDir,InvTrans); 

    return Output;
}


//-----------------------------------------------------------------------------
// Name: Pd
// Type: function
// Desc: Diffuse term for the Ashikhmin-Shirley model 
//-----------------------------------------------------------------------------
float3 Pd(float3 N, float3 L, float3 V, float3 H, float3 Rd, float3 Rs)
{
    // Goal 1: Implement Pd
    //
    // Easy one.  Simply type the equation in that is shown on the slide.
    //

    float3 value = (28/(23*PI)) * Rd * (1-Rs) *      // Color calculation
                   (1 - pow(1-dot(N,V)/2,5))  *      // View dependent term
                   (1 - pow(1-dot(N,L)/2,5));        // Light dependent term
                   
    return value;
} 


//-----------------------------------------------------------------------------
// Name: F
// Type: function
// Desc: helper function to implement Ps.  This is the Shlick Fresnel 
//       approximation
//-----------------------------------------------------------------------------
float3 F(float CosTerm, float3 Rs)
{
    return (Rs + (1 - Rs)*pow(1 - CosTerm,1));   
}


//-----------------------------------------------------------------------------
// Name: E
// Type: function
// Desc: helper function to to implement Ps. 
//       This is the exponent function which is the heart of Anisotropic lighting.
//-----------------------------------------------------------------------------
float E(float3 N, float3 H, float3 TU, float3 TV)
{
    float val = Nu * pow(dot(H, TU),2) + Nv * pow(dot(H, TV),2);
    val /= ( 1.001 - pow(saturate(dot(H, N)), 2) );

    return val;
}


//-----------------------------------------------------------------------------
// Name: Ps
// Type: function
// Desc: Specular term for the Ashikhmin-Shirley model 
//-----------------------------------------------------------------------------
float3 Ps(float3 N, float3 H, float3 L, float3 V,float3 TU, float3 TV, float3 Rs)
{
    // Goal 2: Implement Ps
    //
    // Convert the equation that appears on the slide to HLSL.
    //
    // Suggestions:
    //      You will need to use the sqrt(), pow(), and max() intrisic functions.
    //      The functions for E() and F() have already been supplied for you above.  
    //      Note that the <A,B> notation that appears on the slide means dot(A,B)    

    float3 LeftSide = sqrt( (Nu + 1) * (Nv + 1) ) / (8*PI);

    float3 RightSide = pow( dot(N,H), E(N,H,TU,TV) ) * F(dot(L,H), Rs) /
                      ( dot(L,H) * max( dot(N,L), dot(N,V) ) );

    return LeftSide * RightSide;
}




//-----------------------------------------------------------------------------
// Name: BumpMapPS
// Type: Pixel shader
// Desc: Computes a Ashikhmin-Shirley BRDF model per pixel
//-----------------------------------------------------------------------------
float4 AnisoPS( VS_OUTPUT Input ) : COLOR
{
    float3 Normal = {0,0,1};
    if(UseNormalMap)
		Normal = tex2D(NormalsSampler, Input.TexCoord) * 2 - 1 ; 

    float3 GlossMap = MaterialSpecularColor;
	if(UseGlossMap)
		GlossMap = tex2D(GlossSampler, Input.TexCoord);

    float3 Albedo = MaterialDiffuseColor;
    if(IsMaterialTextured)
		Albedo = tex2D(DiffuseSampler, Input.TexCoord);

    float3 Half = normalize(Input.LightVec + Input.ViewVec);

    // Tangent and Binormal could also vary per pixel just like Normal does
    float3 Tangent = float3(1,0,0);
    float3 Binormal = cross(Normal, Tangent);

    float3 LightColor = float3(5.0f,5.0f,5.0f);
    float3 Rd = Albedo;               // Diffuse reflectance 
    float3 Rs = float3(0.2,0.2,0.2);  // Specular reflectance

    // Compute Anisotropic BRDF using the Ashikhmin-Shirley model 
    float3 Color = Ps(Normal, Half, Input.LightVec, Input.ViewVec, Tangent, Binormal, Rs) * GlossMap + 
                   Pd(Normal, Input.LightVec, Input.ViewVec, Half, Rd, Rs);  
                   
    // All BRDFs must multiplied by dot(N,L)
    Color *= saturate(dot(Normal, Input.LightVec)) * LightColor;

    return float4( Color, 0 );
}


//-----------------------------------------------------------------------------
// Name: Aniso
// Type: Technique                                     
// Desc: Technique that renders with Anisotropic BRDF
//-----------------------------------------------------------------------------
technique Aniso
{
    pass p0
    {
        vertexshader = compile vs_2_0 AnisoVS();
        pixelshader  = compile ps_2_0 AnisoPS();
        
        AlphaBlendEnable = FALSE;       
    }
}


