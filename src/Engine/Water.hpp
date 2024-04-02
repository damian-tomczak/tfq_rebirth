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
  - Czêœæ ca³kowita to numer dnia.
  - Czêœæ po przecinku to pora dnia, od pó³nocy (0.0) poprzez po³udnie (0.5) do nastêpnej pó³nocy (1.0).
- NormalTexScale, CausticsTexScale - skalowanie tekstur
- DirToLight - kierunek DO œwiat³a. Znormalizowany.
- LightColor - kolor odblasku. Alpha nie ma znaczenia. BLACK, aby wy³¹czyæ odblask.
- CausticsColor - kolor, a przede wszystkim jasnoœæ dodatkowych œladów w wodzie tzw. Caustics. Kana³ Alpha nieu¿ywany.
- WaterColor - kolor natywny wody. Alpha oznacza bazow¹ przezroczystoœæ wody.
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
Ma kszta³t albo quada rozci¹gaj¹cego siê w osiach X i Z o podane promienie,
albo wielok¹ta, którego wierzcho³ki powinny le¿eæ w p³aszczyŸnie XZ.
*/
class WaterEntity : public CustomEntity, public WaterBase
{
public:
	WaterEntity(Scene *OwnerScene, float QuadHalfCX, float QuadHalfCZ);
	WaterEntity(Scene *OwnerScene, const VEC3 PolygonPoints[], uint PolygonPointCount);
	~WaterEntity();

	// Zmiana kszta³tu
	void SetQuad(float QuadHalfCX, float QuadHalfCZ);
	void SetPolygon(const VEC3 PolygonPoints[], uint PolygonPointCount);

	virtual void Draw(const ParamsCamera &Cam);
	virtual bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT);

private:
	scoped_ptr<WaterEntity_pimpl> pimpl;
};

class TerrainWater_pimpl;

/*
Woda po³¹czona z terenem.
Utworzyæ obiekt klasy Scene, potem Terrain, potem TerrainWater, potem TerrainRenderer,
potem dodaæ TerrainRenderer do sceny. ¯adnego z nich nie trzeba wtedy zwalniaæ, same siê
nawzajem pozwalniaj¹.
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
