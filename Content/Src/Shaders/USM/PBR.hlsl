sampler LinearSampler;

struct PSSceneIn
{
	float4 Pos: SV_Position;
	float2 Tex: TEXCOORD;
};

cbuffer CameraParams: register(b0)
{
	matrix	ViewProj;
	float3	EyePos;
}

cbuffer MaterialParams: register(b1)
{
	float4 MtlDiffuse;
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

PSSceneIn VSMain(float3 Pos: POSITION, float2 Tex: TEXCOORD)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = Tex;
	return Out;
}
//---------------------------------------------------------------------

PSSceneIn VSMainInstanced(	float3 Pos: POSITION,
							float2 Tex: TEXCOORD,
							float4 World1: TEXCOORD4,
							float4 World2: TEXCOORD5,
							float4 World3: TEXCOORD6,
							float4 World4: TEXCOORD7)
{
	float4x4 InstWorld = float4x4(World1, World2, World3, World4);

	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(Pos, 1), InstWorld);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = Tex;
	return Out;
}
//---------------------------------------------------------------------

PSSceneIn VSMainInstancedConst(float3 Pos: POSITION, float2 Tex: TEXCOORD, uint InstanceID: SV_InstanceID)
{
	float4x4 InstWorld = InstanceData[InstanceID];

	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(Pos, 1), InstWorld);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = Tex;
	return Out;
}
//---------------------------------------------------------------------

#ifndef MAX_BONES_PER_PALETTE
#define MAX_BONES_PER_PALETTE 72
#endif
#ifndef MAX_BONES_PER_VERTEX
#define MAX_BONES_PER_VERTEX 4
#endif
//matrix<float,4,3> SkinPalette[MAX_BONES_PER_PALETTE];

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

PSSceneIn VSMainSkinned(float4	Pos:		POSITION,
						float4	Weights:	BLENDWEIGHT,
						float4	Indices:	BLENDINDICES,
						float2	Tex:		TEXCOORD0)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = SkinnedPosition(Pos, Weights, Indices);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = Tex;
	return Out;
}
//---------------------------------------------------------------------

Texture2D HeightMap;
sampler VSHeightSampler;

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

//???height map can be loaded with mips?
float SampleHeightMap(float2 UV) //, float MipLevel)
{
	return HeightMap.SampleLevel(VSHeightSampler, UV + VSCDLODParams.HMTexelSize.xy * 0.5, 0).x;
}
//---------------------------------------------------------------------

struct PSInSplatted
{
	float4 Pos:				SV_Position;
	float4 VertexConsts:	TEXCOORD0;
	float4 SplatDetUV:		TEXCOORD1;
	float4 PosWorld:		TEXCOORD2;
};

PSInSplatted VSMainCDLOD(	float2	Pos:			POSITION,
							float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
							float2	MorphConsts:	TEXCOORD1)	// x - end / (end - start), y - 1 / (end - start)
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

	PSInSplatted Out;
	Out.PosWorld = float4(Vertex, 1.0);
	Out.Pos = mul(Out.PosWorld, ViewProj);
	Out.VertexConsts = float4(MorphK, DetailMorphK, Out.Pos.w, distance(Vertex, EyePos));
	Out.SplatDetUV = float4(Vertex.xz * VSCDLODParams.TerrainYInvSplat.zw, DetailUV);
	return Out;
}
//---------------------------------------------------------------------

Texture2D TexAlbedo;

float4 PSMain(PSSceneIn In): SV_Target
{
	return TexAlbedo.Sample(LinearSampler, In.Tex) * MtlDiffuse;
}
//---------------------------------------------------------------------

/*
float4 PSMainAlphaTest(PSSceneIn In): SV_Target
{
	float4 Albedo = TexAlbedo.Sample(LinearSampler, In.Tex);
	clip(Albedo.a - 0.5);
	return Albedo * MtlDiffuse;
}
//---------------------------------------------------------------------
*/

Texture2D SplatMap;
Texture2D SplatTex0;
Texture2D SplatTex1;
Texture2D SplatTex2;
Texture2D SplatTex3;
Texture2D SplatTex4;
sampler SplatSampler;

// Version with normals can be found in old code
float3 Splatting(float4 SplatWeights, float2 SplatTexUV)
{
	float4 SplatColors[5];
	SplatColors[0] = SplatTex0.Sample(LinearSampler, SplatTexUV);
	SplatColors[1] = SplatTex1.Sample(LinearSampler, SplatTexUV);
	SplatColors[2] = SplatTex2.Sample(LinearSampler, SplatTexUV);
	SplatColors[3] = SplatTex3.Sample(LinearSampler, SplatTexUV);
	SplatColors[4] = SplatTex4.Sample(LinearSampler, SplatTexUV);

	float WeightRemain = saturate(1.f - SplatWeights.x - SplatWeights.y - SplatWeights.z - SplatWeights.w);
	float3 Color = SplatColors[4].xyz * WeightRemain;
	//!!!need splat texture with 0 alpha channel, my ECCY_SM is with 1 now!
	//for (int i = 0; i < 4; i++)
	for (int i = 0; i < 3; i++)
		Color += SplatWeights[i] * SplatColors[i].xyz;
	return Color;
}
//---------------------------------------------------------------------

//!!!see old code version for more code!
float4 PSMainSplatted(PSInSplatted In): SV_Target
{
	float2 UV = In.PosWorld.xz * PSCDLODParams.WorldToHM.xy + PSCDLODParams.WorldToHM.zw;
	float3 TexDiffuse = Splatting(SplatMap.Sample(SplatSampler, UV), In.SplatDetUV.xy);
	return  float4(TexDiffuse, 1.f);
	//return float4(frac(0.6 * In.PosWorld.y), frac(0.6 * In.PosWorld.y), frac(0.6 * In.PosWorld.y), 1.f);
}
//---------------------------------------------------------------------
