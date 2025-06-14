//--------------------------------------------------------------------------------------
// File: DDSWithoutD3DX.hlsl
//
// The HLSL file for the DDSWithoutD3DX sample for the Direct3D 11 device
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
cbuffer cbPerObject : register( b0 )
{
    matrix  g_mWorldViewProjection  : packoffset( c0 );
    matrix  g_mWorld                : packoffset( c4 );
}

cbuffer cbPerFrame : register( b1 )
{
    float3              g_vLightDir             : packoffset( c0 );
};

//-----------------------------------------------------------------------------------------
// Textures and Samplers
//-----------------------------------------------------------------------------------------
Texture2D    g_txDiffuse : register( t0 );
SamplerState g_samLinear : register( s0 );

//--------------------------------------------------------------------------------------
// shader input/output structure
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Position     : POSITION; // vertex position
    float3 Normal       : NORMAL;   // this normal comes in per-vertex
    float2 TextureUV    : TEXCOORD0;// vertex texture coords
};

struct VS_OUTPUT
{
    float4 Position     : SV_POSITION; // vertex position
    float4 Diffuse      : COLOR0;      // vertex diffuse color (note that COLOR0 is clamped from 0..1)
    float2 TextureUV    : TEXCOORD0;   // vertex texture coords
};

//--------------------------------------------------------------------------------------
// This shader computes standard transform and lighting
//--------------------------------------------------------------------------------------
VS_OUTPUT RenderSceneVS( VS_INPUT input )
{
    VS_OUTPUT Output;
    float3 vNormalWorldSpace;

    // Transform the position from object space to homogeneous projection space
    Output.Position = mul( input.Position, g_mWorldViewProjection );

    // Transform the normal from object space to world space
    vNormalWorldSpace = normalize(mul(input.Normal, (float3x3)g_mWorld)); // normal (world space)

    // Calc diffuse color
    Output.Diffuse.rgb = max(0.3,dot(vNormalWorldSpace, g_vLightDir)).rrr;
    Output.Diffuse.a = 1.0f;

    // Just copy the texture coordinate through
    Output.TextureUV = input.TextureUV;

    return Output;
}

//--------------------------------------------------------------------------------------
// This shader outputs the pixel's color by modulating the texture's
// color with diffuse material color
//--------------------------------------------------------------------------------------
float4 RenderScenePS( VS_OUTPUT In ) : SV_TARGET
{
    // Lookup mesh texture and modulate it with diffuse
    return g_txDiffuse.Sample( g_samLinear, In.TextureUV ) * In.Diffuse;
}
