
#include "Splatting.hlsl"
#line 4 "PSMainSplatted.fx"

struct
{
	float4 WorldToHM;
} PSCDLODParams: register(c6) <string CBuffer = "PSCDLODParams"; int SlotIndex = 0;>;

//!!!see old code version for more code!
float4 PSMainSplatted(	float4	VertexConsts:	TEXCOORD0,
						float4	SplatDetUV:		TEXCOORD1,
						float4	PosWorld:		TEXCOORD2): SV_Target
{
	float2 UV = PosWorld.xz * PSCDLODParams.WorldToHM.xy + PSCDLODParams.WorldToHM.zw;
	float3 TexDiffuse = Splatting(tex2D(SplatSampler, UV), SplatDetUV.xy);
	return float4(TexDiffuse, 1.f);
	//return float4(frac(0.6 * PosWorld.y), frac(0.6 * PosWorld.y), frac(0.6 * PosWorld.y), 1.f);
}
//---------------------------------------------------------------------
