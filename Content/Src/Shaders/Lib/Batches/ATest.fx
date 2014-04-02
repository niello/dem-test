technique t0
{
	pass p0
	{
		//ZFunc             = Equal;	// Can uncomment this and don't use clip in Color PS
		ZFunc             = LessEqual;	// Equal - smth like stencil. Sometimes float error makes this test fail :\
		AlphaBlendEnable  = False;
		AlphaTestEnable   = False;		// Pixel clipping is used instead
	}
}