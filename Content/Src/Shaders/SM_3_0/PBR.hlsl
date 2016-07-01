
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
float4 MtlDiffuse: register(c8) <string CBuffer = "MaterialParams";>;
float AlphaRef: register(c9) <string CBuffer = "MaterialParams";>;

PSSceneIn VSMain(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(In.Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = In.Tex;
	return Out;
}

Texture2D TexAlbedo;
sampler LinearSampler { Texture = TexAlbedo; };

float4 PSMain(PSSceneIn In): COLOR
{
	return tex2D(LinearSampler, In.Tex) * MtlDiffuse;
}

float4 PSMainAlphaTest(PSSceneIn In): COLOR
{
	float4 Albedo = tex2D(LinearSampler, In.Tex);
	clip(Albedo.a - AlphaRef);
	return Albedo * MtlDiffuse;
}
