
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

cbuffer ChangeOnResize: register(c0)	// Camera data
{
	matrix ProjectionMatrix;
}

cbuffer ChangePerObject: register(c1)	// Instance data
{
	matrix WorldMatrix;
}

PSSceneIn VSMain(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;

	Out.Pos = mul(float4(In.Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, ProjectionMatrix);
	Out.Tex = In.Tex;

	return Out;
}

Texture2D TexAlbedo;
sampler LinearSampler;

float4 PSMain(PSSceneIn In): SV_Target
{
#if DEM_LIGHT_COUNT == 0
	return float4(0.5f, 0.5f, 0.5f, 1.f);
#elif DEM_LIGHT_COUNT == 1
	return float4(0.75f, 0.75f, 0.75f, 1.f);
#else
	return TexAlbedo.Sample(LinearSampler, In.Tex);
#endif
}
