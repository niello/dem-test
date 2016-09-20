
// Params and declarations shared between all shaders in a render path.
// They are set once at the start of the frame, so all shaders must be
// compatible to these declarations.

cbuffer CameraParams: register(b0)
{
	matrix	ViewProj;
	float3	EyePos;
}
