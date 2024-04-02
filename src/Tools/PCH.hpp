/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500

#include "Base.hpp"
#include "Math.hpp"
#include "Error.hpp"
#include "Stream.hpp"
#include "Files.hpp"
#include "Profiler.hpp"
#include "Tokenizer.hpp"

#include <map>

#include <windows.h>
#include <d3dx9.h>

using namespace common;

void Write(const string &s);
void Writeln(const string &s);
void Writeln();
// OnceId - je�li niezerowy, tylko jedno ostrze�enie tego typu b�dzie wygenerowane a nast�pne ju� nie
void Warning(const string &s, uint OnceId = 0);
void ThrowCmdLineSyntaxError();

// Parsuje warto�� parametru jako ci�g pod-parametr�w nazwa1:warto��1|nazwa2:warto��2|sama_nazwa|...
class SubParameters
{
private:
	typedef std::pair<string, string> STRING_PAIR;
	typedef std::vector<STRING_PAIR> STRING_PAIR_VECTOR;
	STRING_PAIR_VECTOR m_Data;
	string m_EmptyString;

public:
	SubParameters(const string &s);

	bool IsEmpty() { return m_Data.empty(); }
	uint GetCount() { return m_Data.size(); }
	// Przy przekroczeniu zakresu rzuca wyj�tek
	const string & GetKey(uint Index);
	// Przy przekroczeniu zakresu rzuca wyj�tek
	const string & GetValue(uint Index);
	bool KeyExists(const string &Key);
	// Je�li nie ma, zwraca string()
	const string & GetValue(const string &Key);
	// Je�li nie ma, rzuca wyj�tek
	const string & MustGetValue(const string &Key);
};

struct Task
{
	virtual ~Task();
};
