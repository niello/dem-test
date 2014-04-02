//#include "Lib/Shared.fxh"

technique t0
{
	pass p0
	{
		ColorWriteEnable  = RED|GREEN|BLUE|ALPHA;
		ZEnable           = True;
		ZWriteEnable      = False;
		ZFunc             = LessEqual;       
		FogEnable         = False;
		AlphaBlendEnable  = False;
		AlphaTestEnable   = False;
		//AlphaFunc         = GreaterEqual;
		ScissorTestEnable = False;
		//CullMode          = CW;
	}
}