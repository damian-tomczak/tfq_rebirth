/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\Res_d3d.hpp"
#include "..\Framework\D3dUtils.hpp"
#include "Engine.hpp"
#include "Camera.hpp"
#include "Terrain.hpp"
#include "Grass.hpp"

namespace engine
{

// Opisuje co oznacza dany kana� (R, G, B) na mapie rozmieszczenia trawy.
// Czyli odpowiadaj�ce mu rodzaje trawy i ich intensywno��.
// first = indeks rodzaju trawy w m_Kinds
// second = liczba �d�be� trawy danego rodzaju przy pe�nej intensywno�ci kana�u
typedef std::vector< std::pair<uint, uint> > GRASS_DENSITY_MAP_CHANNEL;

// Rozmiar segmentu w wierzcho�kach terenu.
const uint PATCH_SIZE = 16;
// Pojemno�� bufora VB i IB w quadach
const uint BUFFER_QUAD_COUNT = PATCH_SIZE * PATCH_SIZE * 10 * 5;
const uint BUFFER_VERTEX_COUNT = BUFFER_QUAD_COUNT * 4;
const uint BUFFER_INDEX_COUNT = BUFFER_QUAD_COUNT * 6;

const float WIND_FREQ_FACTOR = 2.0f;
const float WIND_AMPLITUDE_FACTOR = 1.0f;
const float FADE_START_FACTOR = 0.5f;

// U�ywane jako identyfikatory w pliku GrassDesc.dat.
const char * CHANNEL_NAMES[] = { "R", "G", "B" };

const uint FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);

// Dokumentacja znaczenia p�l sk�adowej Diffuse - patrz Trees.txt.
struct VERTEX
{
	VEC3 Pos;
	COLOR Diffuse;
	VEC2 Tex;
};

enum GRASS_SHADER_PARAM
{
	GSP_TEXTURE = 0,
	GSP_VIEW_PROJ,
	GSP_RIGHT_DIR,
	GSP_UP_DIR,
	GSP_WIND_FACTORS,
	GSP_COLOR,
	GSP_FADE_FACTORS,
	GSP_NOISE_TEXTURE,
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktura GRASS_KIND

// Opisuje pojedynczy rodzaj trawy
/*
Wszystkie rodzaje korzystaj� zawsze z jednej tekstury, by� mo�e z r�nych jej miejsc.
Nie ma mo�liwo�ci zmiany koloru trawy na etapie rodzaju trawy.
*/
struct GRASS_KIND
{
	float HalfCX, HalfCXV;
	float HalfCY, HalfCYV;
	RECTF Tex;
	float Wind;

	void LoadFromTokenizer(Tokenizer &tok, uint TextureCX, uint TextureCY);
};

void GRASS_KIND::LoadFromTokenizer(Tokenizer &tok, uint TextureCX, uint TextureCY)
{
	tok.AssertSymbol('{');
	tok.Next();

	tok.AssertIdentifier("CX");
	tok.Next();
	tok.AssertSymbol('=');
	tok.Next();
	this->HalfCX = tok.MustGetFloat();
	tok.Next();
	tok.AssertSymbol(',');
	tok.Next();
	this->HalfCXV = tok.MustGetFloat();
	tok.Next();

	tok.AssertIdentifier("CY");
	tok.Next();
	tok.AssertSymbol('=');
	tok.Next();
	this->HalfCY = tok.MustGetFloat();
	tok.Next();
	tok.AssertSymbol(',');
	tok.Next();
	this->HalfCYV = tok.MustGetFloat();
	tok.Next();

	tok.AssertIdentifier("Tex");
	tok.Next();
	tok.AssertSymbol('=');
	tok.Next();

	this->Tex.left = ( (float)tok.MustGetInt4() + 0.5f ) / (float)TextureCX;
	tok.Next();
	tok.AssertSymbol(',');
	tok.Next();
	this->Tex.top = ( (float)tok.MustGetInt4() + 0.5f ) / (float)TextureCY;
	tok.Next();
	tok.AssertSymbol(',');
	tok.Next();
	this->Tex.right = ( (float)tok.MustGetInt4() + 0.5f ) / (float)TextureCX;
	tok.Next();
	tok.AssertSymbol(',');
	tok.Next();
	this->Tex.bottom = ( (float)tok.MustGetInt4() + 0.5f ) / (float)TextureCY;
	tok.Next();

	tok.AssertIdentifier("Wind");
	tok.Next();
	tok.AssertSymbol('=');
	tok.Next();
	this->Wind = tok.MustGetFloat();
	tok.Next();

	tok.AssertSymbol('}');
	tok.Next();
}

typedef std::vector<GRASS_KIND> GRASS_KIND_VECTOR;

inline uint1 QuadBias_FloatToByte(float x)
{
	return (uint1)(uint)minmax<int>(0, roundo((x * 0.25f + 0.5f) * 255.0f), 255);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Grass_pimpl

// Deskryptor pojedynczego sektora w buforze VB i IB.
struct BUFFER_PATCH_DESC
{
	uint PatchX, PatchZ;
	uint FirstQuad;
	uint QuadCount;
};

class Grass_pimpl
{
public:
	Scene *m_OwnerScene;
	Terrain *m_TerrainObj;
	res::D3dEffect *m_Shader; // Pobierany przy pierwszym u�yciu
	res::D3dTexture *m_Texture; // Pobierany przy pierwszym u�yciu
	res::D3dTexture *m_NoiseTexture; // Pobierany przy pierwszym u�yciu
	float m_WindSinFactor1, m_WindSinFactor2;

	////// Dane wczytane z pliku tekstowego GrassDesc
	string m_TextureName;
	uint m_TextureCX, m_TextureCY;
	GRASS_KIND_VECTOR m_Kinds;
	GRASS_DENSITY_MAP_CHANNEL m_DensityMapChannels[3];

	////// Dane wczytane z obrazka DensityMap
	// Ka�demu pikselowi Density Map odpowiadaj� trzy bajty - odpowiednio R, G, B
	uint m_DensityMapCX;
	uint m_DensityMapCY;
	// Liczba patch�w na X i na Z
	uint m_PatchesX;
	uint m_PatchesZ;
	std::vector<uint1> m_DensityMap;
	/*
	Bufory przechowuj� odpowiadaj�ce sobie wierzcho�ki i indeksy kolejnych quad�w.
	Wszelkie pozycje i d�ugo�ci w tych buforach s� merzone w quadach.
	Ka�demu quadowi odpowiadaj� 4 wierzcho�ki i 6 indeks�w.
	Bufory tworzy sekwencja geometrii trawy dla r�nych sektor�w (Patch), ka�da r�nej d�ugo�ci.
	Indeksy wierzcho�k�w w IB s� wzgl�dem pocz�tku danego patcha, nie wzgl�dem pocz�tku ca�ego bufora VB.
	*/
	scoped_ptr<res::D3dVertexBuffer> m_VB;
	scoped_ptr<res::D3dIndexBuffer> m_IB;
	std::vector<BUFFER_PATCH_DESC> m_BufferDescs;
	uint m_CurrentBufferQuad;
	// Tablica dla ka�dego sektora przechowuje warto�� logiczn� ustawion� na true, je�li wiadomo ju� o nim, �e na pewno jest pusty.
	// Jesli nie jest pusty lub jeszcze nie by� wyliczany, jest false.
	std::vector<bool> m_EmptyPatches;

	void CreateBuffers();
	void DestroyBuffers();
	void LoadGrassDesc(const string &DescFileName);
	void LoadDensityMap(const string &DensityMapFileName);

	void DrawBegin(const ParamsCamera &Cam, const COLORF &Color, float GrassZFar);
	void DrawEnd();
	// Wywo�ywa� mi�dzy DrawBegin i DrawEnd
	void DrawPatch(uint px, uint pz);

private:
	// Zapewnia istnienie i zwraca deskryptow segmentu trawy w buforze.
	// Je�li nie ma nic do rysowania w tym segmencie, zwraca NULL.
	// Zwr�cony wska�nik jest wa�ny tylko do nast�pnego wywo�ania EnsurePatch.
	const BUFFER_PATCH_DESC * EnsurePatch(uint px, uint pz);
	// Jak wy�ej, ale bezwzgl�dnie generuje nowy segment i zwraca jego wska�nik lub NULL je�li pusty.
	const BUFFER_PATCH_DESC * GeneratePatch(uint px, uint pz);
};

void Grass_pimpl::CreateBuffers()
{
	m_VB.reset(new res::D3dVertexBuffer(string(), string(), BUFFER_VERTEX_COUNT, D3DUSAGE_WRITEONLY, FVF, D3DPOOL_MANAGED));
	m_IB.reset(new res::D3dIndexBuffer(string(), string(), BUFFER_INDEX_COUNT, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED));
	m_VB->Lock();
	m_IB->Lock();

	m_CurrentBufferQuad = 0;
}

void Grass_pimpl::DestroyBuffers()
{
	m_IB->Unlock();
	m_VB->Unlock();

	m_IB.reset();
	m_VB.reset();
}

void Grass_pimpl::LoadGrassDesc(const string &DescFileName)
{
	ERR_TRY;

	FileStream GrassDescFile(DescFileName, FM_READ);
	Tokenizer tok(&GrassDescFile, 0);
	tok.Next();

	////// Nag��wek

	tok.AssertIdentifier("GrassDesc");
	tok.Next();

	////// Tekstura

	tok.AssertIdentifier("Texture");
	tok.Next();
	tok.AssertSymbol('{');
	tok.Next();

	tok.AssertToken(Tokenizer::TOKEN_STRING);
	m_TextureName = tok.GetString();
	tok.Next();
	m_TextureCX = tok.MustGetUint4();
	tok.Next();
	tok.AssertSymbol(',');
	tok.Next();
	m_TextureCY = tok.MustGetUint4();
	tok.Next();

	tok.AssertSymbol('}');
	tok.Next();

	////// Definicje rodzaj�w traw

	typedef std::map<string, uint> STRING_UINT_MAP;
	// Odwzorowuje nazwy gatunk�w traw na indeksy do m_Kinds
	STRING_UINT_MAP GrassNames;
	GRASS_KIND Kind;
	string Name;

	tok.AssertIdentifier("Grasses");
	tok.Next();
	tok.AssertSymbol('{');
	tok.Next();
	while (!tok.QuerySymbol('}'))
	{
		tok.AssertToken(Tokenizer::TOKEN_STRING);
		Name = tok.GetString();
		tok.Next();
		Kind.LoadFromTokenizer(tok, m_TextureCX, m_TextureCY);

		m_Kinds.push_back(Kind);
		GrassNames.insert(STRING_UINT_MAP::value_type(Name, m_Kinds.size()-1));
	}
	tok.Next();

	////// Kolory do DensityMap

	STRING_UINT_MAP::iterator mit;
	uint Count;

	tok.AssertIdentifier("DensityMap");
	tok.Next();
	tok.AssertSymbol('{');
	tok.Next();
	for (uint channel_i = 0; channel_i < 3; channel_i++)
	{
		tok.AssertIdentifier(CHANNEL_NAMES[channel_i]);
		tok.Next();
		tok.AssertSymbol('{');
		tok.Next();
		while (!tok.QuerySymbol('}'))
		{
			tok.AssertToken(Tokenizer::TOKEN_STRING);
			Name = tok.GetString();
			tok.Next();

			mit = GrassNames.find(Name);
			if (mit == GrassNames.end())
				throw Error(Format("Nie znaleziono rodzaju trawy \"#\".") % Name);

			Count = tok.MustGetUint4();
			tok.Next();

			m_DensityMapChannels[channel_i].push_back(GRASS_DENSITY_MAP_CHANNEL::value_type(mit->second, Count));

			if (!tok.QuerySymbol('}'))
			{
				tok.AssertSymbol(',');
				tok.Next();
			}
		}
		tok.Next();
	}
	tok.AssertSymbol('}');
	tok.Next();

	ERR_CATCH(Format("Nie mo�na wczyta� opisu trawy z \"#\".") % DescFileName);
}

void Grass_pimpl::LoadDensityMap(const string &DensityMapFileName)
{
	ERR_TRY;

	MemoryTexture tex(DensityMapFileName);
	m_DensityMapCX = tex.GetWidth();
	m_DensityMapCY = tex.GetHeight();
	m_PatchesX = ceil_div<uint>(m_DensityMapCX, PATCH_SIZE);
	m_PatchesZ = ceil_div<uint>(m_DensityMapCY, PATCH_SIZE);
	if (m_DensityMapCX == 0 || m_DensityMapCY == 0)
		throw Error("Mapa rozmieszczenia trawy nie mo�e by� pusta.", __FILE__, __LINE__);
	m_DensityMap.resize(m_DensityMapCX * m_DensityMapCY * 3);
	m_EmptyPatches.resize(m_PatchesX * m_PatchesZ);
	for (uint pi = 0; pi < m_PatchesX * m_PatchesZ; pi++)
		m_EmptyPatches[pi] = false;

	uint x, y, density_map_i = 0;
	const COLOR *Row;
	for (y = 0; y < m_DensityMapCY; y++)
	{
		Row = tex.GetRowData(y);
		for (x = 0; x < m_DensityMapCX; x++)
		{
			m_DensityMap[density_map_i++] = Row[x].R;
			m_DensityMap[density_map_i++] = Row[x].G;
			m_DensityMap[density_map_i++] = Row[x].B;
		}
	}

	ERR_CATCH(Format("Nie mo�na wczyta� mapy rozmieszczenia trawy z \"#\".") % DensityMapFileName);
}

void Grass_pimpl::DrawBegin(const ParamsCamera &Cam, const COLORF &Color, float GrassZFar)
{
	frame::Dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
	frame::Dev->SetRenderState(D3DRS_ALPHAREF, 0x00000080);

	frame::Dev->SetStreamSource(0, m_VB->GetVB(), 0, sizeof(VERTEX));
	frame::Dev->SetIndices(m_IB->GetIB());
	frame::Dev->SetFVF(FVF);

	if (m_Shader == NULL)
		m_Shader = res::g_Manager->MustGetResourceEx<res::D3dEffect>("GrassShader");
	m_Shader->Load();

	if (m_Texture == NULL)
		m_Texture = res::g_Manager->MustGetResourceEx<res::D3dTexture>(m_TextureName);
	m_Texture->Load();

	if (m_NoiseTexture == NULL)
		m_NoiseTexture = res::g_Manager->MustGetResourceEx<res::D3dTexture>("NoiseTexture");
	m_NoiseTexture->Load();

	const VEC3 &RightDir = Cam.GetRightDir();
	VEC3 UpDir = Cam.GetRealUpDir();
	// Haczyk na �adniejsze roz�o�enie billboard�w trawy
	UpDir += VEC3::POSITIVE_Y;
	Normalize(&UpDir);

	VEC2 WindFactors;
	{
		float dt = frame::Timer2.GetDeltaTime();
		const float WindC1 = 0.5f, WindC2 = 0.5f; // Maj� si� sumowa� do 1.

		float WindVecLength = Length(m_OwnerScene->GetWindVec());
		float WindFreq = WindVecLength * WIND_FREQ_FACTOR;
		m_WindSinFactor1 += dt * (PI_X_2 * WindFreq * 1.05f);
		m_WindSinFactor2 += dt * (PI_X_2 * WindFreq * 1.05f);
		WindFactors.x = ( ( WindC1 * sinf(m_WindSinFactor1) + WindC2 * sinf(0.7f * m_WindSinFactor1) ) ) * WindVecLength * WIND_AMPLITUDE_FACTOR;
		WindFactors.y = ( ( WindC1 * sinf(m_WindSinFactor2) + WindC2 * sinf(0.7f * m_WindSinFactor2) ) ) * WindVecLength * WIND_AMPLITUDE_FACTOR;
	}

	// Te wsp�czynniki r�wnania liniowego s� tak dobrane, aby by�o:
	// dla A*z + B dla z = GrassZFar wynosi -1
	// dla A*z + B dla z = GrassZFar*FADE_START_FACTOR wynosi 1
	float FadeFactorA, FadeFactorB;
	FadeFactorA = 2.0f / (GrassZFar * (FADE_START_FACTOR - 1.0f));
	FadeFactorB = - (FADE_START_FACTOR + 1.0f) / (FADE_START_FACTOR - 1.0f);

	ERR_GUARD_DIRECTX_D( m_Shader->Get()->SetTexture(m_Shader->GetParam(GSP_TEXTURE), m_Texture->GetBaseTexture()) );
	ERR_GUARD_DIRECTX_D( m_Shader->Get()->SetMatrix(m_Shader->GetParam(GSP_VIEW_PROJ), (const D3DXMATRIX*)&Cam.GetMatrices().GetViewProj()) );
	ERR_GUARD_DIRECTX_D( m_Shader->Get()->SetVector(m_Shader->GetParam(GSP_RIGHT_DIR), &D3DXVECTOR4(RightDir.x, RightDir.y, RightDir.z, 0.0f)) );
	ERR_GUARD_DIRECTX_D( m_Shader->Get()->SetVector(m_Shader->GetParam(GSP_UP_DIR), &D3DXVECTOR4(UpDir.x, UpDir.y, UpDir.z, 0.0f)) );
	ERR_GUARD_DIRECTX_D( m_Shader->Get()->SetVector(m_Shader->GetParam(GSP_WIND_FACTORS), &D3DXVECTOR4(WindFactors.x, WindFactors.y, 0.0f, 0.0f)) );
	ERR_GUARD_DIRECTX_D( m_Shader->Get()->SetVector(m_Shader->GetParam(GSP_COLOR), &D3DXVECTOR4(Color.R, Color.G, Color.B, 1.0f)) );
	ERR_GUARD_DIRECTX_D( m_Shader->Get()->SetVector(m_Shader->GetParam(GSP_FADE_FACTORS), &D3DXVECTOR4(FadeFactorA, FadeFactorB, 0.0f, 0.0f)) );
	ERR_GUARD_DIRECTX_D( m_Shader->Get()->SetTexture(m_Shader->GetParam(GSP_NOISE_TEXTURE), m_NoiseTexture->GetBaseTexture()) );

	m_Shader->Begin(false);
}

void Grass_pimpl::DrawEnd()
{
	assert(m_Shader != NULL);

	m_Shader->End();
}

void Grass_pimpl::DrawPatch(uint px, uint pz)
{
	const BUFFER_PATCH_DESC * Desc = EnsurePatch(px, pz);

	if (Desc != NULL)
	{
		uint PrimitiveCount = Desc->QuadCount * 2;
		ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(
			D3DPT_TRIANGLELIST, // PrimitiveType
			Desc->FirstQuad * 4, // BaseVertexIndex
			0, // MinVertexIndex
			Desc->QuadCount * 4, // NumVertices
			Desc->FirstQuad * 6, // StartIndex
			PrimitiveCount) );
		frame::RegisterDrawCall(PrimitiveCount);
	}
}

const BUFFER_PATCH_DESC * Grass_pimpl::EnsurePatch(uint px, uint pz)
{
	// Je�li wiadomo o nim, �e jest pusty
	if (m_EmptyPatches[pz * m_PatchesX + px])
		return NULL;

	// Poszukaj, mo�e ju� taki jest
	for (uint pdi = 0; pdi < m_BufferDescs.size(); pdi++)
		if (m_BufferDescs[pdi].PatchX == px && m_BufferDescs[pdi].PatchZ == pz)
			return &m_BufferDescs[pdi];

	// Nie ma - trzeba wygenerowa�
	return GeneratePatch(px, pz);
}

const BUFFER_PATCH_DESC * Grass_pimpl::GeneratePatch(uint px, uint pz)
{
	ERR_TRY;

	RandomGenerator Rand(px + pz);

	// Tu b�d� pozycje wszystkich quad�w podanego patcha dla poszczeg�lnych poziom�w trawy.
	uint KindCount = m_Kinds.size();
	float VertexDistance = m_TerrainObj->GetVertexDistance();
	std::vector< std::vector<VEC3> > Blades(KindCount);
	// Tu b�dzie liczba �d�be� w ca�ym wektorze w sumie
	uint SumBladeCount = 0;

	// Dla ka�dego wierzcho�ka terenu
	{
		uint x1 = px * PATCH_SIZE;
		uint z1 = pz * PATCH_SIZE;
		uint x2 = std::min(x1 + PATCH_SIZE, m_DensityMapCX);
		uint z2 = std::min(z1 + PATCH_SIZE, m_DensityMapCY);
		float WorldOffsetX, WorldOffsetZ;
		uint x, z, channel_kind_i, kind_i, blade_i;
		VEC3 pos;
		const uint1 *DensityMapData;
		// Tu b�dzie pami�tane dla bie��cego wierzcho�ka, ile ma by� �d�be� trawy poszczeg�lnych rodzaj�w
		std::vector<uint> TreeKindCount(KindCount);

		for (z = z1; z < z2; z++)
		{
			WorldOffsetZ = (float)z * VertexDistance;
			DensityMapData = &m_DensityMap[(z * m_DensityMapCX + x1) * 3];

			for (x = x1; x < x2; x++)
			{
				WorldOffsetX = (float)x * VertexDistance;

				ZeroMemory(&TreeKindCount[0], KindCount * sizeof(uint));

				for (channel_kind_i = 0; channel_kind_i < m_DensityMapChannels[0].size(); channel_kind_i++)
					TreeKindCount[m_DensityMapChannels[0][channel_kind_i].first] += m_DensityMapChannels[0][channel_kind_i].second * (*DensityMapData) / 255;
				DensityMapData++;

				for (channel_kind_i = 0; channel_kind_i < m_DensityMapChannels[1].size(); channel_kind_i++)
					TreeKindCount[m_DensityMapChannels[1][channel_kind_i].first] += m_DensityMapChannels[1][channel_kind_i].second * (*DensityMapData) / 255;
				DensityMapData++;

				for (channel_kind_i = 0; channel_kind_i < m_DensityMapChannels[2].size(); channel_kind_i++)
					TreeKindCount[m_DensityMapChannels[2][channel_kind_i].first] += m_DensityMapChannels[2][channel_kind_i].second * (*DensityMapData) / 255;
				DensityMapData++;

				for (kind_i = 0; kind_i < KindCount; kind_i++)
				{
					assert(TreeKindCount[kind_i] <= POISSON_DISC_2D_COUNT);
					for (blade_i = 0; blade_i < TreeKindCount[kind_i]; blade_i++)
					{
						pos.x = frac(POISSON_DISC_2D[blade_i].x + x*0.234f + kind_i*0.434f + z*0.432f) * VertexDistance + WorldOffsetX;
						pos.z = frac(POISSON_DISC_2D[blade_i].y + x*0.356f + kind_i*0.346f + z*0.125f) * VertexDistance + WorldOffsetZ;
						// Losowe przesuni�cie
						pos.y = m_TerrainObj->GetHeight(pos.x, pos.z);
						Blades[kind_i].push_back(pos);
						SumBladeCount++;
					}
				}
			}
		}
	}

	// Je�li brak �d�be� - koniec
	if (SumBladeCount == 0)
	{
		// Zapami�taj na przysz�o��, �e ten sektor jest pusty, �eby go od nowa nie liczy�
		m_EmptyPatches[pz * m_PatchesX + px] = true;
		return NULL;
	}

	// Czy w og�le zmiesci si� w buforze
	assert(SumBladeCount < BUFFER_QUAD_COUNT);
	// Nie zmie�ci si� ju� w dalszym ci�gu bufora - trzeba od pocz�tku
	if (SumBladeCount > BUFFER_QUAD_COUNT - m_CurrentBufferQuad)
		m_CurrentBufferQuad = 0;
	// Utw�rz deskryptor
	BUFFER_PATCH_DESC BufferPatchDesc;
	BufferPatchDesc.PatchX = px;
	BufferPatchDesc.PatchZ = pz;
	BufferPatchDesc.FirstQuad = m_CurrentBufferQuad;
	BufferPatchDesc.QuadCount = SumBladeCount;
	m_CurrentBufferQuad += SumBladeCount;
	// Usu� deskryptory wszystkich segment�w zachodz�cych na ten nowy
	for (uint desc_i = m_BufferDescs.size(); desc_i--; )
	{
		// Wz�r na zachodzenie dw�ch zakres�w
		if (BufferPatchDesc.FirstQuad < m_BufferDescs[desc_i].FirstQuad + m_BufferDescs[desc_i].QuadCount &&
			m_BufferDescs[desc_i].FirstQuad < BufferPatchDesc.FirstQuad + BufferPatchDesc.QuadCount)
		{
			m_BufferDescs.erase(m_BufferDescs.begin() + desc_i);
		}
	}

	// Zablokuj i wype�nij VB
	{
		uint OffsetToLockInVertices = BufferPatchDesc.FirstQuad * 4;
		uint SizeToLockInVertices = BufferPatchDesc.QuadCount * 4;
		res::D3dVertexBuffer::Locker VbLocker(m_VB.get(), OffsetToLockInVertices, SizeToLockInVertices, 0);
		VERTEX *VertexPtr = (VERTEX*)VbLocker.GetData();
		// Wygeneruj quady
		uint blade_i;
		uint1 Random1, Random2;
		float MyHalfCX, MyHalfCY;
		uint1 QuadBiasX1, QuadBiasX2, QuadBiasY1, QuadBiasY2;
		RECTF TexRect;
		for (uint kind_i = 0; kind_i < KindCount; kind_i++)
		{
			const GRASS_KIND &KindDesc = m_Kinds[kind_i];
			TexRect = KindDesc.Tex;

			for (blade_i = 0; blade_i < Blades[kind_i].size(); blade_i++)
			{
				if (Rand.RandBool())
					std::swap(TexRect.left, TexRect.right);
				Random1 = QuadBias_FloatToByte(Rand.RandFloat(-1.0f, 1.0f) * KindDesc.Wind);
				Random2 = QuadBias_FloatToByte(Rand.RandFloat(-1.0f, 1.0f) * KindDesc.Wind);
				MyHalfCX = KindDesc.HalfCX + Rand.RandFloat(- KindDesc.HalfCXV, KindDesc.HalfCXV);
				MyHalfCY = KindDesc.HalfCY + Rand.RandFloat(- KindDesc.HalfCYV, KindDesc.HalfCYV);
				QuadBiasX1 = QuadBias_FloatToByte(- MyHalfCX);
				QuadBiasX2 = QuadBias_FloatToByte(  MyHalfCX);
				QuadBiasY1 = QuadBias_FloatToByte(0.0f);
				QuadBiasY2 = QuadBias_FloatToByte(2.0f * MyHalfCY);

				VertexPtr->Pos = Blades[kind_i][blade_i];
				VertexPtr->Diffuse.R = QuadBiasX1;
				VertexPtr->Diffuse.G = QuadBiasY1;
				VertexPtr->Diffuse.B = 127;
				VertexPtr->Diffuse.A = 127;
				VertexPtr->Tex.x = TexRect.left;
				VertexPtr->Tex.y = TexRect.bottom;
				VertexPtr++;
				VertexPtr->Pos = Blades[kind_i][blade_i];
				VertexPtr->Diffuse.R = QuadBiasX1;
				VertexPtr->Diffuse.G = QuadBiasY2;
				VertexPtr->Diffuse.B = Random1;
				VertexPtr->Diffuse.A = Random2;
				VertexPtr->Tex.x = TexRect.left;
				VertexPtr->Tex.y = TexRect.top;
				VertexPtr++;
				VertexPtr->Pos = Blades[kind_i][blade_i];
				VertexPtr->Diffuse.R = QuadBiasX2;
				VertexPtr->Diffuse.G = QuadBiasY2;
				VertexPtr->Diffuse.B = Random1;
				VertexPtr->Diffuse.A = Random2;
				VertexPtr->Tex.x = TexRect.right;
				VertexPtr->Tex.y = TexRect.top;
				VertexPtr++;
				VertexPtr->Pos = Blades[kind_i][blade_i];
				VertexPtr->Diffuse.R = QuadBiasX2;
				VertexPtr->Diffuse.G = QuadBiasY1;
				VertexPtr->Diffuse.B = 127;
				VertexPtr->Diffuse.A = 127;
				VertexPtr->Tex.x = TexRect.right;
				VertexPtr->Tex.y = TexRect.bottom;
				VertexPtr++;
			}
		}
	}

	// Zablokuj i wype�nij IB
	{
		uint OffsetToLockInIndices = BufferPatchDesc.FirstQuad * 6;
		uint SizeToLockInIndices = BufferPatchDesc.QuadCount * 6;
		res::D3dIndexBuffer::Locker IbLocker(m_IB.get(), OffsetToLockInIndices, SizeToLockInIndices, 0);
		uint2 *IndexPtr = (uint2*)IbLocker.GetData();
		// Wygeneruj indeksy quad�w
		uint vertex_index = 0;
		for (uint blade_i = 0; blade_i < SumBladeCount; blade_i++)
		{
			*IndexPtr = vertex_index;
			IndexPtr++;
			*IndexPtr = vertex_index + 1;
			IndexPtr++;
			*IndexPtr = vertex_index + 2;
			IndexPtr++;
			*IndexPtr = vertex_index;
			IndexPtr++;
			*IndexPtr = vertex_index + 2;
			IndexPtr++;
			*IndexPtr = vertex_index + 3;
			IndexPtr++;

			vertex_index += 4;
		}
	}

	// Dodaj ten nowy deskryptor
	m_BufferDescs.push_back(BufferPatchDesc);
	// Zwr�� nowy deskryptor (oczywi�cie z tablicy, a nie t� zmienn� lokaln�)
	return &m_BufferDescs[m_BufferDescs.size()-1];

	ERR_CATCH_FUNC;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Grass

Grass::Grass(Scene *OwnerScene, Terrain *TerrainObj, const string &DescFileName, const string &DensityMapFileName) :
	pimpl(new Grass_pimpl)
{
	ERR_TRY;

	pimpl->m_OwnerScene = OwnerScene;
	pimpl->m_TerrainObj = TerrainObj;
	pimpl->m_Shader = NULL;
	pimpl->m_Texture = NULL;
	pimpl->m_NoiseTexture = NULL;
	pimpl->m_WindSinFactor1 = 2.253f;
	pimpl->m_WindSinFactor2 = -1.343f;

	LOG(LOG_ENGINE, Format("Grass: Loading. Desc=\"#\", DensityMap=\"#\"") % DescFileName % DensityMapFileName);

	pimpl->CreateBuffers();
	pimpl->LoadGrassDesc(DescFileName);
	pimpl->LoadDensityMap(DensityMapFileName);

	ERR_CATCH("Nie mo�na wczyta� danych trawy.");
}

Grass::~Grass()
{
	pimpl->DestroyBuffers();
}

void Grass::Draw(const ParamsCamera &Cam, float GrassZFar, const COLORF &Color)
{
	// U�� now�, ograniczon� kamer�
	ParamsCamera SecondCamera;
	const ParamsCamera *CamPtr;
	if (GrassZFar < Cam.GetZFar())
	{
		SecondCamera = Cam;
		SecondCamera.SetZFar(GrassZFar);
		CamPtr = &SecondCamera;
	}
	else
	{
		// SecondCamera zostaje niezainicjalizowana
		CamPtr = &Cam;
	}

	const BOX &NewCamBox = CamPtr->GetMatrices().GetFrustumBox();

	pimpl->DrawBegin(Cam, Color, GrassZFar); // Nie CamPtr!!!

	float PatchCX, PatchCZ;
	PatchCX = PatchCZ = PATCH_SIZE * pimpl->m_TerrainObj->GetVertexDistance();
	int x1 = minmax(0, roundo(floorf(NewCamBox.p1.x / PatchCX)), (int)pimpl->m_PatchesX-1);
	int z1 = minmax(0, roundo(floorf(NewCamBox.p1.z / PatchCZ)), (int)pimpl->m_PatchesZ-1);
	int x2 = minmax(0, roundo(ceilf(NewCamBox.p2.x / PatchCX)), (int)pimpl->m_PatchesX-1);
	int z2 = minmax(0, roundo(ceilf(NewCamBox.p2.z / PatchCZ)), (int)pimpl->m_PatchesZ-1);
	int x, z;
	for (z = z1; z <= z2; z++)
	{
		for (x = x1; x <= x2; x++)
		{
			pimpl->DrawPatch(x, z);
		}
	}

	pimpl->DrawEnd();
}


} // namespace engine
