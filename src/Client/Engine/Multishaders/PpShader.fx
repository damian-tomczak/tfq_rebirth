texture Texture;

// Efekt Function
float GrayscaleFactor;
float AFactor;
float BFactor;

// Efekt Tone Mapping
float LuminanceFactor;

// Efekt Heat Haze
texture PerturbationMap;

sampler TextureSampler = sampler_state
{
	Texture = (Texture);
	AddressU = CLAMP;
	AddressV = CLAMP;
	MinFilter = POINT;
	MagFilter = POINT;
	MipFilter = NONE;
};

sampler PerturbationMapSampler = sampler_state
{
	Texture = (PerturbationMap);
	AddressU = WRAP;
	AddressV = WRAP;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
};

struct PS_INPUT
{
	float4 Pos : POSITION;
	half2 Tex : TEXCOORD0;
	#if (HEAT_HAZE == 1)
		half2 PerturbationTex1 : TEXCOORD1;
		half2 PerturbationTex2 : TEXCOORD2;
	#endif
};

void MainPS(PS_INPUT In, out half4 Out : COLOR)
{
	Out = tex2D(TextureSampler, In.Tex);
	
	// Heat Haze
	#if (HEAT_HAZE == 1)
	{
		float Alpha = Out.a;
		// Nie wiem dlaczego, ale to dzia³a poprawnie w³aœnie wtedy kiedy nie robiê ani
		// uœredniania tych dwóch sampli, ani biasowania wyniku do -1..1.
		float2 Perturbation =
			(tex2D(PerturbationMapSampler, In.PerturbationTex1) + 
			tex2D(PerturbationMapSampler, In.PerturbationTex2)).rg;
		Perturbation *= Alpha*Alpha;
		Perturbation *= 0.05;
		Out = tex2D(TextureSampler, In.Tex + Perturbation);
	}
	#endif
	
	// Tone Mapping
	
	if (TONE_MAPPING == 1)
	{
		// Lepszy tone mapping, wg wzoru z PDF-a
		// Uwaga! Z tego wzoru nigdy nie wyjœcie rozjaœnienie, zawsze sciemnienie
		//float LuminanceScaled = AlphaDivLuminance * Grayscale;
		//Out = Out * LuminanceScaled / (1 + LuminanceScaled);

		Out *= LuminanceFactor;
	}
	
	// Efekt Function

	if (GRAYSCALE > 0)
	{	
		half Grayscale = dot(Out, float4(0.229, 0.587, 0.114, 0));
		if (GRAYSCALE == 1)
			Out = Grayscale;
		else if (GRAYSCALE == 2)
			Out = lerp(Out, Grayscale, GrayscaleFactor);
	}
	
	if (FUNCTION)
		Out = Out * AFactor + BFactor;
}

technique
{
	pass
	{
		CullMode = NONE;
		ZEnable = false;
		AlphaBlendEnable = false;
		AlphaTestEnable = false;
		
		VertexShader = NULL; // I tak nie zadzia³a, bo format wierzcho³ka jest XYZRHW
		PixelShader = compile ps_2_a MainPS();
	}
}

