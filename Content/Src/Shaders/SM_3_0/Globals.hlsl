
matrix ViewProj: register(c0) <string Buffer = "CameraParams";>;

float4 VSMain(): POSITION
{
	return float4(0, 0, 0, 1); //mul(float4(0, 0, 0, 1), ViewProj);
}