texture Texture;
float4x4 WorldViewProj;
float3 RightVec;
float3 UpVec;
float3 Movement1;
float3 Movement2;
half4 MainColor;
float3 Translation;

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
	// Znaczenie pól: patrz Fall.cpp.
	half4 Diffuse : COLOR0;
	half2 Tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Pos : POSITION;
	half2 Tex : TEXCOORD0;
};

void GrassVS(VS_INPUT In, out VS_OUTPUT Out)
{
	float2 QuadBias = In.Diffuse.ar * 2 - 1;
	float2 Randoms = In.Diffuse.gb;
	
	float3 WorldPos = frac(In.Pos + Translation + lerp(Movement1, Movement2, Randoms.x)) * 2 - 1;
	WorldPos += QuadBias.x * RightVec + QuadBias.y * UpVec;
	Out.Pos = mul(float4(WorldPos, 1), WorldViewProj);
	
	Out.Tex = In.Tex;
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
