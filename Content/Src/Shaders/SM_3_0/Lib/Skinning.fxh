#ifndef __SKINNING_H__
#define __SKINNING_H__

#include "Lib/Config.fxh"
#include "Lib/Common.fxh"

// Shader functions for vertex skinning.
// (C) 2008 Radon Labs GmbH

#define MAX_BONES 4

//#define USE_JOINT_TEXTURE 0 // not supported on some DX9 cards (e.g. ATI X1xxx), use only on DX10 hardware

// Vertex shader constant skinning, leads to constant waterfalling!
matrix<float,4,3> JointPalette[72]: JointPalette;

//???why indices are float? use uint4
float4 SkinnedPosition(const float4 InPos, const float4 Weights, const float4 Indices)
{
	//// need to re-normalize weights because of compression
	//float4 NormWeights = Weights / dot(Weights, float4(1.0, 1.0, 1.0, 1.0));

	float3 OutPos = mul(InPos, JointPalette[Indices[0]]) * Weights[0];
	for (int i = 1; i < MAX_BONES; i++)
		OutPos += mul(InPos, JointPalette[Indices[i]]) * Weights[i];
	return float4(OutPos, 1.0f);
}
//---------------------------------------------------------------------

//???why indices are float? use uint4
float3 SkinnedNormal(const float3 InNormal, const float4 Weights, const float4 Indices)
{
    // Normals don't need to be 100% perfect, so don't normalize weights
	float3 OutNorm = mul(InNormal, (float3x3)JointPalette[Indices[0]]) * Weights[0];
	for (int i = 1; i < MAX_BONES; i++)
		OutNorm += mul(InNormal, (float3x3)JointPalette[Indices[i]]) * Weights[i];
	return OutNorm;
}
//---------------------------------------------------------------------

#endif
