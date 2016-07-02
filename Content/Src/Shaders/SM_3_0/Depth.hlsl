
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
matrix WorldMatrix: register(c4) <string CBuffer = "InstanceParams";>;

//!!!may premultiply and pass WVP instead of WorldMatrix!
float4 VSMainOpaque(float3 Pos: POSITION): POSITION
{
	float4 OutPos = mul(float4(Pos, 1), WorldMatrix);
	OutPos = mul(OutPos, ViewProj);
	return OutPos;
}

PSSceneIn VSMainAlphaTest(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(In.Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = In.Tex;
	return Out;
}

Texture2D TexAlbedo;
sampler LinearSampler { Texture = TexAlbedo; };

float4 PSMainAlphaTest(PSSceneIn In): COLOR
{
	float Alpha = tex2D(LinearSampler, In.Tex).a;
	clip(Alpha - 0.5);
	return Alpha;
}
