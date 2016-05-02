
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

cbuffer CameraParams: register(c0)
{
	matrix ViewProj;
	//float3 Eye;
}

cbuffer InstanceParams: register(c1)
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

float4 PSMain(PSSceneIn In): SV_Target
{
	return TexAlbedo.Sample(LinearSampler, In.Tex);
}
