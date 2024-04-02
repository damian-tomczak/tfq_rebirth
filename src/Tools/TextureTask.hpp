/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#pragma once

// Abstrakcyjna klasa bazowa dla zadañ
struct TextureTask : public Task
{
	virtual ~TextureTask() { } // Klasa jest polimorficzna
};

// Wygenerowanie tekstury losowej
struct GenRandTextureTask : public TextureTask
{
	uint TextureWidth, TextureHeight;

	GenRandTextureTask(const string &Params);
};

// Wczytanie pliku
// Typ rozpoznawany po rozszerzeniu.
struct InputTextureTask : public TextureTask
{
	string FileName;
};

// Zapisanie pliku
// Typ rozpoznawany po rozszerzeniu.
struct OutputTextureTask : public TextureTask
{
	string FileName;
};

// Pokazanie informacji o siatce
struct InfoTextureTask : public TextureTask
{
	bool Detailed;
};

struct SwizzleTextureTask : public TextureTask
{
	char Swizzle[4];

	SwizzleTextureTask(const string &Params);
};

struct SharpenAlphaTextureTask : public TextureTask
{
	COLOR BackgroundColor;

	SharpenAlphaTextureTask(const string &Params);
};

struct ClampTransparentTextureTask : public TextureTask { };


struct TextureJob
{
	uint1 AlphaThreshold;

	std::vector< common::shared_ptr<TextureTask> > Tasks;

	TextureJob();
	~TextureJob();
};

void DoTextureJob(TextureJob &Job);
