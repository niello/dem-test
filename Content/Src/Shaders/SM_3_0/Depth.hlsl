
struct VSSceneIn
{
	float3 Pos: POSITION;
	float2 Tex: TEXCOORD;
};

struct PSSceneIn
{
	float4 Pos: POSITION;
	float2 Tex: TEXCOORD;
};

matrix ViewProj: register(c0) <string CBuffer = "CameraParams"; int SlotIndex = 0;>;
float3 EyePos: register(c4) <string CBuffer = "CameraParams"; int SlotIndex = 0;>;
matrix WorldMatrix: register(c5) <string CBuffer = "InstanceParams"; int SlotIndex = 2;>;

//!!!may premultiply and pass WVP instead of WorldMatrix!
float4 VSMainOpaque(float3 Pos: POSITION): POSITION
{
	float4 OutPos = mul(float4(Pos, 1), WorldMatrix);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
}
//---------------------------------------------------------------------

#ifndef MAX_BONES_PER_PALETTE
#define MAX_BONES_PER_PALETTE 72
#endif
#ifndef MAX_BONES_PER_VERTEX
#define MAX_BONES_PER_VERTEX 4
#endif
column_major float4x3 SkinPalette[MAX_BONES_PER_PALETTE]: register(c40) <string CBuffer = "SkinParams"; int SlotIndex = 2;>;

//???why indices are float? use uint4
float4 SkinnedPosition(const float4 InPos, const float4 Weights, const float4 Indices)
{
	//// need to re-normalize weights because of compression
	//float4 NormWeights = Weights / dot(Weights, float4(1.0, 1.0, 1.0, 1.0));

	float3 OutPos = mul(InPos, SkinPalette[Indices[0]]).xyz * Weights[0];
	for (int i = 1; i < MAX_BONES_PER_VERTEX; i++)
		OutPos += mul(InPos, SkinPalette[Indices[i]]).xyz * Weights[i];
	return float4(OutPos, 1.0f);
}
//---------------------------------------------------------------------

float4 VSMainSkinnedOpaque(	float4	Pos:		POSITION,
							float4	Weights:	BLENDWEIGHT,
							float4	Indices:	BLENDINDICES): POSITION
{
	float4 OutPos = SkinnedPosition(Pos, Weights, Indices);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
}
//---------------------------------------------------------------------

PSSceneIn VSMainAlphaTest(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(In.Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = In.Tex;
	return Out;
}
//---------------------------------------------------------------------

texture HeightMap;
sampler VSHeightSampler { Texture = HeightMap; };

struct
{
	float4 WorldToHM;
	float4 TerrainYInvSplat;	// x - Y scale, y - Y offset, zw - inv. splat size XZ
	float4 GridConsts;			// x - grid halfsize, y - inv. grid halfsize, zw - texel size
	//float2 HMTextureSize;			// xy - texture size for manual bilinear filtering (change to float4 for this case)
} CDLODParams: register(c5) <string CBuffer = "CDLODParams"; int SlotIndex = 2;>;

/*
float4 WorldToHM: register(c5) <string CBuffer = "CDLODParams"; int SlotIndex = 2;>;
float4 TerrainYInvSplat: register(c6) <string CBuffer = "CDLODParams"; int SlotIndex = 2;>;	// x - Y scale, y - Y offset, zw - inv. splat size XZ
float2 GridConsts: register(c7) <string CBuffer = "CDLODParams"; int SlotIndex = 2;>;		// x - grid halfsize, y - inv. grid halfsize
float2 HMTexInfo: register(c8) <string CBuffer = "CDLODParams"; int SlotIndex = 2;>;		// xy - texel size, zw - texture size for manual bilinear filtering (change to float4 for this case)
*/

//???height map can be loaded with mips?
float SampleHeightMap(float2 UV) //, float MipLevel)
{
	return tex2Dlod(VSHeightSampler, float4(UV + CDLODParams.GridConsts.zw * 0.5, 0, 0)).x;
}
//---------------------------------------------------------------------

void VSMainCDLOD(	float2	Pos:			POSITION,
					float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
					float2	MorphConsts:	TEXCOORD1,	// x - end / (end - start), y - 1 / (end - start)
				out	float4	oPos:			POSITION)
{
	float3 Vertex;
	Vertex.xz = Pos * PatchXZ.xy + PatchXZ.zw;
	float2 HMapUV = Vertex.xz * CDLODParams.WorldToHM.xy + CDLODParams.WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * CDLODParams.TerrainYInvSplat.x + CDLODParams.TerrainYInvSplat.y;

	float MorphK  = 1.0f - clamp(MorphConsts.x - distance(Vertex, EyePos) * MorphConsts.y, 0.0f, 1.0f);
	float2 FracPart = frac(Pos * CDLODParams.GridConsts.xx) * CDLODParams.GridConsts.yy;
	const float2 PosMorphed = Pos - FracPart * MorphK;

	Vertex.xz = PosMorphed * PatchXZ.xy + PatchXZ.zw;
	HMapUV = Vertex.xz * CDLODParams.WorldToHM.xy + CDLODParams.WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * CDLODParams.TerrainYInvSplat.x + CDLODParams.TerrainYInvSplat.y;

	oPos = mul(float4(Vertex, 1), ViewProj);
}
//---------------------------------------------------------------------

Texture2D TexAlbedo;
sampler LinearSampler { Texture = TexAlbedo; };

float4 PSMainAlphaTest(PSSceneIn In): COLOR
{
	float Alpha = tex2D(LinearSampler, In.Tex).a;
	clip(Alpha - 0.5);
	return Alpha;
}
//---------------------------------------------------------------------
