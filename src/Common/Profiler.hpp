/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Profiler - Przyrz�d do pomiar�w czasu i wydajno�ci
 * Dokumentacja: Patrz plik doc/Profiler.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef COMMON_PROFILER_H_
#define COMMON_PROFILER_H_

#include <stack> // :(

namespace common
{

class Profiler;

class ProfilerItem
{
private:
	// Sumaryczny czas wykonania [s]
	double m_Time;
	// Liczba przebieg�w
	double m_Count;
	// Czas rozpocz�cia bie��cego przebiegu [s]
	double m_StartTime;
	string m_strName; // Nazwa elementu
	std::vector<ProfilerItem> m_ItemVector; // Podelementy

	friend class Profiler;
		ProfilerItem* Begin(const string &Name);
		void Start();
		void Stop();
public:
	ProfilerItem(const string &Name);

	const string &GetName() { return m_strName; }
	bool Empty() { return (around(m_Count, 0.1)); }
	double GetCount() { return m_Count; }
	double GetAvgTime() { return ( Empty() ? 0.0 : m_Time / m_Count ); }
	size_t GetItemCount() { return m_ItemVector.size(); }
	ProfilerItem* GetItem(size_t index) { return &m_ItemVector[index]; }
	void FormatString(string *S, unsigned dwLevel);
};

class Profiler
{
private:
	std::stack<ProfilerItem*> m_ItemStack;
	ProfilerItem m_DefaultItem;

public:
	Profiler();
	Profiler(const string &Name);
	void Begin(const string &Name);
	void End();
	ProfilerItem* GetRootItem() { return &m_DefaultItem; }
	// Zapisuje ca�e drzewo profilu do �a�cucha - ka�da pozycja w osobnym wierszu
	// Wci�cia to dwie spacje "  "
	// Ko�ce wiersza to \n
	// Jednostka to milisekundy
	void FormatString(string *S);
};

// Klasa, kt�rej obiekt mo�esz dla wygody utworzy� zamiast wywo�ywa� Begin i End
// profilera. Tworzenie tego obiektu wywo�uje Begin, a usuwanie - End.
// Obiekt automatyczny utworzony na stosie sam si� usunie przy wyj�ciu
// z funkcji. Dlatego warto tworzy� obiekt tej klasy jako zmienn� lokaln� o byle
// jakiej nazwie i nieu�ywan� nigdzie dalej na pocz�tku ka�dej profilowanej
// funkcji.
class Profile
{
private:
	Profiler &m_Profiler;
public:
	// U�ywa g_Profiler
	Profile(const string &Name);
	// U�ywa podanego, w�asnego profilera
	Profile(const string &Name, Profiler &Profiler_);
	~Profile();
};

// G��wny profiler globalny dla w�tku g��wnego
extern Profiler g_Profiler;

} // namespace common

// Dla jeszcze wi�szej wygody, zamiast tworzy� obiekt klasy Profile wystarczy
// na pocz�tku guardowanej do profilowania funkcji czy dowolnego bloku { }
// postawi� to makro.
#define PROFILE_GUARD(Name) common::Profile __profile_guard_object(Name);

#endif

