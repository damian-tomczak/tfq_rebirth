texture Texture;
float4x4 ViewProj;
float3 RightDir;
float3 UpDir;
float2 WindFactors;
float4 Color;
float2 FadeFactors; // Wspó³czynniki A i B równania liniowego dla zanikania
texture NoiseTexture;

sampler Sampler = sampler_state
{
    Texture = (Texture);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

sampler NoiseSampler = sampler_state
{
    Texture = (NoiseTexture);
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
	float2 Tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
	float3 NoiseTex : TEXCOORD1; // Sk³adowa z to wartoœæ g³êbokoœci Z z pozycji
	float2 Pos_zw : TEXCOORD2;
};

void GrassVS(VS_INPUT In, out VS_OUTPUT Out)
{
	float2 QuadBias = In.Diffuse.rg * 4 - 2;
	float2 Randoms = In.Diffuse.ba * 2 - 1;
	float Wind = dot(WindFactors, Randoms);
	float3 WorldPos = In.Pos + (QuadBias.x + Wind) * RightDir + QuadBias.y * UpDir;
	Out.Pos = mul(float4(WorldPos, 1), ViewProj);
	
	Out.Tex = In.Tex;
	
	Out.NoiseTex.xy = float2(WorldPos.x + WorldPos.z, WorldPos.y);
	Out.NoiseTex.z = Out.Pos.z;
}

void GrassPS(VS_OUTPUT In, out half4 Out : COLOR0)
{
	Out = tex2D(Sampler, In.Tex) * Color;

	float Pos_z = In.NoiseTex.z;
	float MyFadeFactor = Pos_z * FadeFactors.x + FadeFactors.y;
	float Fade = tex2D(NoiseSampler, In.NoiseTex) + MyFadeFactor;
	Out.a *= saturate(Fade);
}

technique
{
	pass
	{
		VertexShader = compile vs_2_0 GrassVS();
		PixelShader = compile ps_2_0 GrassPS();
	}
}
