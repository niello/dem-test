
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

tbuffer ChangePerObject: register(t0)	// Instance data (skinning)
{
	int xx[5];
	matrix SkinMatrix[1024];
}

struct CLight
{
	float4	Params1;
	int		Type;
	bool	CastShadow;
};

StructuredBuffer<CLight> LightBuffer[2]: register(t3);
float4 Y[2];

PSSceneIn VSMain(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;

	Out.Pos = mul(float4(In.Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, SkinMatrix[1023]);
	Out.Pos = mul(Out.Pos, ProjectionMatrix);
	Out.Tex = In.Tex;
	
	if (LightBuffer[1][2].CastShadow)
	{
		Out.Tex = pow(abs(Out.Tex * LightBuffer[0][2].Params1.zw + Y[1].xw), xx[4]);
	}

	return Out;
}

Texture2D TexAlbedo[2];
Texture2DArray TexArray[2];
sampler LinearSampler[2];

float4 PSMain(PSSceneIn In): SV_Target
{
#if DEM_LIGHT_COUNT == 0
	return float4(0.5f, 0.5f, 0.5f, 1.f);
#elif DEM_LIGHT_COUNT == 1
	return float4(0.75f, 0.75f, 0.75f, 1.f);
#else
	return TexAlbedo[0].Sample(LinearSampler[1], In.Tex) * TexArray[1].Sample(LinearSampler[1], float3(In.Tex, 5)) * TexAlbedo[1].Sample(LinearSampler[0], In.Tex);
#endif
}
