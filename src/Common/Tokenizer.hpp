/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Tokenizer - Parser sk�adni j�zyka podobnego do C/C++
 * Dokumentacja: Patrz plik doc/Tokenizer.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef COMMON_TOKENIZER_H_
#define COMMON_TOKENIZER_H_

#include "Error.hpp"

namespace common
{

class Stream;

// Klasa b��du dla tokenizera
class TokenizerError : public Error
{
private:
	size_t m_Char, m_Row, m_Col;
public:
	TokenizerError(size_t Char, size_t Row, size_t Col, const string &Msg = "", const string &File = "", int a_Line = 0);

	size_t GetCharNum() { return m_Char; }
	size_t GetRowNum() { return m_Row; }
	size_t GetColNum() { return m_Col; }
};

class Tokenizer_pimpl;

class Tokenizer
{
private:
	scoped_ptr<Tokenizer_pimpl> pimpl;

public:
	// Koniec wiersza '\n' ma by� uznawany za osobnego rodzaju token, nie za zwyczajny bia�y znak
	static const uint FLAG_TOKEN_EOL         = 0x01;
	// �a�cuch "" mo�e si� rozci�ga� na wiele wierszy zawieraj�c w tre�ci koniec wiersza
	static const uint FLAG_MULTILINE_STRINGS = 0x02;

	// Rodzaje token�w
	enum TOKEN
	{
		TOKEN_EOF,        // Koniec ca�ego dokumentu (dalej nie parsowa�)
		TOKEN_EOL,        // Token ko�ca linii (tylko kiedy FLAG_TOKEN_EOL)
		TOKEN_SYMBOL,     // Jednoznakowy symbol (np. kropka)
		TOKEN_INTEGER,    // Liczba ca�kowita, np. -123
		TOKEN_FLOAT,      // Liczba zmiennoprzecinkowa, np. 10.5
		TOKEN_CHAR,       // Sta�a znakowa, np. 'A'
		TOKEN_IDENTIFIER, // Identyfikator, np. abc_123
		TOKEN_KEYWORD,    // S�owo kluczowe, czyli identyfikator pasuj�cy do zarejestrowanych s��w kluczowych
		TOKEN_STRING,     // Sta�a �a�cuchowa, np. "ABC"
	};

	// Tworzy z dokumentu podanego przez �a�cuch char*
	Tokenizer(const char *Input, size_t InputLength, uint Flags);
	// Tworzy z dokumentu podanego przez string
	Tokenizer(const string *Input, uint Flags);
	// Tworzy z dokumentu wczytywanego z dowolnego strumienia
	Tokenizer(Stream *Input, uint Flags);

	~Tokenizer();

	// ==== Konfiguracja ====

	void RegisterKeyword(uint Id, const string &Keyword);
	// Wczytuje ca�� list� s��w kluczowych w postaci tablicy �a�cuch�w char*.
	// S�owa kluczowe b�d� mia�y kolejne identyfikatory 0, 1, 2 itd.
	void RegisterKeywords(const char **Keywords, size_t KeywordCount);

	// ==== Zwraca informacje na temat ostatnio odczytanego tokena ====

	// Odczytuje nast�pny token
	void Next();

	// Zwraca typ
	TOKEN GetToken();

	// Zwraca pozycj�
	size_t GetCharNum();
	size_t GetRowNum();
	size_t GetColNum();

	// Dzia�a zawsze, ale zastosowanie g��wnie dla GetToken() == TOKEN_IDENTIFIER lub TOKEN_STRING
	const string & GetString();
	// Tylko je�li GetToken() == TOKEN_CHAR lub TOKEN_SYMBOL
	char GetChar();
	// Tylko je�li GetToken() == TOKEN_KEYWORD
	uint GetId();
	// Tylko je�li GetToken() == TOKEN_INTEGER
	bool GetUint1(uint1 *Out);
	bool GetUint2(uint2 *Out);
	bool GetUint4(uint4 *Out);
	bool GetUint8(uint8 *Out);
	bool GetInt1(int1 *Out);
	bool GetInt2(int2 *Out);
	bool GetInt4(int4 *Out);
	bool GetInt8(int8 *Out);
	uint1 MustGetUint1();
	uint2 MustGetUint2();
	uint4 MustGetUint4();
	uint8 MustGetUint8();
	int1 MustGetInt1();
	int2 MustGetInt2();
	int4 MustGetInt4();
	int8 MustGetInt8();
	// Tylko je�li GetToken() == TOKEN_INTEGER lub TOKEN_FLOAT
	bool GetFloat(float *Out);
	bool GetDouble(double *Out);
	float MustGetFloat();
	double MustGetDouble();

	// ==== Funkcje pomocnicze ====

	// Rzuca wyj�tek z b��dem parsowania. Domy�lny komunikat.
	void CreateError();
	// Rzuca wyj�tek z b��dem parsowania. W�asny komunikat.
	void CreateError(const string &Msg);
	// Zwraca true, je�li ostatnio sparsowany token jest taki jak tu podany.
	bool QueryToken(TOKEN Token);
	bool QueryToken(TOKEN Token1, TOKEN Token2);
	bool QueryEOF();
	bool QueryEOL();
	bool QuerySymbol(char Symbol);
	bool QueryIdentifier(const string &Identifier);
	bool QueryKeyword(uint KeywordId);
	bool QueryKeyword(const string &Keyword);
	// Rzuca wyj�tek z b��dem parsowania, je�li ostatnio sparsowany token nie jest podanego rodzaju.
	void AssertToken(TOKEN Token);
	// Rzuca wyj�tek z b��dem parsowania, je�li ostatnio sparsowany token nie jest jednego z dw�ch podanych rodzaj�w.
	void AssertToken(TOKEN Token1, TOKEN Token2);
	// Rzuca wyj�tek z b��dem parsowania, je�li ostatnio sparsowany token nie jest taki jak tu podany.
	void AssertEOF();
	void AssertEOL();
	void AssertSymbol(char Symbol);
	void AssertIdentifier(const string &Identifier);
	void AssertKeyword(uint KeywordId);
	void AssertKeyword(const string &Keyword);

	// Zamienia typ tokena na jego czytelny dla u�ytkownika opis tekstowy, np. "Sta�a znakowa"
	static const char * GetTokenName(TOKEN Token);

	// Escapuje �a�cuch zgodnie ze sk�adni� tokenizera
	static const uint ESCAPE_EOL   = 0x01; // Zamienia� znaki ko�ca wiersza na \r \n na sekwencje ucieczki
	static const uint ESCAPE_OTHER = 0x02; // Zamienia� znaki niedost�ne z klawiatury (w tym polskie litery) na \xHH
	static void Escape(string *Out, const string &In, uint EscapeFlags);
};

} // namespace common

#endif
