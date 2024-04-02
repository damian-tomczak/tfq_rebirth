float4x4 WorldViewProj;
float3 HorizonColor;
float3 ZenithColor;
texture StarsTexture;
float StarVisibility; // 0..1

sampler StarsSampler = sampler_state
{
    Texture = (StarsTexture);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

struct SKYDOME_VS_INPUT
{
	float3 Pos : POSITION;
	half4 Diffuse : COLOR0;
	half2 StarsTex : TEXCOORD0;
};

struct SKYDOME_VS_OUTPUT
{
	float4 Pos : POSITION;
	// Tex0.x = szerokoœæ geograficzna od 0 (horyzont) do 1 (zenit)
	half1 Tex0 : TEXCOORD0;
	half2 StarsTex : TEXCOORD1;
};

void SkydomeVS(SKYDOME_VS_INPUT In, out SKYDOME_VS_OUTPUT Out)
{
	Out.Pos = mul(float4(In.Pos, 1), WorldViewProj);
	Out.Tex0.x = In.Diffuse.r;
	Out.StarsTex = In.StarsTex;
}

void SkydomePS(SKYDOME_VS_OUTPUT In, out half4 Out : COLOR0)
{
	Out = half4(lerp(HorizonColor, ZenithColor, In.Tex0.x), 1);
	Out += tex2D(StarsSampler, In.StarsTex) * StarVisibility * In.Tex0.x;
}


technique
{
	pass
	{
		VertexShader = compile vs_2_0 SkydomeVS();
		PixelShader = compile ps_2_0 SkydomePS();
	}
}

