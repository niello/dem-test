
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

tbuffer SkinParams: register(t0)
{
	matrix SkinMatrix[1024];
}

//!!!may premultiply and pass WVP instead of WorldMatrix!
float4 VSMainOpaque(float3 Pos: POSITION): SV_Position
{
	float4 OutPos = mul(float4(Pos, 1), WorldMatrix);
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

Texture2D HeightMap;
sampler VSHeightSampler;

cbuffer CDLODParams: register(b2)
{
	float4 WorldToHM;
	float4 TerrainYInvSplat;	// x - Y scale, y - Y offset, zw - inv. splat size XZ
	float2 GridConsts;			// x - grid halfsize, y - inv. grid halfsize
	float2 HMTexInfo;			// xy - texel size, zw - texture size for manual bilinear filtering (change to float4 for this case)
}

//???height map can be loaded with mips?
float SampleHeightMap(float2 UV) //, float MipLevel)
{
	return HeightMap.SampleLevel(VSHeightSampler, UV + HMTexInfo.xy * 0.5, 0).x;
}
//---------------------------------------------------------------------

void VSMainCDLOD(	float2	Pos:			POSITION,
					float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
					float2	MorphConsts:	TEXCOORD1,	// x - end / (end - start), y - 1 / (end - start)
				out	float4	oPos:			SV_Position)
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
