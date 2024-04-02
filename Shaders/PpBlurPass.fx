texture Texture;

sampler TextureSampler = sampler_state
{
	Texture = (Texture);
	AddressU = CLAMP;
	AddressV = CLAMP;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
};

struct PS_INPUT
{
	half2 Tex0 : TEXCOORD0;
	half2 Tex1 : TEXCOORD1;
	half2 Tex2 : TEXCOORD2;
	half2 Tex3 : TEXCOORD3;
};

void MainPS(PS_INPUT In, out float4 Out : COLOR)
{
	half4 v[4];
	v[0] = tex2D(TextureSampler, In.Tex0);
	v[1] = tex2D(TextureSampler, In.Tex1);
	v[2] = tex2D(TextureSampler, In.Tex2);
	v[3] = tex2D(TextureSampler, In.Tex3);

	half4 Sum = v[0];
	for (int i = 1; i < 4; i++)
		Sum += v[i];
	Out = Sum / 4;
}

technique
{
	pass
	{
		CullMode = NONE;
		ZEnable = false;
		AlphaBlendEnable = false;
		AlphaTestEnable = false;
		
		VertexShader = null; // I tak nie dzia³a, bo jest XYZRHW
		PixelShader = compile ps_2_a MainPS();
	}
}

