//--------------------------------------------------------------------------------------
// File: XACTGame.fx
//
// The effect file for the XACTGame sample.  
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
matrix   g_mWorld;                  // World matrix for object
matrix   g_mView;                   // View matrix for object
matrix   g_mProj;                   // Projection matrix for object
matrix   g_mWorldView;              // World * View matrix
matrix   g_mWorldViewProj;          // World * View * Projection matrix

texture  g_DiffuseTexture;          // diffuse texture 
texture  g_NormalMapTexture;        // normal map

float4   g_matDiffuse;              // Diffuse component of material
float4   g_matSpecular;             // Specular component of material
float    g_matPower;                // Specular power of material
float4   g_vLightPos[4] = { float4(  3.5f, 1.0f,  5.5f, 1.0f ),
                            float4( -3.5f, 1.0f, -5.5f, 1.0f ),
                            float4(  3.5f, 1.0f, -5.5f, 1.0f ),
                            float4( -3.5f, 1.0f,  5.5f, 1.0f ) };  // Light position in world space
float4   g_vLightColor = float4( 0.5f, 0.5f, 0.5f, 1.0f );
float    g_fAnimation;

// These parameters will change based on the config flags.
bool     g_bUseSpecular = false;
bool     g_bUseAnisotropic = false;
int      g_MinFilter = 2;           // Minification filtering
int      g_MaxAnisotropy = 1;       // Maximum anisotropy


//-----------------------------------------------------------------------------
// Texture samplers
//-----------------------------------------------------------------------------
sampler g_DiffuseSampler =
sampler_state
{
    Texture = <g_DiffuseTexture>;
    MinFilter = <g_MinFilter>;
    MagFilter = Linear;
    MipFilter = Linear;
    MaxAnisotropy = <g_MaxAnisotropy>;
};

sampler g_NormalMapSampler =
sampler_state
{
    Texture = <g_NormalMapTexture>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = None;
};


void VS20( float4 vPos : POSITION,
           float3 vNormal : NORMAL,
           float3 vTangent : TANGENT,
           float2 vUV : TEXCOORD0,
           out float4 oPos : POSITION,
           out float2 oUV : TEXCOORD0,
           out float3 oVNormal : TEXCOORD1,
           out float3 oVTangent : TEXCOORD2,
           out float3 oVBinormal : TEXCOORD3,
           out float3 oVtL0 : TEXCOORD4,
           out float3 oVtL1 : TEXCOORD5,
           out float3 oVtL2 : TEXCOORD6,
           out float3 oVtL3 : TEXCOORD7
            )
{

    // Animate along normal based on g_fAnimation (usually 0)
    float4 vPosAnim = vPos; 
    float4 vN = float4(vNormal.x, vNormal.y, vNormal.z, 0.0f);
    vPosAnim += vN * g_fAnimation;
    
    oPos = mul( vPosAnim, g_mWorldViewProj );

    // Compute view space normal
    oVNormal = normalize( mul( vNormal, (float3x3)g_mWorld ) );
    oVTangent = normalize( mul( vTangent, (float3x3)g_mWorld ) );
    oVBinormal = cross( oVNormal, oVTangent );

    // Vertex pos in view space (normalize in pixel shader)
    float3 VtE = -mul( vPos, g_mWorld );

    // Compute view space vertex to light vectors (normalized)
    oVtL0 = normalize( g_vLightPos[0] + VtE );
    oVtL1 = normalize( g_vLightPos[2] + VtE );
    oVtL2 = normalize( g_vLightPos[1] + VtE );
    oVtL3 = normalize( g_vLightPos[3] + VtE );

    // Propogate texcoords
    oUV = vUV;
}


float4 PS20( float2 UV : TEXCOORD0,
             float3 N : TEXCOORD1,
             float3 Tangent : TEXCOORD2,
             float3 Binormal : TEXCOORD3,
             float3 VtL0 : TEXCOORD4,
             float3 VtL1 : TEXCOORD5,
             float3 VtL2 : TEXCOORD6,
             float3 VtL3 : TEXCOORD7
              ) : COLOR0
{
	float3 Normal = tex2D( g_NormalMapSampler, UV*1.0f );
	Normal = (Normal * 2) - 1;

//	float3 Normal = float3(0,1,1);
//	Normal = normalize( Normal );
	
	float3x3 mTangentFrame = { Tangent, Binormal, N };
	Normal = mul( Normal, mTangentFrame );

    // Diffuse luminance
    float LumD = max( 0.0f, dot( Normal, VtL0 ) ) +
                 max( 0.0f, dot( Normal, VtL1 ) ) +
                 max( 0.0f, dot( Normal, VtL2 ) ) +
                 max( 0.0f, dot( Normal, VtL3 ) );

    // Specular luminance
    float4 Specular;
    if( g_bUseSpecular )
    {
        float LumS = pow( max( 0.00001f, dot( Normal, normalize( VtL0 ) ) ), g_matPower ) +
                     pow( max( 0.00001f, dot( Normal, normalize( VtL1 ) ) ), g_matPower ) +
                     pow( max( 0.00001f, dot( Normal, normalize( VtL2 ) ) ), g_matPower ) +
                     pow( max( 0.00001f, dot( Normal, normalize( VtL3 ) ) ), g_matPower ) ;

        Specular = g_matSpecular * LumS * g_vLightColor;
    }
    else
    {
        Specular = 0.0f;
    }

    float4 color = tex2D( g_DiffuseSampler, UV ) * g_matDiffuse * LumD * g_vLightColor + Specular;
    color.a = g_matDiffuse.a;
    return color;
}


void VS( float4 vPos : POSITION,
         float3 vNormal : NORMAL,
         float2 vUV : TEXCOORD0,
         out float4 oDiffuse : COLOR0,
         out float4 oSpecular : COLOR1,
         out float4 oPos : POSITION,
         out float2 oUV : TEXCOORD0 )
{
    // Animate along normal based on g_fAnimation (usually 0)
    float4 vPosAnim = vPos; 
    float4 vN = float4(vNormal.x, vNormal.y, vNormal.z, 0.0f);
    vPosAnim += vN * g_fAnimation;
    
    oPos = mul( vPosAnim, g_mWorldViewProj );

    // Compute view space normal
    float3 N = normalize( mul( vNormal, (float3x3)g_mWorldView ) );

    // Vertex pos in view space
    float3 Vv = mul( vPos, g_mWorldView );

    float LumD = 0.0f;  // Diffuse luminance
    float LumS = 0.0f;  // Specular luminance
    for( int i = 0; i < 4; ++i )
    {
        // Light pos in view space
        float3 Lv = mul( g_vLightPos[i], g_mView );

        // View space vertex-to-light
        float3 VtL = normalize( Lv - Vv );

        LumD += max( 0.0f, dot( N, VtL ) );

        // View space vertex-to-eye
        float3 VtE = normalize( -Vv );

        // Half vector
        float3 H = normalize( VtE + VtL );

        LumS += pow( max( 0.0f, dot( N, H ) ), 40.0f ); // This power is hardcoded to not exceed inst limit.
    }

    oDiffuse = g_matDiffuse * LumD * g_vLightColor;
    oDiffuse.a = g_matDiffuse.a;
    if( g_bUseSpecular )
    {
        oSpecular = LumS * g_vLightColor;
        oSpecular.a = 0.0f;
    }
    else
    {
        oSpecular = 0.0f;   
    }

    oUV = vUV;
}


float4 PS( float4 Diffuse : COLOR0,
           float4 Specular : COLOR1,
           float2 UV : TEXCOORD0 ) : COLOR0
{
    float4 color = tex2D( g_DiffuseSampler, UV ) * Diffuse + Specular;
    return color;
}


//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
technique RenderScenePS20
{
    pass P0
    {
        VertexShader = compile vs_2_0 VS20();
        PixelShader = compile ps_2_0 PS20();
    }
}


technique FFRenderScene
{
    pass P0
    {
        VertexShader = null;
        PixelShader = null;

        WorldTransform[0] = <g_mWorld>;
        ViewTransform = <g_mView>;
        ProjectionTransform = <g_mProj>;

        Texture[0] = <g_DiffuseTexture>;
        MinFilter[0] = <g_MinFilter>;
        MaxAnisotropy[0] = <g_MaxAnisotropy>;
        MagFilter[0] = Linear;
        MipFilter[0] = Linear;
        AddressU[0] = Wrap;
        AddressV[0] = Wrap;
        AddressW[0] = Wrap;
        ColorArg1[0] = Texture;
        ColorArg2[0] = Current;
        ColorOp[0] = Modulate;

        Lighting = True;

        LightEnable[0] = True;
        LightType[0] = Point;
        LightPosition[0] = <g_vLightPos[0]>;
        LightRange[0] = 100.0f;
        LightDiffuse[0] = <g_vLightColor>;
        LightSpecular[0] = <g_vLightColor>;
        LightAttenuation0[0] = 1.0f;
        LightAttenuation1[0] = 0.0f;
        LightAttenuation2[0] = 0.0f;

        LightEnable[1] = True;
        LightType[1] = Point;
        LightPosition[1] = <g_vLightPos[1]>;
        LightRange[1] = 100.0f;
        LightDiffuse[1] = <g_vLightColor>;
        LightSpecular[1] = <g_vLightColor>;
        LightAttenuation0[1] = 1.0f;
        LightAttenuation1[1] = 0.0f;
        LightAttenuation2[1] = 0.0f;

        LightEnable[2] = True;
        LightType[2] = Point;
        LightPosition[2] = <g_vLightPos[2]>;
        LightRange[2] = 100.0f;
        LightDiffuse[2] = <g_vLightColor>;
        LightSpecular[2] = <g_vLightColor>;
        LightAttenuation0[2] = 1.0f;
        LightAttenuation1[2] = 0.0f;
        LightAttenuation2[2] = 0.0f;

        LightEnable[3] = True;
        LightType[3] = Point;
        LightPosition[3] = <g_vLightPos[3]>;
        LightRange[3] = 100.0f;
        LightDiffuse[3] = <g_vLightColor>;
        LightSpecular[3] = <g_vLightColor>;
        LightAttenuation0[3] = 1.0f;
        LightAttenuation1[3] = 0.0f;
        LightAttenuation2[3] = 0.0f;

        NormalizeNormals = true;
        SpecularEnable = <g_bUseSpecular>;
        Ambient = float4( 0.0f, 0.0f, 0.0f, 0.0f );
        DiffuseMaterialSource = Material;
        SpecularMaterialSource = Material;
        MaterialAmbient = float4( 0.0f, 0.0f, 0.0f, 0.0f );
        MaterialDiffuse = <g_matDiffuse>;
        MaterialEmissive = float4( 0.0f, 0.0f, 0.0f, 0.0f );
        MaterialSpecular = <g_matSpecular>;
        MaterialPower = <g_matPower>;
    }
}
