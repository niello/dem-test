#ifndef __DEPTH_ATEST_H__
#define __DEPTH_ATEST_H__

#include "Lib/Skinning.fxh"

// Depth pass functions

// Alpha-tested object depth ***************************************************************************

void VSDepthATest(		float4	Pos:		POSITION,
						float2	UV0:		TEXCOORD0,
					out	float4	oPos:		POSITION,
					out	float3	oUV0Depth:	TEXCOORD0)
{
	oPos = mul(Pos, WorldViewProjection);
	oUV0Depth.xy = UV0;
	oUV0Depth.z = oPos.z; //???!!!oPos.z / oPos.w; - in N3
}
//---------------------------------------------------------------------

void VSDepthATestSkinned(	float4	Pos:		POSITION,
							float4	Weights:	BLENDWEIGHT,
							float4	Indices:	BLENDINDICES,
							float2	UV0:		TEXCOORD0,
						out	float4	oPos:		POSITION,
						out	float3	oUV0Depth:	TEXCOORD0)
{
	oPos = mul(SkinnedPosition(Pos, Weights, Indices), WorldViewProjection);
	oUV0Depth.xy = UV0;
	oUV0Depth.z = oPos.z; //???!!!oPos.z / oPos.w; - in N3
}
//---------------------------------------------------------------------

void VSDepthATestInstanced(		float4	Pos:		POSITION,
								float2	UV0:		TEXCOORD0,
								float4	World1:		TEXCOORD4,
								float4	World2:		TEXCOORD5,
								float4	World3:		TEXCOORD6,
								float4	World4:		TEXCOORD7,
							out	float4	oPos:		POSITION,
							out	float3	oUV0Depth:	TEXCOORD0)
{
	oPos = mul(mul(Pos, float4x4(World1, World2, World3, World4)), ViewProjection);
	oUV0Depth.xy = UV0;
	oUV0Depth.z = oPos.z; //???!!!oPos.z / oPos.w; - in N3
}
//---------------------------------------------------------------------

//!!!No need if z-only! set PS to NULL in tech in this case?
float4 PSDepthATest(float3 UV0Depth: TEXCOORD0): COLOR
{
	float Alpha = tex2D(DiffSampler, UV0Depth.xy).a;
	clip(Alpha - AlphaRef);
	return float4(UV0Depth.z, 0.0f, 0.0f, Alpha); //???what for to output alpha?
}
//---------------------------------------------------------------------

#endif
