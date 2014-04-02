
// Material without lighting, texture color only

shared float4x4 ViewProjection;
shared float4x4 WorldViewProjection;  //???shared?

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

float AlphaRef = 0.5;
int CullMode = 2;

#include "Lib/Depth.fxh"
#include "Lib/DepthATest.fxh"
#line 26 "Unlit.fx"

// Color **************************************************************************

void VSColorUnlit(		float4	Pos:	POSITION,
						float2	UV0:	TEXCOORD0,
					out	float4	oPos:	POSITION,
					out	float2	oUV0:	TEXCOORD0)
{
	oPos = mul(Pos, WorldViewProjection);
	oUV0 = UV0;
}
//---------------------------------------------------------------------

void VSColorUnlitSkinned(	float4	Pos:		POSITION,
							float4	Weights:	BLENDWEIGHT,
							float4	Indices:	BLENDINDICES,
							float2	UV0:		TEXCOORD0,
						out	float4	oPos:		POSITION,
						out	float2	oUV0:		TEXCOORD0)
{
	oPos = mul(SkinnedPosition(Pos, Weights, Indices), WorldViewProjection);
	oUV0 = UV0;
}
//---------------------------------------------------------------------

void VSColorUnlitInstanced(		float4	Pos:	POSITION,
								float2	UV0:	TEXCOORD0,
								float4	World1:	TEXCOORD4,
								float4	World2:	TEXCOORD5,
								float4	World3:	TEXCOORD6,
								float4	World4:	TEXCOORD7,
							out	float4	oPos:	POSITION,
							out	float2	oUV0:	TEXCOORD0)
{
	oPos = mul(Pos, mul(float4x4(World1, World2, World3, World4), ViewProjection));
	oUV0 = UV0;
}
//---------------------------------------------------------------------

float4 PSColorUnlit(float2 UV0: TEXCOORD0, uniform bool UseATest): COLOR
{
	float4 TexDiffuse = tex2D(DiffSampler, UV0);
	if (UseATest) clip(TexDiffuse.a - AlphaRef);
	return TexDiffuse;
}
//---------------------------------------------------------------------

// Flags like LightsN, POM, Parallax etc are invalid for unlit objects, so they aren't added to ignored/optional flags.
// Finding a tech for objects with these flags will fail.

// Depth techs
DepthTechnique(SolidDepth, "Solid|Depth", VSDepth, PSDepth);
DepthTechnique(ATestDepth, "ATest|Depth", VSDepthATest, PSDepthATest);
DepthTechnique(SolidDepthSkinned, "Solid|Depth|Skinned", VSDepthSkinned, PSDepth);
DepthTechnique(ATestDepthSkinned, "ATest|Depth|Skinned", VSDepthATestSkinned, PSDepthATest);
DepthTechnique(SolidDepthInstanced, "Solid|Depth|Instanced", VSDepthInstanced, PSDepth);
DepthTechnique(ATestDepthInstanced, "ATest|Depth|Instanced", VSDepthATestInstanced, PSDepthATest);

// Color techs
SimpleTechnique(Color, "Solid,Alpha,Additive", VSColorUnlit, PSColorUnlit, false);
SimpleTechnique(ColorSk, "Solid|Skinned,Alpha|Skinned", VSColorUnlitSkinned, PSColorUnlit, false);
SimpleTechnique(ColorInst, "Solid|Instanced,Alpha|Instanced", VSColorUnlitInstanced, PSColorUnlit, false);
SimpleTechnique(ColorAt, "ATest", VSColorUnlit, PSColorUnlit, true);
SimpleTechnique(ColorAtSk, "ATest|Skinned", VSColorUnlitSkinned, PSColorUnlit, true);
SimpleTechnique(ColorAtInst, "ATest|Instanced", VSColorUnlitInstanced, PSColorUnlit, true);
