#include "Globals.hlsl"
#include "Skinning.hlsl"

struct PSInSimple
{
	float4	Pos:		SV_Position;
	float3	PosWorld:	TEXCOORD1;
	float3	Normal:		NORMAL;
	float3	View:		VIEW;
	float2	Tex:		TEXCOORD;
	uint4	LightInfo:	LIGHTINFO;
};

struct CInstanceData
{
	uint	LightCount;
	uint3	LightIndices;
	//static uint LightIndices[DEM_MAX_LIGHTS] = (uint[DEM_MAX_LIGHTS])array; // for tight packing
};

// Per-instance data

cbuffer InstanceParams: register(b2)
{
	CInstanceData InstanceData;
}

// Vertex shaders

PSInSimple VSMainSkinned(	float4 Pos:		POSITION,
							float3 Normal:	NORMAL,
							float4 Weights:	BLENDWEIGHT,
							float4 Indices:	BLENDINDICES,
							float2 Tex:		TEXCOORD0)
{
	PSInSimple Out = (PSInSimple)0.0;
	float4 WorldPos = SkinnedPoint(Pos, Weights, Indices);
	Out.Pos = mul(WorldPos, ViewProj);
	Out.PosWorld = WorldPos.xyz;
	Out.Normal = SkinnedVector(Normal, Weights, Indices);
	Out.View = EyePos - WorldPos.xyz;
	Out.Tex = Tex;
	Out.LightInfo.x = InstanceData.LightCount;
	Out.LightInfo.yzw = InstanceData.LightIndices;
	return Out;
}
//---------------------------------------------------------------------
