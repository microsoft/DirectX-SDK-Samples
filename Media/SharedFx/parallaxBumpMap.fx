/*********************************************************************NVMH3****
Path:  NVSDK\Common\media\cgfx
File:  $Id: //sw/devtools/SDK/9.1/SDK/MEDIA/HLSL/parallaxBumpMap.fx#1 $

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

******************************************************************************/

/************* "UN-TWEAKABLES," TRACKED BY CPU APPLICATION **************/
#include <sas\sas.fxh>

int GlobalParameter : SasGlobal                                                             
<
    int3 SasVersion = {1, 1, 0};
    bool SasUiVisible = false;
>;

float4x4 WorldXf : World
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

float3 CameraPos
<
	string SasBindAddress = "Sas.Camera.Position"; 
	bool SasUiVisible = false;
>;

///////////////////////////////////////////////////////////////
/// TWEAKABLES ////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

////////////////////////////////////////////// point light

SasPointLight PointLight 
<
	string SasBindAddress = "Sas.PointLight[0]"; 
	bool SasUiVisible = false;
>;

////////////////////////////////////////////// surface

float3 SurfColor
<
    string SasUiLabel =  "Surface Diffuse";
    string SasUiControl = "ColorPicker";
> = {1.0f, 1.0f, 1.0f};

float Bumpy
<
    string SasUiControl = "Slider";
    float SasUiMin = 0.0;
    float SasUiMax = 2.0;
    int SasUiSteps = 0.01;
    string SasUiLabel = "Bump Height";
> = 1.0;

float Height
<
    string SasUiControl = "Slider";
    float SasUiMin = -0.1;
    float SasUiMax = 0.2;
    int SasUiSteps = 1000;
    string SasUiLabel = "Displacement Height";
> = 0.02;

float Bias
<
    string SasUiControl = "Slider";
    float SasUiMin = -0.5;
    float SasUiMax = 0.5;
    int SasUiSteps = 1000;
    string SasUiLabel = "Displacement Bias";
> = 0.0;

float UScale
<
    string SasUiControl = "Slider";
    float SasUiMin = 1.0;
    float SasUiMax = 16.0;
    int SasUiSteps = 15;
    string SasUiLabel = "U Texture Repeat";
> = 1.0;

float VScale
<
    string SasUiControl = "Slider";
    float SasUiMin = 1.0;
    float SasUiMax = 10.0;
    int SasUiSteps = 9;
    string SasUiLabel = "V Texture Repeat";
> = 1.0;

float Ks
<
    string SasUiControl = "Slider";
    float SasUiMin = 0.0;
    float SasUiMax = 1.0;
    int SasUiSteps = 100;
    string SasUiLabel = "Specular";
> = 0.5;

float SpecExpon
<
    string SasUiControl = "Slider";
    float SasUiMin = 0.0;
    float SasUiMax = 100.0;
    int SasUiSteps = 100;
    string SasUiLabel = "Specular Exponent";
> = 30.0;

////////////////////////////////////////////////////////
/// TEXTURES ///////////////////////////////////////////
////////////////////////////////////////////////////////

texture ColorTexture : DIFFUSE;

sampler2D ColorSampler = sampler_state
{
	Texture = <ColorTexture>;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
	AddressU = WRAP;
	AddressV = WRAP;
};

texture2D NormalTexture
<
    string SasUiLabel = "NormalMap";
    string SasUiControl = "FilePicker";
>;

sampler2D NormalSampler = sampler_state
{
	Texture = <NormalTexture>;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
	AddressU = WRAP;
	AddressV = WRAP;
};

texture2D HeightTexture 
<
    string SasUiLabel = "HeightMap";
    string SasUiControl = "FilePicker";
>;

sampler2D HeightSampler = sampler_state
{
	Texture = <HeightTexture>;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
	AddressU = WRAP;
	AddressV = WRAP;
};

/*********************************************************/
/************* DATA STRUCTS ******************************/
/*********************************************************/

/* data from application vertex buffer */
struct appdata {
    float3 Position	: POSITION;
    float4 UV		: TEXCOORD0;
    float4 Normal	: NORMAL;
    float4 Tangent	: TANGENT0;
    float4 Binormal	: BINORMAL0;
};

struct vertexOutput {
    float4 HPosition	: POSITION;
    float2 UV			: TEXCOORD0;
    float3 LightVec	: TEXCOORD1;
    float3 WorldNormal	: TEXCOORD2;
    float3 WorldView	: TEXCOORD3;
    float3 WorldTangent	: TEXCOORD4;
    float3 WorldBinorm	: TEXCOORD5;
    float3 TanView	: TEXCOORD6;
};

/*********************************************************/
/*********** vertex shader *******************************/
/*********************************************************/

vertexOutput basicVS(appdata IN) {
    vertexOutput OUT;
    
    float4x4 WorldITXf = transpose(inverse(WorldXf));
    float4x4 WvpXf = mul( WorldXf, mul( View, Projection ) );
    
    float3 Nw = normalize(mul(IN.Normal,WorldITXf).xyz);
    float3 Tw = normalize(mul(IN.Tangent,WorldITXf).xyz);
    float3 Bw = normalize(mul(IN.Binormal,WorldITXf).xyz);
    OUT.WorldNormal = Nw;
    OUT.WorldTangent = Tw;
    OUT.WorldBinorm = Bw;
    float4 Po = float4(IN.Position.xyz,1.0);	// object coordinates
    float3 Pw = mul(Po,WorldXf).xyz;		// world coordinates
    
    OUT.LightVec = PointLight.Position - Pw;
    OUT.UV = float2(UScale,VScale) * IN.UV.xy;
    float3 Vn = normalize(CameraPos - Pw);	// obj coords
    OUT.WorldView = Vn;
	float3x3 tanXf = float3x3(Tw,Bw,Nw);
    OUT.TanView = mul(Vn,tanXf);
     //OUT.TanView = mul(tanXf,Vn);
    OUT.HPosition = mul(Po,WvpXf);	// screen clipspace coords
    return OUT;
}

/*********************************************************/
/*********** pixel shader ********************************/
/*********************************************************/

float4 everythingPS(vertexOutput IN) : COLOR {
	float3 tv = normalize(IN.TanView);
    float altitude = tex2D(HeightSampler,IN.UV).x + Bias;
	float2 nuv = Height*altitude*tv.xy;
	nuv += IN.UV;
    float3 Nn = normalize(IN.WorldNormal);
    float3 Tn = normalize(IN.WorldTangent);
    float3 Bn = normalize(IN.WorldBinorm);
    float3 bumps = Bumpy * (tex2D(NormalSampler,nuv).xyz-(0.5).xxx);
    float3 Nb = Nn + (bumps.x * Tn + bumps.y * Bn);
    Nb = normalize(Nb);
    float3 Vn = normalize(IN.WorldView);
    float3 Ln = normalize(IN.LightVec);
    float3 Hn = normalize(Vn + Ln);
    float hdn = dot(Hn,Nb);
    float ldn = dot(Ln,Nb);
    float4 litVec = lit(ldn,hdn,SpecExpon);
    float3 diffContrib = litVec.y * PointLight.Color;
    float3 specContrib = Ks * litVec.z * diffContrib;
    float3 colorTex = tex2D(ColorSampler,nuv).xyz;
    float3 result = colorTex * diffContrib + specContrib;
    return float4(result.xyz,1.0);
}

float4 flatColorPS(vertexOutput IN) : COLOR {
	float3 tv = normalize(IN.TanView);
    float altitude = tex2D(HeightSampler,IN.UV).x + Bias;
	float2 nuv = Height*altitude*tv.xy;
	nuv += IN.UV;
    float3 Nn = normalize(IN.WorldNormal);
    float3 Tn = normalize(IN.WorldTangent);
    float3 Bn = normalize(IN.WorldBinorm);
    float3 bumps = Bumpy * (tex2D(NormalSampler,nuv).xyz-(0.5).xxx);
    float3 Nb = Nn + (bumps.x * Tn + bumps.y * Bn);
    Nb = normalize(Nb);
    float3 Vn = normalize(IN.WorldView);
    float3 Ln = normalize(IN.LightVec);
    float3 Hn = normalize(Vn + Ln);
    float hdn = dot(Hn,Nb);
    float ldn = dot(Ln,Nb);
    float4 litVec = lit(ldn,hdn,SpecExpon);
    float3 diffContrib = litVec.y * PointLight.Color;
    float3 specContrib = Ks * litVec.z * diffContrib;
    float3 result = SurfColor * diffContrib + specContrib;
    return float4(result.xyz,1.0);
}

float4 seevecPS(vertexOutput IN) : COLOR {
	float3 tv = normalize(IN.TanView);
    float altitude = tex2D(HeightSampler,IN.UV).x + Bias;
	float2 nuv = Height*altitude*tv.xy;
	nuv += IN.UV;
	float3 result = 0.5+(tv*0.5);
	//float3 result = 0.5+float3(nuv.xy*0.5,0);
    return float4(result.xy,0.0,1.0);
}

////////////////////////////////////////////////////////////////////
/// TECHNIQUES /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

technique Main 
{
	pass p0
	{
		VertexShader = compile vs_2_0 basicVS();
		ZEnable = true;
		ZWriteEnable = true;
		CullMode = None;
		PixelShader = compile ps_2_0 everythingPS();
	}
}

technique FlatColor
{
	pass p0
	{
		VertexShader = compile vs_2_0 basicVS();
		ZEnable = true;
		ZWriteEnable = true;
		CullMode = None;
		PixelShader = compile ps_2_0 flatColorPS();
	}
}

technique Seevec
{
	pass p0
	{
		VertexShader = compile vs_2_0 basicVS();
		ZEnable = true;
		ZWriteEnable = true;
		CullMode = None;
		PixelShader = compile ps_2_0 seevecPS();
	}
}

/***************************** eof ***/
