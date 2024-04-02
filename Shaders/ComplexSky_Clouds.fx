float4x4 WorldViewProj;
texture NoiseTexture;
float4 NoiseTexA_01; // Wspó³czynnik A równania liniowego dla oktawy 0 (xy) i 1 (zw)
float4 NoiseTexA_23; // Wspó³czynnik A równania liniowego dla oktawy 2 (xy) i 3 (zw)
float4 NoiseTexB_01; // Wspó³czynnik B równania liniowego dla oktawy 0 (xy) i 1 (zw)
float4 NoiseTexB_23; // Wspó³czynnik B równania liniowego dla oktawy 2 (xy) i 3 (zw)
float4 Color1;
float4 Color2;
float Sharpness;
float Threshold;

sampler NoiseSampler = sampler_state
{
    Texture = (NoiseTexture);
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
	half2 Tex : TEXCOORD0;
};

struct SKYDOME_VS_OUTPUT
{
	float4 Pos : POSITION;
	// Tex0.x = szerokoœæ geograficzna od 0 (horyzont) do 1 (zenit)
	half1 Tex0 : TEXCOORD0;
	half2 NoiseTex0 : TEXCOORD1;
	half2 NoiseTex1 : TEXCOORD2;
	half2 NoiseTex2 : TEXCOORD3;
	half2 NoiseTex3 : TEXCOORD4;
};

void SkydomeVS(SKYDOME_VS_INPUT In, out SKYDOME_VS_OUTPUT Out)
{
	Out.Pos = mul(float4(In.Pos, 1), WorldViewProj);
	Out.Tex0.x = In.Diffuse.r;
	Out.NoiseTex0 = In.Tex * NoiseTexA_01.xy + NoiseTexB_01.xy;
	Out.NoiseTex1 = In.Tex * NoiseTexA_01.zw + NoiseTexB_01.zw;
	Out.NoiseTex2 = In.Tex * NoiseTexA_23.xy + NoiseTexB_23.xy;
	Out.NoiseTex3 = In.Tex * NoiseTexA_23.zw + NoiseTexB_23.zw;
}

void SkydomePS(SKYDOME_VS_OUTPUT In, out half4 Out : COLOR0)
{
	half4 CloudSamples;
	CloudSamples.x = tex2D(NoiseSampler, In.NoiseTex0).r;
	CloudSamples.y = tex2D(NoiseSampler, In.NoiseTex1).r;
	CloudSamples.z = tex2D(NoiseSampler, In.NoiseTex2).r;
	CloudSamples.w = tex2D(NoiseSampler, In.NoiseTex3).r;
	half CloudSample = dot(CloudSamples, float4(1.0/1.0, 1.0/2.0, 1.0/4.0, 1.0/8.0));
	
	//float f_1 = 1 - exp( Sharpness * (Threshold-1) );
	float f_1 = 1;
	CloudSample = saturate(1 - exp(Sharpness*Threshold - Sharpness*CloudSample)) / f_1;

	Out = lerp(Color1, Color2, CloudSample);
	Out.a = CloudSample * In.Tex0.x;
}


technique
{
	pass
	{
		VertexShader = compile vs_2_0 SkydomeVS();
		PixelShader = compile ps_2_0 SkydomePS();
	}
}

