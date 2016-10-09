#include "Depth.hlsl"

#ifndef MAX_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT 64
#endif

// Per-instance data

cbuffer InstanceParamsVS: register(b2)	// VS
{
	CInstanceDataVS InstanceDataVS[MAX_INSTANCE_COUNT];
}

// Vertex shaders

float4 VSMainOpaque(float4 Pos: POSITION, uint InstanceID: SV_InstanceID): SV_Position
{
	float4 OutPos = mul(Pos, InstanceDataVS[InstanceID].WorldMatrix);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
}
//---------------------------------------------------------------------

//!!!DUPLICATE CODE, see PBR VSMainInstancedConst!
PSSceneIn VSMainAlphaTest(float4 Pos: POSITION, float2 Tex: TEXCOORD, uint InstanceID: SV_InstanceID)
{
	float4x4 InstWorld = InstanceDataVS[InstanceID].WorldMatrix;

	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(Pos, InstWorld);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = Tex;
	return Out;
}
//---------------------------------------------------------------------
