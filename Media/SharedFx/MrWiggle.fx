/*********************************************************************NVMH3****
File:  $Id: //sw/devtools/SDK/9.1/SDK/MEDIA/HLSL/MrWiggle.fx#1 $

Copyright NVIDIA Corporation 2004
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
    Simple sinusoidal vertex animation on a phong-shaded plastic surface.
		The highlight is done in VERTEX shading -- not as a texture.
		Textured/Untextured versions are supplied
	Note that the normal is also easily distorted, based on the fact that
		dsin(x)/dx is just cos(x)
    Do not let your kids play with this shader, you will not get your
		computer back for a while.

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

SasPointLight PointLight 
<
	string SasBindAddress = "Sas.PointLight[0]"; 
	bool SasUiVisible = false;
>;

float3 CameraPos
<
	string SasBindAddress = "Sas.Camera.Position"; 
	bool SasUiVisible = false;
>;

float Timer 
< 
	string SasBindAddress = "Sas.Time.Now"; 
>;

float TimeScale <
    string SasUiControl = "Slider";
    string SasUiLabel = "Speed";
    float SasUiMin = 0.1;
    float SasUiMax = 10;
    int SasUiStep = 9;
> = 4.0f;

float Horizontal <
    string SasUiControl = "slider";
    float SasUiMin = 0.001;
    float SasUiMax = 10;
    int SasUiStep = 9999;
> = 0.5f;

float Vertical <
    string SasUiControl = "slider";
    float SasUiMin = 0.001;
    float SasUiMax = 10.0;
    int SasUiStep = 9;
> = 0.5;


float3 AmbiColor : Ambient <
    string SasUiLabel =  "Ambient";
    string SasUiControl = "ColorPicker";
> = {0.2f, 0.2f, 0.2f};

float3 SurfColor : DIFFUSE <
    string SasUiLabel =  "Surface Diffuse";
    string SasUiControl = "ColorPicker";
> = {0.9f, 0.9f, 0.9f};

float SpecExpon : SpecularPower <
	string SasUiControl = "Slider"; 
	float SasUiMin = 1.0f; 
	float SasUiMax = 32.0f; 
	int SasUiSteps = 31; 
        string SasUiLabel =  "Specular Power";
> = 5.0;

texture colorTexture : DIFFUSE <
    string SasUiLabel =  "Color Texture (if used)";
>;

sampler2D colorSampler = sampler_state {
	Texture = <colorTexture>;
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
struct vertexOutput {
    float4 HPosition	: POSITION;
    float4 TexCoord0	: TEXCOORD0;
    float4 diffCol	: COLOR0;
    float4 specCol	: COLOR1;
};

/*********** vertex shader ******/

vertexOutput MrWiggleVS(appdata IN) {
    vertexOutput OUT;
    
    float4x4 WorldIT = transpose(inverse(World));
    float4x4 WorldViewProj = mul( World, mul( View, Projection ) );
 
    float3 Nn = normalize(mul(IN.Normal, WorldIT).xyz);
    float timeNow = Timer*TimeScale;
    float4 Po = float4(IN.Position.xyz,1);
    float iny = Po.y * Vertical + timeNow;
    float wiggleX = sin(iny) * Horizontal;
    float wiggleY = cos(iny) * Horizontal; // deriv
    Nn.y = Nn.y + wiggleY;
    Nn = normalize(Nn);
    Po.x = Po.x + wiggleX;
    OUT.HPosition = mul(Po, WorldViewProj);
    float3 Pw = mul(Po, World).xyz;
    float3 Ln = normalize(PointLight.Position - Pw);
    float ldn = dot(Ln,Nn);
    float diffComp = max(0,ldn);
    float3 diffContrib = SurfColor * ( diffComp * PointLight.Color + AmbiColor);
    OUT.diffCol = float4(diffContrib,1);
    OUT.TexCoord0 = IN.UV;
    float3 Vn = normalize(CameraPos - Pw);
    float3 Hn = normalize(Vn + Ln);
    float hdn = pow(max(0,dot(Hn,Nn)),SpecExpon);
    OUT.specCol = float4(hdn * PointLight.Color,1);
    return OUT;
}

/********* pixel shader ********/

float4 MrWigglePS_t(vertexOutput IN) : COLOR {
    float4 result = IN.diffCol * tex2D(colorSampler, IN.TexCoord0) + IN.specCol;
    return result;
}

/*************/

technique Untextured 
{
    pass p0 
    {		
		VertexShader = compile vs_1_1 MrWiggleVS();
		ZEnable = true;
		ZWriteEnable = true;
		CullMode = None;
		// Don't need a pixel shader for a super-simple surface
		SpecularEnable = true;
		ColorArg1[ 0 ] = Diffuse;
		ColorOp[ 0 ]   = SelectArg1;
		ColorArg2[ 0 ] = Specular;
		AlphaArg1[ 0 ] = Diffuse;
		AlphaOp[ 0 ]   = SelectArg1;
    }
}

technique Textured 
{
    pass p0 
    {		
		VertexShader = compile vs_1_1 MrWiggleVS();
		ZEnable = true;
		ZWriteEnable = true;
		CullMode = None;
		PixelShader = compile ps_1_1 MrWigglePS_t();
    }
}

/***************************** eof ***/
