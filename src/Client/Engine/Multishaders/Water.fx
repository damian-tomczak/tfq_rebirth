matrix WorldViewProj;
texture NormalTexture;
float3 CameraPos;
float3 DirToLight;
float3 LightColor;
texture CausticsTexture;
float4 FogColor; // Kolor mg³y
float2 FogFactors; // Wspó³czynniki równania liniowego dla mg³y
float SpecularExponent;
float4 WaterColor; // Alpha oznacza bazow¹ przezroczystoœæ
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
		float4 Fog : TEXCOORD4; // To musi byæ jako TEXCOORD nie DIFFUSE ¿eby nie robi³o siê clampowanie
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
	DirToCam.y = abs(DirToCam.y); // nie zmienia d³ugoœci, pozostaje znormalizowany
	
	// Policz wektor normalny w tym miejscu, w przestrzeni modelu
	float3 NormalSample1 = tex2D(NormalSampler, In.NormalTex1).rgb * 2 - 1;	
	float3 NormalSample2 = tex2D(NormalSampler, In.NormalTex2).rgb * 2 - 1;	
	// Tu jest Swizzle, bo wektor sterczy z tekstury na Z, a do góry ma sterczeæ na Y
	// (to takie uproszczone przekszta³cenie z Tangent Space do Model Space).
	float3 FinalNormal = normalize(((NormalSample1 + NormalSample2)/2).xzy);
	
	// Policz wektory odbite
	float3 ReflectedLight = reflect(-DirToLight, FinalNormal);
	float3 ReflectedCam = reflect(-DirToCam, FinalNormal);

	// Policz odbicie nieba	
	float EnvFactor = ReflectedCam.y; // saturate(dot(ReflectedCam, float3(0, 1, 0)));
	float3 EnvColor = lerp(HorizonColor, ZenithColor, EnvFactor);
	
	// Policz kolor wody jako kolor bazowy rozjaœniony o Caustics
	float3 MyWaterColor = WaterColor;
	MyWaterColor += tex2D(CausticsSampler, In.CausticsTex) * CausticsColor;

	// Policz fresnel
	// Teoretycznie mo¿naby ca³e to wyra¿enie podnosiæ do potêgi, tak siê robi z fresnelem,
	// ale stwierdzi³em, ¿e dla wody najlepiej wygl¹da tu wyk³adnik 1.
	float FresnelFactor = 1 - saturate(dot(FinalNormal, DirToCam));
	float3 FresnelColor = lerp(MyWaterColor, EnvColor, FresnelFactor);
	
	// Policz odblask
	float SpecularFactor = pow(saturate(dot(ReflectedLight, DirToCam)), SpecularExponent);
	float3 SpecularColor = LightColor * SpecularFactor; // Kolor Specular jest addytywny
	
	// Wpisz kolor wyjœciowy jako suma zwyk³ego i odblasku
	Out.rgb = FresnelColor + SpecularColor;
	// Ustaw wartoœæ alfa na bazow¹ plus dodatkowy odblask lub fresnel
	Out.a = lerp(WaterColor.a, 1, max(FresnelFactor, SpecularFactor));

	// Policz mg³ê
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

