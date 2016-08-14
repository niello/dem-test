
struct VSSceneIn
{
	float3 Pos: POSITION;
	float2 Tex: TEXCOORD;
};

struct PSSceneIn
{
	float4 Pos: SV_Position;
	float2 Tex: TEXCOORD;
};

cbuffer CameraParams: register(b0)
{
	matrix ViewProj;
	float3 EyePos;
}

cbuffer InstanceParams: register(b2)
{
	matrix WorldMatrix;
}

#ifndef MAX_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT 64
#endif

cbuffer InstanceParams: register(b2)
{
	matrix InstanceData[MAX_INSTANCE_COUNT];
}

//!!!may premultiply and pass WVP instead of WorldMatrix!
float4 VSMainOpaque(float3 Pos: POSITION): SV_Position
{
	float4 OutPos = mul(float4(Pos, 1), WorldMatrix);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
}
//---------------------------------------------------------------------

float4 VSMainInstancedConstOpaque(float3 Pos: POSITION, uint InstanceID: SV_InstanceID): SV_Position
{
	float4 OutPos = mul(float4(Pos, 1), InstanceData[InstanceID]);
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

tbuffer SkinParams: register(t0)
{
	matrix SkinPalette[MAX_BONES_PER_PALETTE];
}

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

float4 VSMainSkinnedOpaque(float4 Pos: POSITION, float4 Weights: BLENDWEIGHT, float4 Indices: BLENDINDICES): SV_Position
{
	float4 OutPos = SkinnedPosition(Pos, Weights, Indices);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
}
//---------------------------------------------------------------------

//!!!DUPLICATE CODE, see PBR VSMain!
PSSceneIn VSMainAlphaTest(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(In.Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = In.Tex;
	return Out;
}
//---------------------------------------------------------------------

//!!!DUPLICATE CODE, see PBR VSMainInstancedConst!
PSSceneIn VSMainInstancedConstAlphaTest(float3 Pos: POSITION, float2 Tex: TEXCOORD, uint InstanceID: SV_InstanceID)
{
	float4x4 InstWorld = InstanceData[InstanceID];

	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(Pos, 1), InstWorld);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = Tex;
	return Out;
}
//---------------------------------------------------------------------

Texture2D HeightMap;
sampler VSHeightSampler;

cbuffer CDLODParams: register(b2)
{
	struct
	{
		float4 WorldToHM;
		float4 TerrainYInvSplat;	// x - Y scale, y - Y offset, zw - inv. splat size XZ
		float2 HMTexelSize;			// xy - height map texel size, zw - texture size for manual bilinear filtering (change to float4 for this case)
	} CDLODParams;
}

cbuffer GridParams: register(b3)
{
	float2 GridConsts;				// x - grid halfsize, y - inv. grid halfsize
}

//???height map can be loaded with mips?
float SampleHeightMap(float2 UV) //, float MipLevel)
{
	return HeightMap.SampleLevel(VSHeightSampler, UV + CDLODParams.HMTexelSize.xy * 0.5, 0).x;
}
//---------------------------------------------------------------------

void VSMainCDLOD(	float2	Pos:			POSITION,
					float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
					float2	MorphConsts:	TEXCOORD1,	// x - end / (end - start), y - 1 / (end - start)
				out	float4	oPos:			SV_Position)
{
	float3 Vertex;
	Vertex.xz = Pos * PatchXZ.xy + PatchXZ.zw;
	float2 HMapUV = Vertex.xz * CDLODParams.WorldToHM.xy + CDLODParams.WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * CDLODParams.TerrainYInvSplat.x + CDLODParams.TerrainYInvSplat.y;

	float MorphK  = 1.0f - clamp(MorphConsts.x - distance(Vertex, EyePos) * MorphConsts.y, 0.0f, 1.0f);
	float2 FracPart = frac(Pos * GridConsts.xx) * GridConsts.yy;
	const float2 PosMorphed = Pos - FracPart * MorphK;

	Vertex.xz = PosMorphed * PatchXZ.xy + PatchXZ.zw;
	HMapUV = Vertex.xz * CDLODParams.WorldToHM.xy + CDLODParams.WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * CDLODParams.TerrainYInvSplat.x + CDLODParams.TerrainYInvSplat.y;

	oPos = mul(float4(Vertex, 1), ViewProj);
}
//---------------------------------------------------------------------

Texture2D TexAlbedo;
sampler LinearSampler;

void PSMainAlphaTest(PSSceneIn In)
{
	float Alpha = TexAlbedo.Sample(LinearSampler, In.Tex).a;
	clip(Alpha - 0.5);
}
//---------------------------------------------------------------------
