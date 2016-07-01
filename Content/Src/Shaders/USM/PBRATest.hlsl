
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
}

cbuffer MaterialParams: register(b1)
{
	float4 MtlDiffuse;
	float AlphaRef;
}

cbuffer InstanceParams: register(b2)
{
	matrix WorldMatrix;
}

tbuffer SkinParams: register(t0)
{
	matrix SkinMatrix[1024];
}

PSSceneIn VSMain(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(In.Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = In.Tex;
	return Out;
}

Texture2D TexAlbedo;
sampler LinearSampler;

float4 PSMainAlphaTest(PSSceneIn In): SV_Target
{
	float4 Albedo = TexAlbedo.Sample(LinearSampler, In.Tex);
	clip(Albedo.a - AlphaRef);
	return Albedo * MtlDiffuse;
}

// For a color phase, try
float4 PSMain(PSSceneIn In): SV_Target
{
	return TexAlbedo.Sample(LinearSampler, In.Tex) * MtlDiffuse;
}