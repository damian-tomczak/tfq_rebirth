/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Dator - Otoczka na pojedyncz� warto�� dowolnego typu
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

// Og�lny

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

// Cz�ciowa specjalizacja dla warto�ci sta�ej

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
	// Dodaje dator do podanej warto�ci pod podan� nazw�
	// Je�li si� nie uda (ju� taki jest), zwraca false.
	template <typename U>
	bool Add(const string &Name, U *Value)
	{
		return m_Dators.insert(DATOR_MAP::value_type(Name, shared_ptr<GenericDator>(new Dator<U>(Value)))).second;
	}
	// Usuwa dator o podanej nazwie
	// Je�li si� nie uda (nie ma takiego), zwraca false.
	bool Delete(const string &Name);
	// Zwraca true, je�li dator o podanej nazwie istnieje
	bool Exists(const string &Name);

	// Ustawia warto�� datora o podanej nazwie
	// Zwracana warto��:
	// 0 = OK
	// 1 = nie ma datora o tej nazwie
	// 2 = dator jest, ale nie uda�o si� ustawi�
	int SetValue(const string &Name, const string &Value);
	// Zwraca warto�� datora o podanej nazwie
	// Zwracana warto��:
	// 0 = OK
	// 1 = nie ma datora o tej nazwie
	// 2 = dator jest, ale nie uda�o si� pobra�
	int GetValue(const string &Name, string *Value);
};

} // namespace common

#endif
