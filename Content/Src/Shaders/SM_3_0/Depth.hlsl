#include "Globals.hlsl"
#include "CDLOD.hlsl"
#include "Skinning.hlsl"

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

struct CInstanceData
{
	matrix WorldMatrix;
};

CInstanceData InstanceData: register(c5) <string CBuffer = "InstanceParams"; int SlotIndex = 2;>;

Texture2D TexAlbedo;
sampler LinearSampler { Texture = TexAlbedo; };

//!!!may premultiply and pass WVP instead of WorldMatrix!
float4 VSMainOpaque(float3 Pos: POSITION): POSITION
{
	float4 OutPos = mul(float4(Pos, 1), InstanceData.WorldMatrix);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
}
//---------------------------------------------------------------------

float4 VSMainInstancedOpaque(	float3 Pos: POSITION,
								float2 Tex: TEXCOORD,
								float4 World1: TEXCOORD4,
								float4 World2: TEXCOORD5,
								float4 World3: TEXCOORD6,
								float4 World4: TEXCOORD7): POSITION
{
	float4x4 InstWorld = float4x4(World1, World2, World3, World4);
	float4 OutPos = mul(float4(Pos, 1), InstWorld);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
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
	Out.Pos = mul(float4(In.Pos, 1), InstanceData.WorldMatrix);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = In.Tex;
	return Out;
}
//---------------------------------------------------------------------

PSSceneIn VSMainInstancedAlphaTest(	float3 Pos: POSITION,
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

void VSMainCDLOD(	float2	Pos:			POSITION,
					float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
					float2	MorphConsts:	TEXCOORD1,	// x - end / (end - start), y - 1 / (end - start)
				out	float4	oPos:			POSITION)
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

	oPos = mul(float4(Vertex, 1), ViewProj);
}
//---------------------------------------------------------------------

float4 PSMainAlphaTest(PSSceneIn In): COLOR
{
	float Alpha = tex2D(LinearSampler, In.Tex).a;
	clip(Alpha - 0.5);
	return Alpha;
}
//---------------------------------------------------------------------
