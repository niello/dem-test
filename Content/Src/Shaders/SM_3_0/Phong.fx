
// Material with Phong lighting
//???or use Blinn-Phong?
//!!!can precompute half-vector for B-Ph in VSh!

shared float4x4 World; //???shared?
shared float4x4 ViewProjection;
shared float4x4 WorldViewProjection;  //???shared? // Only for depth

shared float3	EyePos;

// Material params
shared float4	MtlDiffuse;	//???shared?
shared float4	MtlSpecular;
shared float4	MtlEmissive;

float AlphaRef = 0.5;
int CullMode = 2;

texture DiffMap0;

sampler DiffSampler = sampler_state
{
	Texture = <DiffMap0>;
	AddressU  = Wrap;
	AddressV  = Wrap;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
	MipMapLodBias = -0.5; //???need?
};

#include "Lib/Depth.fxh"
#include "Lib/DepthATest.fxh"
#include "Lib/Lighting.fxh"
#include "Lib/NormalMapping.fxh"
#line 38 "Phong.fx"

// Color **************************************************************

void VSColorPhong(			float4	Pos:					POSITION,
							float3	Normal:					NORMAL,
							float3	Tangent:				TANGENT,
							float3	Binormal:				BINORMAL,
							float2	UV0:					TEXCOORD0,
					out		float4	oPos:					POSITION,
					out		float2	oUV0:					TEXCOORD0,
					out		float3	oViewTS:				TEXCOORD1,
					out		float3	oLightWS[MAX_LIGHTS]:	TEXCOORD2, //???can use TS? //!!!only for non-directional attenuated lights!
					out		float3	oLightTS[MAX_LIGHTS]:	TEXCOORD6,
					uniform	int		LightCount)
{
	oPos = mul(Pos, WorldViewProjection);
	oUV0 = UV0;

	if (LightCount > 0)
	{
		float4 PosWorld = mul(Pos, World);

		//???need renormalization of vectors?
		float3x3 WorldToTangent = float3x3(	mul(Tangent,  (float3x3)World),
											mul(Binormal, (float3x3)World),
											mul(Normal,   (float3x3)World));

		oViewTS = mul(EyePos - PosWorld.xyz, WorldToTangent);

		for (int i = 0; i < LightCount; i++)
		{
			oLightWS[i] = (LightType[i] == LIGHT_DIR) ? LightDir[i] : LightPos[i] - PosWorld.xyz;
			oLightTS[i] = mul(oLightWS[i], WorldToTangent);
		}
	}
}
//---------------------------------------------------------------------

void VSColorPhongSkinned(	float4	Pos:					POSITION,
							float3	Normal:					NORMAL,
							float3	Tangent:				TANGENT,
							float3	Binormal:				BINORMAL,
							float4	Weights:				BLENDWEIGHT,
							float4	Indices:				BLENDINDICES,
							float2	UV0:					TEXCOORD0,
					out		float4	oPos:					POSITION,
					out		float2	oUV0:					TEXCOORD0,
					out		float3	oViewTS:				TEXCOORD1,
					out		float3	oLightWS[MAX_LIGHTS]:	TEXCOORD2, 	// 2, 3, 4, 5. Only for point & spot lights.
					out		float3	oLightTS[MAX_LIGHTS]:	TEXCOORD6,	// 6, 7, 8, 9
					uniform	int		LightCount)
{
	float4 PosWorld = SkinnedPosition(Pos, Weights, Indices);
	oPos = mul(PosWorld, ViewProjection);

	oUV0 = UV0;

	if (LightCount > 0)
	{
		float3x3 WorldToTangent = float3x3(	SkinnedNormal(Tangent,  Weights, Indices),
											SkinnedNormal(Binormal, Weights, Indices),
											SkinnedNormal(Normal,   Weights, Indices));

		oViewTS = mul(EyePos - PosWorld.xyz, WorldToTangent);

		for (int i = 0; i < LightCount; i++)
		{
			oLightWS[i] = (LightType[i] == LIGHT_DIR) ? LightDir[i] : LightPos[i] - PosWorld.xyz;
			oLightTS[i] = mul(oLightWS[i], WorldToTangent);
		}
	}
}
//---------------------------------------------------------------------

void VSColorPhongInstanced(		float4	Pos:					POSITION,
								float3	Normal:					NORMAL,
								float3	Tangent:				TANGENT,
								float3	Binormal:				BINORMAL,
								float2	UV0:					TEXCOORD0,
								float4	World1:					TEXCOORD4,
								float4	World2:					TEXCOORD5,
								float4	World3:					TEXCOORD6,
								float4	World4:					TEXCOORD7,
						out		float4	oPos:					POSITION,
						out		float2	oUV0:					TEXCOORD0,
						out		float3	oViewTS:				TEXCOORD1,
						out		float3	oLightWS[MAX_LIGHTS]:	TEXCOORD2, //???can use TS? //!!!only for non-dir attenuated lights!
						out		float3	oLightTS[MAX_LIGHTS]:	TEXCOORD6,
						uniform	int		LightCount)
{
	float4x4 InstWorld = float4x4(World1, World2, World3, World4);

	float4 PosWorld = mul(Pos, InstWorld);
	oPos = mul(PosWorld, ViewProjection);
	oUV0 = UV0;

	if (LightCount > 0)
	{
		float3x3 WorldToTangent = float3x3(	mul(Tangent,  (float3x3)InstWorld),
											mul(Binormal, (float3x3)InstWorld),
											mul(Normal,   (float3x3)InstWorld));

		oViewTS = mul(EyePos - PosWorld.xyz, WorldToTangent);

		for (int i = 0; i < LightCount; i++)
		{
			oLightWS[i] = (LightType[i] == LIGHT_DIR) ? LightDir[i] : LightPos[i] - PosWorld.xyz;
			oLightTS[i] = mul(oLightWS[i], WorldToTangent);
		}
	}
}
//---------------------------------------------------------------------

float4 PSColorPhong(		float2	UV0:					TEXCOORD0,
							float3	ViewTS:					TEXCOORD1,
							float3	LightWS[MAX_LIGHTS]:	TEXCOORD2,	// 2, 3, 4, 5. Only for point & spot lights.
							float3	LightTS[MAX_LIGHTS]:	TEXCOORD6,	// 6, 7, 8, 9
					uniform	bool	UseATest,
					uniform	int		LightCount,
					uniform	bool	UseParallax): COLOR
{
	float3 V = normalize(ViewTS);
	float2 UV = UseParallax ? ParallaxUV(UV0, V) : UV0;
	float4 TexDiffuse = tex2D(DiffSampler, UV);

	//!!!ZFunc = Equal allows to skip this test!
	if (UseATest) clip(TexDiffuse.a - AlphaRef);

	float4 SurfaceDiffuse = MtlDiffuse * TexDiffuse;

	// Can use emissive texture
	float4 Result = LightAmbient * SurfaceDiffuse + MtlEmissive;

	if (LightCount > 0)
	{
		float3 N = FetchNormal(UV); //N.y *= -1; // To Lh

		for (int i = 0; i < LightCount; i++)
		{
			float3 L = normalize(LightTS[i]);
			float Intensity = DiffuseLambert(N, L);
			if (LightType[i] != LIGHT_DIR) Intensity *= Attenuation(length(LightWS[i]), LightParams[i].x);
			if (LightType[i] == LIGHT_SPOT) Intensity *= SpotlightFalloff(dot(LightDir[i], normalize(-LightWS[i])), LightParams[i].y, LightParams[i].z);
			Result += Intensity * LightColor[i] * (SurfaceDiffuse + SpecularPhong(N, L, V) * MtlSpecular);
		}
	}

	return Result;
}
//---------------------------------------------------------------------

// Depth techs

//!!!NEED OPTIONAL FLAGS!
DepthTechnique(SolidDepth, "Solid|Depth,Solid|Parallax|Depth", VSDepth, PSDepth);
DepthTechnique(ATestDepth, "ATest|Depth,ATest|Parallax|Depth", VSDepthATest, PSDepthATest);
DepthTechnique(SolidDepthSkinned, "Solid|Depth|Skinned,Solid|Parallax|Depth|Skinned", VSDepthSkinned, PSDepth);
DepthTechnique(ATestDepthSkinned, "ATest|Depth|Skinned,ATest|Parallax|Depth|Skinned", VSDepthATestSkinned, PSDepthATest);
DepthTechnique(SolidDepthInstanced, "Solid|Depth|Instanced,Solid|Parallax|Depth|Instanced", VSDepthInstanced, PSDepth);
DepthTechnique(ATestDepthInstanced, "ATest|Depth|Instanced,ATest|Parallax|Depth|Instanced", VSDepthATestInstanced, PSDepthATest);

// Color techs

// No light flags equals to L0, L1..L4 describe 1 to 4 lights

// Static
LitTechnique(ColorL0, "Solid,Alpha,Additive", VSColorPhong, PSColorPhong, false, 0, false);
LitTechnique(ColorL0Plx, "Solid|Parallax,Alpha|Parallax,Additive|Parallax", VSColorPhong, PSColorPhong, false, 0, true);
LitTechnique(ColorL1, "Solid|L1,Alpha|L1,Additive|L1", VSColorPhong, PSColorPhong, false, 1, false);
LitTechnique(ColorL1Plx, "Solid|L1|Parallax,Alpha|L1|Parallax,Additive|L1|Parallax", VSColorPhong, PSColorPhong, false, 1, true);
LitTechnique(ColorL2, "Solid|L2,Alpha|L2,Additive|L2", VSColorPhong, PSColorPhong, false, 2, false);
LitTechnique(ColorL2Plx, "Solid|L2|Parallax,Alpha|L2|Parallax,Additive|L2|Parallax", VSColorPhong, PSColorPhong, false, 2, true);
LitTechnique(ColorL3, "Solid|L3,Alpha|L3,Additive|L3", VSColorPhong, PSColorPhong, false, 3, false);
LitTechnique(ColorL3Plx, "Solid|L3|Parallax,Alpha|L3|Parallax,Additive|L3|Parallax", VSColorPhong, PSColorPhong, false, 3, true);
LitTechnique(ColorL4, "Solid|L4,Alpha|L4,Additive|L4", VSColorPhong, PSColorPhong, false, 4, false);
LitTechnique(ColorL4Plx, "Solid|L4|Parallax,Alpha|L4|Parallax,Additive|L4|Parallax", VSColorPhong, PSColorPhong, false, 4, true);

LitTechnique(ColorAtL0, "ATest", VSColorPhong, PSColorPhong, true, 0, false);
LitTechnique(ColorAtL0Plx, "ATest|Parallax", VSColorPhong, PSColorPhong, true, 0, true);
LitTechnique(ColorAtL1, "ATest|L1", VSColorPhong, PSColorPhong, true, 1, false);
LitTechnique(ColorAtL1Plx, "ATest|L1|Parallax", VSColorPhong, PSColorPhong, true, 1, true);
LitTechnique(ColorAtL2, "ATest|L2", VSColorPhong, PSColorPhong, true, 2, false);
LitTechnique(ColorAtL2Plx, "ATest|L2|Parallax", VSColorPhong, PSColorPhong, true, 2, true);
LitTechnique(ColorAtL3, "ATest|L3", VSColorPhong, PSColorPhong, true, 3, false);
LitTechnique(ColorAtL3Plx, "ATest|L3|Parallax", VSColorPhong, PSColorPhong, true, 3, true);
LitTechnique(ColorAtL4, "ATest|L4", VSColorPhong, PSColorPhong, true, 4, false);
LitTechnique(ColorAtL4Plx, "ATest|L4|Parallax", VSColorPhong, PSColorPhong, true, 4, true);

// Skinned
LitTechnique(ColorL0Sk, "Solid|Skinned,ATest|Skinned,Alpha|Skinned,Additive|Skinned", VSColorPhongSkinned, PSColorPhong, false, 0, false);
LitTechnique(ColorL0PlxSk, "Solid|Parallax|Skinned,ATest|Parallax|Skinned,Alpha|Parallax|Skinned,Additive|Parallax|Skinned",
			 VSColorPhongSkinned, PSColorPhong, false, 0, true);
LitTechnique(ColorL1Sk, "Solid|L1|Skinned,ATest|L1|Skinned,Alpha|L1|Skinned,Additive|L1|Skinned", VSColorPhongSkinned, PSColorPhong, false, 1, false);
LitTechnique(ColorL1PlxSk, "Solid|L1|Parallax|Skinned,ATest|L1|Parallax|Skinned,Alpha|L1|Parallax|Skinned,Additive|L1|Parallax|Skinned",
			 VSColorPhongSkinned, PSColorPhong, false, 1, true);
LitTechnique(ColorL2Sk, "Solid|L2|Skinned,ATest|L2|Skinned,Alpha|L2|Skinned,Additive|L2|Skinned", VSColorPhongSkinned, PSColorPhong, false, 2, false);
LitTechnique(ColorL2PlxSk, "Solid|L2|Parallax|Skinned,ATest|L2|Parallax|Skinned,Alpha|L2|Parallax|Skinned,Additive|L2|Parallax|Skinned",
			 VSColorPhongSkinned, PSColorPhong, false, 2, true);
LitTechnique(ColorL3Sk, "Solid|L3|Skinned,ATest|L3|Skinned,Alpha|L3|Skinned,Additive|L3|Skinned", VSColorPhongSkinned, PSColorPhong, false, 3, false);
LitTechnique(ColorL3PlxSk, "Solid|L3|Parallax|Skinned,ATest|L3|Parallax|Skinned,Alpha|L3|Parallax|Skinned,Additive|L3|Parallax|Skinned",
			 VSColorPhongSkinned, PSColorPhong, false, 3, true);
LitTechnique(ColorL4Sk, "Solid|L4|Skinned,ATest|L4|Skinned,Alpha|L4|Skinned,Additive|L4|Skinned", VSColorPhongSkinned, PSColorPhong, false, 4, false);
LitTechnique(ColorL4PlxSk, "Solid|L4|Parallax|Skinned,ATest|L4|Parallax|Skinned,Alpha|L4|Parallax|Skinned,Additive|L4|Parallax|Skinned",
			 VSColorPhongSkinned, PSColorPhong, false, 4, true);

//!!!add skinned atest!

// Instanced
LitTechnique(ColorL0Inst, "Solid|Instanced,Alpha|Instanced,Additive|Instanced",
			 VSColorPhongInstanced, PSColorPhong, false, 0, false);
LitTechnique(ColorL0PlxInst, "Solid|Parallax|Instanced,Alpha|Parallax|Instanced,Additive|Parallax|Instanced",
			 VSColorPhongInstanced, PSColorPhong, false, 0, true);
LitTechnique(ColorL1Inst, "Solid|L1|Instanced,Alpha|L1|Instanced,Additive|L1|Instanced",
			 VSColorPhongInstanced, PSColorPhong, false, 1, false);
LitTechnique(ColorL1PlxInst, "Solid|L1|Parallax|Instanced,Alpha|L1|Parallax|Instanced,Additive|L1|Parallax|Instanced",
			 VSColorPhongInstanced, PSColorPhong, false, 1, true);
LitTechnique(ColorL2Inst, "Solid|L2|Instanced,Alpha|L2|Instanced,Additive|L2|Instanced",
			 VSColorPhongInstanced, PSColorPhong, false, 2, false);
LitTechnique(ColorL2PlxInst, "Solid|L2|Parallax|Instanced,Alpha|L2|Parallax|Instanced,Additive|L2|Parallax|Instanced",
			 VSColorPhongInstanced, PSColorPhong, false, 2, true);
LitTechnique(ColorL3Inst, "Solid|L3|Instanced,Alpha|L3|Instanced,Additive|L3|Instanced",
			 VSColorPhongInstanced, PSColorPhong, false, 3, false);
LitTechnique(ColorL3PlxInst, "Solid|L3|Parallax|Instanced,Alpha|L3|Parallax|Instanced,Additive|L3|Parallax|Instanced",
			 VSColorPhongInstanced, PSColorPhong, false, 3, true);
LitTechnique(ColorL4Inst, "Solid|L4|Instanced,Alpha|L4|Instanced,Additive|L4|Instanced",
			 VSColorPhongInstanced, PSColorPhong, false, 4, false);
LitTechnique(ColorL4PlxInst, "Solid|L4|Parallax|Instanced,Alpha|L4|Parallax|Instanced,Additive|L4|Parallax|Instanced",
			 VSColorPhongInstanced, PSColorPhong, false, 4, true);

LitTechnique(ColorAtL0Inst, "ATest|Instanced",
			 VSColorPhongInstanced, PSColorPhong, true, 0, false);
LitTechnique(ColorAtL0PlxInst, "ATest|Parallax|Instanced",
			 VSColorPhongInstanced, PSColorPhong, true, 0, true);
LitTechnique(ColorAtL1Inst, "ATest|L1|Instanced",
			 VSColorPhongInstanced, PSColorPhong, true, 1, false);
LitTechnique(ColorAtL1PlxInst, "ATest|L1|Parallax|Instanced",
			 VSColorPhongInstanced, PSColorPhong, true, 1, true);
LitTechnique(ColorAtL2Inst, "ATest|L2|Instanced",
			 VSColorPhongInstanced, PSColorPhong, true, 2, false);
LitTechnique(ColorAtL2PlxInst, "ATest|L2|Parallax|Instanced",
			 VSColorPhongInstanced, PSColorPhong, true, 2, true);
LitTechnique(ColorAtL3Inst, "ATest|L3|Instanced",
			 VSColorPhongInstanced, PSColorPhong, true, 3, false);
LitTechnique(ColorAtL3PlxInst, "ATest|L3|Parallax|Instanced",
			 VSColorPhongInstanced, PSColorPhong, true, 3, true);
LitTechnique(ColorAtL4Inst, "ATest|L4|Instanced",
			 VSColorPhongInstanced, PSColorPhong, true, 4, false);
LitTechnique(ColorAtL4PlxInst, "ATest|L4|Parallax|Instanced",
			 VSColorPhongInstanced, PSColorPhong, true, 4, true);
