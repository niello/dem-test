
struct PSSceneIn
{
	float4 Pos: SV_Position;
	float4 Colour: COLOR;
	float2 Tex: TEXCOORD;
};

Texture2D BoundTexture;
sampler LinearSampler;

float4 PSMain(PSSceneIn In): SV_Target
{
	return BoundTexture.Sample(LinearSampler, In.Tex) * In.Colour;
}
