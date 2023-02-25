/*********************************************************************NVMH3****
File:  $Id: //sw/devtools/SDK/9.1/SDK/MEDIA/HLSL/velvety.fx#1 $

Copyright NVIDIA Corporation 2002-2004
TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
*AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS
BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.


Comments:
    A simple combination of vertex and pixel shaders with velvety edge effects.

******************************************************************************/
#include <sas\sas.fxh>

int GlobalParameter : SasGlobal                                                             
<
    int3 SasVersion = {1, 1, 0};
    bool SasUiVisible = false;
>;


/************* UN-TWEAKABLES **************/
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

SasPointLight PointLight 
<
	string SasBindAddress = "Sas.PointLight[0]"; 
	bool SasUiVisible = false;
>;






/************* TWEAKABLES **************/


float3 DiffColor <
	string SasUiLabel = "Primary Color";
	string SasUiControl = "ColorPicker"; 
> = {0.5f, 0.5f, 0.5f};

float3 SpecColor <
	string SasUiControl = "ColorPicker"; 
	string SasUiLabel = "Fuzz";
> = {0.7f, 0.7f, 0.75f};

float3 SubColor <
	string SasUiControl = "ColorPicker"; 
	string SasUiLabel = "Under-Color";
> = {0.2f, 0.2f, 1.0f};

float RollOff <	
	string SasUiControl = "Slider"; 
    float UIMin = 0.0;
    float UIMax = 1.0;
    float UIStep = 0.05;
	string SasUiLabel = "Edge Rolloff";
> = 0.3;

///

texture2D ColorTexture
<
	string SasUiLabel = "Color Texture";
	string SasUiControl = "FilePicker"; 
>;

sampler2D ColorSampler = sampler_state
{
    Texture = <ColorTexture>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = None;
};

/************* DATA STRUCTS **************/

/* data from application vertex buffer */
struct appdata {
    float3 Position	: POSITION;
    float4 UV		: TEXCOORD0;
    float4 Normal	: NORMAL;
};

/* data passed from vertex shader to pixel shader */
struct shadedVertexOutput {
    float4 HPosition	: POSITION;
    float4 TexCoord0	: TEXCOORD0;
    float4 diffCol	: COLOR0;
    float4 specCol	: COLOR1;
};

/* data passed from simpler vertex shader to pixel shader */
struct vertexOutput {
    half4 HPosition	: POSITION;
    half4 TexCoord		: TEXCOORD0;
    half3 LightVec		: TEXCOORD1;
    half3 WorldNormal	: TEXCOORD2;
    half3 WorldView	: TEXCOORD5;
};

/*********** vertex shader ******/

shadedVertexOutput velvetVS(appdata IN)
{
    float4x4 WorldIT = transpose(inverse(World));
    float4x4 ViewI = inverse(View);
    float4x4 WorldViewProj = mul(World, mul(View, Projection));
    float3 LightPos = PointLight.Position;

    shadedVertexOutput OUT;
    float3 Nn = normalize(mul(IN.Normal, WorldIT).xyz);
    float4 Po = float4(IN.Position.xyz,1);
    OUT.HPosition = mul(Po, WorldViewProj);
    float3 Pw = mul(Po, World).xyz;
    float3 Ln = normalize(LightPos - Pw);
    float ldn = dot(Ln,Nn);
    float diffComp = max(0,ldn);
    float3 diffContrib = diffComp * DiffColor;
    float subLamb = smoothstep(-RollOff,1.0,ldn) - smoothstep(0.0,1.0,ldn);
    subLamb = max(0.0,subLamb);
    float3 subContrib = subLamb * SubColor;
    OUT.TexCoord0 = IN.UV;
    float3 Vn = normalize(ViewI[3].xyz - Pw);
    float vdn = 1.0-dot(Vn,Nn);
    float3 vecColor = float4(vdn.xxx,1.0);
    OUT.diffCol = float4((subContrib+diffContrib).xyz,1);
    OUT.specCol = float4((vecColor*SpecColor).xyz,1);
    return OUT;
}

vertexOutput simpleVS(appdata IN) {
    vertexOutput OUT;
    
    float4x4 WorldIT = transpose(inverse(World));
    float4x4 ViewI = inverse(View);
    float4x4 WorldViewProj = mul(World, mul(View, Projection));
    float3 LightPos = PointLight.Position;

    half4 normal = normalize(IN.Normal);
    OUT.WorldNormal = mul(normal, WorldIT).xyz;
    half4 Po = half4(IN.Position.xyz,1);
    half3 Pw = mul(Po, World).xyz;
    OUT.LightVec = normalize(LightPos - Pw);
    OUT.TexCoord = IN.UV.xyxx;
    OUT.WorldView = normalize(ViewI[3].xyz - Pw);
    OUT.HPosition = mul(Po, WorldViewProj);
    return OUT;
}

/** pixel shader  **/

void velvet_shared(vertexOutput IN,
			out half3 DiffuseContrib,
			out half3 SpecularContrib)
{
    half3 Ln = normalize(IN.LightVec);
    half3 Nn = normalize(IN.WorldNormal);
    half3 Vn = normalize(IN.WorldView);
    half3 Hn = normalize(Vn + Ln);
    float ldn = dot(Ln,Nn);
    float diffComp = max(0,ldn);
    float3 diffContrib = diffComp * DiffColor;
    float subLamb = smoothstep(-RollOff,1.0,ldn) - smoothstep(0.0,1.0,ldn);
    subLamb = max(0.0,subLamb);
    float3 subContrib = subLamb * SubColor;
    float vdn = 1.0-dot(Vn,Nn);
    float3 vecColor = float4(vdn.xxx,1.0);
    DiffuseContrib = float4((subContrib+diffContrib).xyz,1);
    SpecularContrib = float4((vecColor*SpecColor).xyz,1);
}

half4 velvetPS(vertexOutput IN) : COLOR {
    half3 diffContrib;
    half3 specContrib;
	velvet_shared(IN,diffContrib,specContrib);
    half3 result = diffContrib + specContrib;
    return half4(result,1);
}

half4 velvetPS_t(vertexOutput IN) : COLOR {
    half3 diffContrib;
    half3 specContrib;
	velvet_shared(IN,diffContrib,specContrib);
    half3 map = tex2D(ColorSampler,IN.TexCoord.xy).xyz;
    half3 result = specContrib + (map * diffContrib);
    return half4(result,1);
}

/*************/

technique Textured
{
    pass p0
    {		
		VertexShader = compile vs_1_1 velvetVS();
		ZEnable = true;
		ZWriteEnable = true;
		CullMode = None;
		// PixelShader = compile ps_1_1 velvetPS_t();
		// no pixel shader needed
		SpecularEnable = true;
	    Texture[0] = <ColorTexture>;
	    MinFilter[0] = Linear;
	    MagFilter[0] = Linear;
	    MipFilter[0] = None;
        ColorArg1[ 0 ] = Texture;
        ColorOp[ 0 ] = Modulate;
        ColorArg2[ 0 ] = Diffuse;
    }
}

technique Untextured 
{
    pass p0
    {		
		VertexShader = compile vs_1_1 velvetVS();
		ZEnable = true;
		ZWriteEnable = true;
		CullMode = None;
		SpecularEnable = true;
		ColorArg1[ 0 ] = Diffuse;
		ColorOp[ 0 ]   = SelectArg1;
		ColorArg2[ 0 ] = Specular;
		AlphaArg1[ 0 ] = Diffuse;
		AlphaOp[ 0 ]   = SelectArg1;
    }
}

technique UntexturedPS
{
    pass p0
    {		
		VertexShader = compile vs_1_1 simpleVS();
		ZEnable = true;
		ZWriteEnable = true;
		CullMode = None;
		PixelShader = compile ps_2_0 velvetPS();
    }
}

technique TexturedPS
{
    pass p0
    {		
		VertexShader = compile vs_1_1 simpleVS();
		ZEnable = true;
		ZWriteEnable = true;
		CullMode = None;
		PixelShader = compile ps_2_0 velvetPS_t();
    }
}

/***************************** eof ***/
