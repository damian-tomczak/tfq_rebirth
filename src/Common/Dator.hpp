/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Dator - Otoczka na pojedyncz¹ wartoœæ dowolnego typu
 * Dokumentacja: Patrz plik doc/Dator.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef COMMON_DATOR_H_
#define COMMON_DATOR_H_

#include <map> // :(

namespace common
{

// Klasa bazowa

class GenericDator
{
public:
	virtual ~GenericDator() { }
	virtual bool SetValue(const string &Value) { return false; }
	virtual bool GetValue(string *Value) { return false; }
};

// Ogólny

template <typename T>
class Dator : public GenericDator
{
private:
	T *m_Ptr;

public:
	Dator() : m_Ptr(0) { }
	Dator(T *Ptr) : m_Ptr(Ptr) { }

	T * GetPtr() { return m_Ptr; }
	void SetPtr(T *Ptr) { m_Ptr = Ptr; }

	virtual bool SetValue(const string &Value);
	virtual bool GetValue(string *Value);
};

template <typename T>
inline bool Dator<T>::SetValue(const string &Value)
{
	T Tmp;
	if (m_Ptr == 0) return false;
	if (!StrToSth_obj<T>::IsSupported()) return false;
	if (StrToSth<T>(&Tmp, Value))
	{
		*m_Ptr = Tmp;
		return true;
	}
	else return false;
}

template <typename T>
inline bool Dator<T>::GetValue(string *Value)
{
	if (m_Ptr == 0) return false;
	if (!SthToStr_obj<T>::IsSupported()) return false;
	SthToStr(Value, *m_Ptr);
	return true;
}

// Czêœciowa specjalizacja dla wartoœci sta³ej

template <typename T>
class Dator<const T> : public GenericDator
{
private:
	const T *m_Ptr;

public:
	Dator() : m_Ptr(0) { }
	Dator(const T *Ptr) : m_Ptr(Ptr) { }

	const T * GetPtr() { return m_Ptr; }
	void SetPtr(const T *Ptr) { m_Ptr = Ptr; }

	virtual bool SetValue(const string &Value) { return false; }
	virtual bool GetValue(string *Value);
};

template <typename T>
inline bool Dator<const T>::GetValue(string *Value)
{
	if (m_Ptr == 0) return false;
	SthToStr(Value, *m_Ptr);
	return true;
}

class DatorGroup
{
private:
	typedef std::map< string, shared_ptr<GenericDator> > DATOR_MAP;
	DATOR_MAP m_Dators;

public:
	// Dodaje dator do podanej wartoœci pod podan¹ nazw¹
	// Jeœli siê nie uda (ju¿ taki jest), zwraca false.
	template <typename U>
	bool Add(const string &Name, U *Value)
	{
		return m_Dators.insert(DATOR_MAP::value_type(Name, shared_ptr<GenericDator>(new Dator<U>(Value)))).second;
	}
	// Usuwa dator o podanej nazwie
	// Jeœli siê nie uda (nie ma takiego), zwraca false.
	bool Delete(const string &Name);
	// Zwraca true, jeœli dator o podanej nazwie istnieje
	bool Exists(const string &Name);

	// Ustawia wartoœæ datora o podanej nazwie
	// Zwracana wartoœæ:
	// 0 = OK
	// 1 = nie ma datora o tej nazwie
	// 2 = dator jest, ale nie uda³o siê ustawiæ
	int SetValue(const string &Name, const string &Value);
	// Zwraca wartoœæ datora o podanej nazwie
	// Zwracana wartoœæ:
	// 0 = OK
	// 1 = nie ma datora o tej nazwie
	// 2 = dator jest, ale nie uda³o siê pobraæ
	int GetValue(const string &Name, string *Value);
};

} // namespace common

#endif
