matrix WorldViewProj;
texture NormalTexture;
float3 CameraPos;
float3 DirToLight;
float3 LightColor;
texture CausticsTexture;
float4 FogColor; // Kolor mg�y
float2 FogFactors; // Wsp�czynniki r�wnania liniowego dla mg�y
float SpecularExponent;
float4 WaterColor; // Alpha oznacza bazow� przezroczysto��
float3 CausticsColor;
float3 HorizonColor;
float3 ZenithColor;
float4 TexOffsets; // xy = Time*Vel dla normal mapy 1, zw = Time*Vel dla normal mapy 2
float2 TexScales; // x = skalowanie Normal, y = skalowanie Caustics

sampler NormalSampler = sampler_state
{
	Texture = (NormalTexture);
	AddressU = WRAP;
	AddressV = WRAP;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
};

sampler CausticsSampler = sampler_state
{
	Texture = (CausticsTexture);
	AddressU = WRAP;
	AddressV = WRAP;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
};


struct VS_INPUT
{
	float3 Pos : POSITION;
};

struct VS_OUTPUT
{
	float4 Pos : POSITION;
	float2 NormalTex1 : TEXCOORD0;
	float2 NormalTex2 : TEXCOORD1;
	float2 CausticsTex : TEXCOORD2;
	float3 VecToCam : TEXCOORD3;
	#if (FOG == 1)
		float4 Fog : TEXCOORD4; // To musi by� jako TEXCOORD nie DIFFUSE �eby nie robi�o si� clampowanie
	#endif
};

void WaterVS(VS_INPUT In, out VS_OUTPUT Out)
{
	Out.Pos = mul(float4(In.Pos, 1), WorldViewProj);
	
	const float TEX_SCALE = 1.0 / 5.0;
	Out.NormalTex1 = In.Pos.xz * TexScales.x + TexOffsets.xy;
	Out.NormalTex2 = In.Pos.zx * TexScales.x + TexOffsets.zw;
	Out.CausticsTex = In.Pos.xz * TexScales.y;
	
	Out.VecToCam = CameraPos - In.Pos;

	#if (FOG == 1)
		Out.Fog = FogColor;
		Out.Fog.a = FogFactors.x * Out.Pos.z + FogFactors.y;
	#endif
}

void WaterPS(VS_OUTPUT In, out half4 Out : COLOR0)
{
	float3 DirToCam = normalize(In.VecToCam);
	DirToCam.y = abs(DirToCam.y); // nie zmienia d�ugo�ci, pozostaje znormalizowany
	
	// Policz wektor normalny w tym miejscu, w przestrzeni modelu
	float3 NormalSample1 = tex2D(NormalSampler, In.NormalTex1).rgb * 2 - 1;	
	float3 NormalSample2 = tex2D(NormalSampler, In.NormalTex2).rgb * 2 - 1;	
	// Tu jest Swizzle, bo wektor sterczy z tekstury na Z, a do g�ry ma stercze� na Y
	// (to takie uproszczone przekszta�cenie z Tangent Space do Model Space).
	float3 FinalNormal = normalize(((NormalSample1 + NormalSample2)/2).xzy);
	
	// Policz wektory odbite
	float3 ReflectedLight = reflect(-DirToLight, FinalNormal);
	float3 ReflectedCam = reflect(-DirToCam, FinalNormal);

	// Policz odbicie nieba	
	float EnvFactor = ReflectedCam.y; // saturate(dot(ReflectedCam, float3(0, 1, 0)));
	float3 EnvColor = lerp(HorizonColor, ZenithColor, EnvFactor);
	
	// Policz kolor wody jako kolor bazowy rozja�niony o Caustics
	float3 MyWaterColor = WaterColor;
	MyWaterColor += tex2D(CausticsSampler, In.CausticsTex) * CausticsColor;

	// Policz fresnel
	// Teoretycznie mo�naby ca�e to wyra�enie podnosi� do pot�gi, tak si� robi z fresnelem,
	// ale stwierdzi�em, �e dla wody najlepiej wygl�da tu wyk�adnik 1.
	float FresnelFactor = 1 - saturate(dot(FinalNormal, DirToCam));
	float3 FresnelColor = lerp(MyWaterColor, EnvColor, FresnelFactor);
	
	// Policz odblask
	float SpecularFactor = pow(saturate(dot(ReflectedLight, DirToCam)), SpecularExponent);
	float3 SpecularColor = LightColor * SpecularFactor; // Kolor Specular jest addytywny
	
	// Wpisz kolor wyj�ciowy jako suma zwyk�ego i odblasku
	Out.rgb = FresnelColor + SpecularColor;
	// Ustaw warto�� alfa na bazow� plus dodatkowy odblask lub fresnel
	Out.a = lerp(WaterColor.a, 1, max(FresnelFactor, SpecularFactor));

	// Policz mg��
	#if (FOG == 1)
		Out.rgb = lerp(Out.rgb, In.Fog.rgb, saturate(In.Fog.a));
	#endif
}


technique
{
	pass
	{
		VertexShader = compile vs_2_0 WaterVS();
		PixelShader = compile ps_2_0 WaterPS();
	}
}

