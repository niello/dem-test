#include "Globals.hlsl"
#include "CDLOD.hlsl"
#include "Splatting.hlsl"

#if DEM_LIGHT_COUNT > 0
#define DEM_PBR_LIGHT_INDEX_COUNT DEM_LIGHT_COUNT
#else
#define DEM_PBR_LIGHT_INDEX_COUNT 1
#endif
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
	float4	VertexConsts:							TEXCOORD0;
	float4	SplatDetUV:								TEXCOORD1;
	uint	InstanceID:								INSTANCEID;
};

// For stream instancing, get lights from IA stream in VS and pass to PS
struct PSInSplattedStream
{
	float4	Pos:									SV_Position;
	float3	PosWorld:								WORLDPOS;
	float3	Normal:									NORMAL;
	float3	View:									VIEW;
	float4	VertexConsts:							TEXCOORD0;
	float4	SplatDetUV:								TEXCOORD1;
#if DEM_LIGHT_VECTOR_COUNT > 0
	int4	LightIndices[DEM_LIGHT_VECTOR_COUNT]:	TEXCOORD2;
#endif
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

PSInSplattedConst VSMainConst(float2 Pos: POSITION, uint InstanceID: SV_InstanceID)
{
	CPatchDataVS PatchData = InstanceDataVS[InstanceID];
	float4 PatchXZ = PatchData.PatchXZ;
	float2 MorphConsts = PatchData.MorphConsts;

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

	PSInSplattedConst Out;
	Out.PosWorld = Vertex;
	Out.Pos = mul(float4(Vertex, 1.0f), ViewProj);

	//Out.Normal = normalize(Normal);
	//!!!DBG TMP! to suppress warning
	Out.Normal = float3(0.f, 1.f, 0.f);

	Out.View = EyePos - Vertex;
	Out.VertexConsts = float4(MorphK, DetailMorphK, Out.Pos.w, distance(Vertex, EyePos));
	Out.SplatDetUV = float4(Vertex.xz * VSCDLODParams.TerrainYInvSplat.zw, DetailUV);
	Out.InstanceID = InstanceID;

	return Out;
}
//---------------------------------------------------------------------

PSInSplattedStream VSMainStream(float2	Pos:			POSITION,
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

	PSInSplattedStream Out;
	Out.PosWorld = Vertex;
	Out.Pos = mul(float4(Vertex, 1.0f), ViewProj);

	//Out.Normal = normalize(Normal);
	//!!!DBG TMP! to suppress warning
	Out.Normal = float3(0.f, 1.f, 0.f);

	Out.View = EyePos - Vertex;
	Out.VertexConsts = float4(MorphK, DetailMorphK, Out.Pos.w, distance(Vertex, EyePos));
	Out.SplatDetUV = float4(Vertex.xz * VSCDLODParams.TerrainYInvSplat.zw, DetailUV);
#if DEM_LIGHT_VECTOR_COUNT > 0
	Out.LightIndices = LightIndices;
#endif

	return Out;
}
//---------------------------------------------------------------------

// Pixel shaders

//!!!see old code version for more code!
float4 PSMainConst(PSInSplattedStream In): SV_Target
{
	//get interpolated normal
	//sample normal map
	//!!!TMP!
	//float3 N = normalize(In.Normal.xyz);
	
	float2 UV = In.PosWorld.xz * PSCDLODParams.WorldToHM.xy + PSCDLODParams.WorldToHM.zw;
	float4 SplatWeights = SplatMap.Sample(SplatSampler, UV);
	
	//!!!pass albedo into PBR as argument!

	float3 AlbedoRGB = Splatting(SplatWeights, In.SplatDetUV.xy, LinearSampler);
	return float4(AlbedoRGB, 1.f);
}
//---------------------------------------------------------------------

//!!!see old code version for more code!
float4 PSMainStream(PSInSplattedStream In): SV_Target
{
	//get interpolated normal
	//sample normal map
	//!!!TMP!
	//float3 N = normalize(In.Normal.xyz);
	
	float2 UV = In.PosWorld.xz * PSCDLODParams.WorldToHM.xy + PSCDLODParams.WorldToHM.zw;
	float4 SplatWeights = SplatMap.Sample(SplatSampler, UV);

	float3 AlbedoRGB = Splatting(SplatWeights, In.SplatDetUV.xy, LinearSampler);

//!!!ambient must be applied even without direct lights!
#if DEM_LIGHT_COUNT > 0
	return PSPBR(float4(AlbedoRGB, 1.f), UV, In.PosWorld, In.Normal, In.View, DEM_LIGHT_COUNT, (int[DEM_LIGHT_COUNT])In.LightIndices);
#else
	return float4(AlbedoRGB, 1.f);
#endif
}
//---------------------------------------------------------------------
