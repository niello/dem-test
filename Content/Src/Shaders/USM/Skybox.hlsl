TextureCube TexCubeMap;
sampler CubeMapSampler;

struct VSOutput
{
	float4 Pos: SV_Position;
	float3 Tex: TEXCOORD0;
}

VSOutput VSMain(float4 Pos: POSITION)
{
	VSOutput Out;
	Out.Pos = Pos;
	Out.Tex = Pos.xyz;
	return Out;
}
//---------------------------------------------------------------------

float4 PSMain(VSOutput In)
{
	return float4(0.5f, 0.5f, 0.5f, 1.f);
}
//---------------------------------------------------------------------
