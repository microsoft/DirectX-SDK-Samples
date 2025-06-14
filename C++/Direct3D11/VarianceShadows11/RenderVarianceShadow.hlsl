


//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
cbuffer cbPerObject : register( b0 )
{
	matrix		g_mWorldViewProjection	: packoffset( c0 );
};

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 vPosition	: POSITION;
};

struct VS_OUTPUT
{
	float4 vPosition	: SV_POSITION;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain( VS_INPUT Input )
{
	VS_OUTPUT Output;
	
	
	Output.vPosition = mul( Input.vPosition, g_mWorldViewProjection );

	return Output;
}


float2 PSMain (VS_OUTPUT Input) : SV_TARGET
{
    float2 rt;
    rt.x = Input.vPosition.z;
    rt.y = rt.x * rt.x;
    return rt;
}
