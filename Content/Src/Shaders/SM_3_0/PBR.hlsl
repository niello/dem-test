
struct PSSceneIn
{
	float4 Pos: POSITION;
	float2 Tex: TEXCOORD;
};

matrix ViewProj: register(c0) <string CBuffer = "CameraParams";>;
matrix WorldMatrix: register(c4) <string CBuffer = "InstanceParams";>;
float4 MtlDiffuse: register(c8) <string CBuffer = "MaterialParams";>;

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

Texture2D TexAlbedo;
sampler LinearSampler { Texture = TexAlbedo; };

float4 PSMain(PSSceneIn In): COLOR
{
	return tex2D(LinearSampler, In.Tex) * MtlDiffuse;
}

float4 PSMainAlphaTest(PSSceneIn In): COLOR
{
	float4 Albedo = tex2D(LinearSampler, In.Tex);
	clip(Albedo.a - 0.5);
	return Albedo * MtlDiffuse;
}
