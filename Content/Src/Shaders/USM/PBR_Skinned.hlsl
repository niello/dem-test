#include "Globals.hlsl"
#include "Skinning.hlsl"

struct PSInSimple
{
	float4	Pos:		SV_Position;
	float2	Tex:		TEXCOORD;
	uint4	LightInfo:	LIGHTINFO;
};

struct CInstanceData
{
	uint	LightCount;
	uint3	LightIndices;
//#if DEM_MAX_LIGHTS > 0
//	uint4	LightIndices;
//#endif
	//static uint LightIndices[DEM_MAX_LIGHTS] = (uint[DEM_MAX_LIGHTS])array; // for tight packing
};

// Per-instance data

cbuffer InstanceParams: register(b2)
{
	CInstanceData InstanceData;
}

// Vertex shaders


PSInSimple VSMainSkinned(float4	Pos:		POSITION,
						float4	Weights:	BLENDWEIGHT,
						float4	Indices:	BLENDINDICES,
						float2	Tex:		TEXCOORD0)
{
	PSInSimple Out = (PSInSimple)0.0;
	Out.Pos = SkinnedPosition(Pos, Weights, Indices);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = Tex;
	Out.LightInfo.x = InstanceData.LightCount;
	Out.LightInfo.yzw = InstanceData.LightIndices;
	return Out;
}
//---------------------------------------------------------------------
