#include "Globals.hlsl"
#include "PBR.hlsl"
#include "CDLOD.hlsl"
#include "Splatting.hlsl"

// For constant instancing, get lights by instance ID in PS
struct PSInSplatted
{
	float4	Pos:									SV_Position;
	float4	PosWorld:								WORLDPOS;
	float3	Normal:									NORMAL;
	float4	VertexConsts:							TEXCOORD0;
	float4	SplatDetUV:								TEXCOORD1;
	uint	InstanceID:								INSTANCEID;
};

// For stream instancing, get lights from IA stream in VS and pass to PS
struct PSInSplattedWithLights
{
	float4	Pos:									SV_Position;
	float4	PosWorld:								WORLDPOS;
	float3	Normal:									NORMAL;
	float4	VertexConsts:							TEXCOORD0;
	float4	SplatDetUV:								TEXCOORD1;
#if DEM_LIGHT_VECTOR_COUNT > 0
	int4	LightIndices[DEM_LIGHT_VECTOR_COUNT]:	TEXCOORD2;
#endif
};

// Vertex shaders

PSInSplattedWithLights VSMain(	float2	Pos:			POSITION,
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
	Vertex.y = SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	float MorphK  = 1.0f - clamp(MorphConsts.x - distance(Vertex, EyePos) * MorphConsts.y, 0.0f, 1.0f);
	float2 FracPart = frac(Pos * GridConsts.xx) * GridConsts.yy;
	const float2 PosMorphed = Pos - FracPart * MorphK;

	Vertex.xz = PosMorphed * PatchXZ.xy + PatchXZ.zw;
	HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	float DetailMorphK = 0.0;
	float2 DetailUV = float2(0.0, 0.0);
	/* Detail map
	if (g_useDetailMap)
	{
		DetailUV = PosMorphed * GridToDM.xy + GridToDM.zw;
									//LODLevel			// detailLODLevelsAffected
		DetailMorphK = 1 - saturate(g_quadScale.z + 2.0 - DetailConsts.w) * MorphK;
		Vertex.z += DetailMorphK * (SampleHeightMap(VSHeightDetailSampler, DetailUV.xy) - 0.5) * DetailConsts.z;
	}
	*/

	// We invert Y to load normal maps with +Y (OpenGL-style)
	//???pack to one texture with height map and always sample linearly?
	//float3 Normal = NormalMapVS.Sample(VSLinearSampler, HMapUV).xyz * float3(2.f, -2.f, 2.f) - float3(1.f, -1.f, 1.f);

	PSInSplattedWithLights Out;
	Out.PosWorld = float4(Vertex, 1.0f);
	Out.Pos = mul(Out.PosWorld, ViewProj);

	//Out.Normal = normalize(Normal);
	//!!!DBG TMP! to suppress warning
	Out.Normal = float3(0.f, 1.f, 0.f);

	Out.VertexConsts = float4(MorphK, DetailMorphK, Out.Pos.w, distance(Vertex, EyePos));
	Out.SplatDetUV = float4(Vertex.xz * VSCDLODParams.TerrainYInvSplat.zw, DetailUV);
#if DEM_LIGHT_VECTOR_COUNT > 0
	Out.LightIndices = LightIndices;
#endif
	return Out;
}
//---------------------------------------------------------------------

//!!!see old code version for more code!
float4 PSMain(PSInSplattedWithLights In): SV_Target
{
	//get interpolated normal
	//sample normal map
	//!!!TMP!
	//float3 N = normalize(In.Normal.xyz);
	
	float2 UV = In.PosWorld.xz * PSCDLODParams.WorldToHM.xy + PSCDLODParams.WorldToHM.zw;
	float4 SplatWeights = SplatMap.Sample(SplatSampler, UV);

	float3 TexDiffuse = Splatting(SplatWeights, In.SplatDetUV.xy, LinearSampler);
	return float4(TexDiffuse, 1.f);
}
//---------------------------------------------------------------------
