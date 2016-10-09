#include "Depth.hlsl"
#include "Skinning.hlsl"

// Per-instance data

cbuffer InstanceParamsVS: register(b2)	// VS
{
	CInstanceDataVS InstanceDataVS;
}

// Vertex shaders

//!!!may premultiply and pass WVP instead of WorldMatrix!
float4 VSMainOpaque(float4 Pos: POSITION): SV_Position
{
	float4 OutPos = mul(Pos, InstanceDataVS.WorldMatrix);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
}
//---------------------------------------------------------------------

float4 VSMainSkinnedOpaque(float4 Pos: POSITION, float4 Weights: BLENDWEIGHT, float4 Indices: BLENDINDICES): SV_Position
{
	float4 OutPos = SkinnedPoint(Pos, Weights, Indices);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
}
//---------------------------------------------------------------------

//!!!DUPLICATE CODE, see PBR VSMain!
PSSceneIn VSMainAlphaTest(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(In.Pos, InstanceDataVS.WorldMatrix);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = In.Tex;
	return Out;
}
//---------------------------------------------------------------------
