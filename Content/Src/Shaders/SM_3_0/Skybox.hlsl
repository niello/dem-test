TextureCube TexCubeMap;
sampler CubeMapSampler { Texture = TexCubeMap; };

matrix ViewProj: register(c0) <string CBuffer = "CameraParams"; int SlotIndex = 0;>;
float3 EyePos: register(c4) <string CBuffer = "CameraParams"; int SlotIndex = 0;>;
matrix WorldMatrix: register(c5) <string CBuffer = "InstanceParams"; int SlotIndex = 2;>; //!!!no need in translation!

struct VSOutput
{
	float4 Pos: POSITION;
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

float4 PSMain(VSOutput In): COLOR
{
	return texCUBE(CubeMapSampler, normalize(In.Tex));
}
//---------------------------------------------------------------------
