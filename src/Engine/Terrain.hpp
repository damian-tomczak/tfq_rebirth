/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef TERRAIN_H_
#define TERRAIN_H_

#include "..\Framework\Res_d3d.hpp"

class ParamsCamera;

namespace engine
{

// Liczba musi byæ miêdzy 2 a 5.
const uint TERRAIN_FORMS_PER_PATCH = 4;

struct Terrain_pimpl;

class Terrain : public res::D3dResource
{
public:
	static res::IResource * Create(const string &Name, const string &Group, Tokenizer &Params);
	static void RegisterResourceType();

	Terrain(const string &Name, const string &Group,
		uint CX, uint CZ, const string &HeightmapFileName, float VertexDistance, float MinY, float MaxY,
		const string &FormDescFileName, const string &FormMapFileName, const string &CacheFileName);
	~Terrain();

	const string & GetHeightmapFileName();
	uint GetCX();
	uint GetCZ();
	float GetVertexDistance();
	float GetMinY();
	float GetMaxY();
	// Zwraca potencjalny AABB ca³ego terenu (tzn. nie uzglêdniaj¹cy ¿e mo¿e naprawdê nie byæ minimalnej albo maksymalnej wysokoœci).
	void GetBoundingBox(BOX *OutBox);
	// Zwraca liczbê patchów
	uint GetPatchCount();

	// W tych funkcjach indeksy mog¹ wychodziæ poza zakres, s¹ wtedy ograniczane do poprawnego zakresu (ang. "clamp").

	float GetHeight(int x, int z);
	float GetHeight(float x, float z);
	// Zwraca indeks formy terenu z podanego miejsca
	uint GetFormIndex(int x, int z);
	uint GetFormIndex(float x, float z);
	// Zwraca Tag (znacznik u¿ytkownika do dowolnego wykorzystania) formy terenu z podanego miejsca
	uint GetFormTag(int x, int z);
	uint GetFormTag(float x, float z);

	// Zwraca statystyki wysokoœci dla podanego prostok¹tnego obszaru.
	// Prostok¹t musi byæ prawid³owy (left < right, top < bottom)
	void CalcAreaStats(const RECTF &Area, float *OutMinHeight, float *OutMaxHeight);

	// Zwraca tag dla formy terenu o podanym indeksie
	uint GetTagFromFormIndex(uint FormIndex);
	// Zwraca nazwê tekstury dla formy terenu o podanym indeksie
	const string & GetTextureNameFromFormIndex(uint FormIndex);
	// Zwraca wysokoœæ dla podanej wartoœci heightmapy
	float ValueToHeight(uint1 Value);

	// Zwraca AABB danego patcha
	void CalcPatchBoundingBox(BOX *OutBox, uint PatchIndex);
	// Zwraca nazwy tekstur u¿ywanych w danym patchu
	void GetPatchFormTextureNames(uint PatchIndex, string OutTextureNames[TERRAIN_FORMS_PER_PATCH]);
	// Zwraca skalowanie tekstur u¿ywanych w danych patchu
	void GetPatchTexScale(uint PatchIndex, float OutTexScale[TERRAIN_FORMS_PER_PATCH]);
	// Zwraca indeksy patchów koliduj¹cych z podanym frustumem
	void CalcVisiblePatches(std::vector<uint> *OutPatchIndices, const FRUSTUM_PLANES &FrustumPlanes, const FRUSTUM_POINTS &FrustumPoints);

	// Liczy kolizjê z promieniem
	bool RayCollision(const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT, float MaxT = MAXFLOAT);

	// Wywo³uje SetStreamSource, SetIndices, SetFVF.
	void SetupVbIbFvf();
	// Rysuje geometriê patcha.
	// Nie przestawia ¿adnych stanów.
	void DrawPatchGeometry(uint PatchIndex, const VEC3 &EyePos, float CamZFar);
	
protected:
	// ======== Implementacja IResource ========
	virtual void OnLoad();
	virtual void OnUnload();

	// ======== Implementacja D3dResource ========
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

private:
	scoped_ptr<Terrain_pimpl> pimpl;
};

} // namespace engine

#endif
