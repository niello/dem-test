TextureCube TexCubeMap;
sampler CubeMapSampler;

cbuffer CameraParams: register(b0)
{
	matrix ViewProj;
	float3 EyePos;
}

cbuffer InstanceParams: register(b2)
{
	matrix WorldMatrix;
}

struct VSOutput
{
	float4 Pos: SV_Position;
	float3 Tex: TEXCOORD0;
};

VSOutput VSMain(float4 Pos: POSITION)
{
	float4 PosWorld = mul(Pos, WorldMatrix);

	VSOutput Out;
	Out.Pos = mul(PosWorld, ViewProj).xyww;
	Out.Tex = PosWorld.xyz - EyePos;
	return Out;
}
//---------------------------------------------------------------------

float4 PSMain(VSOutput In): SV_Target
{
	return TexCubeMap.Sample(CubeMapSampler, normalize(In.Tex));
}
//---------------------------------------------------------------------
