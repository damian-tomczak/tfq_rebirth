float4x4 World;
float4x4 View;
float4x4 Proj;

technique
{
	pass
	{
		WorldTransform[0] = (World);
		ViewTransform = (View);
		ProjectionTransform = (Proj);
		ZEnable = true;
		ZWriteEnable = false;
		AlphaBlendEnable = false;
		CullMode = NONE; // To wa¿ne!
		ColorWriteEnable = 0; // TEST: RED | GREEN | BLUE | ALPHA;
		VertexShader = NULL;
		PixelShader = NULL;
		ColorOp[0] = SELECTARG1;
		ColorArg1[0] = DIFFUSE;
		AlphaOp[0] = SELECTARG1;
		AlphaArg1[0] = DIFFUSE;
		ColorOp[1] = DISABLE;
		AlphaOp[1] = DISABLE;
	}
}

