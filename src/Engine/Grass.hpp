/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef GRASS_H_
#define GRASS_H_

class ParamsCamera;

namespace engine
{

class Scene;
class Terrain;

class Grass_pimpl;

/*
Wczytuje, przechowuje i rysuje wszelk¹ trawê na scenie.
Jeœli ma byæ trawa, tworzyæ jeden obiekt tej klasy i przekazaæ go obiektowi
klasy TerrainRenderer.
*/
class Grass
{
public:
	Grass(Scene *OwnerScene, Terrain *TerrainObj, const string &GrassDescFileName, const string &DensityMapFileName);
	~Grass();

	void Draw(const ParamsCamera &Cam, float GrassZFar, const COLORF &Color);

private:
	scoped_ptr<Grass_pimpl> pimpl;
};

} // namespace engine

#endif
