
// CDLOD terrain rendering data and functions

cbuffer VSCDLODParams: register(b4)
{
	struct
	{
		float4 WorldToHM;
		float4 TerrainYInvSplat;	// x - Y scale, y - Y offset, zw - inv. splat size XZ
		float2 HMTexelSize;			// xy - height map texel size, zw - texture size for manual bilinear filtering (change to float4 for this case)
	} VSCDLODParams;
}

cbuffer PatchBatchParams: register(b5)
{
	float2	GridConsts;				// x - grid halfsize, y - inv. grid halfsize
	uint	FirstInstanceIndex;
}

Texture2D HeightMapVS: register(t0);
Texture2D NormalMap: register(t12);		// PS
sampler VSLinearSampler;

float CDLOD_SampleHeightMap(float2 UV) //, float MipLevel)
{
	return HeightMapVS.SampleLevel(VSLinearSampler, UV + VSCDLODParams.HMTexelSize.xy * 0.5, 0).x;
}
//---------------------------------------------------------------------

void CDLOD_ProcessVertex(float2 Pos, float4 PatchXZ, float2 MorphConsts, float3 Eye, out float4 VertexU, out float4 ViewV, out float4 SplatDetUV)
{
	// Get world position of the vertex
	float3 Vertex;
	Vertex.xz = Pos * PatchXZ.xy + PatchXZ.zw;
	float2 HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = CDLOD_SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	float3 View = Eye - Vertex;
	float DistanceToCamera = length(View);

	// Calculate morphed position on the XZ plane for smooth LOD transition
	float MorphK  = 1.0f - saturate(MorphConsts.x - DistanceToCamera * MorphConsts.y);
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

	VertexU = float4(Vertex, HMapUV.x);
	ViewV = float4(View, HMapUV.y);
	SplatDetUV = float4(Vertex.xz * VSCDLODParams.TerrainYInvSplat.zw, DetailUV);
}
//---------------------------------------------------------------------

float3 CDLOD_ProcessVertexOnly(float2 Pos, float4 PatchXZ, float2 MorphConsts, float3 Eye)
{
	// Get world position of the vertex
	float3 Vertex;
	Vertex.xz = Pos * PatchXZ.xy + PatchXZ.zw;
	float2 HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = CDLOD_SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	float3 View = Eye - Vertex;
	float DistanceToCamera = length(View);

	// Calculate morphed position on the XZ plane for smooth LOD transition
	float MorphK  = 1.0f - saturate(MorphConsts.x - DistanceToCamera * MorphConsts.y);
	float2 FracPart = frac(Pos * GridConsts.xx) * GridConsts.yy;
	const float2 PosMorphed = Pos - FracPart * MorphK;

	// Get morphed world position of the vertex
	Vertex.xz = PosMorphed * PatchXZ.xy + PatchXZ.zw;
	HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = CDLOD_SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	return Vertex;
}
//---------------------------------------------------------------------
