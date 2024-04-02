float4x4 WorldViewProj; // Macierz �wiata * widoku * projekcji
texture Texture; // Tekstura g��wna drzewa danego gatunku
float3 RightVec; // Wektor w prawo wraz z dostosowan� d�ugo�ci�, w przestrzeni lokalnej
float3 UpVec; // Wektor do g�ry wraz z dostosowan� d�ugo�ci�, w przestrzeni lokalnej
float3 DirToLight; // Kierunek DO �wiat�a, znormalizowany, w przestrzeni lokalnej
float4 LightColor; // Kolor �wiat�a [* kolor drzewa]
float4 FogColor; // Kolor mg�y
float2 FogFactors; // Wsp�czynniki r�wnania liniowego dla mg�y
float4 QuadWindFactors; // xy to modyfikacja wektora Right, zw to modyfikacja wektora Up

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
	half4 Diffuse : COLOR0;
	half3 Normal : NORMAL;
	half2 Tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Pos : POSITION;
	
	#if (LIGHTING == 1)
		half4 Lighting : COLOR0;
	#endif
	
	#if (FOG == 1)
		half4 Fog : COLOR1;
	#endif
	
	half2 Tex : TEXCOORD0;
	
	#if (RENDER_TO_SHADOW_MAP == 1)
		// Z i W pozycji we wsp. �wiata
		float2 ShadowMapZW : TEXCOORD1;
	#endif
};

void TreeVS(VS_INPUT In, out VS_OUTPUT Out)
{
	float2 QuadBias = In.Diffuse.rg * 4 - 2;
	float2 Randoms = In.Diffuse.ba * 2 - 1;
	QuadBias += QuadWindFactors.xy * Randoms.x + QuadWindFactors.zw * Randoms.y;
	float3 MyPos = In.Pos + QuadBias.x * RightVec + QuadBias.y * UpVec;
	Out.Pos = mul(float4(MyPos, 1), WorldViewProj);
	
	Out.Tex = In.Tex;
	
	#if (LIGHTING == 1)
		Out.Lighting = dot(In.Normal, DirToLight) * 0.5 + 0.5; // Half-Lambert
		Out.Lighting *= LightColor;
	#endif
	
	#if (FOG == 1)
		Out.Fog = FogColor;
		Out.Fog.a = FogFactors.x * Out.Pos.z + FogFactors.y;
	#endif

	#if (RENDER_TO_SHADOW_MAP == 1)
	{
		Out.ShadowMapZW = Out.Pos.zw;
	}
	#endif
}

// Wi�ksza dok�adno�� bo do rendering shadow mapy
#if (RENDER_TO_SHADOW_MAP == 1)
	void TreePS(VS_OUTPUT In, out float4 Out : COLOR0)
#else
	void TreePS(VS_OUTPUT In, out half4 Out : COLOR0)
#endif
{
	Out = tex2D(Sampler, In.Tex);
	
	#if (LIGHTING == 1)
		Out.rgb *= In.Lighting;
	#endif
	
	#if (FOG == 1)
		Out.rgb = lerp(Out.rgb, In.Fog.rgb, In.Fog.a);
	#endif

	// Render do Shadow Mapy
	#if (RENDER_TO_SHADOW_MAP == 1)
	{
		float v = In.ShadowMapZW.x / In.ShadowMapZW.y; // Tak naprawd� to WorldPos.z / WorldPos.w
		// Wykorzystaj sk�adow� R
		#if (SHADOW_MAP_MODE == 1)
			clip(Out.a - 0.5);
			Out = float4(v, v, v, v);
		// Spakuj do sk�adowych RGB
		#elif (SHADOW_MAP_MODE == 2)
			float Alpha = Out.a;
			Out = v * float4( 256*256, 256, 1, 0 ); // Przepisz alfa
			Out.rgb = frac(Out.rgb);
			Out.a = Alpha;
			// W topicu forum (http://www.gamedev.net/community/forums/topic.asp?topic_id=442138 i http://www.codesampler.com/phpBB2/viewtopic.php?t=995)
			// pisz� �e to jest potrzebne, chocia� ayufan m�wi �e shader przycina a nie zaokr�gla.
			// Eksperymentalnie stwierdzi�em, �e lepiej bez tego - jak to by�o to powstawa�y czasem artefakty.
			//Out -= Out.rrgb * float4( 0, 1.0/256.0, 1.0/256.0, 0 );
		#endif
	}
	#endif
}


technique
{
	pass
	{
		VertexShader = compile vs_2_0 TreeVS();
		PixelShader = compile ps_2_0 TreePS();
	}
}

