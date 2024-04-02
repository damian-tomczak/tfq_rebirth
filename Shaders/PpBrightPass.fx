texture Texture;
half AFactor;
half BFactor;

sampler TextureSampler = sampler_state
{
	Texture = (Texture);
	AddressU = CLAMP;
	AddressV = CLAMP;
	MinFilter = POINT;
	MagFilter = POINT;
	MipFilter = NONE;
};

struct PS_INPUT
{
	float4 Pos : POSITION;
	half2 Tex : TEXCOORD0;
};

void MainPS(PS_INPUT In, out half4 Out : COLOR)
{
	Out = tex2D(TextureSampler, In.Tex) * AFactor + BFactor;
}

technique
{
	pass
	{
		CullMode = NONE;
		ZEnable = false;
		AlphaBlendEnable = false;
		AlphaTestEnable = false;
		
		VertexShader = NULL; // I tak nie dzia³a, bo wierzcho³ki s¹ XYZRHW
		PixelShader = compile ps_2_a MainPS();
	}
}

