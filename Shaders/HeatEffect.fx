float4x4 WorldViewProj;
half3 DirToCam;
half ZFar;

struct VS_INPUT
{
	float4 Pos : POSITION;
	half4 Diffuse : COLOR0;
	half3 Normal : NORMAL;
};

struct VS_OUTPUT
{
	float4 Pos : POSITION;
	half4 Diffuse : COLOR0;
};

void MainVS(VS_INPUT In, out VS_OUTPUT Out)
{
	Out.Pos = mul(In.Pos, WorldViewProj);
	
	half CamDot = dot(In.Normal, DirToCam);
	half ZFactor = 1 - (Out.Pos.w / ZFar);
	ZFactor = ZFactor * ZFactor;
	Out.Diffuse = In.Diffuse.a * CamDot * ZFactor;
}

technique
{
	pass
	{
		SrcBlend = SRCALPHA;
		DestBlend = ONE;
		AlphaTestEnable = false;
		ZEnable = true;
		ZWriteEnable = false;
		FogEnable = false;
		CullMode = CCW;
		
		// RETAIL
		ColorWriteEnable = ALPHA;
		AlphaBlendEnable = true;
		
		// TEST OPTYCZNY
		//ColorWriteEnable = RED | GREEN | BLUE;
		//AlphaBlendEnable = false;
		
		Texture[0] = NULL;
		ColorOp[0] = SELECTARG1;
		ColorArg1[0] = DIFFUSE;
		AlphaOp[0] = SELECTARG1;
		AlphaArg1[0] = DIFFUSE;
		ColorOp[1] = DISABLE;
		AlphaOp[1] = DISABLE;
	
		VertexShader = compile vs_2_0 MainVS();
		PixelShader = NULL;
	}
}

