
struct PSSceneIn
{
	//float4 Pos: POSITION;
	float4 Colour: COLOR;
	float2 Tex: TEXCOORD;
};

sampler2D LinearSampler { Texture = BoundTexture; };

float4 PSMain(PSSceneIn In): COLOR0
{
	return tex2D(LinearSampler, In.Tex) * In.Colour;
}
