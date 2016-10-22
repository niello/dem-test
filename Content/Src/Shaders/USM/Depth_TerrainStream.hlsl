#include "Globals.hlsl"
#include "Depth.hlsl"
#include "CDLOD.hlsl"

// Vertex shaders

float4 VSMainOpaque(float2	Pos:			POSITION,
					float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
					float2	MorphConsts:	TEXCOORD1,	// x - end / (end - start), y - 1 / (end - start)
					): SV_Position
{
	float3 Vertex = CDLOD_ProcessVertexOnly(Pos, PatchXZ, MorphConsts, EyePos);
	return mul(float4(Vertex, 1), ViewProj);
}
//---------------------------------------------------------------------
