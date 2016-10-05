#include "Globals.hlsl"
#include "PBR.hlsl"

#ifndef MAX_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT 64
#endif

struct PSInInstanced
{
	float4	Pos:		SV_Position;
	float3	PosWorld:	TEXCOORD1;
	float4	NormalU:	NORMAL;
	float4	ViewV:		VIEW;
	uint	InstanceID:	INSTANCEID;
};

// Per-instance data

cbuffer InstanceParamsVS: register(b2)	// VS
{
	CInstanceDataVS InstanceDataVS[MAX_INSTANCE_COUNT];
}

cbuffer InstanceParamsPS: register(b2)	// PS
{
	CInstanceDataPS InstanceDataPS[MAX_INSTANCE_COUNT];
}

// Vertex shaders

PSInInstanced VSMainInstanced(float4 Pos: POSITION, float3 Normal: NORMAL, float2 UV: TEXCOORD, uint InstanceID: SV_InstanceID)
{
	PSInInstanced Out;
	float4 WorldPos = mul(Pos, InstData.WorldMatrix);
	Out.Pos = mul(WorldPos, ViewProj);
	Out.PosWorld = WorldPos.xyz;
	Out.NormalU = float4(mul(Normal, (float3x3)InstData.WorldMatrix), UV.x);
	Out.ViewV = float4(EyePos - WorldPos.xyz, UV.y);
	Out.InstanceID = InstanceID;
	return Out;
}
//---------------------------------------------------------------------

float4 PSMain(PSInSimple In): SV_Target
{
	return PSPBR(In);
}
//---------------------------------------------------------------------
