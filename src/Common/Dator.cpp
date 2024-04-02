/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Dator - Otoczka na pojedyncz¹ wartoœæ dowolnego typu
 * Dokumentacja: Patrz plik doc/Dator.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
#include "Dator.hpp"

namespace common
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa DatorGroup

bool DatorGroup::Delete(const string &Name)
{
	DATOR_MAP::iterator it = m_Dators.find(Name);
	if (it == m_Dators.end())
		return false;
	m_Dators.erase(it);
	return true;
}

bool DatorGroup::Exists(const string &Name)
{
	DATOR_MAP::iterator it = m_Dators.find(Name);
	return it != m_Dators.end();
}

int DatorGroup::SetValue(const string &Name, const string &Value)
{
	DATOR_MAP::iterator it = m_Dators.find(Name);
	if (it == m_Dators.end())
		return 1;
	return it->second->SetValue(Value) ? 0 : 2;
}

int DatorGroup::GetValue(const string &Name, string *Value)
{
	DATOR_MAP::iterator it = m_Dators.find(Name);
	if (it == m_Dators.end())
		return 1;
	return it->second->GetValue(Value) ? 0 : 2;
}

} // namespace common
