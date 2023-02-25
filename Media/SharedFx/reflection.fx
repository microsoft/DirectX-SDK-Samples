//-----------------------------------------------------------------------------
// File: Reflect.fx
//
//COMPATABILITY:  This file works with SAS 1.0.1 compliant applications.  
//This will not work with MView or EffectEdit.
//Please use DxViewer.
//
//       Texture mapping
//       Diffuse lighting
//       Specular lighting
//       Environment mapping
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

int sas : SasGlobal
<
    bool SasUiVisible = false;
    int3 SasVersion = {1, 1, 0};
>;


float4x4 g_mWorld
<
    bool SasUiVisible = false;
    string SasBindAddress= "Sas.Skeleton.MeshToJointToWorld[0]";
>;     

float4x4 g_mView
<
    bool SasUiVisible = false;
    string SasBindAddress= "Sas.Camera.WorldToView";
>;               // View matrix for object

float4x4 g_mProj
<
    bool SasUiVisible = false;
    string SasBindAddress= "Sas.Camera.Projection";
>;   

float4 g_vLightColor 
<  
    bool SasUiVisible = false;
    string SasBindAddress= "Sas.SpotLight[0].Color";
> = {1.0f, 1.0f, 1.0f, 1.0f}; // Light value

float3 g_vLight 
<  
    bool SasUiVisible = false;
    string SasBindAddress= "Sas.SpotLight[0].Position";
>;                                 // Light position in view space

                               // Time value

// Object material attributes
float4 Diffuse
<
    string SasUiLabel = "Material Diffuse";
    string SasUiControl = "ColorPicker";
>;      // Diffuse color of the material

float4 Specular
<
    string SasUiLabel = "Material Specular";
    string SasUiControl = "ColorPicker";
> = {1.0f, 1.0f, 1.0f, 1.0f};  // Specular color of the material

float  Power
<
    string SasUiLabel = "Material Specular Power";
    string SasUiControl = "Slider"; 
    float SasUiMin = 1.0f; 
    float SasUiMax = 32.0f; 
    int SasUiSteps = 31;
> = 8.0f;


texture2D MaterialTexture 
<
    string SasUiLabel = "Color Texture";
    string SasUiControl= "FilePicker";
>;


bool IsMaterialTextured  = false;


textureCUBE g_txEnvMap 
<
	string SasBindAddress = "Sas.EnvironmentMap";
	bool SasUiVisible = false;
>;


float  Reflectivity
<
    string SasUiLabel = "Material Reflectivity";
    string SasUiControl = "Slider"; 
    float SasUiMin = 0.0f; 
    float SasUiMax = 1.0f; 
    int SasUiSteps = 100;
> = 0.5f; // Reflectivity of the material

//-----------------------------------------------------------------------------
// Texture samplers
//-----------------------------------------------------------------------------
sampler MaterialSampler<bool SasUiVisible = false;> =
sampler_state
{
    Texture = <MaterialTexture>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = None;
};

samplerCUBE g_samEnv<bool SasUiVisible = false;> =
sampler_state
{
    Texture = <g_txEnvMap>;
    MipFilter = Linear;
    MinFilter = Linear;
    MagFilter = Linear;
};


float4x4 inverse_nonSAS(float4x4 m)
{
    float4x4 adj;
    
    // Calculate the adjoint matrix
    adj._11 = m._22*m._33*m._44 + m._23*m._34*m._42 + m._24*m._32*m._43 - m._22*m._34*m._43 - m._23*m._32*m._44 - m._24*m._33*m._42;
    adj._12 = m._12*m._34*m._43 + m._13*m._32*m._44 + m._14*m._33*m._42 - m._12*m._33*m._44 - m._13*m._34*m._42 - m._14*m._32*m._43;
    adj._13 = m._12*m._23*m._44 + m._13*m._24*m._42 + m._14*m._22*m._43 - m._12*m._24*m._43 - m._13*m._22*m._44 - m._14*m._23*m._42;
    adj._14 = m._12*m._24*m._33 + m._13*m._22*m._34 + m._14*m._23*m._32 - m._12*m._23*m._34 - m._13*m._24*m._32 - m._14*m._22*m._33;
    
    adj._21 = m._21*m._34*m._43 + m._23*m._31*m._44 + m._24*m._33*m._41 - m._21*m._33*m._44 - m._23*m._34*m._41 - m._24*m._31*m._43;
    adj._22 = m._11*m._33*m._44 + m._13*m._34*m._41 + m._14*m._31*m._43 - m._11*m._34*m._43 - m._13*m._31*m._44 - m._14*m._33*m._41;
    adj._23 = m._11*m._24*m._43 + m._13*m._21*m._44 + m._14*m._23*m._41 - m._11*m._23*m._44 - m._13*m._24*m._41 - m._14*m._21*m._43;
    adj._24 = m._11*m._23*m._34 + m._13*m._24*m._31 + m._14*m._21*m._33 - m._11*m._24*m._33 - m._13*m._21*m._34 - m._14*m._23*m._31;
    
    adj._31 = m._21*m._32*m._44 + m._22*m._34*m._41 + m._24*m._31*m._42 - m._21*m._34*m._42 - m._22*m._31*m._44 - m._24*m._32*m._41;
    adj._32 = m._11*m._34*m._42 + m._12*m._31*m._44 + m._14*m._32*m._41 - m._11*m._32*m._44 - m._12*m._34*m._41 - m._14*m._31*m._42;
    adj._33 = m._11*m._22*m._44 + m._12*m._24*m._41 + m._14*m._21*m._42 - m._11*m._24*m._42 - m._12*m._21*m._44 - m._14*m._22*m._41;
    adj._34 = m._11*m._24*m._32 + m._12*m._21*m._34 + m._14*m._22*m._31 - m._11*m._22*m._34 - m._12*m._24*m._31 - m._14*m._21*m._32;
    
    adj._41 = m._21*m._33*m._42 + m._22*m._31*m._43 + m._23*m._32*m._41 - m._21*m._32*m._43 - m._22*m._33*m._41 - m._23*m._31*m._42;
    adj._42 = m._11*m._32*m._43 + m._12*m._33*m._41 + m._13*m._31*m._42 - m._11*m._33*m._42 - m._12*m._31*m._43 - m._13*m._32*m._41;
    adj._43 = m._11*m._23*m._42 + m._12*m._21*m._43 + m._13*m._22*m._41 - m._11*m._22*m._43 - m._12*m._23*m._41 - m._13*m._21*m._42;
    adj._44 = m._11*m._22*m._33 + m._12*m._23*m._31 + m._13*m._21*m._32 - m._11*m._23*m._32 - m._12*m._21*m._33 - m._13*m._22*m._31;
    
    // Calculate the determinant
    float det = determinant(m);
    
    float4x4 result = 0;
    if( 0.0f != det )
        result = 1.0f/det * adj;
        
    return result;
}


//-----------------------------------------------------------------------------
// Name: VertScene
// Type: Vertex shader
// Desc: This shader computes standard transform and lighting
//-----------------------------------------------------------------------------
void VertScene( float4 vPos : POSITION,
                float3 vNormal : NORMAL,
                float2 vTex0 : TEXCOORD0,
                out float4 oPos : POSITION,
                out float4 oDiffuse : COLOR0,
                out float2 oTex0 : TEXCOORD0,
                out float3 oViewPos : TEXCOORD1,
                out float3 oViewNormal : TEXCOORD2,
                out float3 oEnvTex : TEXCOORD3 )
{
    float4x4 ViewInv= inverse_nonSAS(g_mView);

    // Transform the position from object space to homogeneous projection space
    oPos = mul( vPos, g_mWorld );
    oPos = mul( oPos, g_mView );
    oPos = mul( oPos, g_mProj );


    // Compute the view-space position
    oViewPos = mul( vPos, g_mWorld );
    oViewPos = mul( float4( oViewPos, 1) , g_mView );

    // Compute view-space normal
    oViewNormal = mul( vNormal, (float3x3)g_mWorld );
    oViewNormal = normalize( mul( oViewNormal, (float3x3)g_mView ) );

    // Compute lighting
    oDiffuse = dot( oViewNormal, normalize( g_vLight - oViewPos ) ) * Diffuse;

    // Just copy the texture coordinate through
    oTex0 = vTex0;

    // Compute the texture coord for the environment map.
    oEnvTex = 2 * dot( -oViewPos, oViewNormal ) * oViewNormal + oViewPos;
    oEnvTex = mul( oEnvTex, (float3x3)ViewInv );
}


//-----------------------------------------------------------------------------
// Name: PixScene
// Type: Pixel shader
// Desc: This shader outputs the pixel's color by modulating the texture's
//         color with diffuse material color
//-----------------------------------------------------------------------------
float4 PixScene( float4 MatDiffuse : COLOR0,
                 float2 Tex0 : TEXCOORD0,
                 float3 ViewPos : TEXCOORD1,
                 float3 ViewNormal : TEXCOORD2,
                 float3 EnvTex : TEXCOORD3 ) : COLOR0
{
    // Compute half vector for specular lighting
    float3 vHalf = normalize( normalize( -ViewPos ) + normalize( g_vLight - ViewPos ) );

    // Compute normal dot half for specular light
    float4 fSpecular = pow( saturate( dot( vHalf, normalize( ViewNormal ) ) ) * Specular, Power );


	float3 texColor = (float3)1;
	if(IsMaterialTextured)
		texColor = tex2D( MaterialSampler, Tex0 );
		
    // Lookup mesh texture and modulate it with diffuse
    return float4( (float3)( g_vLightColor * ( texColor * MatDiffuse + fSpecular ) * ( 1 - Reflectivity )
                             + texCUBE( g_samEnv, EnvTex ) * Reflectivity
                           ), 1.0f );
}


void VertScene1x( float4 vPos : POSITION,
                  float3 vNormal : NORMAL,
                  float2 vTex0 : TEXCOORD0,
                  out float4 oPos : POSITION,
                  out float4 oDiffuse : COLOR0,
                  out float4 oSpecular : COLOR1,
                  out float2 oTex0 : TEXCOORD0,
                  out float3 oEnvTex : TEXCOORD1 )
{
       float4x4 ViewInv= inverse_nonSAS(g_mView);

    // Transform the position from object space to homogeneous projection space
    oPos = mul( vPos, g_mWorld );
    oPos = mul( oPos, g_mView );
    oPos = mul( oPos, g_mProj );

    // Compute the view-space position
    float4 ViewPos = mul( vPos, g_mWorld );
    ViewPos = mul( ViewPos, g_mView );

    // Compute view-space normal
    float3 ViewNormal = mul( vNormal, (float3x3)g_mWorld );
    ViewNormal = normalize( mul( ViewNormal, (float3x3)g_mView ) );

    // Compute diffuse lighting
    oDiffuse = dot( ViewNormal, normalize( g_vLight - ViewPos ) ) * Diffuse;

    // Compute specular lighting
    // Compute half vector
    float3 vHalf = normalize( normalize( -ViewPos ) + normalize( g_vLight - ViewPos ) );

    // Compute normal dot half for specular light
    oSpecular = pow( saturate( dot( vHalf, ViewNormal ) ) * Specular, Power );

    // Just copy the texture coordinate through
    oTex0 = vTex0;

    // Compute the texture coord for the environment map.
    oEnvTex = 2 * dot( -ViewPos, ViewNormal ) * ViewNormal + ViewPos;
    oEnvTex = mul( oEnvTex, (float3x3)ViewInv );
}


float4 PixScene1x( float4 MatDiffuse : COLOR0,
                   float4 MatSpecular : COLOR1,
                   float2 Tex0 : TEXCOORD0,
                   float3 EnvTex : TEXCOORD1 ) : COLOR0
{
    // Lookup mesh texture and modulate it with diffuse
    return ( MatDiffuse * tex2D( MaterialSampler, Tex0 ) + MatSpecular ) * ( 1 - Reflectivity ) + texCUBE( g_samEnv, EnvTex ) * Reflectivity;
}


//-----------------------------------------------------------------------------
// Name: RenderScene
// Type: Technique
// Desc: Renders scene to render target
//-----------------------------------------------------------------------------
technique RenderScene
{
    pass P0
    {
        VertexShader = compile vs_1_1 VertScene();
        PixelShader  = compile ps_2_0 PixScene();
        ZEnable = true;
        AlphaBlendEnable = false;
    }
}


technique RenderScene1x
{
    pass P0
    {
        VertexShader = compile vs_1_1 VertScene1x();
        PixelShader  = compile ps_1_1 PixScene1x();
        ZEnable = true;
        AlphaBlendEnable = false;
    }
}
