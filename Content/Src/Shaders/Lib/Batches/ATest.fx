technique t0
{
	pass p0
	{
		//D3D9 only
		AlphaTestEnable   = False;		// Pixel clipping is used instead

		// Depth-stencil
		//ZFunc             = Equal;	// Can uncomment this and don't use clip in Color PS
		ZFunc             = LessEqual;	// Equal - smth like stencil. Sometimes float error makes this test fail :\

		// Blend
		AlphaBlendEnable  = False;
	}
}