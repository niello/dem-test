
cbuffer CameraParams: register(b0)
{
	matrix ViewProj;
}

float4 VSMain(): SV_Position
{
	return float4(0, 0, 0, 1); //mul(float4(0, 0, 0, 1), ViewProj);
}