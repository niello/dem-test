#ifndef __SHARED_H__
#define __SHARED_H__

//#include "Lib/Posteffect.fxh"

shared float4x4 World; //???shared?
shared float4x4 ViewProjection;
shared float4x4 WorldViewProjection;  //???shared?

shared float3	EyePos;

//???shared?
// Material params
shared float4	MtlDiffuse;
shared float4	MtlSpecular;
shared float4	MtlEmissive;

//!!!duplicate from Lighting.fxh!
#define MAX_LIGHTS 4
shared float4	LightAmbient;				//???shared for all scene? not per-light?
shared int		LightType[MAX_LIGHTS];
shared float3	LightPos[MAX_LIGHTS];
shared float3	LightDir[MAX_LIGHTS];		// NB: light direction must be pre-inverted for directional lights
shared float4	LightColor[MAX_LIGHTS];		// Pre-multiplied by intensity
shared float3	LightParams[MAX_LIGHTS];	// x - range, y - cos half theta, z - cos half phi //!!!can pre-invert range!

#endif
