
// Hardware skinning support

#ifndef MAX_BONES_PER_PALETTE
#define MAX_BONES_PER_PALETTE 72
#endif
#ifndef MAX_BONES_PER_VERTEX
#define MAX_BONES_PER_VERTEX 4
#endif

tbuffer SkinParams: register(t0) // VS
{
	matrix SkinPalette[MAX_BONES_PER_PALETTE]; //!!!can use float4x3 column_major!
}

//???why indices are float? use uint4
float4 SkinnedPoint(const float4 Point, const float4 Weights, const float4 Indices)
{
	float3 Result = mul(Point, SkinPalette[Indices[0]]).xyz * Weights[0];
	for (int i = 1; i < MAX_BONES_PER_VERTEX; ++i)
		Result += mul(Point, SkinPalette[Indices[i]]).xyz * Weights[i];
	return float4(Result, 1.0f);
}
//---------------------------------------------------------------------

//???why indices are float? use uint4
float3 SkinnedVector(const float3 Vector, const float4 Weights, const float4 Indices)
{
	float3 Result = mul(Vector, (float3x3)SkinPalette[Indices[0]]) * Weights[0];
	for (int i = 1; i < MAX_BONES_PER_VERTEX; ++i)
		Result += mul(Vector, (float3x3)SkinPalette[Indices[i]]) * Weights[i];
	return Result;
}
//---------------------------------------------------------------------
