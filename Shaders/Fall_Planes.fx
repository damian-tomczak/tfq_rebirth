texture Texture;
float4x4 WorldViewProj;
half4 MainColor;
float TexOffset;

sampler Sampler = sampler_state
{
    Texture = (Texture);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

struct VS_INPUT
{
	float3 Pos : POSITION;
	half2 Tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Pos : POSITION;
	half2 Tex : TEXCOORD0;
};

void GrassVS(VS_INPUT In, out VS_OUTPUT Out)
{
	Out.Pos = mul(float4(In.Pos, 1), WorldViewProj);
	
	Out.Tex = In.Tex;
	Out.Tex.y += TexOffset;
}

void GrassPS(VS_OUTPUT In, out half4 Out : COLOR0)
{
	Out = tex2D(Sampler, In.Tex) * MainColor;
}

technique
{
	pass
	{
		VertexShader = compile vs_2_0 GrassVS();
		PixelShader = compile ps_2_0 GrassPS();
	}
}
