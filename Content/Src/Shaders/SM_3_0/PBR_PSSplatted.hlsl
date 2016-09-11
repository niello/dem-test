
#include "Splatting.hlsl"
#include "CDLOD.hlsl"
#line 4 "PBR_PSSplatted.hlsl"

//!!!see old code version for more code!
float4 PSMainSplatted(	float4	VertexConsts:	TEXCOORD0,
						float4	SplatDetUV:		TEXCOORD1,
						float4	PosWorld:		TEXCOORD2): SV_Target
{
	float2 UV = PosWorld.xz * PSCDLODParams.WorldToHM.xy + PSCDLODParams.WorldToHM.zw;
	float3 TexDiffuse = Splatting(tex2D(SplatSampler, UV), SplatDetUV.xy);
	return float4(TexDiffuse, 1.f);
}
//---------------------------------------------------------------------
