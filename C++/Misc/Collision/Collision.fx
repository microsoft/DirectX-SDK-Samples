//--------------------------------------------------------------------------------------
// File: Collision.fx
//
// The effect file for the Collision sample
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
float4 g_Color;
float4x4 g_mWorldViewProjection;    // World * View * Projection matrix

//--------------------------------------------------------------------------------------
// Computes position
//--------------------------------------------------------------------------------------
float4 vs_PositionOnly( float4 vPos : POSITION ) : POSITION
{
    return mul( float4(vPos.xyz, 1), g_mWorldViewProjection );
}


//--------------------------------------------------------------------------------------
// Computes Color
//--------------------------------------------------------------------------------------
float4 ps_ConstantColor() : COLOR
{
    return g_Color;
}

//--------------------------------------------------------------------------------------
// Renders scene 
//--------------------------------------------------------------------------------------
technique RenderScene
{
    pass P0
    {          
        VertexShader = compile vs_2_0 vs_PositionOnly();
        PixelShader  = compile ps_2_0 ps_ConstantColor(); 
    }
}
