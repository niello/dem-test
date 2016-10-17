#include "Globals.hlsl"
#include "PBR.hlsl"
#include "NormalMapping.hlsl"
#include "Skinning.hlsl"

struct PSInSingle
{
	float4	Pos:		SV_Position;
	float3	PosWorld:	WORLDPOS;
	float4	NormalU:	NORMAL;
	float4	ViewV:		VIEW;
};

// Per-instance data

cbuffer InstanceParamsVS: register(b2)	// VS
{
	CInstanceDataVS InstanceDataVS;
}

cbuffer InstanceParamsPS: register(b2)	// PS
{
	CInstanceDataPS InstanceDataPS;
}

// Vertex shaders

PSInSingle VSMain(float4 Pos: POSITION, float3 Normal: NORMAL, float2 UV: TEXCOORD)
{
	PSInSingle Out;
	float4 WorldPos = mul(Pos, InstanceDataVS.WorldMatrix);
	Out.Pos = mul(WorldPos, ViewProj);
	Out.PosWorld = WorldPos.xyz;
	Out.NormalU = float4(mul(Normal, (float3x3)InstanceDataVS.WorldMatrix), UV.x);
	Out.ViewV = float4(EyePos - WorldPos.xyz, UV.y);
	return Out;
}
//---------------------------------------------------------------------

PSInSingle VSMainSkinned(	float4 Pos:		POSITION,
							float3 Normal:	NORMAL,
							float4 Weights:	BLENDWEIGHT,
							float4 Indices:	BLENDINDICES,
							float2 UV:		TEXCOORD0)
{
	PSInSingle Out;
	float4 WorldPos = SkinnedPoint(Pos, Weights, Indices);
	Out.Pos = mul(WorldPos, ViewProj);
	Out.PosWorld = WorldPos.xyz;
	Out.NormalU = float4(SkinnedVector(Normal, Weights, Indices), UV.x);
	Out.ViewV = float4(EyePos - WorldPos.xyz, UV.y);
	return Out;
}
//---------------------------------------------------------------------

// Pixel shaders

float4 PSMain(PSInSingle In): SV_Target
{
	float2 UV = float2(In.NormalU.w, In.ViewV.w);

	float4 Albedo = TexAlbedo.Sample(LinearSampler, UV);

	float3 View = In.ViewV.xyz;
	float3 N = SampleNormal(TexNormalMap, LinearSampler, In.NormalU.xyz, View, UV);

	float3 Reflectivity = TexReflectance.Sample(LinearSampler, UV).rgb;
	//float3 Reflectivity = 0.04f.xxx; // Dielectric, in metalness workflow can use uniform float for different dielectrics like gems
	//float3 Reflectivity = float3(1.0f, 0.71f, 0.29f); // Gold
	//float3 Reflectivity = float3(0.95f, 0.64f, 0.54f); // Copper

	float Roughness = TexRoughness.Sample(LinearSampler, UV).r;
	float SqRoughness = Roughness * Roughness;

	float3 V = normalize(View);
	float NdotV = max(0.f, dot(N, V));

	float3 Lighting = PSAmbientPBR(Albedo, N, V, NdotV, Reflectivity, SqRoughness);

	int LightIndices[MAX_LIGHT_COUNT_PER_OBJECT] = (int[MAX_LIGHT_COUNT_PER_OBJECT])InstanceDataPS.LightIndices;
	for (int i = 0; i < InstanceDataPS.LightCount; ++i)
	{
		Lighting += PSDirectPBR(Albedo, N, V, NdotV, Reflectivity, SqRoughness, In.PosWorld, Lights[LightIndices[i]]);
	}

	//???how to calc correct alpha?
	//Unity: Adding more reflectivity (as energy must be taken from somewhere) the diffuse level and the transparency will be reduced
	//automatically. Adding transparency will reduce diffuse level.
	// No translucent metals! Can use alpha-test, but not blend!
	return float4(Lighting, Albedo.a);
}
//---------------------------------------------------------------------
