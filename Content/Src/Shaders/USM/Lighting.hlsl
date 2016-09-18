
// Lighting and related functions

#define LIGHT_TYPE_DIR		0
#define LIGHT_TYPE_POINT	1
#define LIGHT_TYPE_SPOT		2

float3x3 CotangentFrame(float3 Pos, float3 N, float2 UV)
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
	return float3x3(T * invmax, B * invmax, N);
}

// N - interpolated vertex normal, V - interpolated vertex to eye vector
float3 PerturbNormal(float3 N, float3 V, float2 UV)
{
//!!!TMP!
	float3 SampledNormal = float3(0, 1, 0);

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

    float3x3 TBN = CotangentFrame(-V, N, UV);
    return normalize(mul(TBN, SampledNormal));
}