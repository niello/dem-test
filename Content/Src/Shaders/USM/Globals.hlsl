
// Params and declarations shared between all shaders in a render path.
// They are set once at the start of the frame, so all shaders must be
// compatible to these declarations.

cbuffer CameraParams: register(b0)
{
	matrix	ViewProj;
	float3	EyePos;
}

#ifndef MAX_LIGHT_COUNT
#define MAX_LIGHT_COUNT 256
#endif

struct CLight
{
	float3	Color;
	float	Intensity;			//???or pre-multiply color on intensity?
	float3	Position;
	float	InvRange;
	float4	Params;				// Spot: x - cos inner, y - cos outer
	float3	Direction;			// Pre-inverted for directional lights
	uint	Type;
};

cbuffer LightBuffer: register(b3)
{
	CLight Lights[MAX_LIGHT_COUNT];
}
