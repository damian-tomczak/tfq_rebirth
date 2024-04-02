/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Tokenizer - Parser sk�adni j�zyka podobnego do C/C++
 * Dokumentacja: Patrz plik doc/Tokenizer.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
#include <algorithm>
#include <map>
#include "Stream.hpp"
#include "Tokenizer.hpp"


namespace common
{

TokenizerError::TokenizerError(size_t Char, size_t Row, size_t Col, const string &Msg, const string &File, int a_Line) :
	m_Char(Char),
	m_Row(Row),
	m_Col(Col)
{
	if (Msg.empty())
		Push(Format("Tokenizer(wiersz=#, kolumna=#, znak=#)") % m_Row % m_Col % m_Char, __FILE__, __LINE__);
	else
		Push(Format("Tokenizer(wiersz=#, kolumna=#, znak=#) #") % m_Row % m_Col % m_Char % Msg, __FILE__, __LINE__);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Tokenizer_pimpl

typedef std::map<string, uint> KEYWORDS_MAP;

class Tokenizer_pimpl
{
public:
	// Kt�re znaki s� symbolami
	bool m_Symbols[256];

	// Flagi
	bool m_FlagTokenEOL;
	bool m_FlagMultilineStrings;

	// Strumie� do pobierania (jeden z tych trzech strumieni jest != NULL)
	// - Z konstruktora char* - strumie� pami�ciowy, m�j w�asny
	scoped_ptr<MemoryStream> m_MemoryStream;
	// - Z konstruktora string - strumie� �a�cuchowy, m�j w�asny
	scoped_ptr<StringStream> m_StringStream;
	// - Z konstruktora Stream* - strumie� dowolny, zewn�trzny
	Stream *m_ExternalStream;
	// CharReader do jednego z powy�szych strumieni
	scoped_ptr<CharReader> m_CharReader;

	// WARSTWA I - wczytywanie znak�w
	size_t m_CurrChar, m_CurrRow, m_CurrCol;
	bool m_L1End; // albo to jest true...
	char m_L1Char; // ...albo tu jest ostatnio wczytany znak
	void L1Next();

	// Zarejestrowane dane
	KEYWORDS_MAP m_Keywords;

	// Informacje na temat ostatnio odczytanego tokena
	size_t m_LastChar, m_LastRow, m_LastCol;
	Tokenizer::TOKEN m_LastToken;
	string m_LastString; // wype�niany zawsze przy odczytaniu nast�pnego tokena
	uint m_LastId; // wype�niany przy TOKEN_KEYWORD

	~Tokenizer_pimpl() { }

	void InitSymbols();

	char ParseStringChar();
	void Parse();

	template <typename T> bool GetUint(T *Out);
	template <typename T> bool GetInt(T *Out);
	template <typename T> T MustGetUint();
	template <typename T> T MustGetInt();
};

void Tokenizer_pimpl::L1Next()
{
	m_L1End = !m_CharReader->ReadChar(&m_L1Char);

	if (!m_L1End)
	{
		// Liczniki
		m_CurrChar++;
		if (m_L1Char == '\n')
		{
			m_CurrRow++;
			m_CurrCol = 0;
		}
		else
			m_CurrCol++;

		// Sekwencja "\\\n" lub "\\\r\n" jest pomijana - �amanie linii za pomoc� znaku \ ...
		if (m_L1Char == '\\')
		{
			char Ch;
			if (m_CharReader->PeekChar(&Ch) && Ch == '\r')
			{
				m_CharReader->SkipChar();
				m_CurrChar++;
			}
			if (m_CharReader->PeekChar(&Ch) && Ch == '\n')
			{
				m_CharReader->SkipChar();
				m_CurrChar++;
				m_CurrRow++;
				m_CurrCol = 0;
				L1Next();
			}
		}
	}
}

void Tokenizer_pimpl::InitSymbols()
{
	std::fill(&m_Symbols[0], &m_Symbols[256], false);

	const char * DEFAULT_SYMBOLS = "`~!@#$%^&*()=[]{};:,.<>?\\|";

	uint SymbolCount = strlen(DEFAULT_SYMBOLS);
	for (uint i = 0; i < SymbolCount; i++)
		m_Symbols[(uint1)DEFAULT_SYMBOLS[i]] = true;
}

char Tokenizer_pimpl::ParseStringChar()
{
	// Musi by� tu ju� sprawdzone, �e nie jest koniec oraz �e m_L1Char nie jest znakiem ko�cz�cym �a�cuch/znak
	assert(!m_L1End);

	// Escape sequence
	if (m_L1Char == '\\')
	{
		L1Next();
		if (m_L1End)
			throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "Nieoczekiwany koniec danych wewn�trz sekwencji ucieczki", __FILE__, __LINE__);
		// Sta�a szesnastkowa
		if (m_L1Char == 'x')
		{
			uint1 Number1, Number2;
			L1Next();

			// Nast�pne dwa znaki

			if (m_L1End)
				throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "Nieoczekiwany koniec danych wewn�trz sekwencji ucieczki", __FILE__, __LINE__);
			Number1 = HexDigitToNumber(m_L1Char);
			L1Next();

			if (m_L1End)
				throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "Nieoczekiwany koniec danych wewn�trz sekwencji ucieczki", __FILE__, __LINE__);
			Number2 = HexDigitToNumber(m_L1Char);
			L1Next();

			if (Number1 == 0xFF || Number2 == 0xFF)
				throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "B��dna szesnastkowa sekwencja ucieczki", __FILE__, __LINE__);

			return (char)(Number1 << 4 | Number2);
		}
		// Zwyk�a sta�a
		else
		{
			char R;
			switch (m_L1Char)
			{
			case 'n': R = '\n'; break;
			case 't': R = '\t'; break;
			case 'v': R = '\v'; break;
			case 'b': R = '\b'; break;
			case 'r': R = '\r'; break;
			case 'f': R = '\f'; break;
			case 'a': R = '\a'; break;
			case '0': R = '\0'; break;
			case '\\':
			case '?':
			case '"':
			case '\'':
				R = m_L1Char;
				// Zostaje to co jest
				break;
			default:
				throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "Nieznana sekwencja ucieczki: \\" + CharToStrR(m_L1Char), __FILE__, __LINE__);
			}
			L1Next();
			return R;
		}
	}
	else
	{
		char R = m_L1Char;
		L1Next();
		return R;
	}
}

void Tokenizer_pimpl::Parse()
{
	for (;;)
	{
		m_LastChar = m_CurrChar;
		m_LastRow = m_CurrRow;
		m_LastCol = m_CurrCol;

		// EOF
		if (m_L1End)
		{
			m_LastString.clear();
			m_LastToken = Tokenizer::TOKEN_EOF;
			return;
		}

		// Symbol
		if (m_Symbols[(uint1)m_L1Char])
		{
			CharToStr(&m_LastString, m_L1Char);
			m_LastToken = Tokenizer::TOKEN_SYMBOL;
			L1Next();
			return;
		}
		// Bia�y znak
		else if (m_L1Char == ' ' || m_L1Char == '\t' || m_L1Char == '\r' || m_L1Char == '\v')
		{
			L1Next();
			continue;
		}
		// Koniec wiersza
		else if (m_L1Char == '\n')
		{
			// Jako token
			if (m_FlagTokenEOL)
			{
				m_LastString = "\n";
				m_LastToken = Tokenizer::TOKEN_EOL;
				L1Next();
				return;
			}
			// Jako bia�y znak
			else
			{
				L1Next();
				continue;
			}
		}
		// Sta�a znakowa
		else if (m_L1Char == '\'')
		{
			// Nast�pny znak
			L1Next();
			if (m_L1End)
				throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "Nieoczekiwany koniec danych wewn�trz sta�ej znakowej", __FILE__, __LINE__);
			// Znak (sekwencja ucieczki lub zwyk�y)
			CharToStr(&m_LastString, ParseStringChar());
			// Nast�pny znak - to musi by� zako�czenie '
			if (m_L1End)
				throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "Nieoczekiwany koniec danych wewn�trz sta�ej znakowej", __FILE__, __LINE__);
			if (m_L1Char != '\'')
				throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "Oczekiwane zako�czenie sta�ej znakowej: '", __FILE__, __LINE__);
			m_LastToken = Tokenizer::TOKEN_CHAR;
			L1Next();
			return;
		}
		// Sta�a �a�cuchowa
		else if (m_L1Char == '"')
		{
			L1Next();
			m_LastString.clear();
			for (;;)
			{
				// Nast�pny znak
				if (m_L1End)
					throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "Nieoczekiwany koniec danych wewn�trz sta�ej �a�cuchowej", __FILE__, __LINE__);
				// Koniec �a�cucha
				if (m_L1Char == '"')
				{
					L1Next();
					break;
				}
				// Koniec wiersza, podczas gdy nie mo�e go by�
				else if ( (m_L1Char == '\r' || m_L1Char == '\n') && !m_FlagMultilineStrings )
					throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "Niedopuszczalny koniec wiersza wewn�trz sta�ej �a�cuchowej", __FILE__, __LINE__);
				// Znak (sekwencja ucieczki lub zwyk�y)
				else
					m_LastString += ParseStringChar();
			}
			m_LastToken = Tokenizer::TOKEN_STRING;
			return;
		}
		// Identyfikator lub s�owo kluczowe
		else if ( (m_L1Char <= 'Z' && m_L1Char >= 'A') || (m_L1Char <= 'z' && m_L1Char >= 'a') || m_L1Char == '_' )
		{
			CharToStr(&m_LastString, m_L1Char);
			L1Next();
			// Wczytaj nast�pne znaki
			while ( !m_L1End &&
				( (m_L1Char <= 'Z' && m_L1Char >= 'A') || (m_L1Char <= 'z' && m_L1Char >= 'a') || (m_L1Char <= '9' && m_L1Char >= '0') || m_L1Char == '_' ) )
			{
				m_LastString += m_L1Char;
				L1Next();
			}
			// Znajd� s�owo kluczowe
			KEYWORDS_MAP::iterator it = m_Keywords.find(m_LastString);
			if (it == m_Keywords.end())
				m_LastToken = Tokenizer::TOKEN_IDENTIFIER;
			else
			{
				m_LastToken = Tokenizer::TOKEN_KEYWORD;
				m_LastId = it->second;
			}
			return;
		}
		// Znak '/' mo�e rozpoczyna� komentarz // , /* lub by� zwyk�ym symbolem
		else if (m_L1Char == '/')
		{
			L1Next();
			if (!m_L1End)
			{
				// Pocz�tek komentarza //
				if (m_L1Char == '/')
				{
					L1Next();
					// Pomi� znaki do ko�ca wiersza lub ko�ca dokumentu
					while (!m_L1End && m_L1Char != '\n')
						L1Next();
					// Dalsze poszukiwanie tokena
					continue;
				}
				// Pocz�tek komentarza /*
				else if (m_L1Char == '*')
				{
					L1Next();
					bool WasAsterisk = false;
					// Czytaj kolejne znaki
					for (;;)
					{
						if (m_L1End)
							throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "Niezamkni�ty komentarz /* */", __FILE__, __LINE__);
						
						if (m_L1Char == '*')
							WasAsterisk = true;
						else if (m_L1Char == '/' && WasAsterisk)
							// Koniec komentarza
							break;
						else
							WasAsterisk = false;

						L1Next();
					}
					// Dalsze poszukiwanie tokena
					L1Next();
					continue;
				}
			}
			// Zwyk�y symbol '/'
			CharToStr(&m_LastString, '/');
			m_LastToken = Tokenizer::TOKEN_SYMBOL;
			L1Next();
			return;
		}
		// Liczba ca�kowita lub zmiennoprzecinkowa
		else if ( (m_L1Char <= '9' && m_L1Char >= '0') || m_L1Char == '+' || m_L1Char == '-' )
		{
			// Pe�na poprawna sk�adnia liczb to mniej wi�cej (wyra�enie regularne, \o \d \h to cyfra w danym systemie):
			// [+-]?(0[xX]\h+|0\o+|(\d+\.\d*|\d*\.\d+|\d+)([dDeE][+-]?\d+)?)
			// Ale �e nie chce mi si� tego a� tak dok�adnie parsowa�, to zaakceptuj� tutaj dowolny ci�g
			// dowolnych cyfr, liter i znak�w '.', '+' i '-' - ewentualny b��d sk�adni i tak wyjdzie, tylko �e p�niej.
			// Liczba jest na pewno ca�kowita, je�li zaczyna si� od "0x" lub "0X" (nowo��! tu by� b��d i 0xEE rozpoznawa�o jako zmiennoprzecinkowa!)
			// Liczba jest zmiennoprzecinkowa wtedy, kiedy zawiera jeden ze znak�w: [.dDeE]
			CharToStr(&m_LastString, m_L1Char);
			L1Next();
			bool IsFloat = false;
			while (!m_L1End && (
				(m_L1Char <= '9' && m_L1Char >= '0') ||
				(m_L1Char <= 'Z' && m_L1Char >= 'A') ||
				(m_L1Char <= 'z' && m_L1Char >= 'a') ||
				m_L1Char == '+' || m_L1Char == '-' || m_L1Char == '.' ) )
			{
				if (m_L1Char == '.' || m_L1Char == 'd' || m_L1Char == 'D' || m_L1Char == 'e' || m_L1Char == 'E')
					IsFloat = true;
				m_LastString += m_L1Char;
				L1Next();
			}
			if (m_LastString.length() > 1 && m_LastString[0] == '0' && (m_LastString[1] == 'x' || m_LastString[1] == 'X'))
				m_LastToken = Tokenizer::TOKEN_INTEGER;
			else
				m_LastToken = (IsFloat ? Tokenizer::TOKEN_FLOAT : Tokenizer::TOKEN_INTEGER);
			return;
		}
		// Nieznany znak
		else
			throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, Format("Nierozpoznany znak (kod=#): #") % (int)(uint1)m_L1Char % m_L1Char, __FILE__, __LINE__);
	}
}

template <typename T>
bool Tokenizer_pimpl::GetUint(T *Out)
{
	if (m_LastString == "0")
	{
		*Out = T();
		return true;
	}

	// �semkowo lub szesnastkowo
	if (!m_LastString.empty() && m_LastString[0] == '0')
	{
		// Szesnastkowo
		if (m_LastString.length() > 1 && (m_LastString[1] == 'x' || m_LastString[1] == 'X'))
			return ( StrToUint<T>(Out, m_LastString.substr(2), 16) == 0 );
		// �semkowo
		else
			return ( StrToUint<T>(Out, m_LastString.substr(1), 8) == 0 );
	}
	// Dziesi�tnie
	else
		return ( StrToUint<T>(Out, m_LastString) == 0 );
}

template <typename T>
bool Tokenizer_pimpl::GetInt(T *Out)
{
	if (m_LastString == "0" || m_LastString == "-0" || m_LastString == "+0")
	{
		*Out = T();
		return true;
	}

	// Pocz�tkowy '+' lub '-'
	if (!m_LastString.empty() && (m_LastString[0] == '+' || m_LastString[0] == '-'))
	{
		// Plus - tak jakby go nie by�o, ale wszystko jest o znak dalej
		if (m_LastString[0] == '+')
		{
			// �semkowo lub szesnastkowo
			if (m_LastString.length() > 1 && m_LastString[1] == '0')
			{
				// Szesnastkowo
				if (m_LastString.length() > 2 && (m_LastString[2] == 'x' || m_LastString[2] == 'X'))
					return ( StrToInt<T>(Out, m_LastString.substr(3), 16) == 0 );
				// �semkowo
				else
					return ( StrToInt<T>(Out, m_LastString.substr(2), 8) == 0 );
			}
			// Dziesi�tnie
			else
				return ( StrToInt<T>(Out, m_LastString.substr(1)) == 0 );
		}
		// Minus - uwzgl�dnij go
		else
		{
			// �semkowo lub szesnastkowo
			if (m_LastString.length() > 1 && m_LastString[1] == '0')
			{
				// Szesnastkowo
				if (m_LastString.length() > 2 && (m_LastString[2] == 'x' || m_LastString[2] == 'X'))
					return ( StrToInt<T>(Out, "-" + m_LastString.substr(3), 16) == 0 );
				// �semkowo
				else
					return ( StrToInt<T>(Out, "-" + m_LastString.substr(2), 8) == 0 );
			}
			// Dziesi�tnie
			else
				return ( StrToInt<T>(Out, "-" + m_LastString.substr(1)) == 0 );
		}
	}
	else
	{
		// �semkowo lub szesnastkowo
		if (!m_LastString.empty() && m_LastString[0] == '0')
		{
			// Szesnastkowo
			if (m_LastString.length() > 1 && (m_LastString[1] == 'x' || m_LastString[1] == 'X'))
				return ( StrToInt<T>(Out, m_LastString.substr(2), 16) == 0 );
			// �semkowo
			else
				return ( StrToInt<T>(Out, m_LastString.substr(1), 8) == 0 );
		}
		// Dziesi�tnie
		else
			return ( StrToInt<T>(Out, m_LastString) == 0 );
	}
}

template <typename T>
T Tokenizer_pimpl::MustGetUint()
{
	T R;
	if (!GetUint(&R))
		throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "Nie mo�na zamieni� ci�gu znak�w na liczb� bez znaku", __FILE__, __LINE__);
	return R;
}

template <typename T>
T Tokenizer_pimpl::MustGetInt()
{
	T R;
	if (!GetInt(&R))
		throw TokenizerError(m_LastChar, m_LastRow, m_LastCol, "Nie mo�na zamieni� ci�gu znak�w na liczb� ze znakiem", __FILE__, __LINE__);
	return R;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Tokenizer

Tokenizer::Tokenizer(const char *Input, size_t InputLength, uint Flags) :
	pimpl(new Tokenizer_pimpl())
{
	pimpl->InitSymbols();

	pimpl->m_FlagTokenEOL = ((Flags & FLAG_TOKEN_EOL) != 0);
	pimpl->m_FlagMultilineStrings = ((Flags & FLAG_MULTILINE_STRINGS) != 0);

	pimpl->m_MemoryStream.reset(new MemoryStream(InputLength, const_cast<char*>(Input)));
	pimpl->m_StringStream.reset();
	pimpl->m_ExternalStream = NULL;
	pimpl->m_CharReader.reset(new CharReader(pimpl->m_MemoryStream.get()));

	pimpl->m_CurrChar = 0; pimpl->m_CurrRow = 1; pimpl->m_CurrCol = 0;
	pimpl->L1Next();

	pimpl->m_LastChar = 0; pimpl->m_LastRow = 1; pimpl->m_LastCol = 0;
	pimpl->m_LastToken = TOKEN_EOF;
	pimpl->m_LastId = 0;
}

Tokenizer::Tokenizer(const string *Input, uint Flags) :
	pimpl(new Tokenizer_pimpl())
{
	pimpl->InitSymbols();

	pimpl->m_FlagTokenEOL = ((Flags & FLAG_TOKEN_EOL) != 0);
	pimpl->m_FlagMultilineStrings = ((Flags & FLAG_MULTILINE_STRINGS) != 0);

	pimpl->m_MemoryStream.reset();
	pimpl->m_StringStream.reset(new StringStream(const_cast<string*>(Input)));
	pimpl->m_ExternalStream = NULL;
	pimpl->m_CharReader.reset(new CharReader(pimpl->m_StringStream.get()));

	pimpl->m_CurrChar = 0; pimpl->m_CurrRow = 1; pimpl->m_CurrCol = 0;
	pimpl->L1Next();

	pimpl->m_LastChar = 0; pimpl->m_LastRow = 1; pimpl->m_LastCol = 0;
	pimpl->m_LastToken = TOKEN_EOF;
	pimpl->m_LastId = 0;
}

Tokenizer::Tokenizer(Stream *Input, uint Flags) :
	pimpl(new Tokenizer_pimpl())
{
	pimpl->InitSymbols();

	pimpl->m_FlagTokenEOL = ((Flags & FLAG_TOKEN_EOL) != 0);
	pimpl->m_FlagMultilineStrings = ((Flags & FLAG_MULTILINE_STRINGS) != 0);

	pimpl->m_MemoryStream.reset();
	pimpl->m_StringStream.reset();
	pimpl->m_ExternalStream = Input;
	pimpl->m_CharReader.reset(new CharReader(Input));

	pimpl->m_CurrChar = 0; pimpl->m_CurrRow = 1; pimpl->m_CurrCol = 0;
	pimpl->L1Next();

	pimpl->m_LastChar = 0; pimpl->m_LastRow = 1; pimpl->m_LastCol = 0;
	pimpl->m_LastToken = TOKEN_EOF;
	pimpl->m_LastId = 0;
}

Tokenizer::~Tokenizer()
{
	pimpl->m_CharReader.reset();
	pimpl->m_ExternalStream = NULL;
	pimpl->m_StringStream.reset();
	pimpl->m_MemoryStream.reset();

	pimpl.reset();
}

void Tokenizer::RegisterKeyword(uint Id, const string &Keyword)
{
	pimpl->m_Keywords.insert(KEYWORDS_MAP::value_type(Keyword, Id));
}

void Tokenizer::RegisterKeywords(const char **Keywords, size_t KeywordCount)
{
	for (uint i = 0; i < KeywordCount; i++)
		RegisterKeyword(i, Keywords[i]);
}

void Tokenizer::Next()
{
	pimpl->Parse();
}

Tokenizer::TOKEN Tokenizer::GetToken()
{
	return pimpl->m_LastToken;
}

size_t Tokenizer::GetCharNum()
{
	return pimpl->m_LastChar;
}

size_t Tokenizer::GetRowNum()
{
	return pimpl->m_LastRow;
}

size_t Tokenizer::GetColNum()
{
	return pimpl->m_LastCol;
}

const string & Tokenizer::GetString()
{
	return pimpl->m_LastString;
}

char Tokenizer::GetChar()
{
	assert(GetToken() == TOKEN_CHAR || GetToken() == TOKEN_SYMBOL);

	return pimpl->m_LastString[0];
}

uint Tokenizer::GetId()
{
	assert(GetToken() == TOKEN_KEYWORD);

	return pimpl->m_LastId;
}

bool Tokenizer::GetUint1(uint1 *Out) { AssertToken(TOKEN_INTEGER); return pimpl->GetUint<uint1>(Out); }
bool Tokenizer::GetUint2(uint2 *Out) { AssertToken(TOKEN_INTEGER); return pimpl->GetUint<uint2>(Out); }
bool Tokenizer::GetUint4(uint4 *Out) { AssertToken(TOKEN_INTEGER); return pimpl->GetUint<uint4>(Out); }
bool Tokenizer::GetUint8(uint8 *Out) { AssertToken(TOKEN_INTEGER); return pimpl->GetUint<uint8>(Out); }

bool Tokenizer::GetInt1(int1 *Out) { AssertToken(TOKEN_INTEGER); return pimpl->GetInt<int1>(Out); }
bool Tokenizer::GetInt2(int2 *Out) { AssertToken(TOKEN_INTEGER); return pimpl->GetInt<int2>(Out); }
bool Tokenizer::GetInt4(int4 *Out) { AssertToken(TOKEN_INTEGER); return pimpl->GetInt<int4>(Out); }
bool Tokenizer::GetInt8(int8 *Out) { AssertToken(TOKEN_INTEGER); return pimpl->GetInt<int8>(Out); }

uint1 Tokenizer::MustGetUint1() { AssertToken(TOKEN_INTEGER); return pimpl->MustGetUint<uint1>(); }
uint2 Tokenizer::MustGetUint2() { AssertToken(TOKEN_INTEGER); return pimpl->MustGetUint<uint2>(); }
uint4 Tokenizer::MustGetUint4() { AssertToken(TOKEN_INTEGER); return pimpl->MustGetUint<uint4>(); }
uint8 Tokenizer::MustGetUint8() { AssertToken(TOKEN_INTEGER); return pimpl->MustGetUint<uint8>(); }

int1 Tokenizer::MustGetInt1() { AssertToken(TOKEN_INTEGER); return pimpl->MustGetInt<int1>(); }
int2 Tokenizer::MustGetInt2() { AssertToken(TOKEN_INTEGER); return pimpl->MustGetInt<int2>(); }
int4 Tokenizer::MustGetInt4() { AssertToken(TOKEN_INTEGER); return pimpl->MustGetInt<int4>(); }
int8 Tokenizer::MustGetInt8() { AssertToken(TOKEN_INTEGER); return pimpl->MustGetInt<int8>(); }

bool Tokenizer::GetFloat(float *Out)
{
	AssertToken(TOKEN_INTEGER, TOKEN_FLOAT);

	return ( StrToFloat(Out, pimpl->m_LastString) == 0 );
}

bool Tokenizer::GetDouble(double *Out)
{
	AssertToken(TOKEN_INTEGER, TOKEN_FLOAT);

	return ( StrToDouble(Out, pimpl->m_LastString) == 0 );
}

float Tokenizer::MustGetFloat()
{
	float R;
	if (!GetFloat(&R))
		throw TokenizerError(pimpl->m_LastChar, pimpl->m_LastRow, pimpl->m_LastCol, "Nie mo�na zamieni� ci�gu znak�w na liczb� zmiennoprzecinkow� typu float", __FILE__, __LINE__);
	return R;
}

double Tokenizer::MustGetDouble()
{
	double R;
	if (!GetDouble(&R))
		throw TokenizerError(pimpl->m_LastChar, pimpl->m_LastRow, pimpl->m_LastCol, "Nie mo�na zamieni� ci�gu znak�w na liczb� zmiennoprzecinkow� typu double", __FILE__, __LINE__);
	return R;
}

void Tokenizer::CreateError()
{
	throw TokenizerError(pimpl->m_LastChar, pimpl->m_LastRow, pimpl->m_LastCol, "Nieokre�lony b��d", __FILE__, __LINE__);
}

void Tokenizer::CreateError(const string &Msg)
{
	throw TokenizerError(pimpl->m_LastChar, pimpl->m_LastRow, pimpl->m_LastCol, Msg, __FILE__, __LINE__);
}

bool Tokenizer::QueryToken(Tokenizer::TOKEN Token)
{
	return (GetToken() == Token);
}

bool Tokenizer::QueryToken(TOKEN Token1, TOKEN Token2)
{
	return (GetToken() == Token1 || GetToken() == Token2);
}

bool Tokenizer::QueryEOF()
{
	return QueryToken(TOKEN_EOF);
}

bool Tokenizer::QueryEOL()
{
	return QueryToken(TOKEN_EOL);
}

bool Tokenizer::QuerySymbol(char Symbol)
{
	return (GetToken() == TOKEN_SYMBOL && GetChar() == Symbol);
}

bool Tokenizer::QueryIdentifier(const string &Identifier)
{
	return (GetToken() == TOKEN_IDENTIFIER && GetString() == Identifier);
}

bool Tokenizer::QueryKeyword(uint KeywordId)
{
	return (GetToken() == TOKEN_KEYWORD && GetId() == KeywordId);
}

bool Tokenizer::QueryKeyword(const string &Keyword)
{
	return (GetToken() == TOKEN_KEYWORD && GetString() == Keyword);
}

void Tokenizer::AssertToken(Tokenizer::TOKEN Token)
{
	if (Token != GetToken())
		CreateError("Oczekiwany: " + string(GetTokenName(Token)));
}

void Tokenizer::AssertToken(TOKEN Token1, TOKEN Token2)
{
	if (Token1 != GetToken() && Token2 != GetToken())
		CreateError("Oczekiwany: " + string(GetTokenName(Token1)) + " lub " + GetTokenName(Token2));
}

void Tokenizer::AssertEOF()
{
	AssertToken(TOKEN_EOF);
}

void Tokenizer::AssertEOL()
{
	AssertToken(TOKEN_EOL);
}

void Tokenizer::AssertSymbol(char Symbol)
{
	if (GetToken() != TOKEN_SYMBOL || GetChar() != Symbol)
		CreateError("Oczekiwany symbol: " + CharToStrR(Symbol));
}

void Tokenizer::AssertIdentifier(const string &Identifier)
{
	if (GetToken() != TOKEN_IDENTIFIER || GetString() != Identifier)
		CreateError("Oczekiwany identyfikator: " + Identifier);
}

void Tokenizer::AssertKeyword(uint KeywordId)
{
	if (GetToken() != TOKEN_KEYWORD || GetId() != KeywordId)
		CreateError("Oczekiwane s�owo kluczowe nr " + UintToStrR(KeywordId));
}

void Tokenizer::AssertKeyword(const string &Keyword)
{
	if (GetToken() != TOKEN_KEYWORD || GetString() != Keyword)
		CreateError("Oczekiwane s�owo kluczowe: " + Keyword);
}

const char * Tokenizer::GetTokenName(TOKEN Token)
{
	switch (Token)
	{
	case TOKEN_EOF:         return "Koniec danych";
	case TOKEN_EOL:         return "Koniec wiersza";
	case TOKEN_SYMBOL:      return "Symbol";
	case TOKEN_INTEGER:     return "Liczba ca�kowita";
	case TOKEN_FLOAT:       return "Liczba zmiennoprzecinkowa";
	case TOKEN_CHAR:        return "Sta�a znakowa";
	case TOKEN_IDENTIFIER:  return "Identyfikator";
	case TOKEN_KEYWORD:     return "S�owo kluczowe";
	case TOKEN_STRING:      return "�a�cuch";
	default:                return "Nieznany";
	}
}

void Tokenizer::Escape(string *Out, const string &In, uint EscapeFlags)
{
	Out->clear();
	Out->reserve(In.length());

	bool EscapeEOL   = ( (EscapeFlags & ESCAPE_EOL  ) != 0 );
	bool EscapeOther = ( (EscapeFlags & ESCAPE_OTHER) != 0 );

	char Ch;
	for (uint i = 0; i < In.length(); i++)
	{
		Ch = In[i]; 
		if (Ch == '\r')
		{
			if (EscapeEOL)
				*Out += "\\r";
			else
				*Out += '\r';
		}
		else if (Ch == '\n')
		{
			if (EscapeEOL)
				*Out += "\\n";
			else
				*Out += '\n';
		}
		else if (Ch == '\\' || Ch == '\'' || Ch == '\"')
		{
			*Out += '\\';
			*Out += Ch;
		}
		else if (Ch == '\0') *Out += "\\0";
		else if (Ch == '\v') *Out += "\\v";
		else if (Ch == '\b') *Out += "\\b";
		else if (Ch == '\f') *Out += "\\f";
		else if (Ch == '\a') *Out += "\\a";
		else if (Ch == '\t') *Out += Ch;
		else if ( ((uint1)Ch < 32 || (uint1)Ch > 126) && EscapeOther )
		{
			*Out += "\\x";
			string s;
			UintToStr2(&s, (uint1)Ch, 2, 16);
			*Out += s;
		}
		else
			*Out += Ch;
	}
}

} // namespace common
