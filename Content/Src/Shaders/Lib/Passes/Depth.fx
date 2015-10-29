technique t0
{
	pass p0
	{
		//D3D9 only
		AlphaTestEnable   = False;
		FogEnable         = False;

		// Blend
		AlphaBlendEnable  = False;
		ColorWriteEnable  = RED; // Write depth as color for later use in other shaders
		
		// Depth-stencil
		ZEnable           = True;
		ZWriteEnable      = True;
		ZFunc             = LessEqual;
		StencilEnable     = False;
		
		// Rasterizer
		ScissorTestEnable = False;
	}
}