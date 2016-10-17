#include "Globals.hlsl"
#include "Depth.hlsl"
#include "CDLOD.hlsl"

#ifndef MAX_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT 64
#endif

struct CPatchDataVS
{
	float4	PatchXZ;		// xy - scale, zw - offset
	float2	MorphConsts;	// x - end / (end - start), y - 1 / (end - start)
	float2	_CPatchDataVS_PAD;
};

// Per-instance data

cbuffer InstanceParamsVS: register(b2)	// VS
{
	CPatchDataVS InstanceDataVS[MAX_INSTANCE_COUNT];
}

// Vertex shaders

void VSMainOpaque(	float2	Pos:			POSITION,
					float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
					float2	MorphConsts:	TEXCOORD1,	// x - end / (end - start), y - 1 / (end - start)
				out	float4	oPos:			SV_Position)
{
	float3 Vertex;
	Vertex.xz = Pos * PatchXZ.xy + PatchXZ.zw;
	float2 HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = CDLOD_SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	float MorphK  = 1.0f - clamp(MorphConsts.x - distance(Vertex, EyePos) * MorphConsts.y, 0.0f, 1.0f);
	float2 FracPart = frac(Pos * GridConsts.xx) * GridConsts.yy;
	const float2 PosMorphed = Pos - FracPart * MorphK;

	Vertex.xz = PosMorphed * PatchXZ.xy + PatchXZ.zw;
	HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = CDLOD_SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	oPos = mul(float4(Vertex, 1), ViewProj);
}
//---------------------------------------------------------------------
