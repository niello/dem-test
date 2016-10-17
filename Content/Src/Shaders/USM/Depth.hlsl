
struct VSSceneIn
{
	float4 Pos: POSITION;
	float2 Tex: TEXCOORD;
};

struct PSSceneIn
{
	float4 Pos: SV_Position;
	float2 Tex: TEXCOORD;
};

struct CInstanceDataVS
{
	matrix WorldMatrix;
};

// For alpha-test
Texture2D TexAlbedo: register(t0);		// PS
sampler LinearSampler: register(s0);	// PS

// Pixel shaders

void PSMainAlphaTest(PSSceneIn In)
{
	float Alpha = TexAlbedo.Sample(LinearSampler, In.Tex).a;
	clip(Alpha - 0.5);
}
//---------------------------------------------------------------------