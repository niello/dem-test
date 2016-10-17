#include "Globals.hlsl"
#include "PBR.hlsl"

#ifndef MAX_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT 64
#endif

struct PSInInstanced
{
	float4	Pos:		SV_Position;
	float3	PosWorld:	WORLDPOS;
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

PSInInstanced VSMain(float4 Pos: POSITION, float3 Normal: NORMAL, float2 UV: TEXCOORD, uint InstanceID: SV_InstanceID)
{
	CInstanceDataVS InstData = InstanceDataVS[InstanceID];

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

// Pixel shaders

float4 PSMain(PSInInstanced In): SV_Target
{
	CInstanceDataPS InstData = InstanceDataPS[In.InstanceID];
	float2 UV = float2(In.NormalU.w, In.ViewV.w);
	float4 Albedo = TexAlbedo.Sample(LinearSampler, UV);
	return PSPBR(Albedo, UV, In.PosWorld, In.NormalU.xyz, In.ViewV.xyz, InstData.LightCount, (int[MAX_LIGHT_COUNT_PER_OBJECT])InstData.LightIndices);
}
//---------------------------------------------------------------------
