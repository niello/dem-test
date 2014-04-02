#ifndef __SPLATTING_H__
#define __SPLATTING_H__

// Splatting library (merely to reduce a terrain shader file size and to increase its readability)

texture SplatMap;

sampler SplatSampler = sampler_state
{
	Texture = <SplatMap>;
	AddressU  = Clamp;
	AddressV  = Clamp;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
};

texture SplatTex0;
texture SplatTex1;
texture SplatTex2;
texture SplatTex3;
texture SplatTex4;

sampler SplatTex0Sampler = sampler_state
{
	Texture = <SplatTex0>;
	AddressU  = Wrap;
	AddressV  = Wrap;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
};

sampler SplatTex1Sampler = sampler_state
{
	Texture = <SplatTex1>;
	AddressU  = Wrap;
	AddressV  = Wrap;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
};

sampler SplatTex2Sampler = sampler_state
{
	Texture = <SplatTex2>;
	AddressU  = Wrap;
	AddressV  = Wrap;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
};

sampler SplatTex3Sampler = sampler_state
{
	Texture = <SplatTex3>;
	AddressU  = Wrap;
	AddressV  = Wrap;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
};

sampler SplatTex4Sampler = sampler_state
{
	Texture = <SplatTex4>;
	AddressU  = Wrap;
	AddressV  = Wrap;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
};

void Splatting(float4 SplatWeights, float2 SplatTexUV, out float3 oColor)
{
	float4 SplatColors[5];
	float4 SplatNMaps[5]; // x and w will hold normal; .y and .z hold specularPow and specularMul
	SplatColors[0] = tex2D(SplatTex0Sampler, SplatTexUV);
	SplatColors[1] = tex2D(SplatTex1Sampler, SplatTexUV);
	SplatColors[2] = tex2D(SplatTex2Sampler, SplatTexUV);
	SplatColors[3] = tex2D(SplatTex3Sampler, SplatTexUV);
	SplatColors[4] = tex2D(SplatTex4Sampler, SplatTexUV);
	//SplatNMaps[0] = tex2D(SplatNM0Sampler, SplatTexUV);
	//SplatNMaps[1] = tex2D(SplatNM1Sampler, SplatTexUV);
	//SplatNMaps[2] = tex2D(SplatNM2Sampler, SplatTexUV);
	//SplatNMaps[3] = tex2D(SplatNM3Sampler, SplatTexUV);
	//SplatNMaps[4] = tex2D(SplatNM4Sampler, SplatTexUV);

	// y and z are unused, so reuse them for easier/faster lerping later
	//for (int i = 0; i < 5; i++)
	//{
	//	SplatNMaps[i].y = g_splatSpecularPow[i];
	//	SplatNMaps[i].z = g_splatSpecularMul[i];
	//}

	float WeightRemain = min(1.f - SplatWeights.x - SplatWeights.y - SplatWeights.z - SplatWeights.w, 0.f);
	oColor = SplatColors[4].xyz * WeightRemain;
	//float4 SplatNM = SplatNMaps[4] * WeightRemain;
	for (int i = 0; i < 4; i++)
	{
		oColor += SplatWeights[i] * SplatColors[i].xyz;
		//SplatNM += SplatWeights[i] * SplatNMaps[i];
	}

	//float3 SplatNormal = SplatNM * 2.0f - 1.0f; //UncompressDXT5_NM(SplatNM);

	// Apply normal map
	//float3 TangentX = cross(float3(0.f, 1.f, 0.f), Normal);
	//float3 TangentY = cross(Normal, TangentX);
	//oNormal = mul(SplatNormal, float3x3(TangentX, TangentY, Normal));

	//oSpecPow = SplatNM.y;
	//oSpecMul = SplatNM.z;
}
//---------------------------------------------------------------------

#endif

/* // Hack to add more detail to splatmap

// For another use of alpha channel see:
//http://www.m4x0r.com/blog/2010/05/blending-terrain-textures/

	float4 AlphaMod = sin(Normal.xy * 20).xyxy * 0.2 + 1.0;
	AlphaMod.w = 1.0;
	float4 Alphas = float4(SplatColors[1].a, SplatColors[2].a, SplatColors[3].a, SplatColors[4].a);
	Alphas = saturate(Alphas * AlphaMod);
	SplatWeights.x = ModAlpha(0, SplatWeights.x, Alphas.x);
	SplatWeights.y = ModAlpha(1, SplatWeights.y, Alphas.y);
	SplatWeights.z = ModAlpha(2, SplatWeights.z, Alphas.z);
	SplatWeights.w = ModAlpha(3, SplatWeights.w, Alphas.w);
*/

/*
	float Weights[5];      
	float4 SplatWeightRemains = 1.f - SplatWeights;

	Weights[4] = SplatWeights.w;

	float Accum = SplatWeightRemains.w;
	Weights[3] = SplatWeights.z * Accum;

	Accum *= SplatWeightRemains.z;
	Weights[2] = SplatWeights.y * Accum;

	Accum *= SplatWeightRemains.y;
	Weights[1] = SplatWeights.x * Accum;

	Accum *= SplatWeightRemains.x;
	Weights[0] = Accum;
*/
