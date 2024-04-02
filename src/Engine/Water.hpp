/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef WATER_H_
#define WATER_H_

#include "Engine.hpp"

namespace engine
{

class Terrain;

class WaterBase_pimpl;

/*
Klasa bazowa wszelkich rodzaju wody.
Przechowuje parametry wody.
Parametry:
- DateTime - tak jak w ComplexSky.
  - Cz�� ca�kowita to numer dnia.
  - Cz�� po przecinku to pora dnia, od p�nocy (0.0) poprzez po�udnie (0.5) do nast�pnej p�nocy (1.0).
- NormalTexScale, CausticsTexScale - skalowanie tekstur
- DirToLight - kierunek DO �wiat�a. Znormalizowany.
- LightColor - kolor odblasku. Alpha nie ma znaczenia. BLACK, aby wy��czy� odblask.
- CausticsColor - kolor, a przede wszystkim jasno�� dodatkowych �lad�w w wodzie tzw. Caustics. Kana� Alpha nieu�ywany.
- WaterColor - kolor natywny wody. Alpha oznacza bazow� przezroczysto�� wody.
- SkyColors - kolory nieba do odbicia.
  first to kolor horyzontu, second to kolor zenitu.
*/
class WaterBase
{
public:
	WaterBase(Scene *a_Scene);

	float GetDateTime();
	float GetNormalTexScale();
	float GetCausticsTexScale();
	const VEC3 & GetDirToLight();
	const COLORF & GetLightColor();
	const COLORF & GetCausticsColor();

	void SetDateTime(float DateTime);
	void SetNormalTexScale(float NormalTexScale);
	void SetCausticsTexScale(float CausticsTexScale);
	void SetDirToLight(const VEC3 &DirToLight);
	void SetLightColor(const COLORF &LightColor);
	void SetCausticsColor(const COLORF &CausticsColor);

	RoundInterpolator<COLORF> & AccessWaterColor();
	RoundInterpolator<COLORF_PAIR> & AccessSkyColors();

protected:
	scoped_ptr<WaterBase_pimpl> base_pimpl;
};

class WaterEntity_pimpl;

/*
Woda jako encja.
Ma kszta�t albo quada rozci�gaj�cego si� w osiach X i Z o podane promienie,
albo wielok�ta, kt�rego wierzcho�ki powinny le�e� w p�aszczy�nie XZ.
*/
class WaterEntity : public CustomEntity, public WaterBase
{
public:
	WaterEntity(Scene *OwnerScene, float QuadHalfCX, float QuadHalfCZ);
	WaterEntity(Scene *OwnerScene, const VEC3 PolygonPoints[], uint PolygonPointCount);
	~WaterEntity();

	// Zmiana kszta�tu
	void SetQuad(float QuadHalfCX, float QuadHalfCZ);
	void SetPolygon(const VEC3 PolygonPoints[], uint PolygonPointCount);

	virtual void Draw(const ParamsCamera &Cam);
	virtual bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT);

private:
	scoped_ptr<WaterEntity_pimpl> pimpl;
};

class TerrainWater_pimpl;

/*
Woda po��czona z terenem.
Utworzy� obiekt klasy Scene, potem Terrain, potem TerrainWater, potem TerrainRenderer,
potem doda� TerrainRenderer do sceny. �adnego z nich nie trzeba wtedy zwalnia�, same si�
nawzajem pozwalniaj�.
*/
class TerrainWater : public WaterBase
{
public:
	TerrainWater(Scene *OwnerScene, Terrain *TerrainObj, float Height);
	~TerrainWater();

	////// Dla klasy Terrain
	void Draw(const ParamsCamera &Cam);

private:
	scoped_ptr<TerrainWater_pimpl> pimpl;
};

} // namespace engine

#endif
