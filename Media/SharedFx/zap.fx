/*********************************************************************NVMH3****

File:  $Id: //sw/devtools/SDK/9.1/SDK/MEDIA/HLSL/zap.fx#1 $

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
    Gooch-style diffuse texturing, calculated per-vertex.
	Untextured and textured techniques are provided.

******************************************************************************/


#include <sas\sas.fxh>

int GlobalParameter : SasGlobal                                                             
<
    int3 SasVersion = {1, 1, 0};
    bool SasUiVisible = false;
>;


/************* TWEAKABLES **************/

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

float Timer 
< 
	string SasBindAddress = "Sas.Time.Now"; 
>;

float4 DarkColor <
    string SasUiLabel =  "Dark Color";
    string SasUiControl = "ColorPicker";
> = {0.0f, 0.0f, 0.0f,1};

float4 WarmColor <
    string SasUiLabel =  "Zap Color";
    string SasUiControl = "ColorPicker";
> = {0.5f, 0.4f, 0.05f,1};

float NoiseSpeed1 <
	string SasUiControl = "Slider";
	float SasUiMin = 0.0;
	float SasUiMax = 1.0;
	int SasUiSteps = 100;
> = 0.1f;

float NoiseSpeed2 <
	string SasUiControl = "Slider";
	float SasUiMin = 0.0;
	float SasUiMax = 1.0;
	int SasUiSteps = 100;
> = 0.12f;

float NoiseSpeed3 <
	string SasUiControl = "Slider";
	float SasUiMin = 0.0;
	float SasUiMax = 1.0;
	int SasUiSteps = 100;
> = 0.01f;

float Spread <
	string SasUiControl = "Slider";
	float SasUiMin = 0.0;
	float SasUiMax = 4.0;
	int SasUiSteps = 400;
> = 1.0f;

float Gamma <
	string SasUiControl = "Slider";
	float SasUiMin = 0.5;
	float SasUiMax = 2.0;
	int SasUiSteps = 150;
> = 1.0f;

float Gain <
	string SasUiControl = "Slider";
	float SasUiMin = 0.1;
	float SasUiMax = 2.0;
	int SasUiSteps = 190;
> = 1.0f;

float Fader <
	string SasUiControl = "Slider";
	float SasUiMin = 0.1;
	float SasUiMax = 20.0;
	int SasUiSteps = 1900;
> = 1.0f;


texture3D NoiseTex  
<
	string SasResourceAddress = "../Misc/Noise3D.dds";
	bool SasUiVisible = false;
>;

// samplers
sampler NoiseSamp = sampler_state 
{
    texture = <NoiseTex>;
    AddressU  = WRAP;        
    AddressV  = WRAP;
    AddressW  = WRAP;
    MIPFILTER = LINEAR;
    MINFILTER = LINEAR;
    MAGFILTER = LINEAR;
};


/************* DATA STRUCTS **************/

/* data passed from vertex shader to pixel shader */
struct vertexOutput {
    float4 HPosition	: POSITION;
    float2 UV2	: TEXCOORD0;
    float3 UV3	: TEXCOORD1;
};

/*********** vertex shader ******/

static float3 NoiseOff = (Timer*float3(NoiseSpeed1,NoiseSpeed2,NoiseSpeed3));

vertexOutput zapVS(float4 Pos : POSITION,
					float4 UV : TEXCOORD0)
{
    vertexOutput OUT = (vertexOutput)0;
    float4 Po = float4(Pos.xyz,1);
    OUT.UV2 = UV.xy;
    OUT.HPosition = mul(Po, mul(World, mul(View, Projection)));
    OUT.UV3 = NoiseOff + float3(Spread*UV.y,0,0);
    return OUT;
}

/********* pixel shader ********/

float4 zapPS(vertexOutput IN) : COLOR {
	float zap = tex3D(NoiseSamp,IN.UV3).x;
	zap = 0.5 + Gain*(zap-0.5);
	//return zap.xxxx;
	float glow = 2*pow(Fader*abs(IN.UV2.x - zap),Gamma);
    return lerp(WarmColor,DarkColor,glow);
}

/*** TECHNIQUES **********/

technique Zap 
{
    pass p0 
	{		
	    VertexShader = compile vs_1_1 zapVS();
	    ZEnable = true;
	    ZWriteEnable = true;
	    CullMode = None;
	    PixelShader = compile ps_2_a zapPS();
    }
}

/***************************** eof ***/
