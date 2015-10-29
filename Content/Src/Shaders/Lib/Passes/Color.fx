//#include "Lib/Shared.fxh"

technique t0
{
	pass p0
	{
		//D3D9 only
		AlphaTestEnable   = False;
		FogEnable         = False;
		//AlphaFunc         = GreaterEqual;

		// Rasterizer
		//CullMode          = CW;
		ScissorTestEnable = False;

		// Blend
		AlphaBlendEnable  = False;
		ColorWriteEnable  = RED|GREEN|BLUE|ALPHA;

		// Depth-stencil
		ZEnable           = True;
		ZWriteEnable      = False;
		ZFunc             = LessEqual;       
	}
}