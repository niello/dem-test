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
	float4	PosWorldU:								WORLDPOS;
	float4	ViewV:									VIEW;
	//float4	VertexConsts:							TEXCOORD0;	//???need? not used in the current PS!
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
//!!!fix degenerate patch, which turns to a quarterpatch! Test terrain, front view (look at -Z), one of the nearest full patches

	// Get world position of the vertex
	float3 Vertex;
	Vertex.xz = Pos * PatchXZ.xy + PatchXZ.zw;
	float2 HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = CDLOD_SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	float3 View = EyePos - Vertex;
	float DistanceToCamera = length(View);
	
	// Calculate morphed position on the XZ plane for smooth LOD transition
	float MorphK  = 1.0f - clamp(MorphConsts.x - DistanceToCamera * MorphConsts.y, 0.0f, 1.0f);
	float2 FracPart = frac(Pos * GridConsts.xx) * GridConsts.yy;
	const float2 PosMorphed = Pos - FracPart * MorphK;

	// Get morphed world position of the vertex
	Vertex.xz = PosMorphed * PatchXZ.xy + PatchXZ.zw;
	HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = CDLOD_SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	float DetailMorphK = 0.0;
	float2 DetailUV = float2(0.0, 0.0);
	/* Detail map:
	DetailUV = PosMorphed * WorldToDM.xy + WorldToDM.zw;
								//LODLevel			// detailLODLevelsAffected
	DetailMorphK = 1 - saturate(g_quadScale.z + 2.0 - DetailConsts.w) * MorphK;
	
	// Add detail heightmap
	Vertex.z(y?) += DetailMorphK * (CDLOD_SampleHeightMap(DetailUV.xy) - 0.5) * DetailConsts.z;
	*/

	PSInSplattedStream Out;
	Out.Pos = mul(float4(Vertex, 1.0f), ViewProj);
	Out.PosWorldU = float4(Vertex, HMapUV.x);
	Out.ViewV = float4(View, HMapUV.y);
	//Out.VertexConsts = float4(MorphK, DetailMorphK, Out.Pos.w, DistanceToCamera);
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
	//float3 Reflectivity = 0.04f.xxx; // Dielectric, in metalness workflow can use uniform float for different dielectrics like gems
	//float3 Reflectivity = float3(1.0f, 0.71f, 0.29f); // Gold
	//float3 Reflectivity = float3(0.95f, 0.64f, 0.54f); // Copper

	//???splatting needed?
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
		Lighting += PSDirectPBR(Albedo, N, V, NdotV, Reflectivity, SqRoughness, In.PosWorldU.xyz, Lights[LightIndex]);
	}
#else
	int LightIndex = LightIndices[0];
	if (LightIndex >= 0) Lighting += PSDirectPBR(Albedo, N, V, NdotV, Reflectivity, SqRoughness, In.PosWorldU.xyz, Lights[LightIndex]);
#endif
#endif

	//???how to calc correct alpha?
	//Unity: Adding more reflectivity (as energy must be taken from somewhere) the diffuse level and the transparency will be reduced
	//automatically. Adding transparency will reduce diffuse level.
	// No translucent metals! Can use alpha-test, but not blend!
	return float4(Lighting, Albedo.a);
}
//---------------------------------------------------------------------
