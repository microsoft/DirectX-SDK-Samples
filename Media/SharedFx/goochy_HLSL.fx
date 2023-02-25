/*********************************************************************NVMH3****
File:  $Id: //sw/devtools/SDK/9.1/SDK/MEDIA/HLSL/goochy_HLSL.fx#1 $

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
	Gooch shading w/glossy hilight in HLSL ps_2 pixel shader.
	Textured and non-textued versions are supplied.

******************************************************************************/

#include <sas\sas.fxh>

int GlobalParameter : SasGlobal                                                             
<
    int3 SasVersion = {1, 1, 0};
    bool SasUiVisible = false;
>;


/************* TWEAKABLES **************/
float4x4 World : World
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

SasDirectionalLight DirectionalLight 
<
	string SasBindAddress = "Sas.DirectionalLight[0]"; 
	bool SasUiVisible = false;
>;

float3 CameraPos
<
	string SasBindAddress = "Sas.Camera.Position"; 
	bool SasUiVisible = false;
>;

float3 LiteColor <
    string SasUiLabel =  "Bright Surface Color";
    string SasUiControl = "ColorPicker";
> = {0.8f, 0.5f, 0.1f};

float3 DarkColor <
    string SasUiLabel =  "Dark Surface Color";
    string SasUiControl = "ColorPicker";
> = {0.0f, 0.0f, 0.0f};

float3 WarmColor <
    string SasUiLabel =  "Gooch warm tone";
    string SasUiControl = "ColorPicker";
> = {0.5f, 0.4f, 0.05f};

float3 CoolColor <
    string SasUiLabel =  "Gooch cool tone";
    string SasUiControl = "ColorPicker";
> = {0.05f, 0.05f, 0.6f};

float3 SpecColor : Specular <
    string SasUiLabel =  "Hilight color";
    string SasUiControl = "ColorPicker"; 
> = {0.7f, 0.7f, 1.0f};

float SpecExpon : SpecularPower <
    string SasUiControl = "Slider";
    float SasUIMin = 1.0;
    float SasUIMax = 128.0;
    int SasUiSteps = 127;
    string SasUiLabel =  "Specular Power";
> = 40.0;

float GlossTop <
    string SasUiControl = "slider";
    float UIMin = 0.2;
    float UIMax = 1.0;
    int SasUiSteps = 16;
    string SasUiLabel =  "Maximum for Gloss Dropoff";
> = 0.7;

float GlossBot
<
    string SasUiControl = "slider";
    float SasUIMin = 0.05;
    float SasUIMax = 0.95;
    int SasUiSteps = 18;
    string SasUiLabel =  "Minimum for Gloss Dropoff";
> = 0.5;

float GlossDrop
<
    string SasUiControl = "slider";
    float SasUIMin = 0.0;
    float SasUIMax = 1.0;
    int SasUiSteps = 20;
    string SasUiLabel =  "Strength of Glossy Dropoff";
> = 0.2;

texture ColorMap : DIFFUSE <
	string SasUiLabel = "Color Map";
	string SasUiControl = "FilePicker"; 
>;

sampler2D ColorSampler = sampler_state
{
	Texture = <ColorMap>;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
	AddressU = WRAP;
	AddressV = WRAP;
};

//////////////////////////

/* data from application vertex buffer */
struct appdata {
    float3 Position	: POSITION;
    float4 UV		: TEXCOORD0;
    float4 Normal	: NORMAL;
};

/* data passed from vertex shader to pixel shader */
struct vertexOutput {
    float4 HPosition	: POSITION;
    float4 TexCoord	: TEXCOORD0;
    float3 LightVec	: TEXCOORD1;
    float3 WorldNormal	: TEXCOORD2;
    float3 WorldPos	: TEXCOORD3;
    float3 WorldEyePos	: TEXCOORD4;
};

/*********** vertex shader ******/

vertexOutput mainVS(appdata IN)
{
    vertexOutput OUT;
    
    float4x4 WorldIT = transpose(inverse(World));
    float4x4 WorldViewProj = mul( World, mul( View, Projection ) );
    OUT.WorldNormal = mul(IN.Normal, WorldIT).xyz;
    float4 Po = float4(IN.Position.xyz,1);
    float3 Pw = mul(Po, World).xyz;
    OUT.WorldPos = Pw;
    OUT.LightVec = -DirectionalLight.Direction; // hardcode to sas directional light
    OUT.TexCoord = IN.UV;
    OUT.WorldEyePos = CameraPos;
    OUT.HPosition = mul(Po, WorldViewProj);
    return OUT;
}

/*********** pixel shader ******/

void gooch_shared(vertexOutput IN,
		out float4 DiffuseContrib,
		out float4 SpecularContrib)
{
    float3 Ln = normalize(IN.LightVec);
    float3 Nn = normalize(IN.WorldNormal);
    float3 Vn = normalize(IN.WorldEyePos - IN.WorldPos);
    float3 Hn = normalize(Vn + Ln);
    float hdn = pow(max(0,dot(Hn,Nn)),SpecExpon);
    hdn = hdn * (GlossDrop+smoothstep(GlossBot,GlossTop,hdn)*(1.0-GlossDrop));
    SpecularContrib = float4((hdn * SpecColor),1);
    float ldn = dot(Ln,Nn);
    float mixer = 0.5 * (ldn + 1.0);
    float diffComp = max(0,ldn);
    float3 surfColor = lerp(DarkColor,LiteColor,mixer);
    float3 toneColor = lerp(CoolColor,WarmColor,mixer);
    DiffuseContrib = float4((surfColor + toneColor),1);
}

float4 gooch_PS(vertexOutput IN) :COLOR
{
	float4 diffContrib;
	float4 specContrib;
	gooch_shared(IN,diffContrib,specContrib);
    float4 result = diffContrib + specContrib;
    return result;
}

float4 goochT_PS(vertexOutput IN) :COLOR
{
	float4 diffContrib;
	float4 specContrib;
	gooch_shared(IN,diffContrib,specContrib);
    float4 result = tex2D(ColorSampler,IN.TexCoord.xy)*diffContrib + specContrib;
    return result;
}

/*************/

technique Untextured 
{
	pass p0 
	{		
        VertexShader = compile vs_2_0 mainVS();
		ZEnable = true;
		ZWriteEnable = true;
		CullMode = None;
        PixelShader = compile ps_2_0 gooch_PS();
	}
}

technique Textured 
{
	pass p0 
	{		
        VertexShader = compile vs_2_0 mainVS();
		ZEnable = true;
		ZWriteEnable = true;
		CullMode = None;
        PixelShader = compile ps_2_0 goochT_PS();
	}
}

/***************************** eof ***/
