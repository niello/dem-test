
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

matrix ViewProj: register(c0) <string CBuffer = "CameraParams";>;
float3 EyePos: register(c4) <string CBuffer = "CameraParams";>;
matrix WorldMatrix: register(c5) <string CBuffer = "InstanceParams";>;

//!!!may premultiply and pass WVP instead of WorldMatrix!
float4 VSMainOpaque(float3 Pos: POSITION): POSITION
{
	float4 OutPos = mul(float4(Pos, 1), WorldMatrix);
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

float4 WorldToHM: register(c5) <string CBuffer = "CDLODParams";>;
float4 TerrainYInvSplat: register(c6) <string CBuffer = "CDLODParams";>;	// x - Y scale, y - Y offset, zw - inv. splat size XZ
float2 GridConsts: register(c7) <string CBuffer = "CDLODParams";>;			// x - grid halfsize, y - inv. grid halfsize
float2 HMTexInfo: register(c8) <string CBuffer = "CDLODParams";>;			// xy - texel size, zw - texture size for manual bilinear filtering (change to float4 for this case)

//???height map can be loaded with mips?
float SampleHeightMap(float2 UV) //, float MipLevel)
{
	return tex2Dlod(VSHeightSampler, float4(UV + HMTexInfo.xy * 0.5, 0, 0)).x;
}
//---------------------------------------------------------------------

void VSMainCDLOD(	float2	Pos:			POSITION,
					float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
					float2	MorphConsts:	TEXCOORD1,	// x - end / (end - start), y - 1 / (end - start)
				out	float4	oPos:			POSITION)
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
sampler LinearSampler { Texture = TexAlbedo; };

float4 PSMainAlphaTest(PSSceneIn In): COLOR
{
	float Alpha = tex2D(LinearSampler, In.Tex).a;
	clip(Alpha - 0.5);
	return Alpha;
}
//---------------------------------------------------------------------
