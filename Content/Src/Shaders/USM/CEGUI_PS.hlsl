
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

float4 PSMainOpaque(PSSceneIn In): SV_Target
{
	float4 TexColour = BoundTexture.Sample(LinearSampler, In.Tex);
	clip(TexColour.a - 0.5);
	return TexColour * In.Colour;
}
