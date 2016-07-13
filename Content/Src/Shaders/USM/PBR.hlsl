#ifndef MAX_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT 64
#endif

struct PSSceneIn
{
	float4 Pos: SV_Position;
	float2 Tex: TEXCOORD;
};

cbuffer CameraParams: register(b0)
{
	matrix ViewProj;
}

cbuffer MaterialParams: register(b1)
{
	float4 MtlDiffuse;
}

cbuffer InstanceParams: register(b2)
{
	matrix WorldMatrix;
}

cbuffer InstanceParams: register(b2)
{
	matrix InstanceData[MAX_INSTANCE_COUNT];
}

tbuffer SkinParams: register(t0)
{
	matrix SkinMatrix[1024];
}

PSSceneIn VSMain(float3 Pos: POSITION, float2 Tex: TEXCOORD)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = Tex;
	return Out;
}

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

PSSceneIn VSMainInstancedConst(float3 Pos: POSITION, float2 Tex: TEXCOORD, uint InstanceID: SV_InstanceID)
{
	float4x4 InstWorld = InstanceData[InstanceID];

	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(Pos, 1), InstWorld);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = Tex;
	return Out;
}

Texture2D TexAlbedo;
sampler LinearSampler;

float4 PSMain(PSSceneIn In): SV_Target
{
	return TexAlbedo.Sample(LinearSampler, In.Tex) * MtlDiffuse;
}

/*
float4 PSMainAlphaTest(PSSceneIn In): SV_Target
{
	float4 Albedo = TexAlbedo.Sample(LinearSampler, In.Tex);
	clip(Albedo.a - 0.5);
	return Albedo * MtlDiffuse;
}
*/