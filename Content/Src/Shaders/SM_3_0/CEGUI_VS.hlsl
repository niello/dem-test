
struct PSSceneIn
{
	float4 Pos: POSITION;
	float4 Colour: COLOR;
	float2 Tex: TEXCOORD;
};

matrix WorldMatrix;
matrix ProjectionMatrix;

struct VSSceneIn
{
	float3 Pos: POSITION;
	float4 Colour: COLOR;
	float2 Tex: TEXCOORD;
};

PSSceneIn VSMain(VSSceneIn In)
{
	PSSceneIn Out = (PSSceneIn)0.0;

	float4 InPos = float4(In.Pos, 1);
	InPos.xy -= 0.5f;
	Out.Pos = mul(InPos, WorldMatrix);
	Out.Pos = mul(Out.Pos, ProjectionMatrix);
	Out.Pos.z = 0; // Render on the near plane
	Out.Tex = In.Tex;
	Out.Colour.rgba = In.Colour.bgra;

	return Out;
}
