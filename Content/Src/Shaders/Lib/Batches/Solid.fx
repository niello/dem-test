technique t0
{
	pass p0
	{
		//D3D9 only
		AlphaTestEnable   = False; 

		// Depth-stencil
		ZFunc             = LessEqual;

		// Blend
		AlphaBlendEnable  = False;
	}
}