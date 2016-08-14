
Texture2D SplatMap;
Texture2D SplatTex0;
Texture2D SplatTex1;
Texture2D SplatTex2;
Texture2D SplatTex3;
Texture2D SplatTex4;
sampler SplatSampler { Texture = SplatMap; };
sampler LinearSampler[5] { { Texture = SplatTex0; }, { Texture = SplatTex1; }, { Texture = SplatTex2; }, { Texture = SplatTex3; }, { Texture = SplatTex4; } };

// Version with normals can be found in old code
float3 Splatting(float4 SplatWeights, float2 SplatTexUV)
{
	float4 SplatColors[5];
	SplatColors[0] = tex2D(LinearSampler[0], SplatTexUV);
	SplatColors[1] = tex2D(LinearSampler[1], SplatTexUV);
	SplatColors[2] = tex2D(LinearSampler[2], SplatTexUV);
	SplatColors[3] = tex2D(LinearSampler[3], SplatTexUV);
	SplatColors[4] = tex2D(LinearSampler[4], SplatTexUV);
	
	float WeightRemain = saturate(1.f - SplatWeights.x - SplatWeights.y - SplatWeights.z - SplatWeights.w);
	float3 Color = SplatColors[4].xyz * WeightRemain;
	//!!!need splat texture with 0 alpha channel, my ECCY_SM is with 1 now!
	//for (int i = 0; i < 4; i++)
	for (int i = 0; i < 3; i++)
		Color += SplatWeights[i] * SplatColors[i].xyz;
	return Color;
}
//---------------------------------------------------------------------
