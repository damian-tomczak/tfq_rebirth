/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Config - Obs³uga plików konfiguracyjnych
 * Dokumentacja: Patrz plik doc/Config.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef COMMON_CONFIG_H_
#define COMMON_CONFIG_H_

#include <typeinfo>

namespace common
{

class Stream;
class Config_pimpl;

// Abstrakcyjna klasa bazowa dla elementów konfiguracji
class Item
{
public:
	virtual ~Item() { }
};

// Wartoœæ tekstowa - przechowuje ³añcuch
class Value : public Item
{
public:
	string Data;

	Value() { }
	Value(const string &s) : Data(s) { }
};

// Lista, czyli tak naprawdê wektor stringów
class List : public Item
{
public:
	STRING_VECTOR Data;
};

/*
Konfiguracja - tej klasy nale¿y u¿ywaæ zarówno jako g³ówn¹ konfiguracjê jak i
u¿ywana jest do przechowywania podkonfiguracji.
Zawiera mapê: Klucz [string] => Element [Item]
*/
class Config : public Item
{
	DECLARE_NO_COPY_CLASS(Config)
	friend class Config_pimpl;

private:
	scoped_ptr<Config_pimpl> pimpl;

public:
	// Flagi bitowe
	// - Zapisywane dane czy element musi wczeœniej istnieæ. Zostaje zast¹piony
	static const uint4 CM_MUST_EXIST     = 0x02;
	// - Zapisywane dane czy element nie mo¿e wczeœniej istnieæ. Zostaje utworzony
	static const uint4 CM_MUST_NOT_EXIST = 0x04;
	// - W razie nieistnienia, tworzona jest ca³a hierarchia konfiguracji a¿ do zapisywanego elementu
	static const uint4 CM_CREATE_PATH    = 0x08;
	// - Zapisywany element, jeœli ju¿ istnieje, musi byæ tego samego typu co nowy
	static const uint4 CM_SAME_TYPE      = 0x10;

	Config();
	~Config();

	// Wczytuje konfiguracjê, w razie b³êdu rzuca wyj¹tek.
	void LoadFromString(const string &S);
	void LoadFromStream(Stream *S);
	void LoadFromFile(const string &FileName);
	// Zapisuje konfiguracjê, w razie b³êdu rzuca wyj¹tek.
	void SaveToString(string *S) const;
	void SaveToStream(Stream *S) const;
	void SaveToFile(const string &FileName) const;

	// Zwraca element o podanej œcie¿ce lub NULL, jeœli nie znaleziono.
	Item * GetItem(const string &Path);
	// Zwraca element o podanej œcie¿ce. Jeœli nie znaleziono, rzuca wyj¹tek.
	Item * MustGetItem(const string &Path);
	// Zwraca dane elementu o podanej œcie¿ce.
	// Jesli nie znaleziono lub to nie jest element z danymi, zwraca false, a Data jest niezdefiniowane.
	bool GetData(const string &Path, string *Data);
	// Zwraca dane elementu o podanej œcie¿ce.
	// Jesli nie znaleziono lub to nie jest element z danymi, rzuca wyj¹tek.
	void MustGetData(const string &Path, string *Data);

	// Zwraca true, jesli istnieje element o podanej œcie¿ce - niezale¿nie jakiego jest typu.
	bool ItemExists(const string &Path);
	// Jeœli element o podanej œcie¿ce nie istnieje, rzuca wyj¹tek.
	void ItemMustExists(const string &Path);

	// Ustawia element do podanej œciezki. Jeœli siê nie uda, zwraca false.
	bool SetItem(const string &Path, Item *a_Item, uint4 CreateMode);
	// Ustawia element do podanej œciezki. Jeœli siê nie uda, rzuca wyj¹tek.
	void MustSetItem(const string &Path, Item *a_Item, uint4 CreateMode);
	// Ustawia dane elementu tekstowego o podanej œcie¿ce. Jeœli sie nie uda, zwraca false.
	bool SetData(const string &Path, const string &Data, uint4 CreateMode);
	// Ustawia dane elementu tekstowego o podanej œcie¿ce. Jeœli siê nie uda, rzuca wyj¹tek.
	void MustSetData(const string &Path, const string &Data, uint4 CreateMode);

	// Usuwa element o podanej œcie¿ce.
	bool DeleteItem(const string &Path);
	void MustDeleteItem(const string &Path);

	// Zwraca listê nazw elementów
	void GetElementNames(STRING_VECTOR *OutNames);

	// Zwraca element podanego typu spod podanej œciezki.
	// Jako typ podawaæ pochodne Item np. DataItem czy Config.
	// Jeœli nie znaleziono lub element jest innego typu, zwraca NULL.
	template <typename T> T * GetItemEx(const string &Path);
	// Zwraca element podanego typu spod podanej œciezki.
	// Jeœli nie znaleziono lub element jest innego typu, rzuca wyj¹tek.
	template <typename T> T * MustGetItemEx(const string &Path);
	// Zwraca dane spod podanej œcie¿ki skonwertowane do podanego typu.
	// Jeœli nie znaleziono lub b³¹d konwersji, zwraca false.
	template <typename T> bool GetDataEx(const string &Path, T *Data);
	// Zwraca dane spod podanej œcie¿ki skonwertowane do podanego typu.
	// Jeœli nie znaleziono lub b³¹d konwersji, rzuca wyj¹tek.
	template <typename T> void MustGetDataEx(const string &Path, T *Data);
	// Jeœli element o podanej œcie¿ce nie istnieje lub nie jest takiego typu jak podany, zwraca false.
	template <typename T> bool ItemExistsEx(const string &Path);
	// Jeœli element o podanej œcie¿ce nie istnieje lub nie jest takiego typu jak podany, rzuca wyj¹tek.
	template <typename T> void ItemMustExistsEx(const string &Path);
	// Ustawia dane elementu tekstowego o podanej œcie¿ce na ³añcuch skonwertowany z podanej wartoœci danego typu.
	// Jeœli sie nie uda, zwraca false.
	template <typename T> bool SetDataEx(const string &Path, const T &Data, uint4 CreateMode);
	// Ustawia dane elementu tekstowego o podanej œcie¿ce na ³añcuch skonwertowany z podanej wartoœci danego typu.
	// Jeœli sie nie uda, rzuca wyj¹tek.
	template <typename T> void MustSetDataEx(const string &Path, const T &Data, uint4 CreateMode);
};

template <typename T>
inline T * Config::GetItemEx(const string &Path)
{
	Item *i = GetItem(Path);
	if (i == 0)
		return 0;
	if (dynamic_cast<T*>(i) == 0)
		return 0;
	return i;
}

template <typename T>
inline T * Config::MustGetItemEx(const string &Path)
{
	Item *i = MustGetItem(Path);
	T *R;
	if ((R = dynamic_cast<T*>(i)) == 0)
		throw Error("Nie mo¿na pobraæ elementu konfiguracji o œcie¿ce \"" + Path + "\". Typ elementu inny ni¿ oczekiwany.");
	return R;
}

template <typename T>
inline bool Config::GetDataEx(const string &Path, T *Data)
{
	string s;
	if (!GetData(Path, &s))
		return false;
	if (!StrToSth<T>(Data, s))
		return false;
	return true;
}

template <typename T>
inline void Config::MustGetDataEx(const string &Path, T *Data)
{
	string s;
	MustGetData(Path, &s);
	if (!StrToSth<T>(Data, s))
		throw Error("Nie mo¿na odczytaæ danych z konfiguracji spod œciezki: \"" + Path + "\". B³¹d konwersji na typ: " + string(typeid(T).name()));
}

template <typename T>
inline bool Config::ItemExistsEx(const string &Path)
{
	Item *i = GetItem(Path);
	if (i == 0)
		return false;
	if (dynamic_cast<T*>(i) == 0)
		return false;
	return true;
}

template <typename T>
inline void Config::ItemMustExistsEx(const string &Path)
{
	Item *i = GetItem(Path);
	if (i == 0)
		throw Error("Element konfiguracji o œcie¿ce \"" + Path + "\" nie istnieje.");
	if (dynamic_cast<T*>(i) == 0)
		throw Error("Element konfiguracji o œcie¿ce \"" + Path + "\" jest b³êdnego typu.");
}

template <typename T>
inline bool Config::SetDataEx(const string &Path, const T &Data, uint4 CreateMode)
{
	string s;
	SthToStr<T>(&s, Data);
	return SetData(Path, s, CreateMode);
}

template <typename T>
inline void Config::MustSetDataEx(const string &Path, const T &Data, uint4 CreateMode)
{
	string s;
	SthToStr<T>(&s, Data);
	MustSetData(Path, s, CreateMode);
}

// Konfiguracja g³ówna
extern scoped_ptr<Config> g_Config;

} // namespace common

#endif
