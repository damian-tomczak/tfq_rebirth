/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\D3dUtils.hpp"
#include "..\Framework\Res_d3d.hpp"
#include "Engine.hpp"
#include "Camera.hpp"
#include "Sky.hpp"

namespace engine
{


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje globalne

// Parsuje z podanego tokenizera wektor w postaci: x, y, z
// Sk³adniki s¹ typu float.
void ParseVec3(VEC3 *Out, Tokenizer &t)
{
	Out->x = t.MustGetFloat();
	t.Next();

	t.AssertSymbol(',');
	t.Next();

	Out->y = t.MustGetFloat();
	t.Next();

	t.AssertSymbol(',');
	t.Next();

	Out->z = t.MustGetFloat();
	t.Next();
}

// Parsuje podany kolor jako string z nazw¹ typu "red" albo wartoœæ liczbowa typu 0xFFFF80A0.
void ParseColor(COLORF *Out, Tokenizer &t)
{
	if (t.QueryToken(Tokenizer::TOKEN_STRING))
	{
		if (!StrToColor(Out, t.GetString()))
			t.CreateError("B³êdny kolor.");
	}
	else
		ColorToColorf(Out, COLOR(t.MustGetUint4()));
	t.Next();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa SolidSky

class SolidSky_pimpl
{
public:
	COLOR m_Color;
};


SolidSky::SolidSky() :
	pimpl(new SolidSky_pimpl)
{
	pimpl->m_Color = COLOR(0xDEADC0DE);
}

SolidSky::SolidSky(COLOR Color) :
	pimpl(new SolidSky_pimpl)
{
	pimpl->m_Color = Color;
}

SolidSky::~SolidSky()
{
	pimpl.reset();
}

COLOR SolidSky::GetColor()
{
	return pimpl->m_Color;
}

void SolidSky::SetColor(COLOR Color)
{
	pimpl->m_Color = Color;
}

void SolidSky::Draw(const ParamsCamera &Cam)
{
	frame::Dev->Clear(0, NULL, D3DCLEAR_TARGET, pimpl->m_Color.ARGB, 1.0f, 0u);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa SkyboxSky

class SkyboxSky_pimpl
{
public:
	string m_SideTextureNames[6];
	// WskaŸniki pobierane z nazwy przy pierwszym u¿yciu
	res::D3dTexture *m_SideTextures[6];
	COLOR m_Color;
	bool m_SunEnabled;
	VEC3 m_SunDir;
};


SkyboxSky::SkyboxSky() :
	pimpl(new SkyboxSky_pimpl)
{
	pimpl->m_Color = COLOR::WHITE;
	pimpl->m_SunEnabled = false;
	pimpl->m_SunDir = VEC3::POSITIVE_Y;

	for (int i = 0; i < 6; i++)
		pimpl->m_SideTextures[i] = NULL;
}

SkyboxSky::SkyboxSky(const string *TextureNames) :
	pimpl(new SkyboxSky_pimpl)
{
	pimpl->m_Color = COLOR::WHITE;
	pimpl->m_SunEnabled = false;
	pimpl->m_SunDir = VEC3::POSITIVE_Y;

	for (int i = 0; i < 6; i++)
	{
		pimpl->m_SideTextureNames[i] = TextureNames[i];
		pimpl->m_SideTextures[i] = NULL;
	}
}

SkyboxSky::~SkyboxSky()
{
	pimpl.reset();
}

string SkyboxSky::GetSideTextureName(SIDE Side)
{
	return pimpl->m_SideTextureNames[(int)Side];
}

void SkyboxSky::SetSideTextureName(SIDE Side, const string &TextureName)
{
	pimpl->m_SideTextureNames[(int)Side] = TextureName;
	pimpl->m_SideTextures[(int)Side] = NULL;
}

void SkyboxSky::SetSideTextureNames(const string *TextureNames)
{
	for (int i = 0; i < 6; i++)
	{
		pimpl->m_SideTextureNames[i] = TextureNames[i];
		pimpl->m_SideTextures[i] = NULL;
	}
}

COLOR SkyboxSky::GetColor()
{
	return pimpl->m_Color;
}

void SkyboxSky::SetColor(COLOR Color)
{
	pimpl->m_Color = Color;
}

bool SkyboxSky::GetSunEnabled()
{
	return pimpl->m_SunEnabled;
}

void SkyboxSky::SetSunEnabled(bool SunEnabled)
{
	pimpl->m_SunEnabled = SunEnabled;
}

const VEC3 & SkyboxSky::GetSunDir()
{
	return pimpl->m_SunDir;
}

void SkyboxSky::SetSunDir(const VEC3 &SunDir)
{
	pimpl->m_SunDir = SunDir;
}

bool SkyboxSky::GetSunDir(VEC3 *OutSunDir)
{
	if (pimpl->m_SunEnabled)
	{
		*OutSunDir = pimpl->m_SunDir;
		return true;
	}
	else
		return false;
}

void SkyboxSky::Draw(const ParamsCamera &Cam)
{
	bool AnyTexture = false;
	for (int i = 0; i < 6; i++)
	{
		if (!pimpl->m_SideTextureNames[i].empty())
		{
			if (pimpl->m_SideTextures[i] == NULL)
				pimpl->m_SideTextures[i] = res::g_Manager->MustGetResourceEx<res::D3dTexture>(pimpl->m_SideTextureNames[i]);
			pimpl->m_SideTextures[i]->Load();
			AnyTexture = true;
		}
	}

	if (!AnyTexture) return;

	using frame::Dev;

	//frame::RestoreDefaultState();
	res::D3dEffect *Effect = res::g_Manager->MustGetResourceEx<res::D3dEffect>("SkyboxEffect");
	Effect->Load();

	D3DXHANDLE TextureParam = Effect->Get()->GetParameterByName(NULL, "Texture");

	MATRIX World;
	Translation(&World, Cam.GetEyePos());

	Effect->Get()->SetVector("Color", &D3DXVECTOR4(pimpl->m_Color.R, pimpl->m_Color.G, pimpl->m_Color.B, pimpl->m_Color.A));
	Effect->Get()->SetMatrix("WorldViewProj", &math_cast<D3DXMATRIX>(World*Cam.GetMatrices().GetViewProj()));

	UINT Foo;
	Effect->Get()->Begin(&Foo, D3DXFX_DONOTSAVESTATE);
	Effect->Get()->BeginPass(0);

/*	Dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	Dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	Dev->SetRenderState(D3DRS_TEXTUREFACTOR, m_Color.ARGB);

	Dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	Dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	Dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	Dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	Dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	Dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	Dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	Dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
	Dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	Dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	Dev->SetRenderState(D3DRS_TEXTUREFACTOR, m_Color.ARGB);*/

	Dev->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0));

	struct SKYBOX_VERTEX
	{
		VEC3 Pos;
		VEC2 Tex;

		SKYBOX_VERTEX() { }
		SKYBOX_VERTEX(const VEC3 &Pos, const VEC2 &Tex) : Pos(Pos), Tex(Tex) { }
	};

	SKYBOX_VERTEX v[4];

	if (pimpl->m_SideTextures[SIDE_NEAR])
	{
		//Dev->SetTexture(0, pimpl->m_SideTextures[SIDE_NEAR]->GetTexture());
		Effect->Get()->SetTexture(TextureParam, pimpl->m_SideTextures[SIDE_NEAR]->GetTexture());
		Effect->Get()->CommitChanges();
		v[0] = SKYBOX_VERTEX(VEC3(-10.f, -10.f, -10.f), VEC2(1.f, 1.f));
		v[1] = SKYBOX_VERTEX(VEC3(+10.f, -10.f, -10.f), VEC2(0.f, 1.f));
		v[2] = SKYBOX_VERTEX(VEC3(-10.f, +10.f, -10.f), VEC2(1.f, 0.f));
		v[3] = SKYBOX_VERTEX(VEC3(+10.f, +10.f, -10.f), VEC2(0.f, 0.f));
		Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &v, sizeof(SKYBOX_VERTEX));
		frame::RegisterDrawCall(2);
	}

	if (pimpl->m_SideTextures[SIDE_FAR])
	{
		//Dev->SetTexture(0, pimpl->m_SideTextures[SIDE_FAR]->GetTexture());
		Effect->Get()->SetTexture(TextureParam, pimpl->m_SideTextures[SIDE_FAR]->GetTexture());
		Effect->Get()->CommitChanges();
		v[0] = SKYBOX_VERTEX(VEC3(-10.f, -10.f, +10.f), VEC2(0.f, 1.f));
		v[1] = SKYBOX_VERTEX(VEC3(+10.f, -10.f, +10.f), VEC2(1.f, 1.f));
		v[2] = SKYBOX_VERTEX(VEC3(-10.f, +10.f, +10.f), VEC2(0.f, 0.f));
		v[3] = SKYBOX_VERTEX(VEC3(+10.f, +10.f, +10.f), VEC2(1.f, 0.f));
		Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &v, sizeof(SKYBOX_VERTEX));
		frame::RegisterDrawCall(2);
	}

	if (pimpl->m_SideTextures[SIDE_LEFT])
	{
		//Dev->SetTexture(0, pimpl->m_SideTextures[SIDE_LEFT]->GetTexture());
		Effect->Get()->SetTexture(TextureParam, pimpl->m_SideTextures[SIDE_LEFT]->GetTexture());
		Effect->Get()->CommitChanges();
		v[0] = SKYBOX_VERTEX(VEC3(-10.f, -10.f, -10.f), VEC2(0.f, 1.f));
		v[1] = SKYBOX_VERTEX(VEC3(-10.f, -10.f, +10.f), VEC2(1.f, 1.f));
		v[2] = SKYBOX_VERTEX(VEC3(-10.f, +10.f, -10.f), VEC2(0.f, 0.f));
		v[3] = SKYBOX_VERTEX(VEC3(-10.f, +10.f, +10.f), VEC2(1.f, 0.f));
		Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &v, sizeof(SKYBOX_VERTEX));
		frame::RegisterDrawCall(2);
	}

	if (pimpl->m_SideTextures[SIDE_RIGHT])
	{
		//Dev->SetTexture(0, pimpl->m_SideTextures[SIDE_RIGHT]->GetTexture());
		Effect->Get()->SetTexture(TextureParam, pimpl->m_SideTextures[SIDE_RIGHT]->GetTexture());
		Effect->Get()->CommitChanges();
		v[0] = SKYBOX_VERTEX(VEC3(+10.f, -10.f, -10.f), VEC2(1.f, 1.f));
		v[1] = SKYBOX_VERTEX(VEC3(+10.f, -10.f, +10.f), VEC2(0.f, 1.f));
		v[2] = SKYBOX_VERTEX(VEC3(+10.f, +10.f, -10.f), VEC2(1.f, 0.f));
		v[3] = SKYBOX_VERTEX(VEC3(+10.f, +10.f, +10.f), VEC2(0.f, 0.f));
		Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &v, sizeof(SKYBOX_VERTEX));
		frame::RegisterDrawCall(2);
	}

	if (pimpl->m_SideTextures[SIDE_BOTTOM])
	{
		//Dev->SetTexture(0, pimpl->m_SideTextures[SIDE_BOTTOM]->GetTexture());
		Effect->Get()->SetTexture(TextureParam, pimpl->m_SideTextures[SIDE_BOTTOM]->GetTexture());
		Effect->Get()->CommitChanges();
		v[0] = SKYBOX_VERTEX(VEC3(-10.f, -10.f, -10.f), VEC2(0.f, 0.f));
		v[1] = SKYBOX_VERTEX(VEC3(-10.f, -10.f, +10.f), VEC2(1.f, 0.f));
		v[2] = SKYBOX_VERTEX(VEC3(+10.f, -10.f, -10.f), VEC2(0.f, 1.f));
		v[3] = SKYBOX_VERTEX(VEC3(+10.f, -10.f, +10.f), VEC2(1.f, 1.f));
		Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &v, sizeof(SKYBOX_VERTEX));
		frame::RegisterDrawCall(2);
	}

	if (pimpl->m_SideTextures[SIDE_TOP])
	{
		//Dev->SetTexture(0, pimpl->m_SideTextures[SIDE_TOP]->GetTexture());
		Effect->Get()->SetTexture(TextureParam, pimpl->m_SideTextures[SIDE_TOP]->GetTexture());
		Effect->Get()->CommitChanges();
		v[0] = SKYBOX_VERTEX(VEC3(-10.f, +10.f, -10.f), VEC2(0.f, 1.f));
		v[1] = SKYBOX_VERTEX(VEC3(-10.f, +10.f, +10.f), VEC2(1.f, 1.f));
		v[2] = SKYBOX_VERTEX(VEC3(+10.f, +10.f, -10.f), VEC2(0.f, 0.f));
		v[3] = SKYBOX_VERTEX(VEC3(+10.f, +10.f, +10.f), VEC2(1.f, 0.f));
		Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &v, sizeof(SKYBOX_VERTEX));
		frame::RegisterDrawCall(2);
	}

	Effect->Get()->EndPass();
	Effect->Get()->End();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ComplexSky

// Liczba segmentów dooko³a (wschód-zachód)
const uint SKYDOME_SLICES = 32;
// Liczba segmentów na wysokoœæ
const uint SKYDOME_STACKS = 32;
const uint SKYDOME_VERTEX_COUNT = SKYDOME_SLICES * (SKYDOME_STACKS + 1);
const uint SKYDOME_TRIANGLE_COUNT = SKYDOME_SLICES * SKYDOME_STACKS * 2;
const uint SKYDOME_INDEX_COUNT = SKYDOME_TRIANGLE_COUNT * 3;
const float SKYDOME_RADIUS = 10.0f;
const float HEIGHT_FACTOR_EXP = 0.7f;
// Rozci¹gniêcie tekstury gwiazd (1.0 oznacza potwtórzenie 2x2 na ca³ej hemisferze)
const float STARS_TEX_SIZE = 2.0f;
const float GLOW_ANGLE_SIZE = 2.0f;
const float GLOW_MAX_INTENSITY = 0.2f;
// Jaki procent wsp. tekstury z zenitu ma zostaæ zachowany na horyzoncie (0..1)
const float SKY_TEX_STRETCH_FACTOR = 0.4f;

const float DEFAULT_TIME = 0.5f;

struct SKYDOME_VERTEX
{
	VEC3 Pos;
	// Sk³adowa R koduje szerokoœæ geograficzn¹, 0..255 odpowiada 0 radianów (horyzont) .. PI/2 radianów (zenit)
	COLOR Diffuse;
	VEC2 StarsTex;
};

const DWORD SKYDOME_FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);

enum COMPLEX_SKY_SHADER_PARAMS
{
	CSSP_WORLD_VIEW_PROJ,
	CSSP_HORIZON_COLOR,
	CSSP_ZENITH_COLOR,
	CSSP_STARS_TEXTURE,
	CSSP_STAR_VISIBILITY,
};

enum COMPLEX_SKY_CLOUDS_SHADER_PARAMS
{
	CSCSP_WORLD_VIEW_PROJ,
	CSCSP_NOISE_TEXTURE,
	CSCSP_NOISE_TEX_A_01,
	CSCSP_NOISE_TEX_A_23,
	CSCSP_NOISE_TEX_B_01,
	CSCSP_NOISE_TEX_B_23,
	CSCSP_COLOR1,
	CSCSP_COLOR2,
	CSCSP_SHARPNESS,
	CSCSP_THRESHOLD,
};

void ComplexSky::BACKGROUND_DESC::LoadFromTokenizer(Tokenizer &t)
{
	Colors.Items.clear();

	t.AssertSymbol('{');
	t.Next();

	float TimeOfDay;
	COLORF HorizonColor, ZenithColor;
	while (!t.QuerySymbol('}'))
	{
		TimeOfDay = t.MustGetFloat();
		t.Next();

		ParseColor(&HorizonColor, t);
		ParseColor(&ZenithColor, t);

		Colors.Items.push_back(RoundInterpolator<COLORF_PAIR>::ITEM(TimeOfDay, COLORF_PAIR(HorizonColor, ZenithColor)));
		
		if (!t.QuerySymbol('}'))
		{
			t.AssertSymbol(',');
			t.Next();
		}
	}
	t.Next();
}

void ComplexSky::CLOUD_COLORS::LoadFromTokenizer(Tokenizer &t)
{
	Colors.Items.clear();

	t.AssertSymbol('{');
	t.Next();

	float TimeOfDay;
	COLORF Color1, Color2;
	while (!t.QuerySymbol('}'))
	{
		TimeOfDay = t.MustGetFloat();
		t.Next();

		ParseColor(&Color1, t);
		ParseColor(&Color2, t);

		Colors.Items.push_back(RoundInterpolator<COLORF_PAIR>::ITEM(TimeOfDay, COLORF_PAIR(Color1, Color2)));
		
		if (!t.QuerySymbol('}'))
		{
			t.AssertSymbol(',');
			t.Next();
		}
	}
	t.Next();
}

void ComplexSky::CELESTIAL_OBJECT_DESC::LoadFromTokenizer(Tokenizer &t)
{
	t.AssertSymbol('{');
	t.Next();

	t.AssertToken(Tokenizer::TOKEN_STRING);
	this->TextureName = t.GetString();
	t.Next();

	this->Size = t.MustGetFloat();
	t.Next();

	this->Period = t.MustGetFloat();
	t.Next();

	this->Phase = t.MustGetFloat();
	t.Next();

	ParseVec3(&this->OrbitNormal, t);
	Normalize(&this->OrbitNormal);
	
	if (t.QueryIdentifier("HorizonColor"))
	{
		t.Next();

		t.AssertSymbol('=');
		t.Next();

		ParseColor(&this->HorizonColor, t);

		t.AssertIdentifier("ZenithColor");
		t.Next();

		t.AssertSymbol('=');
		t.Next();

		ParseColor(&this->ZenithColor, t);
	}
	else
		this->HorizonColor = this->ZenithColor = COLORF::WHITE;

	if (t.QueryIdentifier("MidnightColor"))
	{
		t.Next();

		t.AssertSymbol('=');
		t.Next();

		ParseColor(&this->MidnightColor, t);

		t.AssertIdentifier("MiddayColor");
		t.Next();

		t.AssertSymbol('=');
		t.Next();

		ParseColor(&this->MiddayColor, t);
	}
	else
		this->MidnightColor = this->MiddayColor = COLORF::WHITE;

	t.AssertSymbol('}');
	t.Next();
}


struct CELESTIAL_OBJECT_CACHE
{
	// Pobierana przy pierwszym u¿yciu. NULL jeœli nie ma lub jeszcze nie pobrana.
	res::D3dTexture *m_Texture;
	// Wyliczane przy pierwszym u¿yciu.
	VEC3 m_UpDir;
	VEC3 m_RightDir;
	bool m_RightUpDirsCalculated;
	VEC3 m_Dir;
	bool m_DirCalculated;
	COLORF m_Color;
	bool m_ColorCalculated;
	VERTEX_X2 m_Vertices[4];
	VERTEX_X2 m_GlowVertices[4];
	bool m_VerticesCalculated;
	bool m_GlowVerticesCalculated;

	void ResetAll()
	{
		m_Texture = NULL;
		m_RightUpDirsCalculated = false;
		m_DirCalculated = false;
		m_ColorCalculated = false;
		m_VerticesCalculated = false;
		m_GlowVerticesCalculated = false;
	}
	void TimeChanged()
	{
		m_DirCalculated = false;
		m_ColorCalculated = false;
		m_VerticesCalculated = false;
		m_GlowVerticesCalculated = false;
	}
};

class ComplexSky_pimpl
{
public:
	Scene *m_OwnerScene;
	float m_Time;
	ComplexSky::BACKGROUND_DESC m_BackgroundDesc;
	COLORF m_BackgroundWeatherFactor;
	// S³oñce
	ComplexSky::CELESTIAL_OBJECT_DESC m_SunDesc;
	CELESTIAL_OBJECT_CACHE m_SunCache;
	// Ksiê¿yce
	// Obydwie tablice maj¹ taki sam rozmiar i ich elementy odpowiadaj¹ sobie.
	std::vector<ComplexSky::CELESTIAL_OBJECT_DESC> m_MoonDesc;
	std::vector<CELESTIAL_OBJECT_CACHE> m_MoonCache;
	// Chmury
	float m_CloudsThreshold;
	float m_CloudsSharpness;
	ComplexSky::CLOUD_COLORS m_CloudColors;
	COLORF_PAIR m_CloudColorWeatherFactors;
	float m_CloudsScaleInv;
	VEC2 m_CurrentCloudsOffset;

	scoped_ptr<res::D3dVertexBuffer> m_SkydomeVB;
	scoped_ptr<res::D3dIndexBuffer> m_SkydomeIB;

	// Kolory t³a dla bie¿¹cego czasu m_Time wyliczone z m_BackgroundDesc
	COLORF m_CurrentHorizonColor;
	COLORF m_CurrentZenithColor;
	bool m_CurrentBackgroundColorsCalculated;

	// Tworzy i wype³nia VB i IB do Skydome
	void CreateSkydome();
	void DestroySkydome();
	void InitDefaultSun();
	void InitDefaultBackgroundColors();
	void InitDefaultCloudColors();
	// Zwraca dan¹ teksturê lub NULL jeœli brak.
	IDirect3DTexture9 * GetStarsTexture();
	IDirect3DTexture9 * GetNoiseTexture();

	void EnsureCurrentBackgroundColors();
	void EnsureCelestialObjectDir(CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc);
	void EnsureCelestialObjectColor(CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc);
	void EnsureCelestialObjectVertices(CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc);
	void EnsureCelestialObjectGlowVertices(CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc);
	// Wype³nia pole Cache.m_Texture lub jeœli brak tekstury, to pozostawia NULL.
	void EnsureCelestialObjectTexture(CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc);

	void FillQuadVertices(VERTEX_X2 *Vertices, float AngleSize, const VEC3 &Dir, const VEC3 &OrbitNormal);
	void DrawSkydome(const ParamsCamera &Cam, const MATRIX &WorldMat);
	void DrawClouds(const ParamsCamera &Cam, const MATRIX &WorldMat);
	void DrawCelestialObjects(const ParamsCamera &Cam, const MATRIX &WorldMat);
	void DrawCelestialObject(const ParamsCamera &Cam, CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc);
	void DrawSunGlow(const ParamsCamera &Cam, CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc);
};

void ComplexSky_pimpl::CreateSkydome()
{
	ERR_TRY;

	m_SkydomeVB.reset(new res::D3dVertexBuffer(string(), string(),
		SKYDOME_VERTEX_COUNT,
		D3DUSAGE_WRITEONLY,
		SKYDOME_FVF,
		D3DPOOL_MANAGED));
	m_SkydomeIB.reset(new res::D3dIndexBuffer(string(), string(),
		SKYDOME_INDEX_COUNT,
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_MANAGED));
	m_SkydomeVB->Lock();
	m_SkydomeIB->Lock();

	// Wype³nij VB
	{
		res::D3dVertexBuffer::Locker vb_lock(m_SkydomeVB.get(), true, 0);
		SKYDOME_VERTEX *vertex = (SKYDOME_VERTEX*)vb_lock.GetData();

		uint Latitude, Longitude;
		float LatitudeF, LongitudeF;
		float LatitudeStep = PI / (float)SKYDOME_STACKS;
		float LongitudeStep = PI_X_2 / (float)SKYDOME_SLICES;
		COLOR Diffuse = COLOR::BLACK;
		float LatitudeFactor, TexFactor;
		for (Latitude = 0, LatitudeF = -PI_2; Latitude <= SKYDOME_STACKS; Latitude++, LatitudeF += LatitudeStep)
		{
			for (Longitude = 0, LongitudeF = 0.0f; Longitude < SKYDOME_SLICES; Longitude++, LongitudeF += LongitudeStep)
			{
				SphericalToCartesian(&vertex->Pos, LongitudeF, LatitudeF, SKYDOME_RADIUS);
				LatitudeFactor = std::max(0.0f, vertex->Pos.y) / SKYDOME_RADIUS;
				LatitudeFactor = powf(LatitudeFactor, HEIGHT_FACTOR_EXP);
				Diffuse.R = (uint)( LatitudeFactor * 255.0f);
				vertex->Diffuse = Diffuse;
				TexFactor = LatitudeFactor * (1.0f - SKY_TEX_STRETCH_FACTOR) + SKY_TEX_STRETCH_FACTOR;
				vertex->StarsTex = VEC2(vertex->Pos.x, vertex->Pos.z) / SKYDOME_RADIUS * STARS_TEX_SIZE / TexFactor;
				vertex++;
			}
		}
	}

	// Wype³nij IB
	{
		res::D3dIndexBuffer::Locker ib_lock(m_SkydomeIB.get(), true, 0);
		uint2 *index = (uint2*)ib_lock.GetData();

		uint Latitude, Longitude;
		for (Latitude = 0; Latitude < SKYDOME_STACKS; Latitude++)
		{
			for (Longitude = 0; Longitude < SKYDOME_SLICES; Longitude++)
			{
				*index = (Latitude  ) * SKYDOME_SLICES + (Longitude+1) % SKYDOME_SLICES; index++;
				*index = (Latitude+1) * SKYDOME_SLICES + (Longitude+1) % SKYDOME_SLICES; index++;
				*index = (Latitude+1) * SKYDOME_SLICES + (Longitude  ); index++;
				*index = (Latitude  ) * SKYDOME_SLICES + (Longitude+1) % SKYDOME_SLICES; index++;
				*index = (Latitude+1) * SKYDOME_SLICES + (Longitude  ); index++;
				*index = (Latitude  ) * SKYDOME_SLICES + (Longitude  ); index++;
			}
		}
	}

	ERR_CATCH_FUNC;
}

void ComplexSky_pimpl::DestroySkydome()
{
	m_SkydomeIB->Unlock();
	m_SkydomeVB->Unlock();
	m_SkydomeIB.reset();
	m_SkydomeVB.reset();
}

void ComplexSky_pimpl::InitDefaultSun()
{
	// Rozmiar naszego S³oñca to podobno 0.53623 stopni, a Ksiê¿yca 0.51791 stopni.

	//m_SunDesc.TextureName =
	m_SunDesc.Size = 0.0093589790479692f;
	m_SunDesc.Period = 1.0f;
	m_SunDesc.Phase = - PI_2;
	m_SunDesc.OrbitNormal = VEC3::POSITIVE_Z;
	m_SunDesc.HorizonColor = m_SunDesc.ZenithColor = COLORF::WHITE;
	m_SunDesc.MidnightColor = m_SunDesc.MiddayColor = COLORF::WHITE;

	m_SunCache.ResetAll();
}

void ComplexSky_pimpl::InitDefaultBackgroundColors()
{
	m_BackgroundDesc.Colors.Items.clear();
	m_BackgroundDesc.Colors.Items.push_back(RoundInterpolator<COLORF_PAIR>::ITEM(0.0f, COLORF_PAIR(COLORF(1.0f, 0.5f, 0.5f, 1.0f), COLORF(1.0f, 0.3f, 0.3f, 1.0f))));
}

void ComplexSky_pimpl::InitDefaultCloudColors()
{
	m_CloudColors.Colors.Items.clear();
	m_CloudColors.Colors.Items.push_back(RoundInterpolator<COLORF_PAIR>::ITEM(0.0f, COLORF_PAIR(COLORF::BLACK, COLORF::WHITE)));
}

IDirect3DTexture9 * ComplexSky_pimpl::GetStarsTexture()
{
	res::D3dTexture *TexRes = res::g_Manager->GetResourceEx<res::D3dTexture>("Sky_Stars");
	if (TexRes == NULL)
		return NULL;
	TexRes->Load();
	return TexRes->GetTexture();
}

IDirect3DTexture9 * ComplexSky_pimpl::GetNoiseTexture()
{
	res::D3dTexture *TexRes = res::g_Manager->GetResourceEx<res::D3dTexture>("NoiseTexture");
	if (TexRes == NULL)
		return NULL;
	TexRes->Load();
	return TexRes->GetTexture();
}

void ComplexSky_pimpl::EnsureCurrentBackgroundColors()
{
	if (!m_CurrentBackgroundColorsCalculated)
	{
		float TimeOfDay = frac(m_Time);
		COLORF_PAIR BackgroundColors;
		m_BackgroundDesc.Colors.Calc(&BackgroundColors, TimeOfDay);
		m_CurrentHorizonColor = BackgroundColors.first;
		m_CurrentZenithColor = BackgroundColors.second;

		m_CurrentBackgroundColorsCalculated = true;
	}
}

void ComplexSky_pimpl::EnsureCelestialObjectDir(CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc)
{
	if (!Cache.m_DirCalculated)
	{
		// Wylicz Right i Up, jeœli trzeba
		if (!Cache.m_RightUpDirsCalculated)
		{
			assert(Desc.OrbitNormal.x != 0.0f || Desc.OrbitNormal.z != 0.0f);

			Cross(&Cache.m_RightDir, VEC3::POSITIVE_Y, Desc.OrbitNormal);
			Normalize(&Cache.m_RightDir);
			Cross(&Cache.m_UpDir, Desc.OrbitNormal, Cache.m_RightDir);
			Normalize(&Cache.m_UpDir);

			Cache.m_RightUpDirsCalculated = true;
		}

		// Wylicz aktualny k¹t
		float Angle = m_Time * (PI_X_2 / Desc.Period) + Desc.Phase;

		// Wylicz kierunek
		Cache.m_Dir = cosf(Angle) * Cache.m_RightDir + sinf(Angle) * Cache.m_UpDir;

		Cache.m_DirCalculated = true;
	}
}

void ComplexSky_pimpl::EnsureCelestialObjectColor(CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc)
{
	if (!Cache.m_ColorCalculated)
	{
		Cache.m_Color = COLORF::WHITE;

		// Od pozycji
		if (Desc.HorizonColor != COLORF::WHITE || Desc.ZenithColor != COLORF::WHITE)
		{
			COLORF HeightColor;
			EnsureCelestialObjectDir(Cache, Desc);

			float HeightFactor = std::max(0.0f, Cache.m_Dir.y);
			HeightFactor = powf(HeightFactor, HEIGHT_FACTOR_EXP);
			Lerp(&HeightColor, Desc.HorizonColor, Desc.ZenithColor, HeightFactor);

			Cache.m_Color *= HeightColor;
		}

		// Od pory dnia
		if (Desc.MidnightColor != COLORF::WHITE || Desc.MiddayColor != COLORF::WHITE)
		{
			COLORF TimeOfDayColor;
			float TimeOfDay = frac(m_Time);
			float T;
			if (TimeOfDay < 0.5f)
				T = 2.0f * TimeOfDay;
			else
				T = 2.0f - 2.0f * TimeOfDay;
			Lerp(&TimeOfDayColor, Desc.MidnightColor, Desc.MiddayColor, T);

			Cache.m_Color *= TimeOfDayColor;
		}

		Cache.m_ColorCalculated = true;
	}
}

void ComplexSky_pimpl::FillQuadVertices(VERTEX_X2 *Vertices, float AngleSize, const VEC3 &Dir, const VEC3 &OrbitNormal)
{
	const VEC3 & Forward = Dir;
	const VEC3 & Right = OrbitNormal;
	VEC3 Up; Cross(&Up, Forward, Right);
	MATRIX RotMat; AxesToMatrix(&RotMat, Right, Up, Forward);

	float Delta = SKYDOME_RADIUS * tanf(AngleSize * 0.5f);
	VEC3 v;

	v = VEC3(-Delta, -Delta, SKYDOME_RADIUS);
	Transform(&Vertices[0].Pos, v, RotMat);
	Vertices[0].Tex = VEC2(0.0f, 0.0f);
	v = VEC3(-Delta,  Delta, SKYDOME_RADIUS);
	Transform(&Vertices[1].Pos, v, RotMat);
	Vertices[1].Tex = VEC2(0.0f, 1.0f);
	v = VEC3( Delta,  Delta, SKYDOME_RADIUS);
	Transform(&Vertices[2].Pos, v, RotMat);
	Vertices[2].Tex = VEC2(1.0f, 1.0f);
	v = VEC3( Delta, -Delta, SKYDOME_RADIUS);
	Transform(&Vertices[3].Pos, v, RotMat);
	Vertices[3].Tex = VEC2(1.0f, 0.0f);

}

void ComplexSky_pimpl::EnsureCelestialObjectVertices(CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc)
{
	if (!Cache.m_VerticesCalculated)
	{
		EnsureCelestialObjectDir(Cache, Desc);
		FillQuadVertices(Cache.m_Vertices, Desc.Size, Cache.m_Dir, Desc.OrbitNormal);
		Cache.m_VerticesCalculated = true;
	}
}

void ComplexSky_pimpl::EnsureCelestialObjectGlowVertices(CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc)
{
	if (!Cache.m_GlowVerticesCalculated)
	{
		EnsureCelestialObjectDir(Cache, Desc);
		FillQuadVertices(Cache.m_GlowVertices, GLOW_ANGLE_SIZE, Cache.m_Dir, Desc.OrbitNormal);
		Cache.m_GlowVerticesCalculated = true;
	}
}

void ComplexSky_pimpl::EnsureCelestialObjectTexture(CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc)
{
	if (Cache.m_Texture == NULL)
	{
		if (Desc.TextureName.empty())
			return;
		Cache.m_Texture = res::g_Manager->GetResourceEx<res::D3dTexture>(Desc.TextureName);
	}
}

void ComplexSky_pimpl::DrawSkydome(const ParamsCamera &Cam, const MATRIX &WorldMat)
{
	frame::RestoreDefaultState();

	float TimeOfDay = frac(m_Time);
	float StarVisibility;
	if (TimeOfDay > 0.75f)
		StarVisibility = 4.0f * TimeOfDay - 3.0f;
	else if (TimeOfDay < 0.25f)
		StarVisibility = 1.0f - 4.0f * TimeOfDay;
	else
		StarVisibility = 0.0f;

	res::D3dEffect *Effect = res::g_Manager->MustGetResourceEx<res::D3dEffect>("ComplexSkyShader");
	Effect->Load();

	ERR_GUARD_DIRECTX_D( frame::Dev->SetRenderState(D3DRS_ZENABLE, FALSE) );
	ERR_GUARD_DIRECTX_D( frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE) );

	EnsureCurrentBackgroundColors();

	COLORF HorizonColor = m_CurrentHorizonColor * m_BackgroundWeatherFactor;
	COLORF ZenithColor = m_CurrentZenithColor * m_BackgroundWeatherFactor;

	ERR_GUARD_DIRECTX_D( Effect->Get()->SetMatrix(Effect->GetParam(CSSP_WORLD_VIEW_PROJ), &math_cast<D3DXMATRIX>(WorldMat * Cam.GetMatrices().GetViewProj())) );
	ERR_GUARD_DIRECTX_D( Effect->Get()->SetVector(Effect->GetParam(CSSP_HORIZON_COLOR), &D3DXVECTOR4(HorizonColor.R, HorizonColor.G, HorizonColor.B, 1.0f)) );
	ERR_GUARD_DIRECTX_D( Effect->Get()->SetVector(Effect->GetParam(CSSP_ZENITH_COLOR), &D3DXVECTOR4(ZenithColor.R, ZenithColor.G, ZenithColor.B, 1.0f)) );
	ERR_GUARD_DIRECTX_D( Effect->Get()->SetTexture(Effect->GetParam(CSSP_STARS_TEXTURE), GetStarsTexture()) );
	ERR_GUARD_DIRECTX_D( Effect->Get()->SetFloat(Effect->GetParam(CSSP_STAR_VISIBILITY), StarVisibility) );

	Effect->Begin(false);

	ERR_GUARD_DIRECTX_D( frame::Dev->SetStreamSource(0, m_SkydomeVB.get()->GetVB(), 0, sizeof(SKYDOME_VERTEX)) );
	ERR_GUARD_DIRECTX_D( frame::Dev->SetIndices(m_SkydomeIB.get()->GetIB()) );
	ERR_GUARD_DIRECTX_D( frame::Dev->SetFVF(SKYDOME_FVF) );

	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, SKYDOME_VERTEX_COUNT, 0, SKYDOME_TRIANGLE_COUNT) );
	frame::RegisterDrawCall(SKYDOME_TRIANGLE_COUNT);

	Effect->End();
}

void ComplexSky_pimpl::DrawClouds(const ParamsCamera &Cam, const MATRIX &WorldMat)
{
	frame::RestoreDefaultState();

	VEC2 NoiseTexA[4];
	VEC2 NoiseTexB[4];

	m_CurrentCloudsOffset += frame::Timer2.GetDeltaTime() * m_OwnerScene->GetWindVec();
	VEC2 OctaveMovement = m_CurrentCloudsOffset;
	for (uint oi = 0; oi < 4; oi++)
	{
		NoiseTexA[oi].x = NoiseTexA[oi].y = 1.0f / expf( (float)(3-oi) ) * m_CloudsScaleInv;
		NoiseTexB[oi] = OctaveMovement;

		NoiseTexB[oi].x *= NoiseTexA[oi].x;
		NoiseTexB[oi].y *= NoiseTexA[oi].y;

		OctaveMovement *= 0.7f; // magic number :)
	}

	res::D3dEffect *Effect = res::g_Manager->MustGetResourceEx<res::D3dEffect>("ComplexSkyCloudsShader");
	Effect->Load();

	ERR_GUARD_DIRECTX( frame::Dev->SetRenderState(D3DRS_ZENABLE, FALSE) );
	ERR_GUARD_DIRECTX( frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE) );
	ERR_GUARD_DIRECTX( frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE) );
	ERR_GUARD_DIRECTX( frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA) );
	ERR_GUARD_DIRECTX( frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA) );

	COLORF_PAIR Colors;
	m_CloudColors.Colors.Calc(&Colors, frac(m_Time));
	Colors.first *= m_CloudColorWeatherFactors.first;
	Colors.second *= m_CloudColorWeatherFactors.second;

	ERR_GUARD_DIRECTX( Effect->Get()->SetMatrix(Effect->GetParam(CSCSP_WORLD_VIEW_PROJ), &math_cast<D3DXMATRIX>(WorldMat * Cam.GetMatrices().GetViewProj())) );
	ERR_GUARD_DIRECTX( Effect->Get()->SetTexture(Effect->GetParam(CSCSP_NOISE_TEXTURE), GetNoiseTexture()) );
	ERR_GUARD_DIRECTX( Effect->Get()->SetVector(Effect->GetParam(CSCSP_NOISE_TEX_A_01), &D3DXVECTOR4(NoiseTexA[0].x, NoiseTexA[0].y, NoiseTexA[1].x, NoiseTexA[1].y)) );
	ERR_GUARD_DIRECTX( Effect->Get()->SetVector(Effect->GetParam(CSCSP_NOISE_TEX_A_23), &D3DXVECTOR4(NoiseTexA[2].x, NoiseTexA[2].y, NoiseTexA[3].x, NoiseTexA[3].y)) );
	ERR_GUARD_DIRECTX( Effect->Get()->SetVector(Effect->GetParam(CSCSP_NOISE_TEX_B_01), &D3DXVECTOR4(NoiseTexB[0].x, NoiseTexB[0].y, NoiseTexB[1].x, NoiseTexB[1].y)) );
	ERR_GUARD_DIRECTX( Effect->Get()->SetVector(Effect->GetParam(CSCSP_NOISE_TEX_B_23), &D3DXVECTOR4(NoiseTexB[2].x, NoiseTexB[2].y, NoiseTexB[3].x, NoiseTexB[3].y)) );
	ERR_GUARD_DIRECTX( Effect->Get()->SetVector(Effect->GetParam(CSCSP_COLOR1), &D3DXVECTOR4(Colors.first.R, Colors.first.G, Colors.first.B, Colors.first.A)) );
	ERR_GUARD_DIRECTX( Effect->Get()->SetVector(Effect->GetParam(CSCSP_COLOR2), &D3DXVECTOR4(Colors.second.R, Colors.second.G, Colors.second.B, Colors.second.A)) );
	ERR_GUARD_DIRECTX( Effect->Get()->SetFloat(Effect->GetParam(CSCSP_SHARPNESS), m_CloudsSharpness) );
	ERR_GUARD_DIRECTX( Effect->Get()->SetFloat(Effect->GetParam(CSCSP_THRESHOLD), m_CloudsThreshold) );

	Effect->Begin(false);

	ERR_GUARD_DIRECTX( frame::Dev->SetStreamSource(0, m_SkydomeVB.get()->GetVB(), 0, sizeof(SKYDOME_VERTEX)) );
	ERR_GUARD_DIRECTX( frame::Dev->SetIndices(m_SkydomeIB.get()->GetIB()) );
	ERR_GUARD_DIRECTX( frame::Dev->SetFVF(SKYDOME_FVF) );

	ERR_GUARD_DIRECTX( frame::Dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, SKYDOME_VERTEX_COUNT, 0, SKYDOME_TRIANGLE_COUNT) );
	frame::RegisterDrawCall(SKYDOME_TRIANGLE_COUNT);

	Effect->End();
}

void ComplexSky_pimpl::DrawCelestialObjects(const ParamsCamera &Cam, const MATRIX &WorldMat)
{
	frame::RestoreDefaultState();

	frame::Dev->SetFVF(VERTEX_X2::FVF);

	frame::Dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	frame::Dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	frame::Dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	frame::Dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	frame::Dev->SetTransform(D3DTS_WORLD, &math_cast<D3DXMATRIX>(WorldMat));
	frame::Dev->SetTransform(D3DTS_VIEW, &math_cast<D3DXMATRIX>(Cam.GetMatrices().GetView()));
	frame::Dev->SetTransform(D3DTS_PROJECTION, &math_cast<D3DXMATRIX>(Cam.GetMatrices().GetProj()));

	// S³oñce
	DrawCelestialObject(Cam, m_SunCache, m_SunDesc);
	// Ksiê¿yce
	for (uint mi = 0; mi < m_MoonDesc.size(); mi++)
		DrawCelestialObject(Cam, m_MoonCache[mi], m_MoonDesc[mi]);

	// Glow od s³oñca
	frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	DrawSunGlow(Cam, m_SunCache, m_SunDesc);
}

void ComplexSky_pimpl::DrawCelestialObject(const ParamsCamera &Cam, CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc)
{
	EnsureCelestialObjectTexture(Cache, Desc);
	if (Cache.m_Texture != NULL)
	{
		EnsureCelestialObjectColor(Cache, Desc);
		if (Cache.m_Color.A > 0.0f)
		{
			Cache.m_Texture->Load();
			frame::Dev->SetTexture(0, Cache.m_Texture->GetBaseTexture());

			frame::Dev->SetRenderState(D3DRS_TEXTUREFACTOR, ColorfToColor(Cache.m_Color).ARGB);

			EnsureCelestialObjectVertices(Cache, Desc);
			frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, Cache.m_Vertices, sizeof(VERTEX_X2));
			frame::RegisterDrawCall(2);
		}
	}
}

void ComplexSky_pimpl::DrawSunGlow(const ParamsCamera &Cam, CELESTIAL_OBJECT_CACHE &Cache, const ComplexSky::CELESTIAL_OBJECT_DESC &Desc)
{
	EnsureCelestialObjectTexture(Cache, Desc);
	if (Cache.m_Texture != NULL)
	{
		EnsureCelestialObjectDir(Cache, Desc);
		if (Cache.m_Dir.y > 0.0f)
		{
			float Intensity = powf(Cache.m_Dir.y, HEIGHT_FACTOR_EXP) * GLOW_MAX_INTENSITY;

			res::D3dTexture *GlowTexture = res::g_Manager->MustGetResourceEx<res::D3dTexture>("Sky_Glow");
			GlowTexture->Load();
			frame::Dev->SetTexture(0, GlowTexture->GetBaseTexture());

			frame::Dev->SetRenderState(D3DRS_TEXTUREFACTOR, 0x00FFFFFF | ((uint)(Intensity * 255.0f) << 24));

			EnsureCelestialObjectGlowVertices(Cache, Desc);
			frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, Cache.m_GlowVertices, sizeof(VERTEX_X2));
			frame::RegisterDrawCall(2);
		}
	}
}

ComplexSky::ComplexSky(Scene *OwnerScene) :
	pimpl(new ComplexSky_pimpl)
{
	pimpl->m_OwnerScene = OwnerScene;
	pimpl->m_Time = DEFAULT_TIME;
	pimpl->m_BackgroundWeatherFactor = COLORF::WHITE;
	pimpl->m_CurrentBackgroundColorsCalculated = false;
	pimpl->m_CloudsThreshold = 0.7f;
	pimpl->m_CloudsSharpness = 5.0f;
	pimpl->m_CloudColorWeatherFactors = COLORF_PAIR(COLORF::WHITE, COLORF::WHITE);
	pimpl->m_CloudsScaleInv = 1.0f;
	pimpl->m_CurrentCloudsOffset = VEC2::ZERO;
	pimpl->InitDefaultSun();
	pimpl->InitDefaultBackgroundColors();
	pimpl->InitDefaultCloudColors();

	pimpl->CreateSkydome();
}

ComplexSky::ComplexSky(Scene *OwnerScene, Tokenizer &t) :
	pimpl(new ComplexSky_pimpl)
{
	pimpl->m_OwnerScene = OwnerScene;
	pimpl->m_Time = DEFAULT_TIME;
	pimpl->m_BackgroundWeatherFactor = COLORF::WHITE;
	pimpl->m_CurrentBackgroundColorsCalculated = false;
	pimpl->m_CloudColorWeatherFactors = COLORF_PAIR(COLORF::WHITE, COLORF::WHITE);
	pimpl->m_CurrentCloudsOffset = VEC2::ZERO;

	LoadFromTokenizer(t);

	pimpl->CreateSkydome();
}

ComplexSky::~ComplexSky()
{
	pimpl->DestroySkydome();
	pimpl.reset();
}

void ComplexSky::LoadFromTokenizer(Tokenizer &t)
{
	t.AssertSymbol('{');
	t.Next();

	t.AssertIdentifier("Background");
	t.Next();
	pimpl->m_BackgroundDesc.LoadFromTokenizer(t);

	t.AssertIdentifier("Sun");
	t.Next();
	pimpl->m_SunDesc.LoadFromTokenizer(t);
	pimpl->m_SunCache.ResetAll();

	while (t.QueryIdentifier("Moon"))
	{
		t.Next();
		CELESTIAL_OBJECT_DESC Desc;
		Desc.LoadFromTokenizer(t);
		AddMoon(Desc);
	}

	t.AssertIdentifier("Clouds");
	t.Next();
	t.AssertSymbol('{');
	t.Next();
	t.AssertIdentifier("ScaleInv");
	t.Next();
	t.AssertSymbol('=');
	t.Next();
	pimpl->m_CloudsScaleInv = t.MustGetFloat();
	t.Next();
	t.AssertIdentifier("Threshold");
	t.Next();
	t.AssertSymbol('=');
	t.Next();
	pimpl->m_CloudsThreshold = t.MustGetFloat();
	t.Next();
	t.AssertIdentifier("Sharpness");
	t.Next();
	t.AssertSymbol('=');
	t.Next();
	pimpl->m_CloudsSharpness = t.MustGetFloat();
	t.Next();
	t.AssertIdentifier("Colors");
	t.Next();
	pimpl->m_CloudColors.LoadFromTokenizer(t);
	t.AssertSymbol('}');
	t.Next();

	t.AssertSymbol('}');
	t.Next();

	pimpl->m_CurrentBackgroundColorsCalculated = false;
}

float ComplexSky::GetTime()
{
	return pimpl->m_Time;
}

void ComplexSky::SetTime(float Time)
{
	pimpl->m_Time = Time;
	pimpl->m_CurrentBackgroundColorsCalculated = false;
	pimpl->m_SunCache.TimeChanged();
	for (uint mi = 0; mi < GetMoonCount(); mi++)
		pimpl->m_MoonCache[mi].TimeChanged();
}

const ComplexSky::BACKGROUND_DESC & ComplexSky::GetBackgroundDesc()
{
	return pimpl->m_BackgroundDesc;
}

void ComplexSky::SetBackgroundDesc(const BACKGROUND_DESC &BackgroundDesc)
{
	pimpl->m_BackgroundDesc = BackgroundDesc;
	pimpl->m_CurrentBackgroundColorsCalculated = false;
}

const ComplexSky::CELESTIAL_OBJECT_DESC & ComplexSky::GetSunDesc()
{
	return pimpl->m_SunDesc;
}

const COLORF & ComplexSky::GetBackgroundWeatherFactor()
{
	return pimpl->m_BackgroundWeatherFactor;
}

void ComplexSky::SetBackgroundWeatherFactor(const COLORF &BackgroundWeatherFactor)
{
	pimpl->m_BackgroundWeatherFactor = BackgroundWeatherFactor;
}

void ComplexSky::SetSunDesc(const CELESTIAL_OBJECT_DESC &Desc)
{
	pimpl->m_SunDesc = Desc;
	pimpl->m_SunCache.ResetAll();
}

const VEC3 & ComplexSky::GetCurrentSunDir()
{
	pimpl->EnsureCelestialObjectDir(pimpl->m_SunCache, pimpl->m_SunDesc);
	return pimpl->m_SunCache.m_Dir;
}

const COLORF & ComplexSky::GetCurrentSunColor()
{
	pimpl->EnsureCelestialObjectColor(pimpl->m_SunCache, pimpl->m_SunDesc);
	return pimpl->m_SunCache.m_Color;
}

uint ComplexSky::GetMoonCount()
{
	return pimpl->m_MoonDesc.size();
}

const ComplexSky::CELESTIAL_OBJECT_DESC & ComplexSky::GetMoonDesc(uint Index)
{
	assert(Index < GetMoonCount());
	return pimpl->m_MoonDesc[Index];
}

void ComplexSky::SetMoonDesc(uint Index, CELESTIAL_OBJECT_DESC &Desc)
{
	assert(Index < GetMoonCount());
	pimpl->m_MoonDesc[Index] = Desc;
	pimpl->m_MoonCache[Index].ResetAll();
}

void ComplexSky::AddMoon(CELESTIAL_OBJECT_DESC &Desc)
{
	pimpl->m_MoonDesc.push_back(Desc);

	CELESTIAL_OBJECT_CACHE Cache;
	Cache.ResetAll();
	pimpl->m_MoonCache.push_back(Cache);
}

void ComplexSky::RemoveMoon(uint Index)
{
	assert(Index < GetMoonCount());
	pimpl->m_MoonDesc.erase(pimpl->m_MoonDesc.begin() + Index);
	pimpl->m_MoonCache.erase(pimpl->m_MoonCache.begin() + Index);
}

void ComplexSky::ClearMoons()
{
	pimpl->m_MoonDesc.clear();
	pimpl->m_MoonCache.clear();
}

const VEC3 & ComplexSky::GetCurrentMoonDir(uint Index)
{
	assert(Index < GetMoonCount());
	pimpl->EnsureCelestialObjectDir(pimpl->m_MoonCache[Index], pimpl->m_MoonDesc[Index]);
	return pimpl->m_MoonCache[Index].m_Dir;
}

float ComplexSky::GetCloudsThreshold()
{
	return pimpl->m_CloudsThreshold;
}

void ComplexSky::SetCloudsThreshold(float CloudsThreshold)
{
	pimpl->m_CloudsThreshold = CloudsThreshold;
}

float ComplexSky::GetCloudsSharpness()
{
	return pimpl->m_CloudsSharpness;
}

void ComplexSky::SetCloudsSharpness(float CloudsSharpness)
{
	pimpl->m_CloudsSharpness = CloudsSharpness;
}

const ComplexSky::CLOUD_COLORS & ComplexSky::GetCloudColors()
{
	return pimpl->m_CloudColors;
}

void ComplexSky::SetCloudColors(const CLOUD_COLORS &CloudColors)
{
	pimpl->m_CloudColors = CloudColors;
}

const COLORF_PAIR & ComplexSky::GetCloudColorWeatherFactors()
{
	return pimpl->m_CloudColorWeatherFactors;
}

void ComplexSky::SetCloudColorWeatherFactors(const COLORF_PAIR &CloudColorWeatherFactors)
{
	pimpl->m_CloudColorWeatherFactors = CloudColorWeatherFactors;
}

float ComplexSky::GetCloudsScaleInv()
{
	return pimpl->m_CloudsScaleInv;
}

void ComplexSky::SetCloudsScaleInv(float CloudsScaleInv)
{
	pimpl->m_CloudsScaleInv = CloudsScaleInv;
}

const COLORF & ComplexSky::GetCurrentHorizonColor()
{
	pimpl->EnsureCurrentBackgroundColors();
	return pimpl->m_CurrentHorizonColor;
}

const COLORF & ComplexSky::GetCurrentZenithColor()
{
	pimpl->EnsureCurrentBackgroundColors();
	return pimpl->m_CurrentZenithColor;
}

void ComplexSky::Draw(const ParamsCamera &Cam)
{
	assert(SKYDOME_RADIUS > Cam.GetZNear() && SKYDOME_RADIUS < Cam.GetZFar() || "SKYDOME_RADIUS poza zakresem ZNear-ZFar kamery.");

	MATRIX World;
	Translation(&World, Cam.GetEyePos());

	pimpl->DrawSkydome(Cam, World);
	pimpl->DrawCelestialObjects(Cam, World);

	if (pimpl->m_CloudsThreshold < 0.99f)
		pimpl->DrawClouds(Cam, World);
}

bool ComplexSky::GetSunDir(VEC3 *OutSunDir)
{
	*OutSunDir = GetCurrentSunDir();
	return true;
}


} // namespace engine
