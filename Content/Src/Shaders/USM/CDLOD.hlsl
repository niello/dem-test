
// CDLOD terrain rendering data and functions

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

cbuffer GridParams: register(b4)
{
	float2 GridConsts;				// x - grid halfsize, y - inv. grid halfsize
}

Texture2D HeightMapVS: register(t0);
Texture2D NormalMap: register(t12);		// PS
sampler VSLinearSampler;

//???height map can be loaded with mips?
float CDLOD_SampleHeightMap(float2 UV) //, float MipLevel)
{
	return HeightMapVS.SampleLevel(VSLinearSampler, UV + VSCDLODParams.HMTexelSize.xy * 0.5, 0).x;
}
//---------------------------------------------------------------------
