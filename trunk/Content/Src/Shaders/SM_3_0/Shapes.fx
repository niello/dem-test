
// Debug shapes, color only

shared float4x4 ViewProjection;

// Color **************************************************************************

void VSColorShape(		float4	Pos:	POSITION,
						float4	Color:	COLOR,
					out	float4	oPos:	POSITION,
					out	float4	oColor:	COLOR)
{
	oPos = mul(Pos, ViewProjection);
	oColor = Color;
}
//---------------------------------------------------------------------

void VSColorPoint(		float4	Pos:	POSITION,
						float4	Color:	COLOR,
					out	float4	oPos:	POSITION,
					out	float4	oColor:	COLOR,
					out	float	oPSize:	PSIZE)
{
	oPSize = Pos.w;
	Pos.w = 1.f;
	oPos = mul(Pos, ViewProjection);
	oColor = Color;
}
//---------------------------------------------------------------------

void VSColorShapeInstanced(		float4	Pos:	POSITION,
								float4	World1:	TEXCOORD4,
								float4	World2:	TEXCOORD5,
								float4	World3:	TEXCOORD6,
								float4	World4:	TEXCOORD7,
								float4	Color:	COLOR,
							out	float4	oPos:	POSITION,
							out	float4	oColor:	COLOR)
{
	oPos = mul(Pos, mul(float4x4(World1, World2, World3, World4), ViewProjection));
	oColor = Color;
}
//---------------------------------------------------------------------

float4 PSColorShape(float4 Color: COLOR): COLOR
{
	return Color;
}
//---------------------------------------------------------------------

technique ColorPrims < string Mask = "Default"; >
{
	pass p0
	{
		VertexShader		= compile vs_3_0 VSColorShape();
		PixelShader			= compile ps_3_0 PSColorShape();
		AlphaBlendEnable	= True;
		AlphaTestEnable		= False;
		CullMode			= None; //CW;
		SrcBlend			= SrcAlpha;
		DestBlend			= InvSrcAlpha;
		ZEnable				= False; //True;
		ZFunc				= LessEqual;
		ZWriteEnable		= False;
	}
}

technique ColorPoints < string Mask = "Point"; >
{
	pass p0
	{
		VertexShader		= compile vs_3_0 VSColorPoint();
		PixelShader			= compile ps_3_0 PSColorShape();
		AlphaBlendEnable	= True;
		AlphaTestEnable		= False;
		CullMode			= None; //CW;
		SrcBlend			= SrcAlpha;
		DestBlend			= InvSrcAlpha;
		ZEnable				= False; //True;
		ZFunc				= LessEqual;
		ZWriteEnable		= False;
	}
}

technique ColorInst < string Mask = "Instanced"; >
{
	pass p0
	{
		VertexShader		= compile vs_3_0 VSColorShapeInstanced();
		PixelShader			= compile ps_3_0 PSColorShape();
		AlphaBlendEnable	= True;
		AlphaTestEnable		= False;
		CullMode			= None; //CW;
		SrcBlend			= SrcAlpha;
		DestBlend			= InvSrcAlpha;
		ZEnable				= False; //True;
		ZFunc				= LessEqual;
		ZWriteEnable		= False;
	}
}
