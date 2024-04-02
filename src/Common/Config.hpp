/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Config - Obs�uga plik�w konfiguracyjnych
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

// Abstrakcyjna klasa bazowa dla element�w konfiguracji
class Item
{
public:
	virtual ~Item() { }
};

// Warto�� tekstowa - przechowuje �a�cuch
class Value : public Item
{
public:
	string Data;

	Value() { }
	Value(const string &s) : Data(s) { }
};

// Lista, czyli tak naprawd� wektor string�w
class List : public Item
{
public:
	STRING_VECTOR Data;
};

/*
Konfiguracja - tej klasy nale�y u�ywa� zar�wno jako g��wn� konfiguracj� jak i
u�ywana jest do przechowywania podkonfiguracji.
Zawiera map�: Klucz [string] => Element [Item]
*/
class Config : public Item
{
	DECLARE_NO_COPY_CLASS(Config)
	friend class Config_pimpl;

private:
	scoped_ptr<Config_pimpl> pimpl;

public:
	// Flagi bitowe
	// - Zapisywane dane czy element musi wcze�niej istnie�. Zostaje zast�piony
	static const uint4 CM_MUST_EXIST     = 0x02;
	// - Zapisywane dane czy element nie mo�e wcze�niej istnie�. Zostaje utworzony
	static const uint4 CM_MUST_NOT_EXIST = 0x04;
	// - W razie nieistnienia, tworzona jest ca�a hierarchia konfiguracji a� do zapisywanego elementu
	static const uint4 CM_CREATE_PATH    = 0x08;
	// - Zapisywany element, je�li ju� istnieje, musi by� tego samego typu co nowy
	static const uint4 CM_SAME_TYPE      = 0x10;

	Config();
	~Config();

	// Wczytuje konfiguracj�, w razie b��du rzuca wyj�tek.
	void LoadFromString(const string &S);
	void LoadFromStream(Stream *S);
	void LoadFromFile(const string &FileName);
	// Zapisuje konfiguracj�, w razie b��du rzuca wyj�tek.
	void SaveToString(string *S) const;
	void SaveToStream(Stream *S) const;
	void SaveToFile(const string &FileName) const;

	// Zwraca element o podanej �cie�ce lub NULL, je�li nie znaleziono.
	Item * GetItem(const string &Path);
	// Zwraca element o podanej �cie�ce. Je�li nie znaleziono, rzuca wyj�tek.
	Item * MustGetItem(const string &Path);
	// Zwraca dane elementu o podanej �cie�ce.
	// Jesli nie znaleziono lub to nie jest element z danymi, zwraca false, a Data jest niezdefiniowane.
	bool GetData(const string &Path, string *Data);
	// Zwraca dane elementu o podanej �cie�ce.
	// Jesli nie znaleziono lub to nie jest element z danymi, rzuca wyj�tek.
	void MustGetData(const string &Path, string *Data);

	// Zwraca true, jesli istnieje element o podanej �cie�ce - niezale�nie jakiego jest typu.
	bool ItemExists(const string &Path);
	// Je�li element o podanej �cie�ce nie istnieje, rzuca wyj�tek.
	void ItemMustExists(const string &Path);

	// Ustawia element do podanej �ciezki. Je�li si� nie uda, zwraca false.
	bool SetItem(const string &Path, Item *a_Item, uint4 CreateMode);
	// Ustawia element do podanej �ciezki. Je�li si� nie uda, rzuca wyj�tek.
	void MustSetItem(const string &Path, Item *a_Item, uint4 CreateMode);
	// Ustawia dane elementu tekstowego o podanej �cie�ce. Je�li sie nie uda, zwraca false.
	bool SetData(const string &Path, const string &Data, uint4 CreateMode);
	// Ustawia dane elementu tekstowego o podanej �cie�ce. Je�li si� nie uda, rzuca wyj�tek.
	void MustSetData(const string &Path, const string &Data, uint4 CreateMode);

	// Usuwa element o podanej �cie�ce.
	bool DeleteItem(const string &Path);
	void MustDeleteItem(const string &Path);

	// Zwraca list� nazw element�w
	void GetElementNames(STRING_VECTOR *OutNames);

	// Zwraca element podanego typu spod podanej �ciezki.
	// Jako typ podawa� pochodne Item np. DataItem czy Config.
	// Je�li nie znaleziono lub element jest innego typu, zwraca NULL.
	template <typename T> T * GetItemEx(const string &Path);
	// Zwraca element podanego typu spod podanej �ciezki.
	// Je�li nie znaleziono lub element jest innego typu, rzuca wyj�tek.
	template <typename T> T * MustGetItemEx(const string &Path);
	// Zwraca dane spod podanej �cie�ki skonwertowane do podanego typu.
	// Je�li nie znaleziono lub b��d konwersji, zwraca false.
	template <typename T> bool GetDataEx(const string &Path, T *Data);
	// Zwraca dane spod podanej �cie�ki skonwertowane do podanego typu.
	// Je�li nie znaleziono lub b��d konwersji, rzuca wyj�tek.
	template <typename T> void MustGetDataEx(const string &Path, T *Data);
	// Je�li element o podanej �cie�ce nie istnieje lub nie jest takiego typu jak podany, zwraca false.
	template <typename T> bool ItemExistsEx(const string &Path);
	// Je�li element o podanej �cie�ce nie istnieje lub nie jest takiego typu jak podany, rzuca wyj�tek.
	template <typename T> void ItemMustExistsEx(const string &Path);
	// Ustawia dane elementu tekstowego o podanej �cie�ce na �a�cuch skonwertowany z podanej warto�ci danego typu.
	// Je�li sie nie uda, zwraca false.
	template <typename T> bool SetDataEx(const string &Path, const T &Data, uint4 CreateMode);
	// Ustawia dane elementu tekstowego o podanej �cie�ce na �a�cuch skonwertowany z podanej warto�ci danego typu.
	// Je�li sie nie uda, rzuca wyj�tek.
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
		throw Error("Nie mo�na pobra� elementu konfiguracji o �cie�ce \"" + Path + "\". Typ elementu inny ni� oczekiwany.");
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
		throw Error("Nie mo�na odczyta� danych z konfiguracji spod �ciezki: \"" + Path + "\". B��d konwersji na typ: " + string(typeid(T).name()));
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
		throw Error("Element konfiguracji o �cie�ce \"" + Path + "\" nie istnieje.");
	if (dynamic_cast<T*>(i) == 0)
		throw Error("Element konfiguracji o �cie�ce \"" + Path + "\" jest b��dnego typu.");
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

// Konfiguracja g��wna
extern scoped_ptr<Config> g_Config;

} // namespace common

#endif
