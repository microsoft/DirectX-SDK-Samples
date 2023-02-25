//--------------------------------------------------------------------------------------
// File: InstallOnDemand.fx
//
// The effect file for the InstallOnDemand sample.  
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
shared float	g_fTime;                   // App's time in seconds
shared float4x4 g_mWorld;                  // World matrix for object
shared float4x4 g_mWorldView;              // World matrix for object
shared float4x4 g_mWorldViewProjection;	   // World * View * Projection matrix
shared float4x4 g_mInvWorldViewProjection; // Inverse WVP for skybox rendering
       texture  g_txScene;
       texture  g_txSkybox;
       float4   g_vMaterial;

sampler2D g_samScene =
sampler_state
{
    Texture = <g_txScene>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
};


sampler g_samSkybox =
sampler_state
{ 
    Texture = <g_txSkybox>;
    MipFilter = Linear;
    MinFilter = Linear;
    MagFilter = Linear;
};


void VS( float4 Pos : POSITION,
         float3 Normal : NORMAL,
         float2 Tex : TEXCOORD0,
         out float4 oPos : POSITION,
         out float2 oTex : TEXCOORD0,
         out float4 oDif : COLOR0 )
{
    oPos = mul( Pos, g_mWorldViewProjection );

    float3 N = normalize( mul( Normal, (float3x3)g_mWorldView ) );
    float3 viewVertToEye = normalize( -(float3)mul( Pos, g_mWorldView ) );

    oDif = max( 0.0f, dot( N, viewVertToEye ) );

    oTex = Tex;
}


float4 PS( float2 Tex : TEXCOORD0, float4 Dif : COLOR0 ) : COLOR0
{
    return tex2D( g_samScene, Tex ) * Dif;
}


void VS_NoTex( float4 Pos : POSITION,
               float3 Normal : NORMAL,
               out float4 oPos : POSITION,
               out float4 oDif : COLOR0 )
{
    oPos = mul( Pos, g_mWorldViewProjection );

    float3 N = normalize( mul( Normal, (float3x3)g_mWorldView ) );
    float3 viewVertToEye = normalize( -(float3)mul( Pos, g_mWorldView ) );

    oDif = max( 0.0f, dot( N, viewVertToEye ) ) * g_vMaterial;
}


float4 PS_NoTex( float4 Dif : COLOR0 ) : COLOR0
{
    return Dif;
}


void VS_Terrain( float4 Pos : POSITION,
                 float3 Normal : NORMAL,
                 float2 Tex : TEXCOORD0,
                 out float4 oPos : POSITION,
                 out float2 oTex : TEXCOORD0,
                 out float4 oDif : COLOR0 )
{
    oPos = mul( Pos, g_mWorldViewProjection );

    float3 N = normalize( mul( Normal, (float3x3)g_mWorld ) );

    // Diffuse is based on dot product of normal and UP vector to
    // maximize the effect of sun.
    oDif = max( 0.0f, dot( N, float3( 0.0f, 1.0f, 0.0f ) ) );

    oTex = Tex;
}


float4 PS_Terrain( float2 Tex : TEXCOORD0, float4 Dif : COLOR0 ) : COLOR0
{
    return tex2D( g_samScene, Tex ) * Dif;
}


float4 PS_Quad( float2 Tex : TEXCOORD0 ) : COLOR0
{
    return tex2D( g_samScene, Tex );
}


float4 PS_BarBackground() : COLOR0
{
    return float4( 0.0f, 0.0f, 0.0f, 0.3f );
}


float4 PS_Bar() : COLOR0
{
    return float4( 1.0f, 1.0f, 1.0f, 0.3f );
}


void VS_Skybox( float4 Pos : POSITION,
                out float4 oPos : POSITION,
                out float3 Tex : TEXCOORD0 )
{
    oPos = Pos;

    Tex = mul( Pos, g_mInvWorldViewProjection );
}

float4 PS_Skybox( float3 Tex : TEXCOORD0 ) : COLOR0
{
    return texCUBE( g_samSkybox, Tex ) * 0.4f;
}


//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
technique RenderScene
{
    pass P0
    {
        VertexShader = compile vs_2_0 VS();
        PixelShader = compile ps_2_0 PS();
    }
}


technique RenderSceneNoTex
{
    pass P0
    {
        VertexShader = compile vs_2_0 VS_NoTex();
        PixelShader = compile ps_2_0 PS_NoTex();
    }
}


technique RenderTerrain
{
    pass P0
    {
        VertexShader = compile vs_2_0 VS_Terrain();
        PixelShader = compile ps_2_0 PS_Terrain();
    }
}


technique RenderQuad
{
    pass p0
    {
        VertexShader = null;
        PixelShader = compile ps_2_0 PS_Quad();
    }
}


technique RenderProgressBarBackground
{
    pass p0
    {
        VertexShader = null;
        PixelShader = compile ps_2_0 PS_BarBackground();
        AlphaBlendEnable = true;
        SrcBlend = SrcAlpha;
        DestBlend = InvSrcAlpha;
    }
}


technique RenderProgressBar
{
    pass p0
    {
        VertexShader = null;
        PixelShader = compile ps_2_0 PS_Bar();
        AlphaBlendEnable = true;
        SrcBlend = SrcAlpha;
        DestBlend = InvSrcAlpha;
    }
}


technique RenderSkybox
{
    pass p0
    {
        VertexShader = compile vs_2_0 VS_Skybox();
        PixelShader = compile ps_2_0 PS_Skybox();
    }
} 
