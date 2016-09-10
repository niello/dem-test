
cbuffer VSCDLODParams: register(b2)
{
	struct
	{
		float4 WorldToHM;
		float4 TerrainYInvSplat;	// x - Y scale, y - Y offset, zw - inv. splat size XZ
		float2 HMTexelSize;			// xy - height map texel size, zw - texture size for manual bilinear filtering (change to float4 for this case)
	} VSCDLODParams;
}

cbuffer PSCDLODParams: register(b2)
{
	struct
	{
		float4 WorldToHM;
	} PSCDLODParams;
}

cbuffer GridParams: register(b3)
{
	float2 GridConsts;				// x - grid halfsize, y - inv. grid halfsize
}

Texture2D HeightMap;
sampler VSHeightSampler;

//???height map can be loaded with mips?
float SampleHeightMap(float2 UV) //, float MipLevel)
{
	return HeightMap.SampleLevel(VSHeightSampler, UV + VSCDLODParams.HMTexelSize.xy * 0.5, 0).x;
}
//---------------------------------------------------------------------
