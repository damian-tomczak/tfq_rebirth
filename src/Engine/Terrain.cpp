/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\D3dUtils.hpp"
#include "Camera.hpp"
#include "Terrain.hpp"

namespace engine
{

const uint LEVEL_COUNT = 3;
// Musi byæ potêg¹ dwójki.
const uint PATCH_SIZE = 32;
// Ten dodatkowy sk³adnik jest na Skirt.
const uint PATCH_VERTEX_COUNT = (PATCH_SIZE + 1) * (PATCH_SIZE + 1) + (PATCH_SIZE + 1) * 4;
// Ile ostatnio u¿ywanych patchów ma byæ trzymanych w VB
const uint VB_PATCH_COUNT = 16;

const uint PATCH_LEVEL_SIZE[LEVEL_COUNT] = {
	PATCH_SIZE / 1,
	PATCH_SIZE / 2,
	PATCH_SIZE / 4,
};

#define PATCH_LEVEL_INDEX_COUNT(PatchLevelSize) ((PatchLevelSize)*(PatchLevelSize)*6 + (PatchLevelSize)*4*6)

const uint PATCH_INDEX_COUNT =
	PATCH_LEVEL_INDEX_COUNT(PATCH_SIZE / 1) +
	PATCH_LEVEL_INDEX_COUNT(PATCH_SIZE / 2) +
	PATCH_LEVEL_INDEX_COUNT(PATCH_SIZE / 4);

const uint PATCH_INDEX_OFFSET[LEVEL_COUNT] =  {
	0,
	PATCH_LEVEL_INDEX_COUNT(PATCH_SIZE),
	PATCH_LEVEL_INDEX_COUNT(PATCH_SIZE) + PATCH_LEVEL_INDEX_COUNT(PATCH_SIZE / 2),
};

enum TERRAIN_EFFECT_PARAMS
{
	TEP_WORLDVIEWPROJ = 0,
	TEP_TEXTURE0,
	TEP_TEXTURE1,
	TEP_TEXTURE2,
	TEP_TEXTURE3,
	TEP_TEXSCALE,
};

// Liczba przebiegów rozmywania form terenu
const uint BLUE_PASSES = 2;

const string CACHE_FILE_HEADER = "TFQ_TERRAIN_10";


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Terrain_pimpl

struct FORM_DESC_DATA
{
	string TextureName;
	uint Tag;
	float TexScale;
	// Indeks tych danych w tablicy wskaŸników m_FormDescData
	uint Index;
};

// Abstrakcyjna klasa bazowa
struct FORM_DESC_ITEM
{
	COLOR FormMapColor;

	virtual ~FORM_DESC_ITEM() = 0;
};

FORM_DESC_ITEM::~FORM_DESC_ITEM() { }

struct FORM_DESC_ITEM_SIMPLE : public FORM_DESC_ITEM
{
	FORM_DESC_DATA Data;

	virtual ~FORM_DESC_ITEM_SIMPLE() { }
};

struct FORM_DESC_ITEM_COMPLEX : public FORM_DESC_ITEM
{
	// Te tablice maj¹ tak¹ sam¹ d³ugoœæ i ich elementy odpowiadaj¹ sobie - wysokoœæ do danej struktury.
	std::vector<uint1> Height;
	std::vector<FORM_DESC_DATA> Data;

	virtual ~FORM_DESC_ITEM_COMPLEX() { }
};

struct FORM_DESC
{
	std::vector< shared_ptr< FORM_DESC_ITEM > > Items;
};

struct VERTEX
{
	VEC3 Pos;
	VEC3 Normal;
	COLOR Diffuse;

	static const uint FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_NORMAL | D3DFVF_TEX0;
};

struct PATCH
{
	float MinY, MaxY;
	// Tablica bêdzie przechowywa³a indeksy do tablicy Terrain_pimpl::m_FormDescData z formami terenu do u¿ycia w tym patchu.
	uint TerrainForms[TERRAIN_FORMS_PER_PATCH];
	VERTEX Vertices[PATCH_VERTEX_COUNT];
};

struct Terrain_pimpl
{
	// ======== DANE G£ÓWNE ========
	string m_HeightmapFileName;
	string m_FormDescFileName;
	string m_FormMapFileName;
	string m_CacheFileName;
	// Liczba quadów na X i na Z
	// Wierzcho³ków jest zawsze o 1 wiêcej.
	uint m_CX, m_CZ;
	// Odleg³oœæ miêdzy s¹siednimi wierzcho³kami na X i Z
	float m_VertexDistance;
	float m_VertexDistanceInv;
	// Wysokoœæ najni¿szego i najwy¿szego punktu
	float m_MinY, m_MaxY;

	// ======== DANE WCZYTYWANE ========
	// Ma d³ugoœæ (m_CX+1) * (m_CZ+1)
	std::vector<uint1> m_Heightmap;
	// Liczba patchów na X i na Z
	uint m_PatchCX, m_PatchCZ;
	std::vector<PATCH> m_Patches;
	scoped_ptr<IDirect3DIndexBuffer9, ReleasePolicy> m_IB;
	scoped_ptr<IDirect3DVertexBuffer9, ReleasePolicy> m_VB;
	// Indeksy patchów wczytanych do VB, MAXUINT4 jeœli ¿aden
	uint PatchesInVb[VB_PATCH_COUNT];
	// Numer klatki w której u¿ywany by³ dany patch w VB
	uint PatchLastUseFrameInVb[VB_PATCH_COUNT];
	FORM_DESC m_FormDesc;
	// Wektor wskaŸników do forum z m_FormDesc, ale zlinearyzowany, s³u¿¹cy do indeksowania form
	std::vector<const FORM_DESC_DATA*> m_FormDescData;
	// Ta tablica jest dwuwymiarowa, ma rozmiar (CX+1) * (CY+1).
	// Dla ka¿dego wierzcho³ka przechowuje indeks formy terenu do tablicy m_FormDescData.
	std::vector<uint1> m_FormMap;

	// Wersja najszybsza, ale nie sprawdza zakresu
	uint1 GetHeight_Fast(uint x, uint z) { return m_Heightmap[z * (m_CX+1) + x]; }
	// Wersja spawdzaj¹ca zakres
	uint1 GetHeight(uint x, uint z);
	// Wersja operuj¹ca na wspó³rzêdnych lokalnych terenu uwzglêdniaj¹cych na wejœciu VertexDistance, a na wyjœciu MinY i MaxY.
	float GetHeight_Float(float x, float z);
	float GetHeight_Float(uint x, uint z) { return GetHeight(x, z) / 256.0f * (m_MaxY - m_MinY) + m_MinY; }

	// Wczytuje Heightmapê z pliku HeightmapFileName. U¿ywa CX i CZ.
	// OutHeightmap musi byæ tablic¹ (CX+1)*(CZ+1) liczb.
	// Urz¹dzenie D3D istnieje i nie jest utracone - mo¿na go u¿ywaæ.
	void LoadHeightmap(uint1 *OutHeightmap);
	void LoadHeightmap_Raw(uint1 *OutHeightmap);
	void LoadHeightmap_Texture(uint1 *OutHeightmap);
	uint GetFormDataIndex(const FORM_DESC_ITEM *Item, uint1 Height);
	void LoadFormMap();
	void CalcFormWeights(std::vector<uint1> *OutFormWeights);
	void GeneratePatches();
	// Wylicza normalne na podstawie heightmapy. Sam rozszerza podany wektor.
	void CalcNormals(std::vector<VEC3> *OutNormals);
	void GeneratePatch(PATCH *OutPatch, uint StartX, uint StartZ, const std::vector<VEC3> &Normals, const std::vector<uint1> &FormWeights);
	void GenerateIndices();
	void CreateVB();
	// £aduje w razie potrzeby patch o podanym indeksie do VB
	// Zwraca offset do pocz¹tku tego VB do pierwszego wierzcho³ka tego patcha, w wierzcho³kach (BaseVertexIndex)
	uint LoadPatch(uint PatchIndex);
	// Wczytuje opis form terenu z pliku m_FormDescFileName do m_FormDesc
	void LoadFormDesc();
	void LoadFormDesc_SimpleItem(Tokenizer &Tok, FORM_DESC_ITEM_SIMPLE *OutItem);
	void LoadFormDesc_ComplexItem(Tokenizer &Tok, FORM_DESC_ITEM_COMPLEX *OutItem);
	void LoadFormDesc_Data(Tokenizer &Tok, FORM_DESC_DATA *OutData);
	// Zapisuje wygenerowane ju¿ patche terenu do pliku m_CacheFileName
	void WritePatchesToCache();
	// Wczytuje patche z pliku m_CacheFileName do tablicy m_Patches.
	// Tablica musi byæ ju¿ rozszerzona do odpowiedniego rozmiaru.
	void LoadPatchesFromCache();

	bool RayCollision_Patch_Vertical(int px, int pz, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT, float MaxT);
	bool RayCollision_Patch(uint px, uint pz, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT, float StartT, float MaxT);
};

uint1 Terrain_pimpl::GetHeight(uint x, uint z)
{
	if (x > m_CX) return 0;
	if (z > m_CZ) return 0;
	return GetHeight_Fast(x, z);
}

float Terrain_pimpl::GetHeight_Float(float x, float z)
{
	x *= m_VertexDistanceInv;
	z *= m_VertexDistanceInv;
	if (x < 0.0f || z < 0.0f) return 0.0f;
	uint ux1 = (uint)x;
	uint uz1 = (uint)z;
	uint ux2 = ux1 + 1;
	uint uz2 = uz1 + 1;
	float xf = frac(x);
	float zf = frac(z);
	return Lerp(
		Lerp(GetHeight_Float(ux1, uz1), GetHeight_Float(ux2, uz1), xf),
		Lerp(GetHeight_Float(ux1, uz2), GetHeight_Float(ux2, uz2), xf), zf);
}

void Terrain_pimpl::LoadHeightmap_Raw(uint1 *OutHeightmap)
{
	LOG(0x08, Format("Terrain: Loading raw heightmap from \"#\".") % m_HeightmapFileName);

	FileStream File(m_HeightmapFileName, FM_READ);
	File.MustRead(OutHeightmap, (m_CX+1) * (m_CZ+1) * sizeof(uint1));
}

void Terrain_pimpl::LoadHeightmap_Texture(uint1 *OutHeightmap)
{
	LOG(0x08, Format("Terrain: Loading texture heightmap from \"#\".") % m_HeightmapFileName);

	uint VertexCX = m_CX + 1;
	uint VertexCZ = m_CZ + 1;

	MemoryTexture texture(m_HeightmapFileName);

	uint x, z, xc = std::min(VertexCX, texture.GetWidth()), zc = std::min(VertexCZ, texture.GetHeight());
	const DWORD *LockedLine;
	uint1 *HeightmapPtr = OutHeightmap;
	for (z = 0; z < zc; z++)
	{
		LockedLine = (const DWORD*)texture.GetRowData(z);
		for (x = 0; x < xc; x++)
		{
			*HeightmapPtr = (uint1)( LockedLine[x] >> 8 );
			HeightmapPtr++;
		}
		// Wype³nij zerami resztê linii
		if (texture.GetWidth() < VertexCX)
		{
			ZeroMem(HeightmapPtr, (VertexCX - texture.GetWidth()) * sizeof(uint1));
			HeightmapPtr += (VertexCX - texture.GetWidth());
		}
	}
	// Wype³nij zerami pozosta³e linie
	if (texture.GetHeight() < VertexCZ)
		ZeroMem(HeightmapPtr, VertexCX * (VertexCZ - texture.GetHeight()) * sizeof(uint1));
}

void Terrain_pimpl::LoadHeightmap(uint1 *OutHeightmap)
{
	ERR_TRY;

	string HeightmapFileNameU; UpperCase(&HeightmapFileNameU, m_HeightmapFileName);
	string Ext; ExtractFileExt(&Ext, HeightmapFileNameU);

	if (Ext == ".RAW")
		LoadHeightmap_Raw(OutHeightmap);
	else
		LoadHeightmap_Texture(OutHeightmap);

	ERR_CATCH(Format("Nie mo¿na wczytaæ heightmapy z pliku \"#\".") % m_HeightmapFileName);
}

uint Terrain_pimpl::GetFormDataIndex(const FORM_DESC_ITEM *Item, uint1 Height)
{
	const FORM_DESC_ITEM_SIMPLE *simple_item;
	const FORM_DESC_ITEM_COMPLEX *complex_item;

	if ((simple_item = dynamic_cast<const FORM_DESC_ITEM_SIMPLE*>(Item)) != NULL)
	{
		return simple_item->Data.Index;
	}
	else if ((complex_item = dynamic_cast<const FORM_DESC_ITEM_COMPLEX*>(Item)) != NULL)
	{
		uint j = 0;
		while (j < complex_item->Height.size()-1 && Height >= complex_item->Height[j+1])
			j++;
		return complex_item->Data[j].Index;
	}
	else
	{
		assert(0 && "WTF?!");
		return 0xDEADC0DE;
	}
}

void Terrain_pimpl::LoadFormMap()
{
	ERR_TRY;

	LOG(0x08, Format("Terrain: Loading form map from \"#\".") % m_FormMapFileName);

	uint VertexCX = m_CX + 1;
	uint VertexCZ = m_CZ + 1;

	// Utwórz tablicê docelow¹ form terenu
	m_FormMap.resize(VertexCX * VertexCZ);

	// Wczytaj form mapê
	MemoryTexture mem_tex(m_FormMapFileName);

	// Tablica wszystkich FORM_DESC_ITEM, tak jak m_FormDesc, ale wymieszane dla optymalizacji (kolejnoœæ nie ma znaczenia).
	assert(!m_FormDesc.Items.empty());
	std::vector<FORM_DESC_ITEM*> FormDesc;
	for (uint i = 0; i < m_FormDesc.Items.size(); i++)
		FormDesc.push_back(m_FormDesc.Items[i].get());

	// Jazda!
	uint x, z, map_i, i, best_i, dist, best_dist;
	bool Found;
	const COLOR * mem_tex_row;
	COLOR color;
	for (z = 0, map_i = 0; z < VertexCZ; z++)
	{
		mem_tex_row = mem_tex.GetRowData(z);

		for (x = 0; x < VertexCX; x++)
		{
			// Wierzcho³ek poza zakresem
			// Ten przypadek da³oby siê rozpisaæ optymalniej ale w koñcu to nie jest coœ nad czym warto siê zastanawiaæ.
			// To nie powinno wystêpowaæ.
			if (x > m_CX || z > m_CZ)
				color = COLOR::BLACK;
			// Wierzcho³ek w zakresie form mapy
			else
				color = mem_tex_row[x];

			Found = false;

			// 1. Spróbuj dopasowaæ kolor bezpoœrednio
			for (i = 0; i < FormDesc.size(); i++)
			{
				if (FormDesc[i]->FormMapColor == color)
				{
					m_FormMap[map_i] = (uint1)GetFormDataIndex(FormDesc[i], m_Heightmap[z * (m_CX+1) + x]);

					// Optymalizacja, bo nastêpne piksele te¿ pewnie bêd¹ tego samego koloru wiêc znajdzie siê szybciej.
					if (i > 0)
						std::swap(FormDesc[i], FormDesc[0]);

					Found = true;
					break;
				}
			}

			// 2. ZnajdŸ najbardziej podobny kolor
			if (!Found)
			{
				best_i = 0;
				best_dist = ColorDistance(color, FormDesc[0]->FormMapColor);
				for (i = 1; i < FormDesc.size(); i++)
				{
					dist = ColorDistance(color, FormDesc[i]->FormMapColor);
					if (dist < best_dist)
					{
						best_dist = dist;
						best_i = i;
					}
				}

				m_FormMap[map_i] = (uint1)GetFormDataIndex(FormDesc[best_i], m_Heightmap[z * (m_CX+1) + x]);
			}
			
			map_i++;
		}
	}

	ERR_CATCH(Format("Nie mo¿na wczytaæ mapy form terenu z pliku \"#\".") % m_FormMapFileName);
}

void Terrain_pimpl::CalcFormWeights(std::vector<uint1> *OutFormWeights)
{
	uint VertexCX = m_CX + 1;
	uint VertexCZ = m_CZ + 1;
	uint FormDataCount = m_FormDescData.size();
	uint Index;
	OutFormWeights->resize(VertexCX * VertexCZ * FormDataCount);

	// Inicjalizacja
	// Dla ka¿dego wierzcho³ka waga formy która w m_FormMap jest przypisana do tego wierzcho³ka bêdzie wynosi³a 255.
	// Pozosta³e formy w danym wierzcho³ku bêd¹ równe 0.
	ZeroMemory(&(*OutFormWeights)[0], OutFormWeights->size() * sizeof(uint1));
	uint x, z, i, sum, count;
	for (z = 0; z < VertexCZ; z++)
	{
		for (x = 0; x < VertexCX; x++)
		{
			Index = z * VertexCX + x;
			(*OutFormWeights)[Index * FormDataCount + m_FormMap[Index]] = 255;
		}
	}

	// Blur
	for (uint pass = 0; pass < BLUE_PASSES; pass++)
	{
		for (z = 0; z < VertexCZ; z++)
		{
			for (x = 0; x < VertexCX; x++)
			{
				Index = z * VertexCX + x;
				for (i = 0; i < FormDataCount; i++)
				{
					sum = (uint)(*OutFormWeights)[Index * FormDataCount + i];
					count = 1;

					if (x > 0)
					{
						sum += (uint)(*OutFormWeights)[(z * VertexCX + (x-1)) * FormDataCount + i];
						count++;
					}
					if (z > 0)
					{
						sum += (uint)(*OutFormWeights)[((z-1) * VertexCX + x) * FormDataCount + i];
						count++;
					}
					if (x < VertexCX-1)
					{
						sum += (uint)(*OutFormWeights)[(z * VertexCX + (x+1)) * FormDataCount + i];
						count++;
					}
					if (z < VertexCZ-1)
					{
						sum += (uint)(*OutFormWeights)[((z+1) * VertexCX + x) * FormDataCount + i];
						count++;
					}

					(*OutFormWeights)[Index * FormDataCount + i] = (uint1)(sum / count);
				}
			}
		}
	}
}

void Terrain_pimpl::GeneratePatches()
{
	ERR_TRY;

	m_PatchCX = ceil_div<uint>(m_CX, PATCH_SIZE);
	m_PatchCZ = ceil_div<uint>(m_CZ, PATCH_SIZE);
	uint PatchCount = m_PatchCX * m_PatchCZ;
	m_Patches.resize(PatchCount);

	// Jeœli plik cache istnieje i jest aktualny, wczytaj go
	DATETIME HeightmapFileTime;
	DATETIME FormDescFileTime;
	DATETIME FormMapFileTime;
	DATETIME CacheFileTime;
	if (GetFileItemInfo(m_HeightmapFileName, NULL, NULL, &HeightmapFileTime, NULL, NULL) &&
		GetFileItemInfo(m_FormDescFileName, NULL, NULL, &FormDescFileTime, NULL, NULL) &&
		GetFileItemInfo(m_FormMapFileName, NULL, NULL, &FormMapFileTime, NULL, NULL) &&
		GetFileItemInfo(m_CacheFileName, NULL, NULL, &CacheFileTime, NULL, NULL) &&
		HeightmapFileTime < CacheFileTime &&
		FormDescFileTime < CacheFileTime &&
		FormMapFileTime < CacheFileTime)
	{
		LoadPatchesFromCache();
	}
	// Jeœli plik cache nie istnia³ lub nie by³ aktualny, wygeneruj patche od nowa
	else
	{
		LOG(0x08, "Terrain: Generating patches...");

		// Policz normalne
		std::vector<VEC3> Normals;
		CalcNormals(&Normals);

		// Policz wagi form terenu
		// Ta tablica ma 3 wymiary i rozmiar (m_CX+1) * (m_CZ+1) * m_FormDescData.size()
		std::vector<uint1> FormWeights;
		CalcFormWeights(&FormWeights);

		// Wygeneruj poszczególne patche
		uint x, z, pz, px;
		for (z = 0, pz = 0; z < m_CZ; z += PATCH_SIZE, pz++)
		{
			for (x = 0, px = 0; x < m_CX; x += PATCH_SIZE, px++)
			{
				GeneratePatch(&m_Patches[pz*m_PatchCX+px], x, z, Normals, FormWeights);
			}
		}

		// Zapisz plik cache
		WritePatchesToCache();
	}

	ERR_CATCH("Nie mo¿na wygenerowaæ fragmentów mapy.");
}

void Terrain_pimpl::CalcNormals(std::vector<VEC3> *OutNormals)
{
	uint cx_plus_1 = m_CX + 1;
	uint VertexCount = cx_plus_1 * (m_CZ+1);
	uint x, z, i;
	VEC3 v, v1, v2, v2n, vn, Pos;

	// Wylicz wysokoœci - to przyspieszy obliczenia
	std::vector<float> Heights;
	Heights.resize(VertexCount);
	for (z = 0, i = 0; z <= m_CZ; z++)
	{
		for (x = 0; x <= m_CX; x++)
		{
			Heights[i++] = GetHeight_Float(x, z);
		}
	}

	// Wylicz normalne
	OutNormals->resize(VertexCount);
	for (z = 0, i = 0; z <= m_CZ; z++)
	{
		for (x = 0; x <= m_CX; x++)
		{
			// Proste liczenie normalnej - brzydkie
			//Vertex->Normal = VEC3(
			//	GetHeight_Float(x - 1, z) - GetHeight_Float(x + 1, z),
			//	2.0f * m_VertexDistance,
			//	GetHeight_Float(x, z - 1) - GetHeight_Float(x, z + 1));
			//Normalize(&Vertex->Normal);

			// Moje stare, powolne ale ³adne liczenie normalnej
			vn = VEC3::ZERO;
			Pos = VEC3(x*m_VertexDistance, Heights[x + z*cx_plus_1], z*m_VertexDistance);
			// lewo
			if (x > 0)
			{
				v = VEC3((x-1)*m_VertexDistance, Heights[(x-1) + z*cx_plus_1], z*m_VertexDistance);
				v1 = v - Pos;
				Cross(&v2, VEC3::NEGATIVE_Z, v1);
				Normalize(&v2n, v2);
				vn += v2n;
			}
			// prawo
			if (x < m_CX)
			{
				v = VEC3((x+1)*m_VertexDistance, Heights[(x+1) + z*cx_plus_1], z*m_VertexDistance);
				v1 = v - Pos;
				Cross(&v2, VEC3::POSITIVE_Z, v1);
				Normalize(&v2n, v2);
				vn += v2n;
			}
			// góra
			if (z > 0)
			{
				v = VEC3(x*m_VertexDistance, Heights[x + (z-1)*cx_plus_1], (z-1)*m_VertexDistance);
				v1 = v - Pos;
				Cross(&v2, VEC3::POSITIVE_X, v1);
				Normalize(&v2n, v2);
				vn += v2n;
			}
			// dó³
			if (z < m_CZ)
			{
				v = VEC3(x*m_VertexDistance, Heights[x + (z+1)*cx_plus_1], (z+1)*m_VertexDistance);
				v1 = v - Pos;
				Cross(&v2, VEC3::NEGATIVE_X, v1);
				Normalize(&v2n, v2);
				vn += v2n;
			}
			// góra-lewo
			if (x > 0 && z > 0)
			{
				v = VEC3((x-1)*m_VertexDistance, Heights[(x-1) + (z-1)*cx_plus_1], (z-1)*m_VertexDistance);
				v1 = v - Pos;
				Cross(&v2, VEC3(+1.0f, 0.0f, -1.0f), v1);
				Normalize(&v2n, v2);
				vn += v2n;
			}
			// góra-prawo
			if (x < m_CX && z > 0)
			{
				v = VEC3((x+1)*m_VertexDistance, Heights[(x+1) + (z-1)*cx_plus_1], (z-1)*m_VertexDistance);
				v1 = v - Pos;
				Cross(&v2, VEC3(+1.0f, 0.0f, +1.0f), v1);
				Normalize(&v2n, v2);
				vn += v2n;
			}
			// dó³-lewo
			if (x > 0 && z < m_CZ)
			{
				v = VEC3((x-1)*m_VertexDistance, Heights[(x-1) + (z+1)*cx_plus_1], (z+1)*m_VertexDistance);
				v1 = v - Pos;
				Cross(&v2, VEC3(-1.0f, 0.0f, -1.0f), v1);
				Normalize(&v2n, v2);
				vn += v2n;
			}
			// dó³-prawo
			if (x < m_CX && z < m_CZ)
			{
				v = VEC3((x+1)*m_VertexDistance, Heights[(x+1) + (z+1)*cx_plus_1], (z+1)*m_VertexDistance);
				v1 = v - Pos;
				Cross(&v2, VEC3(-1.0f, 0.0f, +1.0f), v1);
				Normalize(&v2n, v2);
				vn += v2n;
			}
			Normalize(&(*OutNormals)[i++], vn);
		}
	}
}

void Terrain_pimpl::GeneratePatch(PATCH *OutPatch, uint StartX, uint StartZ, const std::vector<VEC3> &Normals, const std::vector<uint1> &FormWeights)
{
	uint FormCount = m_FormDescData.size();
	uint VertexCX = m_CX + 1;
	uint VertexCZ = m_CZ + 1;

	assert(StartX < m_CX);
	assert(StartZ < m_CZ);
	assert(FormCount > 0);

	// Tu mi siê ju¿ wszystko pokie³basi³o :(
	// Lepiej nie ruszaæ póki dzia³a.
	// x2, z2 wyznaczaj¹ wierzcho³ek za-ostatni bez uwzglêdnienia wyjœcia poza zakres heightmapy.
	// real_x2, real_z2 s¹ ograniczone do takiego zakresu, ¿e w razie wyjœcia poza wskazuj¹ na wierzcho³ek ostatni nadal wa¿ny.
	// real2_x2, real2_z2 s¹ ograniczone do takiego zakresu, ¿e w razie wyjœcia poza wskazuj¹ na wierzcho³ek za-ostatni
	uint x1 = StartX, x2 = StartX + PATCH_SIZE + 1;
	uint z1 = StartZ, z2 = StartZ + PATCH_SIZE + 1;
	uint x, z, i;
	uint real_x2 = std::min(x2, m_CX);
	uint real_z2 = std::min(z2, m_CZ);
	uint real2_x2 = std::min(x2, m_CX+1);
	uint real2_z2 = std::min(z2, m_CZ+1);
	
	// Wybierz formy terenu dla tego fragmentu
	{
		// Suma wag dla danej formy terenu
		// Pary indeks-formy-terenu -> liczba-u¿yæ
		std::vector< std::pair<uint, uint> > FormSumWeights;
		FormSumWeights.resize(FormCount);
		for (i = 0; i < FormCount; i++)
			FormSumWeights[i] = std::pair<uint, int>(i, 0);
		for (z = z1; z < real2_z2; z++)
		{
			for (x = x1; x < real2_x2; x++)
			{
				for (i = 0; i < FormCount; i++)
				{
					FormSumWeights[i].second += (uint)FormWeights[(z*VertexCX + x)*FormCount + i];
				}
			}
		}

		// Posortuj malej¹co wg liczby u¿yæ
		struct FormSumWeightsCompare
		{
			bool operator () (const std::pair<uint, uint> &p1, const std::pair<uint, uint> &p2)
			{
				return p1.second > p2.second;
			}
		};
		std::sort(FormSumWeights.begin(), FormSumWeights.end(), FormSumWeightsCompare());

		// Posortuj TERRAIN_FORMS_PER_PATCH pierwszych wg numeru formy terenu
		// Kiedy to robiê, jest brzydki efekt ¿e np. przy ³¹czeniu trawy z drog¹ widaæ piasek.
		// Kiedy tego nie robiê, jest brzydki efekt ¿e w niektórych miejscach (np. jak przechodzi droga) widaæ ³¹czenia patchów.
		// Tak Ÿle i tak niedobrze :/
		struct FormSumWeightsCompare2
		{
			bool operator () (const std::pair<uint, uint> &p1, const std::pair<uint, uint> &p2)
			{
				return p1.first < p2.first;
			}
		};
		std::sort(FormSumWeights.begin(), (TERRAIN_FORMS_PER_PATCH > FormSumWeights.size() ? FormSumWeights.end() : FormSumWeights.begin() + TERRAIN_FORMS_PER_PATCH), FormSumWeightsCompare2());

		// Spisz formy do u¿ycia w tym patchu
		for (i = 0; i < TERRAIN_FORMS_PER_PATCH; i++)
			// To min jest na wypadek gdyby wszystkich mo¿liwych form terenu by³o mniej ni¿ wynosi sta³a TERRAIN_FORMS_PER_PATCH.
			OutPatch->TerrainForms[i] = FormSumWeights[std::min(i, FormSumWeights.size()-1)].first;
		
		// SprawdŸ, czy wierzcho³ki tego fragmentu nie u¿ywaj¹ wiêcej form terenu ni¿ te spisane wy¿ej.
		// Jeœli tak, wyœwietl ostrze¿enie.
		if (TERRAIN_FORMS_PER_PATCH < FormSumWeights.size() && FormSumWeights[TERRAIN_FORMS_PER_PATCH].second > 0)
			LOG(0x100, Format("Terrain: Too complex terrain forms in patch starting from #,#.") % StartX % StartZ);
	}

	// Wierzcho³ki zwyk³e
	VERTEX *Vertex = &OutPatch->Vertices[0];
	VEC3 v, v1, v2, v2n, vn;
	float Height;
	OutPatch->MinY = OutPatch->MaxY = GetHeight_Float(x1, z1);
	for (z = z1; z < z2; z++)
	{
		for (x = x1; x < x2; x++)
		{
			// Wierzcho³ek poza zakresem - zdegerowany
			// - Poza zakresem i na X i na Z
			if (x > m_CX && z > m_CZ)
				// Przepisz ostatni wierzcho³ek, z rogu, z koñca
				*Vertex = OutPatch->Vertices[(m_CZ-z1)*(PATCH_SIZE+1) + (m_CX-x1)];
			// - Poza zakresem tylko na X
			else if (x > m_CX)
				// Przepisz ostatni normalny wierzho³ek z tego wiersza
				*Vertex = OutPatch->Vertices[(z-z1)*(PATCH_SIZE+1) + (m_CX-x1)];
			// - Poza zakresem tylko na Z
			else if (z > m_CZ)
				// Przepisz ostatni normalny wierzcho³ek z tej kolumny
				*Vertex = OutPatch->Vertices[(m_CZ-z1)*(PATCH_SIZE+1) + (x-x1)];
			// Wierzcho³ek normalny, nie jest poza zakresem
			else
			{
				Height = GetHeight_Float(x, z);

				OutPatch->MinY = std::min(OutPatch->MinY, Height);
				OutPatch->MaxY = std::max(OutPatch->MaxY, Height);

				Vertex->Pos = VEC3(
					(float)x * m_VertexDistance,
					Height,
					(float)z * m_VertexDistance);

				Vertex->Normal = Normals[z*(m_CX+1) + x];

				// Waga formy i-tej dla wierzcho³ka x,z wynosi: FormWeights[(z*VertexCX + x)*FormCount + i]
				float FormWeightsSum = 0.0f;
				for (i = 0; i < std::min(TERRAIN_FORMS_PER_PATCH, FormCount); i++)
					FormWeightsSum += FormWeights[(z*VertexCX + x)*FormCount + OutPatch->TerrainForms[i]];
				if (FormWeightsSum == 0.0f)
					FormWeightsSum = 0.0f;
				uint Bits = 24;
				Vertex->Diffuse.ARGB = 0x00000000;
				for (i = 1; i < std::min(TERRAIN_FORMS_PER_PATCH, FormCount); i++)
				{
					Vertex->Diffuse.ARGB |= (uint)( (float)FormWeights[(z*VertexCX + x)*FormCount + OutPatch->TerrainForms[i]] / FormWeightsSum * 255.0f ) << Bits;
					Bits -= 8;
				}
			}

			Vertex++;
		}
	}

	// Skirt
	// - Lewy
	for (z = z1, i = 0; z < z2; z++, i++)
	{
		*Vertex = OutPatch->Vertices[i * (PATCH_SIZE+1)];
		// Skirt jest tylko jeœli to nie brzeg ca³ej heightmapy, w przeciwnym wypadku jest degenerowany.
		if (x1 > 0)
			Vertex->Pos.y = m_MinY;
		Vertex++;
	}
	// - Prawy
	for (z = z1, i = 0; z < z2; z++, i++)
	{
		*Vertex = OutPatch->Vertices[i * (PATCH_SIZE+1) + PATCH_SIZE];
		// Skirt jest tylko jeœli to nie brzeg ca³ej heightmapy, w przeciwnym wypadku jest degenerowany.
		if (x2 < m_CX)
			Vertex->Pos.y = m_MinY;
		Vertex++;
	}
	// - Bliski
	for (x = x1, i = 0; x < x2; x++, i++)
	{
		*Vertex = OutPatch->Vertices[i];
		// Skirt jest tylko jeœli to nie brzeg ca³ej heightmapy, w przeciwnym wypadku jest degenerowany.
		if (z1 > 0)
			Vertex->Pos.y = m_MinY;
		Vertex++;
	}
	// - Daleki
	for (x = x1, i = PATCH_SIZE * (PATCH_SIZE+1); x < x2; x++, i++)
	{
		// Skirt jest tylko jeœli to nie brzeg ca³ej heightmapy, w przeciwnym wypadku jest degenerowany.
		*Vertex = OutPatch->Vertices[i];
		if (z2 < m_CZ)
			Vertex->Pos.y = m_MinY;
		Vertex++;
	}
}

void Terrain_pimpl::GenerateIndices()
{
	IDirect3DIndexBuffer9 *IbPtr;
	HRESULT hr = frame::Dev->CreateIndexBuffer(
		PATCH_INDEX_COUNT * sizeof(uint2),
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_MANAGED,
		&IbPtr,
		NULL);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na utworzyæ bufora indeksów dla terenu.", __FILE__, __LINE__);
	m_IB.reset(IbPtr);

	IndexBufferLock IbLock(m_IB.get(), 0);
	uint2 *Indices = (uint2*)IbLock.GetData();

	// Kolejne poziomy LOD dla indeksów
	uint Step = 1;
	uint LevelSize = PATCH_SIZE; // Liczba quadów na szerokoœæ i na d³ugoœæ
	uint ii = 0, x, z;
	for (uint Level = 0; Level < LEVEL_COUNT; Level++)
	{
		// Zwyk³a siatka
		for (z = 0; z < PATCH_SIZE; z += Step)
		{
			for (x = 0; x < PATCH_SIZE; x += Step)
			{
				Indices[ii++] = (z     ) * (PATCH_SIZE+1) + x;
				Indices[ii++] = (z+Step) * (PATCH_SIZE+1) + x;
				Indices[ii++] = (z+Step) * (PATCH_SIZE+1) + x+Step;
				Indices[ii++] = (z     ) * (PATCH_SIZE+1) + x;
				Indices[ii++] = (z+Step) * (PATCH_SIZE+1) + x+Step;
				Indices[ii++] = (z     ) * (PATCH_SIZE+1) + x+Step;
			}
		}

		// Skirt
		uint SkirtVertexIndex = (PATCH_SIZE + 1) * (PATCH_SIZE + 1);
		uint LastRowVertexIndex = (PATCH_SIZE + 1) * PATCH_SIZE;
		// - Lewy
		for (uint z = 0; z < PATCH_SIZE; z += Step)
		{
			Indices[ii++] = SkirtVertexIndex + z;
			Indices[ii++] = SkirtVertexIndex + z+Step;
			Indices[ii++] = (z+Step) * (PATCH_SIZE+1);
			Indices[ii++] = SkirtVertexIndex + z;
			Indices[ii++] = (z+Step) * (PATCH_SIZE+1);
			Indices[ii++] = (z     ) * (PATCH_SIZE+1);
		}
		SkirtVertexIndex += PATCH_SIZE+1;
		// - Prawy
		for (uint z = 0; z < PATCH_SIZE; z += Step)
		{
			Indices[ii++] = (z     ) * (PATCH_SIZE+1) + PATCH_SIZE;
			Indices[ii++] = (z+Step) * (PATCH_SIZE+1) + PATCH_SIZE;
			Indices[ii++] = SkirtVertexIndex + z+Step;
			Indices[ii++] = (z     ) * (PATCH_SIZE+1) + PATCH_SIZE;
			Indices[ii++] = SkirtVertexIndex + z+Step;
			Indices[ii++] = SkirtVertexIndex + z;
		}
		// - Bliski
		SkirtVertexIndex += PATCH_SIZE+1;
		for (uint x = 0; x < PATCH_SIZE; x += Step)
		{
			Indices[ii++] = SkirtVertexIndex + x;
			Indices[ii++] = x;
			Indices[ii++] = x+Step;
			Indices[ii++] = SkirtVertexIndex + x;
			Indices[ii++] = x+Step;
			Indices[ii++] = SkirtVertexIndex + x+Step;
		}
		// - Daleki
		SkirtVertexIndex += PATCH_SIZE+1;
		for (uint x = 0; x < PATCH_SIZE; x += Step)
		{
			Indices[ii++] = LastRowVertexIndex + x;
			Indices[ii++] = SkirtVertexIndex + x;
			Indices[ii++] = SkirtVertexIndex + x+Step;
			Indices[ii++] = LastRowVertexIndex + x;
			Indices[ii++] = SkirtVertexIndex + x+Step;
			Indices[ii++] = LastRowVertexIndex + x+Step;
		}

		Step *= 2;
		LevelSize /= 2;
	}
}

void Terrain_pimpl::CreateVB()
{
	IDirect3DVertexBuffer9 *VbPtr;
	HRESULT hr = frame::Dev->CreateVertexBuffer(
		PATCH_VERTEX_COUNT * sizeof(VERTEX) * VB_PATCH_COUNT,
		D3DUSAGE_WRITEONLY,
		VERTEX::FVF,
		D3DPOOL_MANAGED,
		&VbPtr,
		NULL);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na utworzyæ bufora indeksów dla terenu.", __FILE__, __LINE__);
	m_VB.reset(VbPtr);

	for (uint i = 0; i < VB_PATCH_COUNT; i++)
	{
		PatchLastUseFrameInVb[i] = frame::GetFrameNumber();
		PatchesInVb[i] = MAXUINT4;
	}
}

uint Terrain_pimpl::LoadPatch(uint PatchIndex)
{
	// Poszukaj czy ju¿ jest
	for (uint i = 0; i < VB_PATCH_COUNT; i++)
	{
		if (PatchesInVb[i] == PatchIndex)
		{
			PatchLastUseFrameInVb[i] = frame::GetFrameNumber();
			return i * PATCH_VERTEX_COUNT;
		}
	}

	// Nie ma:
	
	// ZnajdŸ najdawniej u¿ywany patch w buforze
	uint Found_i = 0;
	for (uint i = 1; i < VB_PATCH_COUNT; i++)
	{
		if (PatchLastUseFrameInVb[i] < PatchLastUseFrameInVb[Found_i])
			Found_i = i;
	}

	// Nadpisz go nowym patchem
	{
		VertexBufferLock vb_lock(m_VB.get(), 0, Found_i * PATCH_VERTEX_COUNT * sizeof(VERTEX), PATCH_VERTEX_COUNT * sizeof(VERTEX));
		CopyMem(vb_lock.GetData(), &m_Patches[PatchIndex].Vertices[0], PATCH_VERTEX_COUNT * sizeof(VERTEX));
	}

	PatchesInVb[Found_i] = PatchIndex;
	PatchLastUseFrameInVb[Found_i] = frame::GetFrameNumber();
	return Found_i * PATCH_VERTEX_COUNT;
}

void Terrain_pimpl::LoadFormDesc()
{
	ERR_TRY;

	LOG(0x08, Format("Terrain: Loading form description from \"#\".") % m_FormDescFileName);

	FileStream File(m_FormDescFileName, FM_READ);
	Tokenizer Tok(&File, 0);
	Tok.Next();

	// Nag³ówek
	Tok.AssertIdentifier("TerrainForms");
	Tok.Next();
	if (Tok.MustGetUint4() != 1) Tok.CreateError("B³êdna wersja.");
	Tok.Next();

	while (!Tok.QueryEOF())
	{
		// Kolor
		COLOR FormMapColor = Tok.MustGetUint4();
		Tok.Next();
		// Typ
		Tok.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
		if (Tok.GetString() == "simple")
		{
			Tok.Next();
			shared_ptr<FORM_DESC_ITEM> Item(new FORM_DESC_ITEM_SIMPLE);
			Item->FormMapColor = FormMapColor;
			LoadFormDesc_SimpleItem(Tok, (FORM_DESC_ITEM_SIMPLE*)Item.get());
			m_FormDesc.Items.push_back(Item);
		}
		else if (Tok.GetString() == "complex")
		{
			Tok.Next();
			shared_ptr<FORM_DESC_ITEM> Item(new FORM_DESC_ITEM_COMPLEX);
			Item->FormMapColor = FormMapColor;
			LoadFormDesc_ComplexItem(Tok, (FORM_DESC_ITEM_COMPLEX*)Item.get());
			m_FormDesc.Items.push_back(Item);
		}
		else
			Tok.CreateError("Oczekiwany identyfikator \"simple\" lub \"complex\".");
	}

	// Wczytaj formy do wektora, uzupe³nij Index w oryginalnych
	FORM_DESC_ITEM_SIMPLE *SimpleItem;
	FORM_DESC_ITEM_COMPLEX *ComplexItem;
	for (uint i = 0; i < m_FormDesc.Items.size(); i++)
	{
		if ((SimpleItem = dynamic_cast<FORM_DESC_ITEM_SIMPLE*>(m_FormDesc.Items[i].get())) != NULL)
		{
			SimpleItem->Data.Index = m_FormDescData.size();
			m_FormDescData.push_back(&SimpleItem->Data);
		}
		else if ((ComplexItem = dynamic_cast<FORM_DESC_ITEM_COMPLEX*>(m_FormDesc.Items[i].get())) != NULL)
		{
			for (uint j = 0; j < ComplexItem->Data.size(); j++)
			{
				ComplexItem->Data[j].Index = m_FormDescData.size();
				m_FormDescData.push_back(&ComplexItem->Data[j]);
			}
		}
		else
			assert(0 && "WTF?!");
	}

	// SprawdŸ liczbê
	if (m_FormDescData.size() > MAXUINT1)
		throw Error("Zbyt wiele form terenu. Dozwolone co najwy¿ej 255.", __FILE__, __LINE__);
	else if (m_FormDescData.empty())
		throw Error("Brak form terenu. Wymagana co najmniej jedna.", __FILE__, __LINE__);

	ERR_CATCH(Format("Nie mo¿na wczytaæ opisu form terenu z pliku \"#\".") % m_FormDescFileName);
}

void Terrain_pimpl::LoadFormDesc_SimpleItem(Tokenizer &Tok, FORM_DESC_ITEM_SIMPLE *OutItem)
{
	LoadFormDesc_Data(Tok, &OutItem->Data);
}

void Terrain_pimpl::LoadFormDesc_ComplexItem(Tokenizer &Tok, FORM_DESC_ITEM_COMPLEX *OutItem)
{
	Tok.AssertSymbol('{');
	Tok.Next();

	while (!Tok.QuerySymbol('}'))
	{
		// Wysokoœæ
		uint1 Height = Tok.MustGetUint1();
		Tok.Next();
		// Pierwsza wysokoœæ musi wynosiæ 0
		if (OutItem->Height.empty())
		{
			if (Height > 0)
				Tok.CreateError("Pierwsza wyokoœæ musi wynosiæ 0.");
		}
		// Ka¿da nastêpna wysokoœæ musi byæ wiêksza od poprzedniej
		else
		{
			if (Height <= OutItem->Height[OutItem->Height.size()-1])
				Tok.CreateError("Wysokoœæ musi byæ wiêksza od poprzedniej.");
		}
		// Dane
		FORM_DESC_DATA Data;
		LoadFormDesc_Data(Tok, &Data);

		OutItem->Height.push_back(Height);
		OutItem->Data.push_back(Data);
	}
	Tok.Next();
}

void Terrain_pimpl::LoadFormDesc_Data(Tokenizer &Tok, FORM_DESC_DATA *OutData)
{
	OutData->Tag = 0;
	OutData->TexScale = 1.0f;

	Tok.AssertSymbol('{');
	Tok.Next();

	// Nazwa tekstury
	Tok.AssertToken(Tokenizer::TOKEN_STRING);
	OutData->TextureName = Tok.GetString();
	Tok.Next();

	// Dodatkowe parametry
	while (!Tok.QuerySymbol('}'))
	{
		Tok.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
		if (Tok.GetString() == "Tag")
		{
			Tok.Next();
			Tok.AssertSymbol('=');
			Tok.Next();
			OutData->Tag = Tok.MustGetUint4();
			Tok.Next();
		}
		else if (Tok.GetString() == "TexScale")
		{
			Tok.Next();
			Tok.AssertSymbol('=');
			Tok.Next();
			OutData->TexScale = Tok.MustGetFloat();
			Tok.Next();
		}
		else
			Tok.CreateError("Nierozpoznany identyfikator: " + Tok.GetString());
	}
	Tok.Next();
}

void Terrain_pimpl::WritePatchesToCache()
{
	ERR_TRY;

	FileStream F(m_CacheFileName, FM_WRITE);

	// Nag³ówek
	F.WriteStringF(CACHE_FILE_HEADER);

	// Patche
	for (uint pi = 0; pi < m_Patches.size(); pi++)
	{
		const PATCH &Patch = m_Patches[pi];

		// Zakres wysokoœci
		F.WriteEx(Patch.MinY);
		F.WriteEx(Patch.MaxY);

		// Formy terenu
		F.Write(Patch.TerrainForms, TERRAIN_FORMS_PER_PATCH * sizeof(uint));
		
		// Wierzcho³ki
		F.Write(Patch.Vertices, PATCH_VERTEX_COUNT * sizeof(VERTEX));
	}

	ERR_CATCH(Format("Nie mo¿na zapisaæ pliku tymczasowego terenu \"#\".") % m_CacheFileName);
}

void Terrain_pimpl::LoadPatchesFromCache()
{
	ERR_TRY;

	LOG(0x08, Format("Terrain: Loading patches from cache \"#\".") % m_CacheFileName);

	FileStream F(m_CacheFileName, FM_READ);

	// Nag³ówek
	string Header;
	F.ReadStringF(&Header, CACHE_FILE_HEADER.length());
	if (Header != CACHE_FILE_HEADER)
		throw Error("B³êdny nag³ówek lub wersja pliku.");

	// Patche
	for (uint pi = 0; pi < m_Patches.size(); pi++)
	{
		PATCH &Patch = m_Patches[pi];

		// Zakres wysokoœci
		F.ReadEx(&Patch.MinY);
		F.ReadEx(&Patch.MaxY);

		// Formy terenu
		F.MustRead(Patch.TerrainForms, TERRAIN_FORMS_PER_PATCH * sizeof(uint));

		// Wierzcho³ki
		F.MustRead(Patch.Vertices, PATCH_VERTEX_COUNT * sizeof(VERTEX));
	}

	ERR_CATCH(Format("Nie mo¿na wczytaæ pliku tymczasowego terenu \"#\".") % m_CacheFileName);
}

bool Terrain_pimpl::RayCollision_Patch_Vertical(int px, int pz, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT, float MaxT)
{
	// Wiemy, ¿e RayDir.x i RayDir.z s¹ 0 lub prawie 0.
	if (px < 0 || pz < 0) return false;
	uint PatchIndex = (uint)pz * m_PatchCX + (uint)px;

	if (
		(RayDir.y > 0.0f && RayOrig.y > m_Patches[PatchIndex].MaxY) ||
		(RayDir.y < 0.0f && RayOrig.y < m_Patches[PatchIndex].MinY))
	{
		return false;
	}

	float h = GetHeight_Float(RayOrig.x, RayOrig.z);
	*OutT = (h - RayOrig.y) / RayDir.y;
	return (*OutT >= 0.0f && *OutT <= MaxT);
}

bool Terrain_pimpl::RayCollision_Patch(uint px, uint pz, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT, float StartT, float MaxT)
{
	//LOG(1024, Format("RayCollision_Patch: px=#, pz=#, StartT=#, MaxT=#") % px % pz % StartT % MaxT);

	// ZnajdŸ wierzcho³ek, w którym le¿y pocz¹tek tego promienia
	float WorldToVertexFactor = m_VertexDistanceInv;
	float VertexToWorldFactor = m_VertexDistance;
	int next_vx, curr_vx = (int)((RayOrig.x + RayDir.x * StartT) * WorldToVertexFactor);
	int next_vz, curr_vz = (int)((RayOrig.z + RayDir.z * StartT) * WorldToVertexFactor);
	float curr_t = StartT, next_t;

	float bound_x, bound_z, x1, x2, z1, z2, y1, y2;
	int What; // 0 = nic, 1 = return false, 2 = continue
	float h11, h12, h21, h22, min_h, max_h;
	VEC3 v11, v12, v21, v22;
	int vx1 = (int)(px * PATCH_SIZE);
	int vz1 = (int)(pz * PATCH_SIZE);
	int vx2 = (int)((px + 1) * PATCH_SIZE);
	int vz2 = (int)((pz + 1) * PATCH_SIZE);
	for (;;)
	{
		What = 0;
		// Przejœcie do wierzcho³ka na prawo
		if (RayDir.x > 0.0f)
		{
			// Oblicz pozycjê we wsp. œwiata prawej granicy bie¿¹cego wierzcho³ka
			bound_x = (float)(curr_vx + 1) * VertexToWorldFactor;
			// Oblicz parametr T dla prawej granicy bie¿¹cego wierzcho³ka
			next_t = (bound_x - RayOrig.x) / RayDir.x;
			// Oblicz parametr Z dla prawej granicy bie¿¹cego wierzcho³ka
			bound_z = RayOrig.z + RayDir.z * next_t;
			// Jeœli Z jest w zakresie tego wierzcho³ka, znaleziono przejœcie do wierzcho³ka na prawo
			z1 = (float)(curr_vz    ) * VertexToWorldFactor;
			z2 = (float)(curr_vz + 1) * VertexToWorldFactor;
			if (bound_z >= z1 && bound_z <= z2)
			{
				next_vx = curr_vx + 1;
				next_vz = curr_vz;
				// jeœli wyszliœmy poza patch w t¹ stronê, koniec, nie ma kolizji
				if (next_vx >= vx2) What = 1;
				// Jeœli koniec ustalonego zakresu parametru T, podobnie
				else if (next_t >= MaxT) What = 1;
				else What = 2;
			}
		}
		// Przejœcie do wierzcho³ka na lewo
		else if (RayDir.x < 0.0f)
		{
			bound_x = (float)(curr_vx) * VertexToWorldFactor;
			next_t = (bound_x - RayOrig.x) / RayDir.x;
			bound_z = RayOrig.z + RayDir.z * next_t;
			z1 = (float)(curr_vz    ) * VertexToWorldFactor;
			z2 = (float)(curr_vz + 1) * VertexToWorldFactor;
			if (bound_z >= z1 && bound_z <= z2)
			{
				next_vx = curr_vx - 1;
				next_vz = curr_vz;
				if (next_vx < 0) What = 1;
				else if (next_t >= MaxT) What = 1;
				else What = 2;
			}
		}

		if (What == 0)
		{
			// Przejœcie do wierzcho³ka na dalej
			if (RayDir.z > 0.0f)
			{
				bound_z = (float)(curr_vz + 1) * VertexToWorldFactor;
				next_t = (bound_z - RayOrig.z) / RayDir.z;
				bound_x = RayOrig.x + RayDir.x * next_t;
				x1 = (float)(curr_vx    ) * VertexToWorldFactor;
				x2 = (float)(curr_vx + 1) * VertexToWorldFactor;
				if (bound_x >= x1 && bound_x <= x2)
				{
					next_vx = curr_vx;
					next_vz = curr_vz + 1;
					if (next_vz >= vz2) What = 1;
					else if (next_t >= MaxT) What = 1;
					else What = 2;
				}
			}
			// Przejœcie do patcha na bli¿ej
			else if (RayDir.z < 0.0f)
			{
				bound_z = (float)(curr_vz) * VertexToWorldFactor;
				next_t = (bound_z - RayOrig.z) / RayDir.z;
				bound_x = RayOrig.x + RayDir.x * next_t;
				x1 = (float)(curr_vx    ) * VertexToWorldFactor;
				x2 = (float)(curr_vx + 1) * VertexToWorldFactor;
				if (bound_x >= x1 && bound_x <= x2)
				{
					next_vx = curr_vx;
					next_vz = curr_vz - 1;
					if (next_vz < 0) What = 1;
					else if (next_t >= MaxT) What = 1;
					else What = 2;
				}
			}
		}

		// Jeœli wierzcho³ek jest w zakresie (kwadracik rozpoczynaj¹cy siê od tego wierzcho³ka)
		if (curr_vx >= vx1 && curr_vz >= vz1 && curr_vx < vx2 && curr_vz < vz2)
		{
			// Oblicz zakres Y dla promienia w tym kwadraciku, sprawdŸ z MinY i MaxY tego kwadracika
			h11 = GetHeight_Float((uint)curr_vx, (uint)curr_vz);
			h21 = GetHeight_Float((uint)(curr_vx + 1), (uint)curr_vz);
			h12 = GetHeight_Float((uint)curr_vx, (uint)(curr_vz + 1));
			h22 = GetHeight_Float((uint)(curr_vx + 1), (uint)(curr_vz + 1));
			min_h = std::min(std::min(h11, h21), std::min(h12, h22));
			max_h = std::max(std::max(h11, h21), std::max(h12, h22));
			y1 = RayOrig.y + RayDir.y * curr_t;
			y2 = RayOrig.y + RayDir.y * next_t;
			if (!(
				(y1 < min_h && y2 < min_h) ||
				(y1 > max_h && y2 > max_h) ))
			{
				// SprawdŸ kolizjê z tym kwadracikiem
				v11 = VEC3((curr_vx    ) * VertexToWorldFactor, h11, curr_vz * VertexToWorldFactor);
				v21 = VEC3((curr_vx + 1) * VertexToWorldFactor, h21, curr_vz * VertexToWorldFactor);
				v12 = VEC3((curr_vx    ) * VertexToWorldFactor, h12, (curr_vz + 1) * VertexToWorldFactor);
				v22 = VEC3((curr_vx + 1) * VertexToWorldFactor, h22, (curr_vz + 1) * VertexToWorldFactor);
				if (RayToTriangle(RayOrig, RayDir, v11, v12, v22, true, OutT))
					return true;
				if (RayToTriangle(RayOrig, RayDir, v11, v22, v21, true, OutT))
					return true;
			}
		}

		if (What == 1)
			return false;

		if (What == 0)
		{
			LOG(1024, "Bleee"); // WTF?!?! Zdarzy³o mi siê to raz
			return false;
		}

		curr_vx = next_vx;
		curr_vz = next_vz;
		curr_t = next_t;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Terrain

res::IResource * Terrain::Create(const string &Name, const string &Group, Tokenizer &Params)
{
	Params.AssertIdentifier("CX");
	Params.Next();
	Params.AssertSymbol('=');
	Params.Next();
	uint CX = Params.MustGetUint4();
	Params.Next();

	Params.AssertIdentifier("CZ");
	Params.Next();
	Params.AssertSymbol('=');
	Params.Next();
	uint CZ = Params.MustGetUint4();
	Params.Next();

	Params.AssertIdentifier("VertexDistance");
	Params.Next();
	Params.AssertSymbol('=');
	Params.Next();
	float VertexDistance = Params.MustGetFloat();
	Params.Next();

	Params.AssertIdentifier("MinY");
	Params.Next();
	Params.AssertSymbol('=');
	Params.Next();
	float MinY = Params.MustGetFloat();
	Params.Next();

	Params.AssertIdentifier("MaxY");
	Params.Next();
	Params.AssertSymbol('=');
	Params.Next();
	float MaxY = Params.MustGetFloat();
	Params.Next();

	Params.AssertIdentifier("HeightmapFileName");
	Params.Next();
	Params.AssertSymbol('=');
	Params.Next();
	Params.AssertToken(Tokenizer::TOKEN_STRING);
	string HeightmapFileName = Params.GetString();
	Params.Next();

	Params.AssertIdentifier("FormDescFileName");
	Params.Next();
	Params.AssertSymbol('=');
	Params.Next();
	Params.AssertToken(Tokenizer::TOKEN_STRING);
	string FormDescFileName = Params.GetString();
	Params.Next();

	Params.AssertIdentifier("FormMapFileName");
	Params.Next();
	Params.AssertSymbol('=');
	Params.Next();
	Params.AssertToken(Tokenizer::TOKEN_STRING);
	string FormMapFileName = Params.GetString();
	Params.Next();

	Params.AssertIdentifier("CacheFileName");
	Params.Next();
	Params.AssertSymbol('=');
	Params.Next();
	Params.AssertToken(Tokenizer::TOKEN_STRING);
	string CacheFileName = Params.GetString();
	Params.Next();

	Params.AssertSymbol(';');
	Params.Next();

	return new Terrain(Name, Group, CX, CZ, HeightmapFileName, VertexDistance, MinY, MaxY, FormDescFileName,
		FormMapFileName, CacheFileName);
}

void Terrain::RegisterResourceType()
{
	res::g_Manager->RegisterResourceType("Terrain", &Terrain::Create);
}

Terrain::Terrain(const string &Name, const string &Group,
	uint CX, uint CZ, const string &HeightmapFileName, float VertexDistance, float MinY, float MaxY,
	const string &FormDescFileName, const string &FormMapFileName, const string &CacheFileName)
:
	D3dResource(Name, Group),
	pimpl(new Terrain_pimpl)
{
	assert(VertexDistance > 0.0f);

	pimpl->m_HeightmapFileName = HeightmapFileName;
	pimpl->m_CX = CX;
	pimpl->m_CZ = CZ;
	pimpl->m_VertexDistance = VertexDistance;
	pimpl->m_VertexDistanceInv = 1.0f / VertexDistance;
	pimpl->m_MinY = MinY;
	pimpl->m_MaxY = MaxY;
	pimpl->m_FormDescFileName = FormDescFileName;
	pimpl->m_FormMapFileName = FormMapFileName;
	pimpl->m_CacheFileName = CacheFileName;
}

Terrain::~Terrain()
{
	pimpl.reset();
}

const string & Terrain::GetHeightmapFileName() { return pimpl->m_HeightmapFileName; }
uint Terrain::GetCX() { return pimpl->m_CX; }
uint Terrain::GetCZ() { return pimpl->m_CZ; }
float Terrain::GetVertexDistance() { return pimpl->m_VertexDistance; }
float Terrain::GetMinY() { return pimpl->m_MinY; }
float Terrain::GetMaxY() { return pimpl->m_MaxY; }

void Terrain::GetBoundingBox(BOX *OutBox)
{
	OutBox->p1.x = 0.f;
	OutBox->p1.y = pimpl->m_MinY;
	OutBox->p1.z = 0.f;

	OutBox->p2.x = (float)pimpl->m_CX * pimpl->m_VertexDistance;
	OutBox->p2.y = pimpl->m_MaxY;
	OutBox->p2.z = (float)pimpl->m_CZ * pimpl->m_VertexDistance;
}

uint Terrain::GetPatchCount()
{
	return pimpl->m_PatchCX * pimpl->m_PatchCZ;
}

float Terrain::GetHeight(int x, int z)
{
	if (x < 0) x = 0; else if (x > (int)pimpl->m_CX) x = pimpl->m_CX;
	if (z < 0) z = 0; else if (z > (int)pimpl->m_CZ) z = pimpl->m_CZ;

	return pimpl->GetHeight_Float((uint)x, (uint)z);
}

float Terrain::GetHeight(float x, float z)
{
	return pimpl->GetHeight_Float(x, z);
}

uint Terrain::GetFormIndex(int x, int z)
{
	x = minmax<int>(0, x, (int)pimpl->m_CX);
	z = minmax<int>(0, z, (int)pimpl->m_CZ);
	return (uint)pimpl->m_FormMap[z * (pimpl->m_CX+1) + x];
}

uint Terrain::GetFormIndex(float x, float z)
{
	return GetFormIndex(round(x * pimpl->m_VertexDistanceInv), round(z * pimpl->m_VertexDistanceInv));
}

uint Terrain::GetFormTag(int x, int z)
{
	return pimpl->m_FormDescData[GetFormIndex(x, z)]->Tag;
}

uint Terrain::GetFormTag(float x, float z)
{
	return pimpl->m_FormDescData[GetFormIndex(x, z)]->Tag;
}

void Terrain::CalcAreaStats(const RECTF &Area, float *OutMinHeight, float *OutMaxHeight)
{
	// ZnajdŸ zakres wierzcho³ków zawarty w ca³oœci w podanym prostok¹cie
	int x1 = round(ceilf(Area.left * pimpl->m_VertexDistanceInv));
	int z1 = round(ceilf(Area.top  * pimpl->m_VertexDistanceInv));
	int x2 = round(floorf(Area.right  * pimpl->m_VertexDistanceInv));
	int z2 = round(floorf(Area.bottom * pimpl->m_VertexDistanceInv));

	// Ogranicz te indeksy wierzcho³ków do poprawnego zakresu
	x1 = minmax(0, x1, (int)pimpl->m_CX);
	x2 = minmax(0, x2, (int)pimpl->m_CX);
	z1 = minmax(0, z1, (int)pimpl->m_CZ);
	z2 = minmax(0, z2, (int)pimpl->m_CZ);

	// Inicjalizacja - wierzcho³ek left-top
	float Height = GetHeight(Area.left, Area.top);
	*OutMinHeight = Height;
	*OutMaxHeight = Height;

	// Przelicz wierzcho³ki
	int x, z;
	float xf, zf;
	for (z = z1; z <= z2; z++)
	{
		for (x = x1; x <= x2; x++)
		{
			Height = GetHeight(x, z);
			if (Height < *OutMinHeight) *OutMinHeight = Height;
			if (Height > *OutMaxHeight) *OutMaxHeight = Height;
		}
	}

	// Lewa krawêdŸ, prawa krawêdŸ
	for (z = z1, zf = z1 * pimpl->m_VertexDistance; z <= z2; z++, zf += pimpl->m_VertexDistance)
	{
		Height = GetHeight(Area.left, zf);
		if (Height < *OutMinHeight) *OutMinHeight = Height;
		if (Height > *OutMaxHeight) *OutMaxHeight = Height;

		Height = GetHeight(Area.right, zf);
		if (Height < *OutMinHeight) *OutMinHeight = Height;
		if (Height > *OutMaxHeight) *OutMaxHeight = Height;
	}

	// KrawêdŸ bliska, krawêdŸ daleka
	for (x = x1, xf = x1 * pimpl->m_VertexDistance; x <= x2; x++, xf += pimpl->m_VertexDistance)
	{
		Height = GetHeight(xf, Area.top);
		if (Height < *OutMinHeight) *OutMinHeight = Height;
		if (Height > *OutMaxHeight) *OutMaxHeight = Height;

		Height = GetHeight(xf, Area.bottom);
		if (Height < *OutMinHeight) *OutMinHeight = Height;
		if (Height > *OutMaxHeight) *OutMaxHeight = Height;
	}

	// Wierzcho³ek right-top
	Height = GetHeight(Area.right, Area.top);
	if (Height < *OutMinHeight) *OutMinHeight = Height;
	if (Height > *OutMaxHeight) *OutMaxHeight = Height;

	// Wierzcho³ek left-bottom
	Height = GetHeight(Area.left, Area.bottom);
	if (Height < *OutMinHeight) *OutMinHeight = Height;
	if (Height > *OutMaxHeight) *OutMaxHeight = Height;

	// Wierzcho³ek right-bottom
	Height = GetHeight(Area.right, Area.bottom);
	if (Height < *OutMinHeight) *OutMinHeight = Height;
	if (Height > *OutMaxHeight) *OutMaxHeight = Height;
}

uint Terrain::GetTagFromFormIndex(uint FormIndex)
{
	assert(FormIndex < pimpl->m_FormDescData.size());
	return pimpl->m_FormDescData[FormIndex]->Tag;
}

const string & Terrain::GetTextureNameFromFormIndex(uint FormIndex)
{
	assert(FormIndex < pimpl->m_FormDescData.size());
	return pimpl->m_FormDescData[FormIndex]->TextureName;
}

float Terrain::ValueToHeight(uint1 Value)
{
	return Value / 256.0f * (pimpl->m_MaxY - pimpl->m_MinY) + pimpl->m_MinY;
}

void Terrain::CalcPatchBoundingBox(BOX *OutBox, uint PatchIndex)
{
	uint x = PatchIndex % pimpl->m_PatchCX;
	uint z = PatchIndex / pimpl->m_PatchCX;

	OutBox->p1.x = x * PATCH_SIZE * pimpl->m_VertexDistance;
	OutBox->p1.z = z * PATCH_SIZE * pimpl->m_VertexDistance;
	OutBox->p1.y = pimpl->m_Patches[PatchIndex].MinY;
	OutBox->p2.x = (x+1) * PATCH_SIZE * pimpl->m_VertexDistance;
	OutBox->p2.z = (z+1) * PATCH_SIZE * pimpl->m_VertexDistance;
	OutBox->p2.y = pimpl->m_Patches[PatchIndex].MaxY;
}

void Terrain::GetPatchFormTextureNames(uint PatchIndex, string OutTextureNames[TERRAIN_FORMS_PER_PATCH])
{
	for (uint ti = 0; ti < TERRAIN_FORMS_PER_PATCH; ti++)
		OutTextureNames[ti] = pimpl->m_FormDescData[pimpl->m_Patches[PatchIndex].TerrainForms[ti]]->TextureName;
}

void Terrain::GetPatchTexScale(uint PatchIndex, float OutTexScale[TERRAIN_FORMS_PER_PATCH])
{
	for (uint ti = 0; ti < TERRAIN_FORMS_PER_PATCH; ti++)
		OutTexScale[ti] = pimpl->m_FormDescData[pimpl->m_Patches[PatchIndex].TerrainForms[ti]]->TexScale;
}

void Terrain::CalcVisiblePatches(std::vector<uint> *OutPatchIndices, const FRUSTUM_PLANES &FrustumPlanes, const FRUSTUM_POINTS &FrustumPoints)
{
	OutPatchIndices->clear();

	BOX FrustumBox; FrustumPoints.CalcBoundingBox(&FrustumBox);
	float PatchCX, PatchCZ;
	PatchCX = PatchCZ = PATCH_SIZE * pimpl->m_VertexDistance;

	int x1 = minmax(0, roundo(floorf(FrustumBox.p1.x / PatchCX)), (int)pimpl->m_PatchCX-1);
	int z1 = minmax(0, roundo(floorf(FrustumBox.p1.z / PatchCZ)), (int)pimpl->m_PatchCZ-1);
	int x2 = minmax(0, roundo(ceilf(FrustumBox.p2.x / PatchCX)), (int)pimpl->m_PatchCX-1);
	int z2 = minmax(0, roundo(ceilf(FrustumBox.p2.z / PatchCZ)), (int)pimpl->m_PatchCZ-1);
	int x, z, pi;
	BOX PatchBox;
	for (z = z1; z <= z2; z++)
	{
		for (x = x1; x <= x2; x++)
		{
			pi = z * pimpl->m_PatchCX + x;
			CalcPatchBoundingBox(&PatchBox, pi);
			if (BoxToFrustum_Fast(PatchBox, FrustumPlanes))
				OutPatchIndices->push_back(pi);
		}
	}
}

bool Terrain::RayCollision(const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT, float MaxT)
{
	// ZnajdŸ patch, w którym le¿y pocz¹tek tego promienia
	float WorldToPatchFactor = pimpl->m_VertexDistanceInv / PATCH_SIZE;
	float PatchToWorldFactor = 1.0f / WorldToPatchFactor;
	int next_px, curr_px = round(floorf(RayOrig.x * WorldToPatchFactor));
	int next_pz, curr_pz = round(floorf(RayOrig.z * WorldToPatchFactor));
	uint PatchIndex;
	// Nie optymalizuje szybkiego przechodzenia do prawid³owego zakresu px,pz bo uznajê,
	// ¿e nie bêdzie promieni rzucanych dalego spoza terenu - teren powinien byæ ca³ym œwiatem gry.
	float curr_t = 0.0f, next_t;

	// Przypadek szczególny: promieñ pionowy
	if (FLOAT_ALMOST_ZERO(RayDir.x) && FLOAT_ALMOST_ZERO(RayDir.z))
	{
		PatchIndex = (uint)curr_pz * pimpl->m_PatchCX + (uint)curr_px;
		if (RayOrig.x < 0.0f ||
			RayOrig.z < 0.0f ||
			RayOrig.x > pimpl->m_CX * pimpl->m_VertexDistance ||
			RayOrig.z > pimpl->m_CZ * pimpl->m_VertexDistance)
		{
			return false;
		}
		else if (
			RayDir.y > 0.0f && RayOrig.y > pimpl->m_Patches[PatchIndex].MaxY ||
			RayDir.y < 0.0f && RayOrig.y < pimpl->m_Patches[PatchIndex].MinY)
		{
			return false;
		}
		else
			return pimpl->RayCollision_Patch_Vertical(curr_px, curr_pz, RayOrig, RayDir, OutT, MaxT);
	}

	float bound_x, bound_z, x1, x2, z1, z2, y1, y2;
	int What; // 0 = nic, 1 = return false, 2 = continue
	for (;;)
	{
		What = 0;
		// Przejœcie do patcha na prawo
		if (RayDir.x > 0.0f)
		{
			// Oblicz pozycjê we wsp. œwiata prawej granicy bie¿¹cego patcha
			bound_x = (float)(curr_px + 1) * PatchToWorldFactor;
			// Oblicz parametr T dla prawej granicy bie¿¹cego patcha
			next_t = (bound_x - RayOrig.x) / RayDir.x;
			// Oblicz parametr Z dla prawej granicy bie¿¹cego patcha
			bound_z = RayOrig.z + RayDir.z * next_t;
			// Jeœli Z jest w zakresie tego patcha, znaleziono przejœcie do patcha na prawo
			z1 = (float)(curr_pz    ) * PatchToWorldFactor;
			z2 = (float)(curr_pz + 1) * PatchToWorldFactor;
			if (bound_z >= z1 && bound_z <= z2)
			{
				next_px = curr_px + 1;
				next_pz = curr_pz;
				// jeœli wyszliœmy poza teren w t¹ stronê, koniec, nie ma kolizji
				if (next_px >= (int)pimpl->m_PatchCX) What = 1;
				// Jeœli koniec ustalonego zakresu parametru T, podobnie
				else if (next_t >= MaxT) What = 1;
				else What = 2;
			}
		}
		// Przejœcie do patcha na lewo
		else if (RayDir.x < 0.0f)
		{
			bound_x = (float)(curr_px) * PatchToWorldFactor;
			next_t = (bound_x - RayOrig.x) / RayDir.x;
			bound_z = RayOrig.z + RayDir.z * next_t;
			z1 = (float)(curr_pz    ) * PatchToWorldFactor;
			z2 = (float)(curr_pz + 1) * PatchToWorldFactor;
			if (bound_z >= z1 && bound_z <= z2)
			{
				next_px = curr_px - 1;
				next_pz = curr_pz;
				if (next_px < 0) What = 1;
				else if (next_t >= MaxT) What = 1;
				else What = 2;
			}
		}

		if (What == 0)
		{
			// Przejœcie do patcha na dalej
			if (RayDir.z > 0.0f)
			{
				bound_z = (float)(curr_pz + 1) * PatchToWorldFactor;
				next_t = (bound_z - RayOrig.z) / RayDir.z;
				bound_x = RayOrig.x + RayDir.x * next_t;
				x1 = (float)(curr_px    ) * PatchToWorldFactor;
				x2 = (float)(curr_px + 1) * PatchToWorldFactor;
				if (bound_x >= x1 && bound_x <= x2)
				{
					next_px = curr_px;
					next_pz = curr_pz + 1;
					if (next_pz >= (int)pimpl->m_PatchCZ) What = 1;
					else if (next_t >= MaxT) What = 1;
					else What = 2;
				}
			}
			// Przejœcie do patcha na bli¿ej
			else if (RayDir.z < 0.0f)
			{
				bound_z = (float)(curr_pz) * PatchToWorldFactor;
				next_t = (bound_z - RayOrig.z) / RayDir.z;
				bound_x = RayOrig.x + RayDir.x * next_t;
				x1 = (float)(curr_px    ) * PatchToWorldFactor;
				x2 = (float)(curr_px + 1) * PatchToWorldFactor;
				if (bound_x >= x1 && bound_x <= x2)
				{
					next_px = curr_px;
					next_pz = curr_pz - 1;
					if (next_pz < 0) What = 1;
					else if (next_t >= MaxT) What = 1;
					else What = 2;
				}
			}
		}

		// Jeœli patch jest w zakresie
		if (curr_px >= 0 && curr_pz >= 0 && curr_px < (int)pimpl->m_PatchCX && curr_pz < (int)pimpl->m_PatchCZ)
		{
			
			// Oblicz zakres Y dla promienia w tym patchu, sprawdŸ z MinY i MaxY tego patcha
			y1 = RayOrig.y + RayDir.y * curr_t;
			y2 = RayOrig.y + RayDir.y * next_t;
			PatchIndex = (uint)curr_pz * pimpl->m_PatchCX + (uint)curr_px;
			if (!(
				(y1 < pimpl->m_Patches[PatchIndex].MinY && y2 < pimpl->m_Patches[PatchIndex].MinY) ||
				(y1 > pimpl->m_Patches[PatchIndex].MaxY && y2 > pimpl->m_Patches[PatchIndex].MaxY) ))
			{
				// SprawdŸ kolizjê z tym patchem
				if (pimpl->RayCollision_Patch((uint)curr_px, (uint)curr_pz, RayOrig, RayDir, OutT, curr_t, next_t))
					return (*OutT <= MaxT);
			}
		}

		if (What == 1)
			return false;

		curr_px = next_px;
		curr_pz = next_pz;
		curr_t = next_t;
	}
}

void Terrain::SetupVbIbFvf()
{
	frame::Dev->SetStreamSource(0, pimpl->m_VB.get(), 0, sizeof(VERTEX));
	frame::Dev->SetIndices(pimpl->m_IB.get());
	frame::Dev->SetFVF(VERTEX::FVF);
}

void Terrain::DrawPatchGeometry(uint PatchIndex, const VEC3 &EyePos, float CamZFar)
{
	uint px = PatchIndex % pimpl->m_PatchCX;
	uint pz = PatchIndex / pimpl->m_PatchCX;
	uint x = px * PATCH_SIZE;
	uint z = pz * PATCH_SIZE;
	VEC2 PatchPoint;
	PatchPoint.x = minmax(x * pimpl->m_VertexDistance, EyePos.x, (x+1) * pimpl->m_VertexDistance);
	PatchPoint.y = minmax(z * pimpl->m_VertexDistance, EyePos.z, (z+1) * pimpl->m_VertexDistance);
	float Dist = DistanceSq(PatchPoint, VEC2(EyePos.x, EyePos.z)) / (CamZFar * CamZFar);
	// Ten magiczny wzór odwzorowuje przedzia³ Dist = 0.0 .. 1.0 na równe przedzia³y 0, 1, ..., LEVEL_COUNT
	uint Level = (uint)minmax<int>(0, round((Dist) * (float)LEVEL_COUNT - 0.5f), LEVEL_COUNT-1);
	uint TriangleCount = PATCH_LEVEL_INDEX_COUNT(PATCH_LEVEL_SIZE[Level]) / 3;
	// Test - bez skirt
	//TriangleCount = (PATCH_LEVEL_SIZE[Level])*(PATCH_LEVEL_SIZE[Level]) * 6 / 3;

	uint PatchBaseVertexIndex = pimpl->LoadPatch(PatchIndex);

	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST,
		PatchBaseVertexIndex,
		0,
		PATCH_VERTEX_COUNT,
		PATCH_INDEX_OFFSET[Level],
		TriangleCount) );
	frame::RegisterDrawCall(TriangleCount);
}

void Terrain::OnLoad()
{
	ERR_TRY;

	LOG(0x08, Format("Terrain: \"#\" Loading...") % GetName() % pimpl->m_HeightmapFileName);

	// Poniewa¿ zasób terenu mo¿e byæ te¿ potrzebny do liczenia kolizji itp., nie tylko do rysowania,
	// a z drugiej strony do jego wczytania trzeba byæ mo¿e u¿yæ tekstur D3D, podczas ³adowania
	// zasobu tego rodzaju urz¹dzenie nie mo¿e byæ utracone. Niestety. Nie wiem jak to lepiej rozwi¹zaæ.
	if (frame::Dev == NULL || frame::GetDeviceLost())
		throw Error(Format("Podczas ³adowania zasobu terenu \"#\" urz¹dzenie Direct3D nie istnieje lub jest utracone.") % GetName(), __FILE__, __LINE__);

	// Wczytaj Heightmapê
	pimpl->m_Heightmap.resize((pimpl->m_CX + 1) * (pimpl->m_CZ + 1));
	pimpl->LoadHeightmap(&pimpl->m_Heightmap[0]);
	pimpl->LoadFormDesc();
	pimpl->LoadFormMap();
	pimpl->GeneratePatches();
	pimpl->GenerateIndices();
	pimpl->CreateVB();

	D3dResource::OnLoad();

	ERR_CATCH(Format("Nie mo¿na wczytaæ zasobu terenu \"#\".") % GetName());
}

void Terrain::OnUnload()
{
	D3dResource::OnUnload();

	// ...
}

void Terrain::OnDeviceCreate()
{
	// ...
}

void Terrain::OnDeviceDestroy()
{
	// ...
}

void Terrain::OnDeviceRestore()
{
}

void Terrain::OnDeviceInvalidate()
{
}


} // namespace engine
