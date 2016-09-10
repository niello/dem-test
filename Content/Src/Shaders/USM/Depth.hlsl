#include "Globals.hlsl"
#include "Skinning.hlsl"
#include "CDLOD.hlsl"

struct VSSceneIn
{
	float3 Pos: POSITION;
	float2 Tex: TEXCOORD;
};

struct PSSceneIn
{
	float4 Pos: SV_Position;
	float2 Tex: TEXCOORD;
};

struct CInstanceData
{
	matrix	WorldMatrix;
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

#ifndef MAX_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT 64
#endif

cbuffer InstanceParams: register(b2)
{
	CInstanceData InstanceDataArray[MAX_INSTANCE_COUNT];
}

// For alpha-test
Texture2D TexAlbedo;
sampler LinearSampler;

// Vertex shaders

//!!!may premultiply and pass WVP instead of WorldMatrix!
float4 VSMainOpaque(float3 Pos: POSITION): SV_Position
{
	float4 OutPos = mul(float4(Pos, 1), InstanceData.WorldMatrix);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
}
//---------------------------------------------------------------------

float4 VSMainInstancedConstOpaque(float3 Pos: POSITION, uint InstanceID: SV_InstanceID): SV_Position
{
	float4 OutPos = mul(float4(Pos, 1), InstanceDataArray[InstanceID].WorldMatrix);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
}
//---------------------------------------------------------------------

float4 VSMainSkinnedOpaque(float4 Pos: POSITION, float4 Weights: BLENDWEIGHT, float4 Indices: BLENDINDICES): SV_Position
{
	float4 OutPos = SkinnedPosition(Pos, Weights, Indices);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
}
//---------------------------------------------------------------------

//!!!DUPLICATE CODE, see PBR VSMain!
PSSceneIn VSMainAlphaTest(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(In.Pos, 1), InstanceData.WorldMatrix);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = In.Tex;
	return Out;
}
//---------------------------------------------------------------------

//!!!DUPLICATE CODE, see PBR VSMainInstancedConst!
PSSceneIn VSMainInstancedConstAlphaTest(float3 Pos: POSITION, float2 Tex: TEXCOORD, uint InstanceID: SV_InstanceID)
{
	float4x4 InstWorld = InstanceDataArray[InstanceID].WorldMatrix;

	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(Pos, 1), InstWorld);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = Tex;
	return Out;
}
//---------------------------------------------------------------------

void VSMainCDLOD(	float2	Pos:			POSITION,
					float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
					float2	MorphConsts:	TEXCOORD1,	// x - end / (end - start), y - 1 / (end - start)
				out	float4	oPos:			SV_Position)
{
	float3 Vertex;
	Vertex.xz = Pos * PatchXZ.xy + PatchXZ.zw;
	float2 HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	float MorphK  = 1.0f - clamp(MorphConsts.x - distance(Vertex, EyePos) * MorphConsts.y, 0.0f, 1.0f);
	float2 FracPart = frac(Pos * GridConsts.xx) * GridConsts.yy;
	const float2 PosMorphed = Pos - FracPart * MorphK;

	Vertex.xz = PosMorphed * PatchXZ.xy + PatchXZ.zw;
	HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	oPos = mul(float4(Vertex, 1), ViewProj);
}
//---------------------------------------------------------------------

// Pixel shaders

void PSMainAlphaTest(PSSceneIn In)
{
	float Alpha = TexAlbedo.Sample(LinearSampler, In.Tex).a;
	clip(Alpha - 0.5);
}
//---------------------------------------------------------------------
