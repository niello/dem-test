#ifndef __DEPTH_H__
#define __DEPTH_H__

#include "Lib/Skinning.fxh"

// Depth pass functions

// Solid object depth *****************************************************************************

void VSDepth(		float4	Pos:	POSITION,
				out	float4	oPos:	POSITION,
				out	float	oDepth:	TEXCOORD0)
{
	oPos = mul(Pos, WorldViewProjection);
	oDepth = oPos.z; //???need or can get from oPos in PS? or POSITION is interpolated differently than TEXCOORDN?
}
//---------------------------------------------------------------------

void VSDepthSkinned(	float4	Pos:		POSITION,
						float4	Weights:	BLENDWEIGHT,
						float4	Indices:	BLENDINDICES,
					out	float4	oPos:		POSITION,
					out	float	oDepth:		TEXCOORD0)
{
	//oPos = mul(SkinnedPosition(Pos, Weights, Indices), WorldViewProjection);
	oPos = mul(SkinnedPosition(Pos, Weights, Indices), ViewProjection);
	oDepth = oPos.z; //???need or can get from oPos in PS? or POSITION is interpolated differently than TEXCOORDN?
}
//---------------------------------------------------------------------

void VSDepthInstanced(		float4	Pos:	POSITION,
							float4	World1:	TEXCOORD4,
							float4	World2:	TEXCOORD5,
							float4	World3:	TEXCOORD6,
							float4	World4:	TEXCOORD7,
						out	float4	oPos:	POSITION,
						out	float	oDepth:	TEXCOORD0)
{
	oPos = mul(mul(Pos, float4x4(World1, World2, World3, World4)), ViewProjection);
	oDepth = oPos.z; //???need or can get from oPos in PS? or POSITION is interpolated differently than TEXCOORDN?
}
//---------------------------------------------------------------------

//!!!No need if z-only! set PS to NULL in tech in this case?
float4 PSDepth(float Depth: TEXCOORD0): COLOR
{
	return float4(Depth, 0.0f, 0.0f, 1.0f);
	//???return float4(Pos.z, 0.0f, 0.0f, 1.0f);
}
//---------------------------------------------------------------------

#endif
