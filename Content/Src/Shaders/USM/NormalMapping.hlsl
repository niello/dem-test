
// Normal mapping and normal blending functions

// http://www.thetenthplanet.de/archives/1180
// N - interpolated vertex normal, Pos - interpolated unnormalized eye to surface vector
float3 PerturbNormal(float3 SampledNormal, float3 N, float3 Pos, float2 UV)
{
	// get edge vectors of the pixel triangle
	float3 dp1 = ddx(Pos);
	float3 dp2 = ddy(Pos);
	float2 duv1 = ddx(UV);
	float2 duv2 = ddy(UV);

	// solve the linear system
	float3 dp2perp = cross(dp2, N);
	float3 dp1perp = cross(N, dp1);
	float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	// construct a scale-invariant frame 
	float invmax = rsqrt(max(dot(T, T), dot(B, B)));
	float3x3 TBN = float3x3(T * invmax, B * invmax, N);

	return normalize(mul(SampledNormal, TBN));
}
//---------------------------------------------------------------------

float3 SampleNormal(Texture2D TexNormalMap, sampler LinearSampler, float3 Normal, float3 View, float2 UV)
{
	// Sample normal map and calculate per-pixel normal
	// We invert Y to load normal maps with +Y (OpenGL-style)
	// NB: View must be not normalized
	float4 NM = TexNormalMap.Sample(LinearSampler, UV);	
	float3 SampledNormal = NM.xyz * float3(2.f, -2.f, 2.f) - float3(1.f, -1.f, 1.f); // May use (255.f / 127.f) - (128.f / 127.f));
	return PerturbNormal(SampledNormal, normalize(Normal), View, UV);
}
//---------------------------------------------------------------------

// Normal sampling:
//float3 SampledNormal = tex2D(TexNormalMap, UV).xyz;
//#ifdef WITH_NORMALMAP_UNSIGNED
    //SampledNormal = SampledNormal * 255./127. - 128./127.;
//#endif
//#ifdef WITH_NORMALMAP_2CHANNEL
//    SampledNormal.z = sqrt(1.f - dot(SampledNormal.xy, SampledNormal.xy));
//#endif
//#ifdef WITH_NORMALMAP_GREEN_UP
//    SampledNormal.y = -SampledNormal.y;
//#endif
