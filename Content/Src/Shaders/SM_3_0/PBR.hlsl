
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

matrix ViewProj: register(c0);
matrix WorldMatrix: register(c4);

PSSceneIn VSMain(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;
	Out.Pos = mul(float4(In.Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, ViewProj);
	Out.Tex = In.Tex;
	return Out;
}

//Texture2D TexAlbedo;
//sampler LinearSampler { Texture = TexAlbedo; };

float4 PSMain(PSSceneIn In): COLOR
{
	return float4(0.95f, 0.95f, 0.1f, 1.f); //tex2D(LinearSampler, In.Tex)
}
