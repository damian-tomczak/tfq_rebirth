float4x4 WorldViewProj;

struct VS_INPUT
{
	float4 Pos : POSITION;
	half4 Diffuse : COLOR0;
};

struct VS_OUTPUT
{
	float4 Pos : POSITION;
	half4 Diffuse : COLOR0;
};

void MainVS(VS_INPUT In, out VS_OUTPUT Out)
{
	Out.Pos = mul(In.Pos, WorldViewProj);
	Out.Diffuse = In.Diffuse;
}

void MainPS(VS_OUTPUT In, out float4 Out : COLOR0)
{
	Out = In.Diffuse;
}


technique Solid
{
	pass
	{
		ZEnable = true;
		AlphaBlendEnable = false;
		CullMode = NONE;
		VertexShader = compile vs_2_0 MainVS();
		PixelShader = compile ps_2_0 MainPS();
	}
}

technique Lines
{
	pass
	{
		ZEnable = true;
		AlphaBlendEnable = false;
		CullMode = NONE;
		VertexShader = compile vs_2_0 MainVS();
		PixelShader = compile ps_2_0 MainPS();
	}
}
