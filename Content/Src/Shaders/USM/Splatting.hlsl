
// Splatting with up to 5 colour textures blended with a 4-channel splat map

//!!!FIXME! since my test texture has alpha = 1 this shader supports only 4 textures

Texture2D SplatMap;
Texture2D SplatTex0;
Texture2D SplatTex1;
Texture2D SplatTex2;
Texture2D SplatTex3;
Texture2D SplatTex4;
sampler SplatSampler;

// Version with normals can be found in old code
float3 Splatting(float4 SplatWeights, float2 SplatTexUV, sampler ColourTexSampler)
{
	float4 SplatColors[5];
	SplatColors[0] = SplatTex0.Sample(ColourTexSampler, SplatTexUV);
	SplatColors[1] = SplatTex1.Sample(ColourTexSampler, SplatTexUV);
	SplatColors[2] = SplatTex2.Sample(ColourTexSampler, SplatTexUV);
	SplatColors[3] = SplatTex3.Sample(ColourTexSampler, SplatTexUV);
	SplatColors[4] = SplatTex4.Sample(ColourTexSampler, SplatTexUV);

	float WeightRemain = saturate(1.f - SplatWeights.x - SplatWeights.y - SplatWeights.z - SplatWeights.w);
	float3 Color = SplatColors[4].xyz * WeightRemain;
	//!!!need splat texture with 0 alpha channel, my ECCY_SM is with 1 now!
	//for (int i = 0; i < 4; i++)
	for (int i = 0; i < 3; i++)
		Color += SplatWeights[i] * SplatColors[i].xyz;
	return Color;
}
//---------------------------------------------------------------------
