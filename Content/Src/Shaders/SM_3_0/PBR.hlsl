
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
matrix WorldMatrix: register(c6);

struct CData
{
	float4x4 W;
	float2 mx;
	float2 m;
};

struct CData2
{
	float3 tt;
	CData D;
};

CData2 UniformStruct[2];
float4 Y[2];

struct CCounters
{
	int3 LoopCount[2];
	int LoopCount2;
};
CCounters Counters;
bool IsFirst;

PSSceneIn VSMain(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;

	Out.Pos = mul(float4(In.Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, ProjectionMatrix);
	Out.Tex = In.Tex;
	if (IsFirst)
	{
		for (int i = 0; i < Counters.LoopCount[1].y; ++i)
		{
			Out.Tex = Out.Tex * 0.5f + UniformStruct[1].tt.zx;
		}
	}
	else
	{
		for (int i = 0; i < Counters.LoopCount2; ++i)
		{
			Out.Tex = Out.Tex * 0.25f + UniformStruct[1].D.W[3].wz + UniformStruct[1].D.m;
		}
	}

	return Out;
}

Texture2D TexAlbedo;
Texture2D TexAlbedo2;
sampler LinearSampler[2] { { Texture = TexAlbedo2; }, { Texture = TexAlbedo; } };

float4 PSMain(PSSceneIn In): COLOR
{
#if DEM_LIGHT_COUNT == 0
	return float4(0.5f, 0.5f, Y[1].x, 1.f);
#elif DEM_LIGHT_COUNT == 1
	return float4(0.75f, Y[1].y, 1.f, 1.f);
#else
	return tex2D(LinearSampler[0], In.Tex) * tex2D(LinearSampler[1], In.Tex);
#endif
}
