
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

matrix ProjectionMatrix: register(c0);

struct CData
{
	int LoopCount;
	matrix WorldMatrix; //: register(c8);
};

struct CData2
{
	CData D;
	float4 XX[3];
};

CData2 UniformStruct;
float4 Y[2];

PSSceneIn VSMain(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;

	Out.Pos = mul(float4(In.Pos, 1), UniformStruct.D.WorldMatrix);
	Out.Pos = mul(Out.Pos, ProjectionMatrix);
	Out.Tex = In.Tex;
	for (int i = 0; i < UniformStruct.D.LoopCount; ++i)
	{
		Out.Tex = Out.Tex * 0.5f + UniformStruct.XX[2].zw;
	}

	return Out;
}

Texture2D TexAlbedo;
sampler LinearSampler { Texture = TexAlbedo; };

float4 PSMain(PSSceneIn In): COLOR
{
#if DEM_LIGHT_COUNT == 0
	return float4(0.5f, 0.5f, Y[1].x, 1.f);
#elif DEM_LIGHT_COUNT == 1
	return float4(0.75f, Y[1].y, 1.f, 1.f);
#else
	return tex2D(LinearSampler, In.Tex);
#endif
}
