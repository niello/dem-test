#include "Globals.hlsl"
#include "CDLOD.hlsl"
#include "NormalMapping.hlsl"
#include "Splatting.hlsl"
#include "PBR.hlsl"

#ifndef MAX_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT 64
#endif

// For constant instancing, get lights by instance ID in PS
struct PSInSplattedConst
{
	float4	Pos:		SV_Position;
	float4	PosWorldU:	WORLDPOS;
	float4	ViewV:		VIEW;
	float4	SplatDetUV:	TEXCOORD0;
	uint	InstanceID:	INSTANCEID;
};

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

cbuffer InstanceParamsPS: register(b2)	// PS
{
	CInstanceDataPS InstanceDataPS[MAX_INSTANCE_COUNT];
}

// Vertex shaders

PSInSplattedConst VSMain(float2 Pos: POSITION, uint InstanceID: SV_InstanceID)
{
	uint RealInstanceID = FirstInstanceIndex + InstanceID;
	CPatchDataVS PatchData = InstanceDataVS[RealInstanceID];

	float4 VertexU;
	float4 ViewV;
	float4 SplatDetUV;
	CDLOD_ProcessVertex(Pos, PatchData.PatchXZ, PatchData.MorphConsts, EyePos, VertexU, ViewV, SplatDetUV);

	PSInSplattedConst Out;
	Out.Pos = mul(float4(VertexU.xyz, 1.0f), ViewProj);
	Out.PosWorldU = VertexU;
	Out.ViewV = ViewV;
	Out.SplatDetUV = SplatDetUV;
	Out.InstanceID = RealInstanceID;

	return Out;
}
//---------------------------------------------------------------------

// Pixel shaders

float4 PSMain(PSInSplattedConst In): SV_Target
{
	float2 UV = float2(In.PosWorldU.w, In.ViewV.w);
	float4 SplatWeights = SplatMap.Sample(SplatSampler, UV);

	float4 Albedo = float4(Splatting(SplatWeights, In.SplatDetUV.xy, LinearSampler), 1.f);

	// Sample geometry normals (top axis Z) and swizzle to model space (top axis Y)
	// Then sample per-pixel tangent space normal and transform it to model space
	//!!!splatting needed!
	float3 NMSample = NormalMap.Sample(LinearSampler, UV).xyz;
	NMSample = NMSample * float3(2.f, -2.f, 2.f) - float3(1.f, -1.f, 1.f);	
	float3 Normal = NMSample.xzy;
	float3 View = In.ViewV.xyz;
	float3 N = SampleNormal(TexNormalMap, LinearSampler, Normal, View, UV);

	//???splatting or always dielectric?
	float3 Reflectivity = TexReflectance.Sample(LinearSampler, UV).rgb;

	//???splatting needed?
	float Roughness = TexRoughness.Sample(LinearSampler, UV).r;
	float SqRoughness = Roughness * Roughness;

	float3 V = normalize(View);
	float NdotV = max(0.f, dot(N, V));

	float3 Lighting = PSAmbientPBR(Albedo, N, V, NdotV, Reflectivity, SqRoughness);

	CInstanceDataPS InstData = InstanceDataPS[In.InstanceID];
	int LightIndices[MAX_LIGHT_COUNT_PER_OBJECT] = (int[MAX_LIGHT_COUNT_PER_OBJECT])InstData.LightIndices;
	for (int i = 0; i < InstData.LightCount; ++i)
	{
		Lighting += PSDirectPBR(Albedo, N, V, NdotV, Reflectivity, SqRoughness, In.PosWorldU.xyz, Lights[LightIndices[i]]);
	}

	//???how to calc correct alpha?
	//Unity: Adding more reflectivity (as energy must be taken from somewhere) the diffuse level and the transparency will be reduced
	//automatically. Adding transparency will reduce diffuse level.
	// No translucent metals! Can use alpha-test, but not blend!
	return float4(Lighting, Albedo.a);
}
//---------------------------------------------------------------------
