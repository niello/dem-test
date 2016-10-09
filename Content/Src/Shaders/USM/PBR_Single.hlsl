#include "Globals.hlsl"
#include "PBR.hlsl"
#include "Skinning.hlsl"

struct PSInSingle
{
	float4	Pos:		SV_Position;
	float3	PosWorld:	TEXCOORD1;
	float4	NormalU:	NORMAL;
	float4	ViewV:		VIEW;
};

// Per-instance data

cbuffer InstanceParamsVS: register(b2)	// VS
{
	CInstanceDataVS InstanceDataVS;
}

cbuffer InstanceParamsPS: register(b2)	// PS
{
	CInstanceDataPS InstanceDataPS;
}

// Vertex shaders

PSInSingle VSMain(float4 Pos: POSITION, float3 Normal: NORMAL, float2 UV: TEXCOORD)
{
	PSInSingle Out;
	float4 WorldPos = mul(Pos, InstanceDataVS.WorldMatrix);
	Out.Pos = mul(WorldPos, ViewProj);
	Out.PosWorld = WorldPos.xyz;
	Out.NormalU = float4(mul(Normal, (float3x3)InstanceDataVS.WorldMatrix), UV.x);
	Out.ViewV = float4(EyePos - WorldPos.xyz, UV.y);
	return Out;
}
//---------------------------------------------------------------------

PSInSingle VSMainSkinned(	float4 Pos:		POSITION,
							float3 Normal:	NORMAL,
							float4 Weights:	BLENDWEIGHT,
							float4 Indices:	BLENDINDICES,
							float2 UV:		TEXCOORD0)
{
	PSInSingle Out;
	float4 WorldPos = SkinnedPoint(Pos, Weights, Indices);
	Out.Pos = mul(WorldPos, ViewProj);
	Out.PosWorld = WorldPos.xyz;
	Out.NormalU = float4(SkinnedVector(Normal, Weights, Indices), UV.x);
	Out.ViewV = float4(EyePos - WorldPos.xyz, UV.y);
	return Out;
}
//---------------------------------------------------------------------

float4 PSMain(PSInSingle In): SV_Target
{
	float2 UV = float2(In.NormalU.w, In.ViewV.w);
	return PSPBR(UV, In.PosWorld, In.NormalU.xyz, In.ViewV.xyz, InstanceDataPS.LightCount, (int[MAX_LIGHT_COUNT_PER_OBJECT])InstanceDataPS.LightIndices);
}
//---------------------------------------------------------------------
