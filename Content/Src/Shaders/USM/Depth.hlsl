
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


/*
Texture2D TexAlbedo;
sampler LinearSampler;

float4 PSMain(PSSceneIn In): SV_Target
{
	return TexAlbedo.Sample(LinearSampler, In.Tex) * MtlDiffuse;
}
*/
