technique t0
{
	pass p0
	{
		ColorWriteEnable  = RED; // Write depth as color for later use in other shaders
		ZEnable           = True;
		ZWriteEnable      = True;
		ZFunc             = LessEqual;
		StencilEnable     = False;
		FogEnable         = False;
		AlphaBlendEnable  = False;
		AlphaTestEnable   = False;
		ScissorTestEnable = False;
	}
}