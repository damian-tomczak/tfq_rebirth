float4x4 WorldViewProj;
texture EmissiveTexture;
texture DiffuseTexture;
float4 DiffuseColor;
float4 TeamColor;
float4 AmbientColor;
float4x4 TextureMatrix;
float3 CameraPos;
texture EnvironmentalTexture;
float4 FogColor;
float2 FogFactors;
float FresnelColor;
float FresnelPower;
float LightColor;
float3 DirToLight;
texture NormalTexture;
float SpecularColor;
float SpecularPower;
float3 LightPos;
float LightRangeSq;
float LightCosFov2;
float LightCosFov2Factor;
float ShadowFactor;
float ShadowFactorA;
float ShadowFactorB;
texture ShadowMapTexture;
float4x4 ShadowMapMatrix;
float ShadowMapSize;
float ShadowMapSizeRcp;
float ZFar;
float3 LightPos_World;
float ShadowEpsilon;
float LightRangeSq_World;
float4x3 BoneMatrices[32];
texture TerrainTexture0;
texture TerrainTexture1;
texture TerrainTexture2;
texture TerrainTexture3;
float TerrainTexScale; // Kolejne elementy odpowiadaj� skalowaniu kolejnych tekstur


sampler EmissiveSampler = sampler_state
{
    Texture = (EmissiveTexture);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

sampler DiffuseSampler = sampler_state
{
    Texture = (DiffuseTexture);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

sampler EnvironmentalSampler = sampler_state
{
    Texture = (EnvironmentalTexture);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
	AddressW = CLAMP;
};

sampler NormalSampler = sampler_state
{
    Texture = (NormalTexture);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

sampler ShadowMapSampler = sampler_state
{
    Texture = (ShadowMapTexture);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = BORDER;
    AddressV = BORDER;
    AddressW = BORDER;
    // Wszystko co poza shadow map� o�wietlone
	BorderColor = 0xFFFFFFFF;
};

sampler TerrainSampler0 = sampler_state { Texture = (TerrainTexture0); MinFilter = LINEAR; MagFilter = LINEAR; MipFilter = LINEAR; AddressU = WRAP; AddressV = WRAP; };
sampler TerrainSampler1 = sampler_state { Texture = (TerrainTexture1); MinFilter = LINEAR; MagFilter = LINEAR; MipFilter = LINEAR; AddressU = WRAP; AddressV = WRAP; };
sampler TerrainSampler2 = sampler_state { Texture = (TerrainTexture2); MinFilter = LINEAR; MagFilter = LINEAR; MipFilter = LINEAR; AddressU = WRAP; AddressV = WRAP; };
sampler TerrainSampler3 = sampler_state { Texture = (TerrainTexture3); MinFilter = LINEAR; MagFilter = LINEAR; MipFilter = LINEAR; AddressU = WRAP; AddressV = WRAP; };

struct VS_INPUT
{
	float3 Pos : POSITION;
	half3 Normal : NORMAL;
	#if (TERRAIN == 1)
		half4 TerrainTextureWeights: COLOR0;
	#endif
	half BoneWeight0 : BLENDWEIGHT0;
	int4 BoneIndices : BLENDINDICES0;
	half2 Tex : TEXCOORD0;
	half3 Tangent : TEXCOORD1;
	half3 Binormal : TEXCOORD2;
};

struct VS_OUTPUT
{
	float4 Pos : POSITION;

	#if (TERRAIN == 1)	
		half4 TerrainTextureWeights : TEXCOORD0;
	#elif (PASS == 0 || PASS == 1 || PASS == 3 || PASS == 4 || PASS == 5 || PASS == 6 || (PASS == 7 && ALPHA_TESTING == 1))
		half2 Tex : TEXCOORD0;
	#endif
	
	#if (PASS == 1 || (PASS == 3 && USE_FOG == 1)) // Fog
		// PASS == 1: RGB przechowuje kolor, A przechowuje intensywno��
		// PASS == 3: Wa�ny tylko A, przechowuje zanikanie Alpha z odleg�o�ci�
		half4 Fog : COLOR0;
	#elif (PASS == 2) // WireFrame
		half4 Color : COLOR0;
	#elif ((PASS == 4 || PASS == 5 || PASS == 6) && PER_PIXEL == 0) // �wiat�o + Per Vertex
		half4 Diffuse : COLOR0;
	#endif
	
	#if ((PASS == 4 || PASS == 5 || PASS == 6) && PER_PIXEL == 0 && SPECULAR != 0)
		half4 Specular : COLOR1;
	#endif
	#if (FRESNEL_TERM == 1)
		// FresnelColor * intensywno�� wyliczona dla danego wierzcho�ka
		half4 Fresnel : COLOR1;
	#endif
	
	#if (ENVIRONMENTAL_MAPPING == 1)
		half3 ReflectedEnvDir : TEXCOORD1;
	#elif (PASS == 4 && PER_PIXEL == 1) // �wiat�o Directional + Per Pixel
		// Directional: Kierunek do �wiat�a, w Tangent Space
		half3 DirToLight : TEXCOORD1;
	#elif (PASS == 5 && SHADOW_MAP_MODE != 0)	
		// Point + ShadowMapping: Wektor od �wiat�a do wierzcho�ka w World Space
		float3 VecFromLight_World : TEXCOORD1;
	#elif (PASS == 6 && PER_PIXEL == 1)
		// Spot: Wektor do �wiat�a w Model Space
		float3 VecToLight_Model : TEXCOORD1;
	#elif (PASS == 7)
		#if (POINT_SHADOW_MAP == 1)
			float3 VecToLight : TEXCOORD1;
		#else
			// Z i W pozycji we wsp. �wiata
			float2 ShadowMapZW : TEXCOORD1;
		#endif
	#endif
	
	#if ((PASS == 4 || PASS == 5 || PASS == 6) && SPECULAR != 0 && PER_PIXEL == 1)
		half3 VecToCam : TEXCOORD2;
	#endif
	
	#if (PASS == 5 || PASS == 6)
		// Wektor od wierzcho�ka do �wiat�a, w Tangent Space
		half3 VecToLight : TEXCOORD3;
	#endif
	
	#if ((PASS == 4 || PASS == 6) && SHADOW_MAP_MODE != 0)
		// Wsp�rz�dne shadow mapy
		float4 ShadowMapPos : TEXCOORD4;
	#endif

	#if (VARIABLE_SHADOW_FACTOR == 1)
		float ShadowFactor : TEXCOORD5;
	#endif
	
	#if (TERRAIN == 1)
		half4 TerrainTex01 : TEXCOORD6;
		half4 TerrainTex23 : TEXCOORD7;
	#endif
};

#if (FRESNEL_TERM == 1)
	half4 CalcFresnel(float3 VertexPos, float3 VertexNormal)
	{
		float3 DirToCam = normalize(CameraPos - VertexPos);
		float Dot = dot(DirToCam, VertexNormal);
		return FresnelColor * pow(1 - abs(Dot), FresnelPower);
	}
#endif

#if (SKINNING == 1)

	float3 SkinPos(float3 Pos, VS_INPUT Input)
	{
		float3 OutputPos = mul(float4(Pos, 1), BoneMatrices[Input.BoneIndices[0]]) * Input.BoneWeight0;
		OutputPos += mul(float4(Pos, 1), BoneMatrices[Input.BoneIndices[1]]) * (1 - Input.BoneWeight0);
		return OutputPos;
	}
	
	half3 SkinNormal(half3 Normal, VS_INPUT Input)
	{
		half3 OutputNormal = mul(Normal, (half3x3)BoneMatrices[Input.BoneIndices[0]]) * Input.BoneWeight0;
		OutputNormal += mul(Normal, (half3x3)BoneMatrices[Input.BoneIndices[1]]) * (1 - Input.BoneWeight0);
		return OutputNormal;
	}

#endif

void MainVS(VS_INPUT In, out VS_OUTPUT Out)
{
	float3 InputPos;
	half3 InputNormal;
	half3 InputTangent;
	half3 InputBinormal;
	#if (SKINNING == 1)
	{
		InputPos = SkinPos(In.Pos, In);
		InputNormal = SkinNormal(In.Normal, In);
		InputTangent = SkinNormal(In.Tangent, In);
		InputBinormal = SkinNormal(In.Binormal, In);
	}
	#else
	{
		InputPos = In.Pos;
		InputNormal = In.Normal;
		InputTangent = In.Tangent;
		InputBinormal = In.Binormal;
	}
	#endif
	
	#if (TERRAIN == 1)
		Out.TerrainTextureWeights = In.TerrainTextureWeights;
	#endif
	
	Out.Pos = mul(float4(InputPos, 1), WorldViewProj);
	
	// To co wsp�lne ma PASS 0 (Base) i 3 (Translucent)
	#if (PASS == 0 || PASS == 3)
	{
		// Wsp. tekstury
		#if (TERRAIN == 1)
		{
		}
		#else
		{
			#if (TEXTURE_ANIMATION == 1)
				Out.Tex = mul(float4(In.Tex, 0, 1), (float4x2)TextureMatrix);
			#else
				Out.Tex = In.Tex;
			#endif
		}
		#endif
	
		// ReflectedEnvDir do env mappingu
		#if (ENVIRONMENTAL_MAPPING == 1)
		{
			float3 VecFromCam = (float3)InputPos - CameraPos; // Normalizowa� czy nie?
			float3 Reflected = reflect(VecFromCam, InputNormal);
			// Normalizowa� czy nie, oto jest pytanie.
			// W TFQ6 te� by�o tu zadane to pytanie ale ostatecznie nie by�o normalizowane.
			// W TFQ6 tu by�o Reflected mno�one przez macierz (float4x3)World.
			// To by�o bez sensu bo z translacj�, ale przy mno�eniu przez (float3x3)World to by mia�o sens,
			// bo oznacza�oby �e Env Mapa jest w przestrzeni �wiata i nie obraca si� razem z modelem.
			// Tym nie mniej dla optymalizacji daruj� sobie to tutaj.
			Out.ReflectedEnvDir = Reflected;
		}
		#endif
		
		#if (FRESNEL_TERM == 1)
			Out.Fresnel = CalcFresnel(InputPos, InputNormal);
		#endif
	}
	#endif

	#if (PASS == 0) // Base
	{
		// Nic dodatkowego.
	}
	#elif (PASS == 1) // Fog
	{
		Out.Fog = FogColor;
		Out.Fog.a = FogFactors.x * Out.Pos.z + FogFactors.y;
	}
	#elif (PASS == 2) // WireFrame
	{
		#if (COLOR_MODE == 0) // Material
			Out.Color = DiffuseColor;
		#elif (COLOR_MODE == 1) // Entity
			Out.Color = TeamColor;
		#else // (COLOR_MODE == 2) // Modulate
			Out.Color = DiffuseColor * TeamColor;
		#endif
		
		#if (USE_FOG == 1)
		{
			float FogIntensity = FogFactors.x * Out.Pos.z + FogFactors.y;
			Out.Color.a *= min(1, 1 - FogIntensity);
		}
		#endif
	}
	#elif (PASS == 3) // Translucent
	{
		#if (USE_FOG == 1)
		{
			// Fog
			float FogIntensity = FogFactors.x * Out.Pos.z + FogFactors.y;
			Out.Fog = float4(1, 1, 1, min(1, 1 - FogIntensity));
		}
		#endif
	}
	#elif (PASS == 4 || PASS == 5 || PASS == 6) // �wiat�a
	{
		// Wsp. tekstury
		#if (TERRAIN == 1)
		{
		}
		#else
		{
			#if (TEXTURE_ANIMATION == 1)
				Out.Tex = mul(float4(In.Tex, 0, 1), (float4x2)TextureMatrix);
			#else
				Out.Tex = In.Tex;
			#endif
		}
		#endif
		
		// Shadow mapping (nak�adanie shadow mapy)
		#if (SHADOW_MAP_MODE != 0)
			#if (PASS == 5)
				// Tu w ShadowMapMatrix jest macierz �wiata
				Out.VecFromLight_World = mul(InputPos - (float3)LightPos, (float3x3)ShadowMapMatrix);
			#else
				Out.ShadowMapPos = mul(float4(InputPos, 1), ShadowMapMatrix);
			#endif

			#if (VARIABLE_SHADOW_FACTOR == 1)
				Out.ShadowFactor = Out.Pos.z * ShadowFactorA + ShadowFactorB;
			#endif
		#endif

		#if (PER_PIXEL == 1)
		{
			// Ta macierz przekszta�ca wektory ze wsp. lokalnych modelu do przestrzeni stycznej.
			float3x3 TangentSpace;
			TangentSpace[0] = InputTangent;
			TangentSpace[1] = InputBinormal;
			TangentSpace[2] = InputNormal;

			// Directional - przeka� kierunek padania �wiat�a w tangent space
			#if (PASS == 4)
			{
				// Zanegowany kierunek �wiecenia �wiat�a, w tangent space
				Out.DirToLight = mul(TangentSpace, DirToLight);
			}
			#endif
			// Point, spot - przeka� wektor do pozycji �wiat�a w tangent space
			#if (PASS == 5 || PASS == 6)
			{
				float3 VecToLight = LightPos - InputPos;
				Out.VecToLight = mul(TangentSpace, VecToLight);
				
				// Spot - przeka� wektor do �wiat�a w Model Space
				#if (PASS == 6)
					Out.VecToLight_Model = VecToLight;
				#endif
			}
			#endif
			
			// DirToCam
			#if (SPECULAR != 0)
			{
				half3 VecToCam = (float3)CameraPos - InputPos;
				Out.VecToCam = mul(TangentSpace, VecToCam);
			}
			#endif
		}
		// Per Vertex
		#else
		{
			// Policz te dwie zmienne zale�nie od typu �wiat�a
			half3 MyDirToLight;
			half Attn;
			// - Directional
			#if (PASS == 4)
			{
				MyDirToLight = DirToLight;
				Attn = 1;
			}
			// - Point, spot
			#elif (PASS == 5 || PASS == 6)
			{
				float3 VecToLight = (float3)LightPos - InputPos;
				MyDirToLight = normalize(VecToLight);
				
				Attn = 1 - min(1, dot(VecToLight, VecToLight) / LightRangeSq);
				
				// Spot
				#if (PASS == 6)
				{
					half LightDirDot = max(0, dot(DirToLight, MyDirToLight));
					// Ostry
					#if (SPOT_SMOOTH == 0)
						Attn *= step(LightCosFov2, LightDirDot);
					// G�adki
					#else
						Attn *= max(0, (LightDirDot - LightCosFov2) * LightCosFov2Factor);
					#endif
				}
				#endif
			}
			#endif
			
			// Czy tu nie trzeba saturate? Skoro dzia�a, to widocznie nie...
			half N_dot_L = dot(InputNormal, MyDirToLight);
			#if (HALF_LAMBERT == 1)
				N_dot_L = N_dot_L * 0.5 + 0.5;
			#endif
			half Diffuse = N_dot_L * Attn;
			Out.Diffuse = Diffuse * LightColor;
			
			#if (SPECULAR != 0) // a raczej == 1, bo aniso przy per vertex jest zabronione
			{
				half3 MyDirToCam = normalize(CameraPos - InputPos);
				half3 Half = normalize(MyDirToLight + MyDirToCam);
				// Czy tu nie trzeba saturate? Skoro dzia�a, to widocznie nie...
				half N_dot_H = dot(InputNormal, Half);
				half Specular = pow(N_dot_H, SpecularPower);
				// Tu by� max() na (Specular*LightColor), ale chyba niepotrzebny
				Out.Specular = Specular * LightColor * Diffuse * SpecularColor;
			}
			
			#endif
		}
		#endif
	}
	// Wype�nienie Shadow Mapy
	#elif (PASS == 7)
	{
		#if (POINT_SHADOW_MAP == 1)
			Out.VecToLight = InputPos - LightPos;
		#else
			Out.ShadowMapZW = Out.Pos.zw;
		#endif

		#if (ALPHA_TESTING == 1)
		{
			// Wsp. tekstury
			#if (TEXTURE_ANIMATION == 1)
				Out.Tex = mul(float4(In.Tex, 0, 1), (float4x2)TextureMatrix);
			#else
				Out.Tex = In.Tex;
			#endif
		}
		#endif
	}
	#endif
}

// Przetwarza kolor wg ustawie� Team Color
// Alfa zwraca niezmieniony
half4 ProcessTeamColor(half4 DiffuseColor)
{
	#if (COLOR_MODE == 1)
	{
		half4 R = DiffuseColor * TeamColor;
		R.a = DiffuseColor.a;
		return R;
	}
	#elif (COLOR_MODE == 2)
	{
		half4 R = lerp(DiffuseColor, TeamColor, DiffuseColor.a);
		R.a = DiffuseColor.a;
		return R;
	}
	#else // COLOR_MODE == 0
		return DiffuseColor;
	#endif
}

// Przetwarza Alpha stosownie do COLOR_MODE i uwzgl�dniaj�c zmienn� TeamColor
// Sk�adowe RGB pozostawia bez zmian.
half4 ProcessAlphaMode(half4 Color)
{
	#if (ALPHA_MODE == 0)
		return Color;
	#elif (ALPHA_MODE == 1)
		return half4(Color.rgb, TeamColor.a);
	#else // (ALPHA_MODE == 2)
		return half4(Color.rgb, Color.a * TeamColor.a);
	#endif
}

#if ((PASS==4 || PASS==5 || PASS==6) && SHADOW_MAP_MODE != 0)
half SampleShadowMap(VS_OUTPUT In)
{
	const float4 RGBA_Factors = float4(1/256/256, 1/256, 1, 0);

	// Cube Map
	#if (PASS == 5)
	{
		// Tu jest m�j autorski, genialny algorytm filtrowania Cube Shadow Mapy
		float3 VecFromLight = In.VecFromLight_World;
		float DistFromLight = dot(VecFromLight, VecFromLight);
		float3 ShadowTexC = VecFromLight;
		float4 samples;
		ShadowTexC = normalize(ShadowTexC);
		float3 lerps = frac(ShadowTexC * ShadowMapSize);
		#if (SHADOW_MAP_MODE == 1)
			// Stare, jedna pr�bka
			//float sample = texCUBE(ShadowMapSampler, ShadowTexC);
			//return (DistFromLight / LightRangeSq_World) < (sample + ShadowEpsilon);
			samples.x = texCUBE(ShadowMapSampler, ShadowTexC + float3(0, 0, 0));
			samples.y = texCUBE(ShadowMapSampler, ShadowTexC + float3(ShadowMapSizeRcp, ShadowMapSizeRcp, 0));
			samples.z = texCUBE(ShadowMapSampler, ShadowTexC + float3(ShadowMapSizeRcp, 0, ShadowMapSizeRcp));
			samples.w = texCUBE(ShadowMapSampler, ShadowTexC + float3(0, ShadowMapSizeRcp, ShadowMapSizeRcp));
		#elif (SHADOW_MAP_MODE == 2)
			samples.x = dot(RGBA_Factors, texCUBE(ShadowMapSampler, ShadowTexC + float3(0, 0, 0)));
			samples.y = dot(RGBA_Factors, texCUBE(ShadowMapSampler, ShadowTexC + float3(ShadowMapSizeRcp, ShadowMapSizeRcp, 0)));
			samples.z = dot(RGBA_Factors, texCUBE(ShadowMapSampler, ShadowTexC + float3(ShadowMapSizeRcp, 0, ShadowMapSizeRcp)));
			samples.w = dot(RGBA_Factors, texCUBE(ShadowMapSampler, ShadowTexC + float3(0, ShadowMapSizeRcp, ShadowMapSizeRcp)));
		#endif
		half4 compared_samples = (DistFromLight / LightRangeSq_World) < (samples + ShadowEpsilon);
		return (compared_samples.x + compared_samples.y + compared_samples.z + compared_samples.w) / 4;
	}
	#else
	{
		// To saturate tutaj naprawia b��d na kraw�dzi z obszarem drugiej strony �wiat�a, kt�rego szuka�em 2 dni!!!
		// (rozwi�zanie znalezione metod� Voodoo programming :)
		float4 ShadowMapPos = saturate(In.ShadowMapPos / In.ShadowMapPos.w);
		float2 texelpos = ShadowMapPos.xy * ShadowMapSize;
		float2 lerps = frac(texelpos);
		
		float4 samples;
	
		// Wykorzystana sk�adowa R
		#if (SHADOW_MAP_MODE == 1)
			samples.x = tex2D(ShadowMapSampler, ShadowMapPos.xy).r;
			samples.y = tex2D(ShadowMapSampler, ShadowMapPos.xy + half2(ShadowMapSizeRcp, 0               )).r;
			samples.z = tex2D(ShadowMapSampler, ShadowMapPos.xy + half2(0,                ShadowMapSizeRcp)).r;
			samples.w = tex2D(ShadowMapSampler, ShadowMapPos.xy + half2(ShadowMapSizeRcp, ShadowMapSizeRcp)).r;
		// Rozpakowanie ze sk�adowych RGB
		#elif (SHADOW_MAP_MODE == 2)
			samples.x = dot(RGBA_Factors, tex2D(ShadowMapSampler, ShadowMapPos.xy));
			samples.y = dot(RGBA_Factors, tex2D(ShadowMapSampler, ShadowMapPos.xy + half2(ShadowMapSizeRcp, 0               )));
			samples.z = dot(RGBA_Factors, tex2D(ShadowMapSampler, ShadowMapPos.xy + half2(0,                ShadowMapSizeRcp)));
			samples.w = dot(RGBA_Factors, tex2D(ShadowMapSampler, ShadowMapPos.xy + half2(ShadowMapSizeRcp, ShadowMapSizeRcp)));
		#endif
	
		float4 compared_samples = (ShadowMapPos.zzzz <= samples);
		
		return lerp(
			lerp(compared_samples.x, compared_samples.y, lerps.x),
			lerp(compared_samples.z, compared_samples.w, lerps.x),
			lerps.y).r;
	}
	#endif
}
#endif

// Wi�ksza dok�adno�� bo do rendering shadow mapy
#if (PASS == 7)
	void MainPS(VS_OUTPUT In, out float4 Out : COLOR0)
#else
	void MainPS(VS_OUTPUT In, out half4 Out : COLOR0)
#endif
{
	#if (PASS == 0) // Base
	{
		float4 MyDiffuseColor;
		// O�wietlenie Ambient (lub pe�ne je�li brak o�wietlenia)
		#if (AMBIENT_MODE == 0)
		{
			MyDiffuseColor = half4(0, 0, 0, 1);
			#if (ALPHA_TESTING == 1)
				MyDiffuseColor.a = tex2D(DiffuseSampler, In.Tex).a;
			#endif
			Out = MyDiffuseColor;
		}
		#else
		{
			#if (TERRAIN == 1)
			{
				MyDiffuseColor = tex2D(TerrainSampler0, In.TerrainTex01.xy);
				MyDiffuseColor = lerp(MyDiffuseColor, tex2D(TerrainSampler1, In.TerrainTex01.zw), In.TerrainTextureWeights.a);
				MyDiffuseColor = lerp(MyDiffuseColor, tex2D(TerrainSampler2, In.TerrainTex23.xy), In.TerrainTextureWeights.r);
				MyDiffuseColor = lerp(MyDiffuseColor, tex2D(TerrainSampler3, In.TerrainTex23.zw), In.TerrainTextureWeights.g);
				Out = MyDiffuseColor;
			}
			#else
			{
				#if (DIFFUSE_TEXTURE == 1)
					MyDiffuseColor = tex2D(DiffuseSampler, In.Tex);
				#else
					MyDiffuseColor = DiffuseColor;
				#endif
				
				Out = ProcessTeamColor(MyDiffuseColor);
			}
			#endif
			
			#if (AMBIENT_MODE == 2)
				Out *= AmbientColor;
			#endif
		}
		#endif
		
		// Dodaj Emissive
		#if (EMISSIVE == 1)
			Out += tex2D(EmissiveSampler, In.Tex);
		#endif
		
		// Dodaj Environmental Mapping
		#if (ENVIRONMENTAL_MAPPING == 1)
		{
			float4 EnvSample = texCUBE(EnvironmentalSampler, In.ReflectedEnvDir);
			Out += EnvSample * EnvSample.a;
		}
		#endif
		
		// Dodaj Fresnel Term
		#if (FRESNEL_TERM == 1)
			Out += In.Fresnel;
		#endif
		
		// Je�li u�ywamy Alpha-Testing, przepisz oryginaln� warto�� Alpha z Diffuse
		#if ALPHA_TESTING
			Out.a = MyDiffuseColor.a;
		#endif
	}
	#elif (PASS == 1) // Fog
	{
		Out = In.Fog;
	}
	#elif (PASS == 2) // WireFrame
	{
		Out = In.Color;
	}
	#elif (PASS == 3) // Translucent
	{
		float4 MyDiffuseColor;
		#if (DIFFUSE_TEXTURE == 1)
			MyDiffuseColor = tex2D(DiffuseSampler, In.Tex);
		#else
			MyDiffuseColor = DiffuseColor;
		#endif
		
		Out = ProcessTeamColor(MyDiffuseColor);
		Out = ProcessAlphaMode(Out);
		
		// Dodaj Emissive (��cznie z Alpha)
		#if (EMISSIVE == 1)
		{
			float4 EmissiveSample = tex2D(EmissiveSampler, In.Tex);
			Out += EmissiveSample * EmissiveSample.a;
		}
		#endif
		
		// Dodaj Environmental Mapping (��cznie z Alpha)
		#if (ENVIRONMENTAL_MAPPING == 1)
		{
			float4 EnvSample = texCUBE(EnvironmentalSampler, In.ReflectedEnvDir);
			Out += EnvSample * EnvSample.a;
		}
		#endif
		
		// Dodaj Fresnel Term (��cznie z Alpha)
		#if (FRESNEL_TERM == 1)
			Out += In.Fresnel * In.Fresnel.a;
		#endif
		
		// Mg�a
		#if (USE_FOG)
			Out.a *= In.Fog.a;
		#endif
	}
	#elif (PASS == 4 || PASS == 5 || PASS == 6) // O�wietlenie
	{
		float4 DiffuseTextureColor;
		#if (TERRAIN == 1)
		{
			DiffuseTextureColor = tex2D(TerrainSampler0, In.TerrainTex01.xy);
			DiffuseTextureColor = lerp(DiffuseTextureColor, tex2D(TerrainSampler1, In.TerrainTex01.zw), In.TerrainTextureWeights.a);
			DiffuseTextureColor = lerp(DiffuseTextureColor, tex2D(TerrainSampler2, In.TerrainTex23.xy), In.TerrainTextureWeights.r);
			DiffuseTextureColor = lerp(DiffuseTextureColor, tex2D(TerrainSampler3, In.TerrainTex23.zw), In.TerrainTextureWeights.g);
		}
		#else
		{
			#if (DIFFUSE_TEXTURE == 1)
				DiffuseTextureColor = tex2D(DiffuseSampler, In.Tex);
			#else
				DiffuseTextureColor = DiffuseColor;
			#endif
			DiffuseTextureColor = ProcessTeamColor(DiffuseTextureColor);
		}
		#endif

		// Per pixel
		#if (PER_PIXEL == 1)
		{
			// Pobierz wektor normalny
			half3 Normal;
			#if (NORMAL_TEXTURE == 1)
				Normal = tex2D(NormalSampler, In.Tex)*2-1; // (x-0.5)*2 teoretycznie lepsze, w praktyce 1 instrukcja wiecej niz (x)*2-1
			#else
				Normal = half3(0, 0, 1);
			#endif
			// Ta normalizacja praktycznie nic nie zmienia, ale Krzysiek K twierdzi �e zmienia bo inaczej specular zanika przy mip mappingu.
			Normal = normalize(Normal);
			
			// Policz te dwie zmienne zale�nie od typu �wiat�a
			half3 MyDirToLight;
			half Attn;
			// - Directional
			#if (PASS == 4)
			{
				MyDirToLight = In.DirToLight;
				Attn = 1;
			}
			// - Point, spot
			#elif (PASS == 5 || PASS == 6)
			{
				half3 VecToLight = In.VecToLight;
				MyDirToLight = normalize(VecToLight);
				Attn = 1 - saturate(dot(VecToLight, VecToLight) / LightRangeSq);

				// Spot
				#if (PASS == 6)
				{
					// Wersja 1 - ta dzia�a (model space)
					// Tu by� max(0, ) z ca�o�ci, ale wygl�da na to, �e niepotrzebny
					half LightDirDot = dot(DirToLight, normalize(In.VecToLight_Model));
					// Wersja 2 - ta jest fajniejsza, ale nie dzia�a (tangent space)
					// Tzn nie dzia�a�a do czasu a� zorientowa��m si�, �e ten wektor DirToLight,
					// mimo �e ju� do shadera przesy�any jako znormalizowany, trzeba po przekszta�ceniu
					// do Tangent Space tu w PS normalizowa� - wtedy dzia�a.
					// Ale jednak nie! Wtedy jest lepiej, ale nadal �le dzia�a.
					//half LightDirDot = max(0, dot(MyDirToLight, normalize(In.DirToLight)));
					
					// Ostry
					#if (SPOT_SMOOTH == 0)
						Attn *= step(LightCosFov2, LightDirDot);
					// G�adki
					#else
						// Tu by mo�na zomptymalizowa�, przesy�a� do shadera (1 - LightCosFov2) jako osobn� sta�� - jaki� LightCosFov2_rcp (albo nawet 1/(1 - LightCosFov2))
						Attn *= saturate((LightDirDot - LightCosFov2) * LightCosFov2Factor);
					#endif
				}
				#endif
			}
			#endif
			
			// Czy tu nie trzeba saturate? Skoro jest, to pewnie znaczy �e tak.
			half N_dot_L = dot(Normal, MyDirToLight);
			#if (HALF_LAMBERT == 1)
				N_dot_L = N_dot_L * 0.5 + 0.5;
			#endif
			half Diffuse = N_dot_L * Attn;
			// Zabezpieczenie �eby to co jest ty�em do �wiat�a na pewno nie �wieci�o
			#if (NORMAL_TEXTURE == 1 && HALF_LAMBERT == 0)
				Diffuse *= (dot(half3(0,0,1), MyDirToLight) >= 0);
			#endif
			half4 OutDiffuse = Diffuse * LightColor;
			
			half4 OutSpecular;
			#if (SPECULAR != 0)
			{
				float3 DirToCam = normalize(In.VecToCam);
				half N_dot_H;
				// Zwyk�y Specular
				#if (SPECULAR == 1)
				{
					half3 Half = normalize(MyDirToLight + DirToCam);
					// Czy tu nie trzeba saturate? Skoro jest, to pewnie znaczy �e tak.
					N_dot_H = dot(Normal, Half);
				}
				// Anisotropic lighting
				// Skomplikowana funkcja dw�ch zmiennych :(
				// Zakodowanie jej do tekstury mog�oby tu pom�c, ale z kolei ma�a dok�adno�� tekstury owocuje tym, �e po podniesieniu wyniku do pot�gi wychodz� schodki.
				// Z kolei zakodowana razem z pot�g� wymaga�aby osobnych tekstur dla r�nych wyk�adnik�w.
				#elif (SPECULAR == 2)
				{
					// Oryginalne
					//half LdotT = dot(MyDirToLight, float3(1, 0, 0));
					//half VdotT = dot(In.DirToCam, float3(1, 0, 0));
					// Uproszczone
					half LdotT = MyDirToLight.x;
					half VdotT = DirToCam.x;

					N_dot_H = sqrt(1 - LdotT*LdotT) * sqrt(1 - VdotT*VdotT) - LdotT*VdotT;
				}
				#else // (SPECULAR == 3)
				{
					// Oryginalne
					//half LdotT = dot(MyDirToLight, half3(0, 1, 0));
					//half VdotT = dot(In.DirToCam, half3(0, 1, 0));
					half LdotT = MyDirToLight.y;
					half VdotT = DirToCam.y;
					
					N_dot_H = sqrt(1 - LdotT*LdotT) * sqrt(1 - VdotT*VdotT) - LdotT*VdotT;
				}
				#endif
				// Tu nie trzeba saturate? Skoro dzia�a bez niego, to pewnie znaczy �e nie.
				half Specular = pow(N_dot_H, SpecularPower);
				// Tu by� max(0, ) ale najwidoczniej jest niepotrzebny.
				OutSpecular = Specular * LightColor * Diffuse * SpecularColor;
			}
			#endif
			
			Out = DiffuseTextureColor * OutDiffuse;
			#if (SPECULAR != 0)
			{
				#if (GLOSS_MAP == 1)
					Out += OutSpecular * DiffuseTextureColor.a;
				#else
					Out += OutSpecular;
				#endif
			}
			#endif
		}
		// Per vertex
		#else
		{
			Out = DiffuseTextureColor * In.Diffuse;
			#if (SPECULAR != 0)
			{
				#if (GLOSS_MAP == 1)
					Out += In.Specular * DiffuseTextureColor.a;
				#else
					Out += In.Specular;
				#endif
			}
			#endif
		}
		#endif

		// Shadow mapping
		#if (SHADOW_MAP_MODE != 0)
			float MyShadowFactor;
			#if (VARIABLE_SHADOW_FACTOR == 1)
				MyShadowFactor = lerp(ShadowFactor, 1, saturate(In.ShadowFactor));
			#else
				MyShadowFactor = ShadowFactor;
			#endif
			Out *= lerp(MyShadowFactor, 1, SampleShadowMap(In));
		#endif
	}
	// Render do Shadow Mapy
	#elif (PASS == 7)
	{
		#if (POINT_SHADOW_MAP == 1)
			float v = dot(In.VecToLight, In.VecToLight) / LightRangeSq;
		#else
			float v = In.ShadowMapZW.x / In.ShadowMapZW.y; // Tak naprawd� to WorldPos.z / WorldPos.w
		#endif
		// Wykorzystaj sk�adow� R
		#if (SHADOW_MAP_MODE == 1)
			Out = float4(v, v, v, v);
		// Spakuj do sk�adowych RGB
		#elif (SHADOW_MAP_MODE == 2)
			Out = v * float4( 256*256, 256, 1, 0 );
			Out = frac(Out);
			// W topicu forum (http://www.gamedev.net/community/forums/topic.asp?topic_id=442138 i http://www.codesampler.com/phpBB2/viewtopic.php?t=995)
			// pisz� �e to jest potrzebne, chocia� ayufan m�wi �e shader przycina a nie zaokr�gla.
			// Eksperymentalnie stwierdzi�em, �e lepiej bez tego - jak to by�o to powstawa�y czasem artefakty.
			//Out -= Out.rrgb * float4( 0, 1.0/256.0, 1.0/256.0, 0 );
		#endif
		
		#if (ALPHA_TESTING == 1 && SHADOW_MAP_MODE == 1)
		{
			float4 DiffuseTextureColor;
			#if (DIFFUSE_TEXTURE == 1)
				DiffuseTextureColor = tex2D(DiffuseSampler, In.Tex);
			#else
				DiffuseTextureColor = DiffuseColor;
			#endif
			
			Out.a = DiffuseTextureColor.a;
		}
		#endif
	}
	#endif
}

technique
{
	pass
	{
		VertexShader = compile vs_2_0 MainVS();
		PixelShader = compile ps_2_0 MainPS();
	}
}

