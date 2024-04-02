/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Profiler - Przyrz¹d do pomiarów czasu i wydajnoœci
 * Dokumentacja: Patrz plik doc/Profiler.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
#include "Profiler.hpp"


namespace common
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ProfilerItem

ProfilerItem::ProfilerItem(const string &Name)
{
	m_Time = 0.0;
	m_Count = 0.0;
	m_strName = Name;
}

ProfilerItem* ProfilerItem::Begin(const string &Name)
{
	// Poszukiwanie istniej¹cego elementu
	for (std::vector<ProfilerItem>::iterator it = m_ItemVector.begin(); it != m_ItemVector.end(); ++it)
	{
		if (it->GetName() == Name)
			return &(*it);
	}
	// Nie znaleziono - utworzenie nowego
	m_ItemVector.push_back( ProfilerItem(Name) );
	return &(*(--m_ItemVector.end()));
}

void ProfilerItem::Start()
{
	m_StartTime = g_Timer.GetTimeD();
}

void ProfilerItem::Stop()
{
	m_Time += ( g_Timer.GetTimeD() - m_StartTime );
	m_Count += 1.0;
}

void ProfilerItem::FormatString(string *S, unsigned dwLevel)
{
	unsigned i;
	if (dwLevel > 0)
	{
		string Tmp;
		DupeString(&Tmp, "  ", dwLevel-1);
		*S += Tmp;
		*S += Format("# : # ms (#)\n")
			% m_strName
			% DoubleToStrR(GetAvgTime()*1000.0)
			% UintToStrR(static_cast<uint4>(GetCount()));
	}
	for (i = 0; i < GetItemCount(); i++)
		GetItem(i)->FormatString(S, dwLevel+1);
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Profiler

Profiler::Profiler() : m_DefaultItem("")
{
	m_ItemStack.push(&m_DefaultItem);
}

Profiler::Profiler(const string &Name) : m_DefaultItem(Name)
{
	m_ItemStack.push(&m_DefaultItem);
}

void Profiler::Begin(const string &Name)
{
	ProfilerItem* pItem = ( m_ItemStack.top() )->Begin(Name);
	pItem->Start();
	m_ItemStack.push(pItem);
}

void Profiler::End()
{
	if (m_ItemStack.size() > 1)
	{
		ProfilerItem* pItem = m_ItemStack.top();
		pItem->Stop();
		m_ItemStack.pop();
	}
	else
		// B³¹d !!!
		;
}

void Profiler::FormatString(string *S)
{
	S->clear();
	GetRootItem()->FormatString(S, 0);
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Profile

Profile::Profile(const string &Name) :
	m_Profiler(g_Profiler)
{
	m_Profiler.Begin(Name);
}

Profile::Profile(const string &Name, Profiler &Profiler_) :
	m_Profiler(Profiler_)
{
	m_Profiler.Begin(Name);
}

Profile::~Profile()
{
	m_Profiler.End();
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Elementy globalne

Profiler g_Profiler;

} // namespace common
