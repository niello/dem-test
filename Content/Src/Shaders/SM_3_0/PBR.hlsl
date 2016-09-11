#include "Globals.hlsl"
#include "CDLOD.hlsl"
#include "Skinning.hlsl"

struct PSSceneIn
{
	float4 Pos: POSITION;
	float2 Tex: TEXCOORD;
};

struct CInstanceData
{
	matrix WorldMatrix;
};

CInstanceData InstanceData: register(c5) <string CBuffer = "InstanceParams"; int SlotIndex = 2;>;
float4 MtlDiffuse: register(c9) <string CBuffer = "MaterialParams"; int SlotIndex = 1;>;

Texture2D TexAlbedo;
sampler LinearSampler { Texture = TexAlbedo; };

PSSceneIn StandardVS(float3 Pos, float2 Tex, matrix World)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(Pos, 1), World);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = Tex;
	return Out;
}
//---------------------------------------------------------------------

PSSceneIn VSMain(float3 Pos: POSITION, float2 Tex: TEXCOORD)
{
	return StandardVS(Pos, Tex, InstanceData.WorldMatrix);
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
	return StandardVS(Pos, Tex, InstWorld);
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
