
// CDLOD terrain rendering data and functions

struct
{
	float4 WorldToHM;
	float4 TerrainYInvSplat;	// x - Y scale, y - Y offset, zw - inv. splat size XZ
	float2 HMTexelSize;			// xy - height map texel size, zw - texture size for manual bilinear filtering (change to float4 for this case)
} VSCDLODParams: register(c6) <string CBuffer = "VSCDLODParams"; int SlotIndex = 2;>;

struct
{
	float4 WorldToHM;
} PSCDLODParams: register(c6) <string CBuffer = "PSCDLODParams"; int SlotIndex = 0;>;

float2 GridConsts: register(c5) <string CBuffer = "VSCDLODParams"; int SlotIndex = 2;>; // x - grid halfsize, y - inv. grid halfsize

texture HeightMap;
sampler VSHeightSampler { Texture = HeightMap; };

//???height map can be loaded with mips?
float SampleHeightMap(float2 UV) //, float MipLevel)
{
	return tex2Dlod(VSHeightSampler, float4(UV + VSCDLODParams.HMTexelSize.xy * 0.5, 0, 0)).x;
}
//---------------------------------------------------------------------
