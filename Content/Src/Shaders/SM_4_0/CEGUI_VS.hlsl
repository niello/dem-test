
struct PSSceneIn
{
	float4 Pos: SV_Position;
	float4 Colour: COLOR;
	float2 Tex: TEXCOORD;
};

cbuffer ChangePerObject
{
	matrix WorldMatrix;
}

cbuffer ChangeOnResize
{
	matrix ProjectionMatrix;
}

struct VSSceneIn
{
	float3 Pos: POSITION;
	float4 Colour: COLOR;
	float2 Tex: TEXCOORD;
};

PSSceneIn VSMain(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;

	Out.Pos = mul(float4(In.Pos, 1), WorldMatrix);
	Out.Pos = mul(Out.Pos, ProjectionMatrix);
	Out.Tex = In.Tex;
	Out.Colour.rgba = In.Colour.bgra;

	return Out;
}
