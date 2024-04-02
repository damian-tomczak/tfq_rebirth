/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "PCH.hpp"
#include <set>
#include <iostream>

std::set<uint> g_WarningIds;

Task::~Task()
{
}

void Write(const string &s)
{
	string s2;
	Charset_Convert(&s2, s, CHARSET_WINDOWS, CHARSET_IBM);
	std::cout << s2;
}

void Writeln(const string &s)
{
	string s2;
	Charset_Convert(&s2, s, CHARSET_WINDOWS, CHARSET_IBM);
	std::cout << s2 << std::endl;
}

void Writeln()
{
	std::cout << std::endl;
}

void Warning(const string &s, uint OnceId)
{
	if (OnceId > 0)
		if (g_WarningIds.find(OnceId) != g_WarningIds.end())
			return;

	Write("Warning: ");
	Writeln(s);

	g_WarningIds.insert(OnceId);
}

void ThrowCmdLineSyntaxError()
{
	throw Error("Command line syntax error.");
}

SubParameters::SubParameters(const string &s)
{
	uint Index = 0;
	string Tmp;

	while (Split(s, "|", &Tmp, &Index))
	{
		uint ColonPos = Tmp.find(':');
		if (ColonPos == string::npos)
			m_Data.push_back(STRING_PAIR(Tmp, string()));
		else
			m_Data.push_back(STRING_PAIR(Tmp.substr(0, ColonPos), Tmp.substr(ColonPos+1)));
	}
}

const string & SubParameters::GetKey(uint Index)
{
	if (Index >= GetCount())
		throw Error("Too few sub-parameters.");

	return m_Data[Index].first;
}

const string & SubParameters::GetValue(uint Index)
{
	if (Index >= GetCount())
		throw Error("Too few sub-parameters.");

	return m_Data[Index].second;
}

bool SubParameters::KeyExists(const string &Key)
{
	for (uint i = 0; i < m_Data.size(); i++)
		if (m_Data[i].first == Key)
			return true;
	return false;
}

const string & SubParameters::GetValue(const string &Key)
{
	for (uint i = 0; i < m_Data.size(); i++)
		if (m_Data[i].first == Key)
			return m_Data[i].second;
	return m_EmptyString;
}

const string & SubParameters::MustGetValue(const string &Key)
{
	for (uint i = 0; i < m_Data.size(); i++)
		if (m_Data[i].first == Key)
			return m_Data[i].second;
	throw Error(Format("Missing sub-parameter \"#\".") % Key);
	return m_EmptyString; // Compiler's Food.
}
