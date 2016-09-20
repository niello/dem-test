#include "Globals.hlsl"
#include "CDLOD.hlsl"
#include "Splatting.hlsl"
#include "Lighting.hlsl"

struct PSInSimple
{
	float4	Pos:		SV_Position;
	float3	PosWorld:	TEXCOORD1;
	float3	Normal:		NORMAL;
	float3	View:		VIEW;
	float2	UV:			TEXCOORD;
	uint4	LightInfo:	LIGHTINFO;
};

struct PSInSplatted
{
	float4 Pos:				SV_Position;
	float4 VertexConsts:	TEXCOORD0;
	float4 SplatDetUV:		TEXCOORD1;
	float4 PosWorld:		TEXCOORD2;
};

struct CInstanceData
{
	matrix	WorldMatrix;
	uint	LightCount;
	uint3	LightIndices;
	//static uint LightIndices[DEM_MAX_LIGHTS] = (uint[DEM_MAX_LIGHTS])array; // for tight packing
};

// Per-material data

cbuffer MaterialParams: register(b1)
{
	float4 MtlDiffuse;
}

// Per-instance data

cbuffer InstanceParams: register(b2)
{
	CInstanceData InstanceData;
}

#ifndef MAX_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT 64
#endif

cbuffer InstanceParams: register(b2)
{
	CInstanceData InstanceDataArray[MAX_INSTANCE_COUNT];
}

Texture2D TexAlbedo: register(t0);
sampler LinearSampler: register(s0);

// Vertex shaders

PSInSimple StandardVS(float3 Pos, float3 Normal, float2 UV, matrix World, uint LightCount, uint3 LightIndices)
{
	PSInSimple Out = (PSInSimple)0.0;
	float4 WorldPos = mul(float4(Pos, 1), World);
	Out.Pos = mul(WorldPos, ViewProj);
	Out.PosWorld = WorldPos.xyz;
	Out.Normal = mul(Normal, (float3x3)World);
	Out.View = EyePos - WorldPos.xyz;
	Out.UV = UV;
	Out.LightInfo.x = LightCount;
	Out.LightInfo.yzw = LightIndices;
	return Out;
}
//---------------------------------------------------------------------

PSInSimple VSMain(float3 Pos: POSITION, float3 Normal: NORMAL, float2 UV: TEXCOORD)
{
	return StandardVS(Pos, Normal, UV, InstanceData.WorldMatrix, InstanceData.LightCount, InstanceData.LightIndices);
}
//---------------------------------------------------------------------

PSInSimple VSMainInstanced(	float3 Pos: POSITION,
							float3 Normal: NORMAL,
							float2 UV: TEXCOORD,
							float4 World1: TEXCOORD4,
							float4 World2: TEXCOORD5,
							float4 World3: TEXCOORD6,
							float4 World4: TEXCOORD7)
{
	float4x4 InstWorld = float4x4(World1, World2, World3, World4);
	return StandardVS(Pos, Normal, UV, InstWorld, 0, uint3(0, 0, 0));
}
//---------------------------------------------------------------------

PSInSimple VSMainInstancedConst(float3 Pos: POSITION, float3 Normal: NORMAL, float2 UV: TEXCOORD, uint InstanceID: SV_InstanceID)
{
	CInstanceData InstData = InstanceDataArray[InstanceID];
	return StandardVS(Pos, Normal, UV, InstData.WorldMatrix, InstData.LightCount, InstData.LightIndices);
}
//---------------------------------------------------------------------

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

// Pixel shaders

float4 PSMain(PSInSimple In): SV_Target
{
	// Sample normal map and calculate per-pixel normal
	// NB: In.View must be not normalized
	float3 N = PerturbNormal(In.Normal, -In.View, In.UV);
	float3 V = normalize(In.View);

	// Sample albedo
	float4 Albedo = TexAlbedo.Sample(LinearSampler, In.UV);
	float3 AlbedoRGB = Albedo.rgb;

	// Sample reflectivity (common one-channel value 0.02-0.04 for all insulators)
	float3 Reflectivity = 0.03f.xxx;

	// Sample roughness
	float Roughness = 0.75f;
	float SqRoughness = Roughness * Roughness;

	static const float PI = 3.14159265359f;
	static const float INV_PI = 0.318309886f;

	float3 LightColor = float3(0, 0, 0);
	for (uint i = 0; i < In.LightInfo[0]; ++i)
	{
		CLight CurrLight = Lights[In.LightInfo[i + 1]];

		float Intensity; // = CurrLight.Intensity //!!!or pre-multiply on color!
		float3 L;
		if (CurrLight.Type == LIGHT_TYPE_DIR)
		{
			L = CurrLight.InvDirection;
			Intensity = PI; //???why mul PI? see PBRViewer!
		}
		else
		{
			L = CurrLight.Position - In.PosWorld;
			float SqDistanceToLight = dot(L, L);
			L *= rsqrt(SqDistanceToLight);

			// Calculate attenuation and falloff
			//???!!!pre-square inv. range on CPU if no other uses? if there are, pass both InvRange and SqInvRange!
			//???why mul PI? see PBRViewer!
			//???what attenuation formula to use?
			Intensity = PI * saturate(1.f - SqDistanceToLight * CurrLight.InvRange * CurrLight.InvRange);
			if (CurrLight.Type == LIGHT_TYPE_SPOT)
				Intensity *= SpotlightFalloffFunction(dot(CurrLight.InvDirection, L), CurrLight.Params.x, CurrLight.Params.y);
		}

		float3 H = normalize(L + V);
		float NdotV = saturate(dot(N, V));
		float NdotL = saturate(dot(N, L));
		float NdotH = saturate(dot(N, H));
		float VdotH = saturate(dot(V, H));
		float3 DiffuseColor = AlbedoRGB * INV_PI * DiffuseOrenNayar(N, L, V, NdotL, NdotV, SqRoughness);
		float3 SpecularColor = SpecularCookTorrance(Reflectivity, SqRoughness, NdotV, NdotL, NdotH, VdotH);

		LightColor += CurrLight.Color * Intensity * NdotL * (DiffuseColor * (1.f - SpecularColor) + SpecularColor);
	}

	return MtlDiffuse * float4(LightColor, Albedo.a);
}
//---------------------------------------------------------------------

//!!!see old code version for more code!
float4 PSMainSplatted(PSInSplatted In): SV_Target
{
	float2 UV = In.PosWorld.xz * PSCDLODParams.WorldToHM.xy + PSCDLODParams.WorldToHM.zw;
	float3 TexDiffuse = Splatting(SplatMap.Sample(SplatSampler, UV), In.SplatDetUV.xy, LinearSampler);
	return float4(TexDiffuse, 1.f);
}
//---------------------------------------------------------------------
