
// Hardware skinning support.

#ifndef MAX_BONES_PER_PALETTE
#define MAX_BONES_PER_PALETTE 72
#endif
#ifndef MAX_BONES_PER_VERTEX
#define MAX_BONES_PER_VERTEX 4
#endif

tbuffer SkinParams: register(t0)
{
	matrix SkinPalette[MAX_BONES_PER_PALETTE]; //!!!can use float4x3 column_major!
}

//???why indices are float? use uint4
float4 SkinnedPosition(const float4 InPos, const float4 Weights, const float4 Indices)
{
	//// need to re-normalize weights because of compression
	//float4 NormWeights = Weights / dot(Weights, float4(1.0, 1.0, 1.0, 1.0));

	float3 OutPos = mul(InPos, SkinPalette[Indices[0]]).xyz * Weights[0];
	for (int i = 1; i < MAX_BONES_PER_VERTEX; i++)
		OutPos += mul(InPos, SkinPalette[Indices[i]]).xyz * Weights[i];
	return float4(OutPos, 1.0f);
}
//---------------------------------------------------------------------
