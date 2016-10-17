#include "Globals.hlsl"
#include "NormalMapping.hlsl"
#include "CDLOD.hlsl"
#include "Splatting.hlsl"

#if DEM_LIGHT_COUNT > 0
#define DEM_PBR_LIGHT_INDEX_COUNT DEM_LIGHT_COUNT
#else
#define DEM_PBR_LIGHT_INDEX_COUNT 1
#endif
#include "PBR.hlsl"

// For stream instancing, get lights from IA stream in VS and pass to PS
struct PSInSplattedStream
{
	float4	Pos:									SV_Position;
	float3	PosWorld:								WORLDPOS;
	float3	View:									VIEW;
	float4	VertexConsts:							TEXCOORD0;	//???need?
	float4	SplatDetUV:								TEXCOORD1;
#if DEM_LIGHT_VECTOR_COUNT > 0
	int4	LightIndices[DEM_LIGHT_VECTOR_COUNT]:	TEXCOORD2;
#endif
};

// Vertex shaders

PSInSplattedStream VSMain(	float2	Pos:			POSITION,
							float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
							float2	MorphConsts:	TEXCOORD1	// x - end / (end - start), y - 1 / (end - start)
#if DEM_LIGHT_VECTOR_COUNT > 0
							, int4	LightIndices[DEM_LIGHT_VECTOR_COUNT]:	TEXCOORD2
#endif
						)
{
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
		Vertex.z(y?) += DetailMorphK * (CDLOD_SampleHeightMap(VSHeightDetailSampler, DetailUV.xy) - 0.5) * DetailConsts.z;
	}
	*/

	PSInSplattedStream Out;
	Out.PosWorld = Vertex;
	Out.Pos = mul(float4(Vertex, 1.0f), ViewProj);
	Out.View = EyePos - Vertex;
	Out.VertexConsts = float4(MorphK, DetailMorphK, Out.Pos.w, length(Out.View));
	Out.SplatDetUV = float4(Vertex.xz * VSCDLODParams.TerrainYInvSplat.zw, DetailUV);
#if DEM_LIGHT_VECTOR_COUNT > 0
	Out.LightIndices = LightIndices;
#endif

	return Out;
}
//---------------------------------------------------------------------

// Pixel shaders

float4 PSMain(PSInSplattedStream In): SV_Target
{
	float2 UV = In.PosWorld.xz * PSCDLODParams.WorldToHM.xy + PSCDLODParams.WorldToHM.zw;
	float4 SplatWeights = SplatMap.Sample(SplatSampler, UV);

	float4 Albedo = float4(Splatting(SplatWeights, In.SplatDetUV.xy, LinearSampler), 1.f);

	float3 NMSample = NormalMap.Sample(LinearSampler, UV).xyz;
	NMSample = NMSample * float3(2.f, -2.f, 2.f) - float3(1.f, -1.f, 1.f);	
	float3 Normal = NMSample.xzy;

	float3 View = In.View;
	float3 N = SampleNormal(TexNormalMap, LinearSampler, Normal, View, UV);

	float3 Reflectivity = TexReflectance.Sample(LinearSampler, UV).rgb;
	//float3 Reflectivity = 0.04f.xxx; // Dielectric, in metalness workflow can use uniform float for different dielectrics like gems
	//float3 Reflectivity = float3(1.0f, 0.71f, 0.29f); // Gold
	//float3 Reflectivity = float3(0.95f, 0.64f, 0.54f); // Copper

	float Roughness = TexRoughness.Sample(LinearSampler, UV).r;
	float SqRoughness = Roughness * Roughness;

	float3 V = normalize(View);
	float NdotV = max(0.f, dot(N, V));

	float3 Lighting = PSAmbientPBR(Albedo, N, V, NdotV, Reflectivity, SqRoughness);

#if DEM_LIGHT_COUNT > 0
	int LightIndices[DEM_LIGHT_COUNT] = (int[DEM_LIGHT_COUNT])In.LightIndices;
#if DEM_LIGHT_COUNT > 1 // To suppress loop unroll warning
	for (int i = 0; i < DEM_LIGHT_COUNT; ++i)
	{
		int LightIndex = LightIndices[i];
		if (LightIndex < 0) break;
		Lighting += PSDirectPBR(Albedo, N, V, NdotV, Reflectivity, SqRoughness, In.PosWorld, Lights[LightIndex]);
	}
#else
	int LightIndex = LightIndices[0];
	if (LightIndex >= 0) Lighting += PSDirectPBR(Albedo, N, V, NdotV, Reflectivity, SqRoughness, In.PosWorld, Lights[LightIndex]);
#endif
#endif

	//???how to calc correct alpha?
	//Unity: Adding more reflectivity (as energy must be taken from somewhere) the diffuse level and the transparency will be reduced
	//automatically. Adding transparency will reduce diffuse level.
	// No translucent metals! Can use alpha-test, but not blend!
	return float4(Lighting, Albedo.a);
}
//---------------------------------------------------------------------
