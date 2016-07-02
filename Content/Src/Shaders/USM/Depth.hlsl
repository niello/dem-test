
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

//!!!DUPLICATE CODE, see PBR VSMain!
PSSceneIn VSMainAlphaTest(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(In.Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = In.Tex;
	return Out;
}

Texture2D TexAlbedo;
sampler LinearSampler;

void PSMainAlphaTest(PSSceneIn In)
{
	float Alpha = TexAlbedo.Sample(LinearSampler, In.Tex).a;
	clip(Alpha - 0.5);
}
