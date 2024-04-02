/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#pragma once

// Abstrakcyjna klasa bazowa dla zadañ
struct MapTask : public Task
{
	virtual ~MapTask() { } // Klasa jest polimorficzna
};

// Wczytanie pliku
// Typ rozpoznawany po rozszerzeniu.
struct InputMapTask : public MapTask
{
	string FileName;
};

// Zapisanie pliku
// Typ rozpoznawany po rozszerzeniu.
struct OutputMapTask : public MapTask
{
	string FileName;
};

// Pokazanie informacji o siatce
struct InfoMapTask : public MapTask
{
	bool Detailed;
};

struct MapJob
{
	std::vector< common::shared_ptr<MapTask> > Tasks;

	BOX Bounds; // BOX::ZERO jeœli nieokreœlone
	string DescFileName;
	// Parametry dla octree
	float OctreeK;
	uint MaxOctreeDepth;

	MapJob();
	~MapJob();
};

void DoMapJob(MapJob &Job);
