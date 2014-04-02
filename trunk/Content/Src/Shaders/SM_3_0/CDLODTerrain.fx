
// CDLOD terrain shader
// Technology by Filip Strugar (Copyright (C) 2009)
// http://vertexasylum.com/2010/07/11/oh-no-another-terrain-rendering-paper/

// Now only directional light is supported, and Lambert diffuse lighting model is used

shared float4x4 ViewProjection;
shared float3	EyePos;

texture HeightMap;

sampler VSHeightSampler = sampler_state
{
	Texture = <HeightMap>;
	AddressU  = Clamp;
	AddressV  = Clamp;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Point;
};

float AlphaRef = 0.5;
int CullMode = 1;

#include "Lib/Common.fxh"
#include "Lib/Splatting.fxh"
#include "Lib/Lighting.fxh"
#include "Lib/NormalMapping.fxh"
#line 31 "CDLODTerrain.fx"

float4 WorldToHM;
float4 TerrainYInvSplat;	// x - Y scale, y - Y offset, zw - inv. splat size XZ
float2 GridConsts;			// x - grid halfsize, y - inv. grid halfsize
float2 HMTexInfo;			// xy - texel size, zw - texture size for manual bilinear filtering (change to float4 for this case)

// Utils **************************************************************************

float SampleHeightMap(float2 UV) //, float MipLevel)
{
	return tex2Dlod(VSHeightSampler, float4(UV + HMTexInfo.xy * 0.5, 0, 0)).x;
}
//---------------------------------------------------------------------

// Depth **************************************************************************

void VSTerrainDepth(	float2	Pos:			POSITION,
						float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
						float2	MorphConsts:	TEXCOORD1,	// x - end / (end - start), y - 1 / (end - start)
					out	float4	oPos:			POSITION,
					out	float	oDepth:			TEXCOORD0)
{
	float3 Vertex;
	Vertex.xz = Pos * PatchXZ.xy + PatchXZ.zw;
	float2 HMapUV = Vertex.xz * WorldToHM.xy + WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * TerrainYInvSplat.x + TerrainYInvSplat.y;

	float MorphK  = 1.0f - clamp(MorphConsts.x - distance(Vertex, EyePos) * MorphConsts.y, 0.0f, 1.0f);
	float2 FracPart = frac(Pos * GridConsts.xx) * GridConsts.yy;
	const float2 PosMorphed = Pos - FracPart * MorphK;

	Vertex.xz = PosMorphed * PatchXZ.xy + PatchXZ.zw;
	HMapUV = Vertex.xz * WorldToHM.xy + WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * TerrainYInvSplat.x + TerrainYInvSplat.y;

	oPos = mul(float4(Vertex, 1.0), ViewProjection);
	oDepth = oPos.z;
}
//---------------------------------------------------------------------

float4 PSDepth(float Depth: TEXCOORD0): COLOR
{
	return float4(Depth, 0.0f, 0.0f, 1.0f);
}
//---------------------------------------------------------------------

// Color **************************************************************************

void VSTerrainColor(	float2	Pos:			POSITION,
						float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
						float2	MorphConsts:	TEXCOORD1,	// x - end / (end - start), y - 1 / (end - start)
					out	float4	oPos:			POSITION,
					out	float4	oVertexConsts:	TEXCOORD0,
					out	float4	oSplatDetUV:	TEXCOORD1,
					out	float4	oPosWorld:		TEXCOORD2,
					uniform	int	LightCount)
{
	float3 Vertex;
	Vertex.xz = Pos * PatchXZ.xy + PatchXZ.zw;
	float2 HMapUV = Vertex.xz * WorldToHM.xy + WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * TerrainYInvSplat.x + TerrainYInvSplat.y;

	float MorphK  = 1.0f - clamp(MorphConsts.x - distance(Vertex, EyePos) * MorphConsts.y, 0.0f, 1.0f);
	float2 FracPart = frac(Pos * GridConsts.xx) * GridConsts.yy;
	const float2 PosMorphed = Pos - FracPart * MorphK;

	Vertex.xz = PosMorphed * PatchXZ.xy + PatchXZ.zw;
	HMapUV = Vertex.xz * WorldToHM.xy + WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * TerrainYInvSplat.x + TerrainYInvSplat.y;

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
	oPos = mul(oPosWorld, ViewProjection);
	oVertexConsts = float4(MorphK, DetailMorphK, oPos.w, distance(Vertex, EyePos));
	oSplatDetUV = float4(Vertex.xz * TerrainYInvSplat.zw, DetailUV);
}
//---------------------------------------------------------------------

//!!!NM & Splatting tex were Weighted with their parents by MorphK (VertexConsts.x)
float4 PSTerrainColor(		float4	VertexConsts:	TEXCOORD0,
							float4	SplatDetUV:		TEXCOORD1,
							float4	PosWorld:		TEXCOORD2,
					uniform	bool	UseATest,
					uniform	int		LightCount,
					uniform	bool	UseParallax): COLOR
{
	//WorldToNM, WorldToSM == WorldToHM; - for all textures that cover the whole terrain
	float2 UV = PosWorld.xz * WorldToHM.xy + WorldToHM.zw;
	float3 N = FetchNormal(UV);

/* // Detail map
	N.xy += normalize(tex2D(NMDetailSampler, SplatDetUV.zw).xy - 0.5f) * VertexConsts.y; // fadeout
	N.z = sqrt(1.f - N.x * N.x - N.y * N.y);
*/

	float3 TexDiffuse;
	float SpecPow = 32.0f;
	float SpecMul = 1.0f;
	Splatting(tex2D(SplatSampler, UV), SplatDetUV.xy, TexDiffuse);

	float4 SurfaceDiffuse = float4(TexDiffuse, 1.f);
	float4 Result = LightAmbient * SurfaceDiffuse;

	//float3 V = normalize(EyePos - PosWorld.xyz);
	for (int i = 0; i < LightCount; i++)
		Result += DiffuseLambert(N, LightDir[i]) * LightColor[i] * SurfaceDiffuse;

	// Can apply fog here, using a distance to the camera

	return Result; //???enforce alpha to be 1?
}
//---------------------------------------------------------------------

DepthTechnique(Depth, "Depth", VSTerrainDepth, PSDepth);
LitTechnique(Color, "Default", VSTerrainColor, PSTerrainColor, false, 0, false);
LitTechnique(ColorL1, "L1", VSTerrainColor, PSTerrainColor, false, 1, false);

/* Manual bilinear Vertex texture filtering:

	const float2 texelSize   = HMTexInfo.xy; 
	const float2 textureSize = HMTexInfo.zw;

	uv = uv.xy * textureSize - float2(0.5, 0.5);
	float2 uvf = floor(uv.xy);
	float2 f = uv - uvf;
	uv = (uvf + float2(0.5, 0.5)) * texelSize;

	float t00 = tex2Dlod(heightmapSampler, float4(uv.x, uv.y, 0, mipLevel)).x;
	float t10 = tex2Dlod(heightmapSampler, float4(uv.x + texelSize.x, uv.y, 0, mipLevel)).x;

	float tA = lerp(t00, t10, f.x);

	float t01 = tex2Dlod(heightmapSampler, float4(uv.x, uv.y + texelSize.y, 0, mipLevel)).x;
	float t11 = tex2Dlod(heightmapSampler, float4(uv.x + texelSize.x, uv.y + texelSize.y, 0, mipLevel)).x;

	float tB = lerp(t01, t11, f.x);

	return lerp(tA, tB, f.y);
*/