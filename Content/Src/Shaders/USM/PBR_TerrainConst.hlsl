#include "Globals.hlsl"
#include "NormalMapping.hlsl"
#include "CDLOD.hlsl"
#include "Splatting.hlsl"
#include "PBR.hlsl"

#ifndef MAX_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT 64
#endif

// For constant instancing, get lights by instance ID in PS
struct PSInSplattedConst
{
	float4	Pos:									SV_Position;
	float3	PosWorld:								WORLDPOS;
	float3	Normal:									NORMAL;
	float3	View:									VIEW;
	float4	VertexConsts:							TEXCOORD0;	//???need?
	float4	SplatDetUV:								TEXCOORD1;
	uint	InstanceID:								INSTANCEID;
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
	CPatchDataVS PatchData = InstanceDataVS[InstanceID];
	float4 PatchXZ = PatchData.PatchXZ;
	float2 MorphConsts = PatchData.MorphConsts;

	float3 Vertex;
	Vertex.xz = Pos * PatchXZ.xy + PatchXZ.zw;
	float2 HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = CDLOD_SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	float MorphK  = 1.0f - clamp(MorphConsts.x - distance(Vertex, EyePos) * MorphConsts.y, 0.0f, 1.0f);
	float2 FracPart = frac(Pos * GridConsts.xx) * GridConsts.yy;
	const float2 PosMorphed = Pos - FracPart * MorphK;

	Vertex.xz = PosMorphed * PatchXZ.xy + PatchXZ.zw;
	HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = CDLOD_SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	float DetailMorphK = 0.0;
	float2 DetailUV = float2(0.0, 0.0);
	/* Detail map
	if (g_useDetailMap)
	{
		DetailUV = PosMorphed * GridToDM.xy + GridToDM.zw;
									//LODLevel			// detailLODLevelsAffected
		DetailMorphK = 1 - saturate(g_quadScale.z + 2.0 - DetailConsts.w) * MorphK;
		Vertex.z += DetailMorphK * (CDLOD_SampleHeightMap(VSHeightDetailSampler, DetailUV.xy) - 0.5) * DetailConsts.z;
	}
	*/

	float3 Normal = CDLOD_SampleNormalMap(HMapUV);

	PSInSplattedConst Out;
	Out.PosWorld = Vertex;
	Out.Pos = mul(float4(Vertex, 1.0f), ViewProj);
	Out.Normal = normalize(Normal);
	Out.View = EyePos - Vertex;
	Out.VertexConsts = float4(MorphK, DetailMorphK, Out.Pos.w, distance(Vertex, EyePos));
	Out.SplatDetUV = float4(Vertex.xz * VSCDLODParams.TerrainYInvSplat.zw, DetailUV);
	Out.InstanceID = InstanceID;

	return Out;
}
//---------------------------------------------------------------------

// Pixel shaders

float4 PSMain(PSInSplattedConst In): SV_Target
{
	float2 UV = In.PosWorld.xz * PSCDLODParams.WorldToHM.xy + PSCDLODParams.WorldToHM.zw;
	float4 SplatWeights = SplatMap.Sample(SplatSampler, UV);

	float4 Albedo = float4(Splatting(SplatWeights, In.SplatDetUV.xy, LinearSampler), 1.f);

	float3 View = In.View;
	float3 N = SampleNormal(TexNormalMap, LinearSampler, In.Normal, View, UV);

	float3 Reflectivity = TexReflectance.Sample(LinearSampler, UV).rgb;
	//float3 Reflectivity = 0.04f.xxx; // Dielectric, in metalness workflow can use uniform float for different dielectrics like gems
	//float3 Reflectivity = float3(1.0f, 0.71f, 0.29f); // Gold
	//float3 Reflectivity = float3(0.95f, 0.64f, 0.54f); // Copper

	float Roughness = TexRoughness.Sample(LinearSampler, UV).r;
	float SqRoughness = Roughness * Roughness;

	float3 V = normalize(View);
	float NdotV = max(0.f, dot(N, V));

	float3 Lighting = PSAmbientPBR(Albedo, N, V, NdotV, Reflectivity, SqRoughness);

	CInstanceDataPS InstData = InstanceDataPS[In.InstanceID];
	int LightIndices[MAX_LIGHT_COUNT_PER_OBJECT] = (int[MAX_LIGHT_COUNT_PER_OBJECT])InstData.LightIndices;
	for (int i = 0; i < InstData.LightCount; ++i)
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
