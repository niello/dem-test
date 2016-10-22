#include "Globals.hlsl"
#include "Depth.hlsl"
#include "CDLOD.hlsl"

#ifndef MAX_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT 64
#endif

// Per-instance data

struct CPatchDataVS
{
	float4	PatchXZ;		// xy - scale, zw - offset
	float2	MorphConsts;	// x - end / (end - start), y - 1 / (end - start)
	float2	_CPatchDataVS_PAD;
};

cbuffer InstanceParamsVS: register(b2)	// VS
{
	CPatchDataVS InstanceDataVS[MAX_INSTANCE_COUNT];
}

// Vertex shaders

float4 VSMainOpaque(float2 Pos: POSITION, uint InstanceID: SV_InstanceID): SV_Position
{
	uint RealInstanceID = FirstInstanceIndex + InstanceID;
	CPatchDataVS PatchData = InstanceDataVS[RealInstanceID];

	float3 Vertex = CDLOD_ProcessVertexOnly(Pos, PatchData.PatchXZ, PatchData.MorphConsts, EyePos);
	return mul(float4(Vertex, 1), ViewProj);
}
//---------------------------------------------------------------------
