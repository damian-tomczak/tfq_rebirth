texture Texture;
float4x4 WorldViewProj;
float4 Color;

sampler TextureSampler = sampler_state
{
	Texture = (Texture);
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

struct VS_INPUT
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
};

void MainVS(VS_INPUT In, out VS_OUTPUT Out)
{
	Out.Pos = mul(In.Pos, WorldViewProj);
	Out.Tex = In.Tex;
}

void MainPS(VS_OUTPUT In, out float4 Out : COLOR0)
{
	Out = tex2D(TextureSampler, In.Tex) * Color;
}

technique
{
	pass
	{
		ZEnable = false;
		CullMode = NONE;
		AlphaBlendEnable = false;
		AlphaTestEnable = false;
		ColorWriteEnable = 0xFFFFFFFF;
		FogEnable = false;
		VertexShader = compile vs_2_0 MainVS();
		PixelShader = compile ps_2_0 MainPS();
	}
}

