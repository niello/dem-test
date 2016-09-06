
struct PSSceneIn
{
	float4 Pos: POSITION;
	float2 Tex: TEXCOORD;
};

matrix ViewProj: register(c0) <string CBuffer = "CameraParams"; int SlotIndex = 0;>;
float3 EyePos: register(c4) <string CBuffer = "CameraParams"; int SlotIndex = 0;>;
matrix WorldMatrix: register(c5) <string CBuffer = "InstanceParams"; int SlotIndex = 2;>;
float4 MtlDiffuse: register(c9) <string CBuffer = "MaterialParams"; int SlotIndex = 1;>;

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

texture HeightMap;
sampler VSHeightSampler { Texture = HeightMap; };

float2 GridConsts: register(c5) <string CBuffer = "VSCDLODParams"; int SlotIndex = 2;>; // x - grid halfsize, y - inv. grid halfsize

struct
{
	float4 WorldToHM;
	float4 TerrainYInvSplat;	// x - Y scale, y - Y offset, zw - inv. splat size XZ
	float2 HMTexelSize;			// xy - height map texel size, zw - texture size for manual bilinear filtering (change to float4 for this case)
} VSCDLODParams: register(c6) <string CBuffer = "VSCDLODParams"; int SlotIndex = 2;>;

//???height map can be loaded with mips?
float SampleHeightMap(float2 UV) //, float MipLevel)
{
	return tex2Dlod(VSHeightSampler, float4(UV + VSCDLODParams.HMTexelSize.xy * 0.5, 0, 0)).x;
}
//---------------------------------------------------------------------

void VSMainCDLOD(	float2	Pos:			POSITION,
					float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
					float2	MorphConsts:	TEXCOORD1,	// x - end / (end - start), y - 1 / (end - start)
				out	float4	oPos:			POSITION,
				out	float4	oVertexConsts:	TEXCOORD0,
				out	float4	oSplatDetUV:	TEXCOORD1,
				out	float4	oPosWorld:		TEXCOORD2)
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

	oPosWorld = float4(Vertex, 1.0);
	oPos = mul(oPosWorld, ViewProj);
	oVertexConsts = float4(MorphK, DetailMorphK, oPos.w, distance(Vertex, EyePos));
	oSplatDetUV = float4(Vertex.xz * VSCDLODParams.TerrainYInvSplat.zw, DetailUV);
}
//---------------------------------------------------------------------

Texture2D TexAlbedo;
sampler LinearSampler { Texture = TexAlbedo; };

float4 PSMain(PSSceneIn In): COLOR
{
	return tex2D(LinearSampler, In.Tex) * MtlDiffuse;
}
//---------------------------------------------------------------------

float4 PSMainAlphaTest(PSSceneIn In): COLOR
{
	float4 Albedo = tex2D(LinearSampler, In.Tex);
	clip(Albedo.a - 0.5);
	return Albedo * MtlDiffuse;
}
//---------------------------------------------------------------------
