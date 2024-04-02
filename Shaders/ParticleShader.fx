float4x4 WorldViewProj;
float3 RightDir; // W przestrzeni modelu
float3 UpDir; // W przestrzeni modelu
texture Texture;
float4 Data[25];


sampler TextureSampler = sampler_state
{
	Texture = (Texture);
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = BORDER;
	AddressV = BORDER;
	BorderColor = 0x00000000;
};

struct VS_INPUT
{
	float3 Pos : POSITION;
	float3 VertexFactors : COLOR0;
	float2 ParticleFactor : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Pos : POSITION;
	float4 Color : COLOR0;
	float2 Tex : TEXCOORD0;
};

void VS(VS_INPUT In, out VS_OUTPUT Out)
{
	// Parametry tej cz¹steczki
	
	float ParticleP = In.ParticleFactor.x; // Wspó³czynnik liniowy
	float3 ParticleR = frac(In.ParticleFactor.yyy * float3(324.2135, 231.3542, 147.4534));
	
	// Wylicz parametr g³ówny
	
	float TimePeriod = Data[21].w + Data[22].w * ParticleP + Data[23].w * ParticleR;
	float TimePhase = Data[24].x + Data[24].y * ParticleP + Data[24].z * ParticleR;
	float t = frac(Data[24].w / TimePeriod + TimePhase);
	
	// Wylicz podstawowe parametry
	
	float3 PosA2 = (float3)Data[ 9] + (float3)Data[10] * ParticleP + (float3)Data[11] * ParticleR.xyz;
	float3 PosA1 = (float3)Data[12] + (float3)Data[13] * ParticleP + (float3)Data[14] * ParticleR.zyx;
	float3 PosB2 = (float3)Data[15] + (float3)Data[16] * ParticleP + (float3)Data[17] * ParticleR.y;
	float3 PosB1 = (float3)Data[18] + (float3)Data[19] * ParticleP + (float3)Data[20] * ParticleR.z;
	float3 PosB0 = (float3)Data[21] + (float3)Data[22] * ParticleP + (float3)Data[23] * ParticleR.zyx;
	float4 ColorA2 = Data[ 0] + Data[ 1] * ParticleP + Data[ 2] * ParticleR.xyzz;
	float4 ColorA1 = Data[ 3] + Data[ 4] * ParticleP + Data[ 5] * ParticleR.yzxy;
	float4 ColorA0 = Data[ 6] + Data[ 7] * ParticleP + Data[ 8] * ParticleR.zxyx;
	float OrientationA1 = Data[ 9].w + Data[10].w * ParticleP + Data[11].w * ParticleR.x;
	float OrientationA0 = Data[12].w + Data[13].w * ParticleP + Data[14].w * ParticleR.y;
	float SizeA1 = Data[15].w + Data[16].w * ParticleP + Data[17].w * ParticleR.z;
	float SizeA0 = Data[18].w + Data[19].w * ParticleP + Data[20].w * ParticleR.x;

	float3 Pos = In.Pos + t * (PosA1 + t * PosA2) + PosB2 * sin(PosB1 * t + PosB0);
	float4 Color = ColorA0 + t * (ColorA1 + t * ColorA2);
	float Orientation = OrientationA1 * t + OrientationA0;
	float Size = SizeA1 * t + SizeA0;
	
	// Wylicz dalsze dane
	
	float HalfSizeDiagonal = Size * (1.41421356237309504880 / 2);
	// Mapowanie 0 .. 1 na 1/4*PI .. 7/4*PI
	// Wzór który to robi: y = 3/2*PI * x + 1/4*PI
	float Angle = In.VertexFactors.r * 4.71238898 + 0.785398163 + Orientation;
	
	// Wylicz koñcowe dane
	
	Pos = Pos + (RightDir * cos(Angle) + UpDir * sin(Angle)) * HalfSizeDiagonal;
	Out.Pos = mul(float4(Pos, 1), WorldViewProj);
	Out.Color = Color;
	Out.Tex = float2(In.VertexFactors.g, In.VertexFactors.b);
}

void PS(VS_OUTPUT In, out float4 Out : COLOR0)
{
	Out = In.Color * tex2D(TextureSampler, In.Tex);
}

technique
{
	pass
	{
		VertexShader = compile vs_2_0 VS();
		PixelShader = compile ps_2_0 PS();
	}
}
