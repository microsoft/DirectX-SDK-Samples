//-----------------------------------------------------------------------------
// File: SceneVS.hlsl
//
// Desc: Vertex shader for the scene.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------


cbuffer cbPerObject : register( b0 )
{
    row_major matrix    g_mWorldViewProjection	: packoffset( c0 );
}

struct SceneVS_Input
{
    float4 pos   : POSITION;
    float4 color : COLOR;
};

struct SceneVS_Output
{
    float4 pos   : SV_POSITION;
    float4 color : COLOR0;
};

SceneVS_Output SceneVS( SceneVS_Input input )
{
    SceneVS_Output output;

    output.color = input.color;
    output.pos   = mul(input.pos, g_mWorldViewProjection );

    return output;
}
