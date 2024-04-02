/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Stream - Hierarchia klas strumieni
 * Dokumentacja: Patrz plik doc/Stream.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef COMMON_STREAM_H_
#define COMMON_STREAM_H_

namespace common
{

extern const size_t BUFFER_SIZE;

// [Wewn�trzne]
void _ThrowBufEndError(const char *File, int Line);

enum DECODE_TOLERANCE
{
	DECODE_TOLERANCE_NONE,       // �adne dodatkowe znaki nie b�d� tolerowane
	DECODE_TOLERANCE_WHITESPACE, // Bia�e znaki b�d� ignorowane i nie spowoduj� b��du
	DECODE_TOLERANCE_ALL,        // Wszelkie nieznane znaki b�d� ignorowane i nie spowoduj� b��du
};


// Abstrakcyjna klasa bazowa strumieni danych binarnych
class Stream
{
public:
	// wirtualny destruktor (klasa jest polimorficzna)
	virtual ~Stream() { }

	// ======== ZAPISYWANIE ========

	// Zapisuje dane
	// (W oryginale: zg�asza b��d)
	virtual void Write(const void *Data, size_t Size);
	// Domy�lnie nie robi nic.
	virtual void Flush() { }

	// Zapisuje dane, sama odczytuje rozmiar przekazanej zmiennej
	template <typename T>
	void WriteEx(const T &x) { return Write(&x, sizeof(x)); }
	// Zapisuje rozmiar na 1B i �a�cuch
	// Je�li �a�cuch za d�ugi - b��d.
	void WriteString1(const string &s);
	// Zapisuje rozmiar na 2B i �a�cuch
	// Je�li �a�cuch za d�ugi - b��d.
	void WriteString2(const string &s);
	// Zapisuje rozmiar na 4B i �a�cuch
	void WriteString4(const string &s);
	// Zapisuje �a�cuch bez rozmiaru
	void WriteStringF(const string &s);
	// Zapisuje warto�� logiczn� za pomoc� jednego bajtu
	void WriteBool(bool b);

	// ======== ODCZYTYWANIE ========

	// Odczytuje dane
	// Size to liczba bajt�w do odczytania
	// Zwraca liczb� bajt�w, jakie uda�o si� odczyta�
	// Je�li osi�gni�to koniec, funkcja nie zwraca b��du, a liczba odczytanych bajt�w to 0.
	// (W oryginale: zg�asza b��d)
	virtual size_t Read(void *Data, size_t MaxLength);
	// Odczytuje tyle bajt�w, ile si� za��da
	// Je�li koniec pliku albo je�li odczytano mniej, zg�asza b��d.
	// (Mo�na j� prze�adowa�, ale nie trzeba - ma swoj� wersj� oryginaln�)
	virtual void MustRead(void *Data, size_t Length);
	// Tak samo jak MustRead, ale sama odczytuje rozmiar przekazanej zmiennej
	// Zwraca true, je�li osi�gni�to koniec strumienia
	// (W oryginale: zg�asza b��d)
	virtual bool End();
	// Pomija co najwy�ej podan� liczb� bajt�w (chyba �e wcze�niej osi�gni�to koniec).
	// Zwraca liczb� pomini�tych bajt�w.
	// (Mo�na j� prze�adowa�, ale nie trzeba - ma swoj� wersj� oryginaln�)
	virtual size_t Skip(size_t MaxLength);

	template <typename T>
	void ReadEx(T *x) { MustRead(x, sizeof(*x)); }
	// Odczytuje �a�cuch poprzedzony rozmiarem 1B
	void ReadString1(string *s);
	// Odczytuje �a�cuch poprzedzony rozmiarem 2B
	void ReadString2(string *s);
	// Odczytuje �a�cuch poprzedzony rozmiarem 4B
	void ReadString4(string *s);
	// Odczytuje �a�cuch bez zapisanego rozmiaru
	void ReadStringF(string *s, size_t Length);
	// Odczytuje tyle znak�w do �a�cucha, ile si� da do ko�ca strumienia
	void ReadStringToEnd(string *s);
	// Odczytuje warto�� logiczn� za pomoc� jednego bajtu
	void ReadBool(bool *b);
	// Pomija koniecznie podan� liczb� bajt�w.
	// Je�li wcze�niej osi�gni�to koniec, zg�asza b��d.
	void MustSkip(size_t Length);

	// ======== KOPIOWANIE ========
	// Odczytuje podan� co najwy�ej liczb� bajt�w z podanego strumienia
	// Zwraca liczb� odczytanych bajt�w
	size_t CopyFrom(Stream *s, size_t Size);
	// Odczytuje dok�adnie podan� liczb� bajt�w z podanego strumienia
	// Je�li odczyta mniej, zg�asza b��d.
	void MustCopyFrom(Stream *s, size_t Size);
	// Odczytuje dane do ko�ca z podanego strumienia
	void CopyFromToEnd(Stream *s);
};

// Abstrakcyjna klasa bazowa strumieni pozwalaj�cych na odczytywanie rozmiaru i zmian� pozycji
class SeekableStream : public Stream
{
public:
	// ======== Implementacja Stream ========
	// (W Stream domy�lnie rzuca b��d, tu w SeekableStream dostaje ju� wersj� oryginaln�. Mo�na j� dalej nadpisa�.)
	virtual bool End();
	// (W Stream ma wersj� oryginaln�, tu w SeekableStream dostaje lepsz�. Mo�na j� dalej nadpisa�.)
	virtual size_t Skip(size_t MaxLength);

	// Zwraca rozmiar
	// (W oryginale: zg�asza b��d)
	virtual size_t GetSize();
	// Zwraca pozycj� wzgl�dem pocz�tku
	// (W oryginale: zg�asza b��d)
	virtual int GetPos();
	// Ustawia pozycj� wzgl�dem pocz�tku
	// pos musi by� dodatnie.
	// (W oryginale: zg�asza b��d)
	virtual void SetPos(int pos);
	// Ustawia pozycj� wzgl�dem bie��cej
	// pos mo�e by� dodatnie albo ujemne.
	// (Mo�na j� nadpisa�, ale ma wersj� oryginaln�)
	virtual void SetPosFromCurrent(int pos);
	// Ustawia pozycj� wzgl�dem ko�cowej
	// Np. abu ustawi� na ostatni znak, przesu� do -1.
	// (Mo�na j� nadpisa�, ale ma wersj� oryginaln�)
	virtual void SetPosFromEnd(int pos);
	// Ustawia pozycj� na pocz�tku
	// (Mo�na j� nadpisa�, ale ma wersj� oryginaln�)
	virtual void Rewind();
	// Ustawia rozmiar na podany
	// Po tym wywo�aniu pozycja kursora jest niezdefiniowana.
	// (W oryginale: zg�asza b��d)
	virtual void SetSize(size_t Size);
	// Obcina strumie� od bie��cej pozycji do ko�ca, czyli ustawia taki rozmiar jak bie��ca pozycja.
	// (Mo�na j� nadpisa�, ale ma wersj� oryginaln�)
	virtual void Truncate();
	// Opr�nia strumie�
	// (Mo�na j� nadpisa�, ale ma wersj� oryginaln�)
	virtual void Clear();
};

// Nak�adka przyspieszaj�ca zapisuj�ca do strumienia pojedyncze znaki i inne rzeczy.
/*
- Nie jest strumieniem.
- Tylko zapis.
- Jest szybka, poniewa�:
  1. Nie ma metod wirtualnych.
  2. Buforuje, wi�c nie u�ywa za ka�dym razem metod wirtualnych strumienia.
- Sama zapisuje dane pozosta�e w buforze w destruktorze.
  Je�li chcesz to zrobi� wcze�niej i z kontrol� b��d�w, wywo�aj Flush.
- W trybie Debug dzia�a nawet troch� wolniej ni� zapisywanie po bajcie prosto do strumienia.
  W trybie Release daje przyspieszenie prawie 10x wzgl�dem zapisywania po bajcie prosto do strumienia.
- Celowo nie u�ywam w tej klasie ERR_TRY .. ERR_CATCH_FUNC, �eby by�o szybciej.
*/
class CharWriter
{
private:
	Stream *m_Stream;
	std::vector<char> m_Buf;
	uint m_BufIndex;

	// Wykonuje Flush danych z bufora do strumienia nie sprawdzaj�c ju�, czy bufor nie jest pusty.
	// Oczywi�cie zeruje te� BufIndex.
	void DoFlush() { m_Stream->Write(&m_Buf[0], m_BufIndex); m_BufIndex = 0; }

public:
	CharWriter(Stream *a_Stream) : m_Stream(a_Stream), m_BufIndex(0) { m_Buf.resize(BUFFER_SIZE); }
	~CharWriter();

	// Pojedynczy znak
	void WriteChar(char Ch) { if (m_BufIndex == BUFFER_SIZE) DoFlush(); m_Buf[m_BufIndex++] = Ch; }
	// Zapisuje po prostu znaki �a�cucha, bez �adnej d�ugo�ci
	void WriteString(const string &s);
	// �a�cuch zako�czony zerem
	void WriteString(const char *s) { WriteString(s, strlen(s)); }
	// �a�cuch podanej d�ugo�ci
	void WriteString(const char *s, size_t Length) { WriteData(s, Length); }
	// Surowe dane binarne
	void WriteData(const void *Data, size_t Length);
	// Zapisuje dowoln� informacj� przerobion� na posta� tekstow�, o ile da si� przerobi�
	template <typename T> void WriteEx(const T &x) { string s; SthToStr<T>(&s, x); WriteString(s); }
	// Wymusza opr�nienie bufora i wys�anie pozosta�ych w nim danych do strumienia
	void Flush() { if (m_BufIndex > 0) DoFlush(); }
};

// Nak�adka przyspieszaj�ca odczytuj�ca ze strumienia pojedyncze znaki i inne rzeczy.
/*
- Nie jest strumieniem.
- Tylko odczyt.
- Jest szybka, poniewa�:
  1. Nie ma metod wirtualnych.
  2. Buforuje, wi�c nie u�ywa za ka�dym razem metod wirtualnych strumienia.
- Szybko�� wzgl�dem wczytywania pojedynczych znak�w prosto ze strumienia: Debug = 10 razy, Release = 450 razy!
- Celowo nie u�ywam w tej klasie ERR_TRY .. ERR_CATCH_FUNC, �eby by�o szybciej.
*/
class CharReader
{
private:
	Stream *m_Stream;
	std::vector<char> m_Buf;
	// Miejsce, do kt�rego doczyta�em z bufora
	uint m_BufBeg;
	// Miejsce, do kt�rego bufor jest wype�niony danymi
	uint m_BufEnd;

	// Wczytuje now� porcj� danych do strumienia
	// Wywo�ywa� tylko kiedy bufor si� sko�czy�, tzn. m_BufBeg == m_BufEnd.
	// Je�li sko�czy� si� strumie� i nic nie uda�o si� wczyta�, zwraca false.
	// Je�li nie, zapewnia �e w buforze b�dzie co najmniej jeden znak.
	bool EnsureNewChars();

public:
	CharReader(Stream *a_Stream) : m_Stream(a_Stream), m_BufBeg(0), m_BufEnd(0) { m_Buf.resize(BUFFER_SIZE); }

	// Czy jeste�my na ko�cu danych?
	bool End() {
		return (m_BufBeg == m_BufEnd) && m_Stream->End(); // Nic nie zosta�o w buforze i nic nie zosta�o w strumieniu.
	}
	// Je�li mo�na odczyta� nast�pny znak, wczytuje go i zwraca true.
	// Je�li nie, nie zmienia Out i zwraca false. Oznacza to koniec strumienia.
	bool ReadChar(char *Out) { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) return false; } *Out = m_Buf[m_BufBeg++]; return true; }
	// Je�li mo�na odczyta� nast�pny znak, wczytuje go.
	// Je�li nie, rzuca wyj�tek.
	char MustReadChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) _ThrowBufEndError(__FILE__, __LINE__); } return m_Buf[m_BufBeg++]; }
	// Je�li mo�na odczyta� nast�pny znak, podgl�da go zwracaj�c przez Out i zwraca true. Nie przesuwa "kursora".
	// Je�li nie, nie zmienia Out i zwraca false. Oznacza to koniec strumienia.
	bool PeekChar(char *Out) { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) return false; } *Out = m_Buf[m_BufBeg]; return true; }
	// Je�li mo�na odczyta� nast�pny znak, podgl�da go zwracaj�c. Nie przesuwa "kursora".
	// Je�li nie, rzuca wyj�tek.
	char MustPeekChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) _ThrowBufEndError(__FILE__, __LINE__); } return m_Buf[m_BufBeg]; }
	// Wczytuje co najwy�ej MaxLength znak�w do podanego stringa.
	// StringStream jest czyszczony - nie musi by� pusty ani zaalokowany.
	// Zwraca liczb� odczytanych znak�w. Mniej ni� ��dano oznacza koniec strumienia.
	size_t ReadString(string *Out, size_t MaxLength);
	// Wczytuje Length znak�w do podanego stringa.
	// StringStream jest czyszczony - nie musi by� pusty ani zaalokowany.
	// Je�li nie uda si� odczyta� wszystkich Length znak�w, rzuca wyj�tek.
	void MustReadString(string *Out, size_t Length);
	// Wczytuje co najwy�ej MaxLength znak�w pod podany wska�nik.
	// Out musi by� tablic� zaalokowan� do d�ugo�ci co najmniej MaxLength.
	// Zwraca liczb� odczytanych znak�w. Mniej ni� ��dano oznacza koniec strumienia.
	size_t ReadString(char *Out, size_t MaxLength);
	// Wczytuje Length znak�w pod podany wska�nik.
	// Out musi by� tablic� zaalokowan� do d�ugo�ci co najmniej Length.
	// Je�li nie uda si� odczyta� wszystkich Length znak�w, rzuca wyj�tek.
	void MustReadString(char *Out, size_t Length);
	// Wczytuje co najwy�ej MaxLength bajt�w pod podany wska�nik.
	// Out musi by� tablic� zaalokowan� do d�ugo�ci co najmniej MaxLength bajt�w.
	// Zwraca liczb� odczytanych bajt�w. Mniej ni� ��dano oznacza koniec strumienia.
	size_t ReadData(void *Out, size_t MaxLength);
	// Wczytuje Length bajt�w pod podany wska�nik.
	// Out musi by� tablic� zaalokowan� do d�ugo�ci co najmniej Length bajt�w.
	// Je�li nie uda si� odczyta� wszystkich Length bajt�w, rzuca wyj�tek.
	void MustReadData(void *Out, size_t Length);
	// Je�li jest nast�pny znak do odczytania, pomija go i zwraca true.
	// Je�li nie, zwraca false. Oznacza to koniec strumienia.
	bool SkipChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) return false; } m_BufBeg++; return true; }
	// Je�li jest nast�pny znak do odczytania, pomija go.
	// Je�li nie, rzuca wyj�tek.
	void MustSkipChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) _ThrowBufEndError(__FILE__, __LINE__); } m_BufBeg++; }
	// Pomija co najwy�ej MaxLength znak�w.
	// Zwraca liczb� pomini�tych znak�w. Mniej ni� ��dano oznacza koniec strumienia.
	size_t Skip(size_t MaxLength);
	// Pomija Length znak�w.
	// Je�li nie uda si� pomin�� wszystkich Length znak�w, rzuca wyj�tek.
	void MustSkip(size_t Length);
	// Wczytuje linijk� tekstu i je�li si� uda�o, zwraca true.
	// Je�li nie uda�o si� wczyta� ani jednego znaku, bo to ju� koniec strumienia, zwraca false.
	// Czyta znaki do napotkania ko�ca strumienia lub ko�ca wiersza, kt�rym s� "\r", "\n" i "\r\n".
	// Koniec wiersza przeskakuje, ale nie do��cza go do Out.
	bool ReadLine(string *Out);
	// Wczytuje linijk� tekstu.
	// Je�li nie uda�o si� wczyta� ani jednego znaku, bo to ju� koniec strumienia, rzuca wyj�tek.
	// Czyta znaki do napotkania ko�ca strumienia lub ko�ca wiersza, kt�rym s� "\r", "\n" i "\r\n".
	// Koniec wiersza przeskakuje, ale nie do��cza go do Out.
	void MustReadLine(string *Out);
};

// Strumie� pami�ci statycznej
// Strumie� ma sta�� d�ugo�� i nie mo�e by� rozszerzany - ani automatycznie, ani r�cznie.
class MemoryStream : public SeekableStream
{
private:
	char *m_Data;
	size_t m_Size;
	int m_Pos;
	bool m_InternalAlloc;

public:
	// Podaj wska�nik 0, aby strumie� sam zaalkowa�, a w destruktorze zwolni� pami��.
	// Podaj wska�nik, je�li ma korzysta� z twojego obszaru pami�ci.
	MemoryStream(size_t Size, void *Data = 0);
	virtual ~MemoryStream();

	virtual void Write(const void *Data, size_t Size);
	virtual size_t Read(void *Data, size_t Size);
	virtual void MustRead(void *Data, size_t Size);

	virtual size_t GetSize();
	virtual int GetPos();
	virtual void SetPos(int pos);
	virtual void Rewind();

	char *Data() { return m_Data; }
};

// Strumie� pami�ci, ale dynamicznej.
// Strumie� mo�na rozszerza� i sam te� si� rozszerza.
class VectorStream : public SeekableStream
{
private:
	char *m_Data;
	size_t m_Size;
	size_t m_Capacity;
	int m_Pos;

	// Zmienia capacity, a tym samym przealokowuje pami��.
	// O niczym sama nie decyduje ani niczego nie sprawdza.
	void Reserve(size_t NewCapacity);

public:
	VectorStream();
	virtual ~VectorStream();

	virtual void Write(const void *Data, size_t Size);
	virtual size_t Read(void *Data, size_t Size);
	virtual void MustRead(void *Data, size_t Size);
	virtual size_t GetSize() { return m_Size; }
	virtual int GetPos() { return m_Pos; }
	virtual void SetPos(int pos) { m_Pos = pos; }
	virtual void Rewind() { m_Pos = 0; }
	virtual void SetSize(size_t Size);

	size_t GetCapacity() { return m_Capacity; }
	void SetCapacity(size_t Capacity);
	// Nie musi pozosta� aktualny po wywo�aniach metod tej klasy!
	// (mo�e zosta� przealokowany)
	char *Data() { return m_Data; }
};

// Strumie� oparty na �a�cuchu std::string
// Potrafi si� rozszerza�.
class StringStream : public SeekableStream
{
private:
	std::string *m_Data;
	int m_Pos;
	bool m_InternalAlloc;

public:
	// Podaj wska�nik 0, aby strumie� sam zaalokowa�, a w destruktorze zwolni� �a�cuch.
	// Podaj wska�nik, je�li ma korzysta� z twojego obszaru pami�ci.
	StringStream(string *Data = 0);
	virtual ~StringStream();

	virtual void Write(const void *Data, size_t Size);
	virtual size_t Read(void *Data, size_t Size);
	virtual void MustRead(void *Data, size_t Size);
	virtual size_t GetSize() { return m_Data->length(); }
	virtual int GetPos() { return m_Pos; }
	virtual void SetPos(int pos) { m_Pos = pos; }
	virtual void Rewind() { m_Pos = 0; }
	virtual void SetSize(size_t Size) { m_Data->resize(Size); }
	virtual void Clear() { m_Data->clear(); }

	size_t GetCapacity() { return m_Data->capacity(); }
	void SetCapacity(size_t Capacity) { m_Data->reserve(Capacity); }
	string *Data() { return m_Data; }
};

// Abstrakcyjna klasa bazowa nak�adek na strumienie
class OverlayStream : public Stream
{
private:
	Stream *m_Stream;

public:
	OverlayStream(Stream *a_Stream) : m_Stream(a_Stream) { }

	Stream * GetStream() { return m_Stream; }

	// ======== Implementacja Stream ========
	virtual bool End() { return m_Stream->End(); }
	virtual void Flush() { m_Stream->Flush(); }
};

// Nak�adka na strumie� zliczaj�ca ilo�� zapisanych i odczytanych bajt�w
class CounterOverlayStream : public OverlayStream
{
private:
	uint4 m_WriteCounter;
	uint4 m_ReadCounter;

public:
	CounterOverlayStream(Stream *a_Stream);

	// Z klasy Stream
	virtual void Write(const void *Data, size_t Size);
	virtual size_t Read(void *Data, size_t Size);
	virtual void MustRead(void *Data, size_t Size);
	virtual void Flush();

	uint4 GetWriteCounter() { return m_WriteCounter; }
	uint4 GetReadCounter() { return m_ReadCounter; }
	void ResetWriteCounter() { m_WriteCounter = 0; }
	void ResetReadCounter() { m_ReadCounter = 0; }
};

// Nak�adka na strumie� ograniczaj�ca liczb� odczytywanych i zapisywanych
// bajt�w. Jesli przekroczy limit podczas zapisywania, zapisuje ile si� da i
// rzuca b��d zapisu. Je�li przekroczy limit podczas odczytywania zachowuje si�
// tak, jakby strumie� osi�gn�� koniec.
class LimitOverlayStream : public OverlayStream
{
private:
	// Przechowuj� aktualny limit, jaki pozosta�.
	// S� zmniejszane przez funkcje odczytuj�ce i zapisuj�ce.
	uint4 m_WriteLimit;
	uint4 m_ReadLimit;

public:
	LimitOverlayStream(Stream *a_Stream, uint4 WriteLimit, uint4 ReadLimit);

	// Z klasy Stream
	virtual void Write(const void *Data, size_t Size);
	virtual size_t Read(void *Data, size_t Size);
	virtual void Flush();

	// Ustawia nowy limit
	void SetWriteLimit(uint4 WriteLimit);
	void SetReadLimit(uint4 ReadLimit);
};

// Ten strumie� zapisuje zapisywane dane do wielu pod��czonych do niego strumieni na raz.
class MultiWriterStream : public Stream
{
private:
	std::vector<Stream*> m_Streams;

public:
	MultiWriterStream() { }
	MultiWriterStream(Stream *a_Streams[], uint StreamCount);

	// Dodaje strumie� do listy.
	void AddStream(Stream *a_Stream) { m_Streams.push_back(a_Stream); }
	// Usuwa strumie� z listy. Je�li takiego nie znajdzie, nic nie robi.
	void RemoveStream(Stream *a_Stream);
	// Usuwa wszystkie strumienie z listy.
	void ClearStreams() { m_Streams.clear(); }
	size_t GetStreamCount() { return m_Streams.size(); }

	// ======== Implementacja Stream ========
	virtual void Write(const void *Data, size_t Size);
	virtual void Flush();
};

// Klasa obliczaj�ca hash 32-bitowy z kolejno podawanych blok�w danych
// Algorytm: Jenkins One-at-a-time hash, http://www.burtleburtle.net/bob/hash/doobs.html
// [Strumie� nie seekable, tylko zapis]
// Prawid�owe u�ycie: Write, Write (lub inne funkcje zapisuj�ce), ..., Finish
// Po wywo�aniu Finish nie mo�na dalej zapisywa� danych!
// Reset - rozpoczyna liczenie sumy od nowa.
class Hash_Calc : public Stream
{
private:
	uint4 m_Hash;

public:
	Hash_Calc() : m_Hash(0) { }

	// ======== Implementacja Stream ========
	virtual void Write(const void *Data, size_t Size);

	// Ko�czy obliczenia i zwraca policzon� sum�
	uint4 Finish();
	// Rozpoczyna liczenie nowej sumy
	void Reset() { m_Hash = 0; }

	// ======== Statyczne ========
	// Po prostu oblicza sum� kontroln� z podanych danych
	static uint4 Calc(const void *Buf, uint4 BufLen);
	static uint4 Calc(const string &s);
};

// Klasa obliczaj�ca sum� CRC32 z kolejno podawanych blok�w danych
// [Strumie� nie seekable, tylko zapis]
// GetResult - wyliczon� dotychczas sum� mo�na otrzymywa� w ka�dej chwili, a potem dalej dodawa� nowe dane.
// Reset - rozpoczyna liczenie nowej sumy kontrolnej.
class CRC32_Calc : public Stream
{
private:
	uint m_CRC;

public:
	CRC32_Calc() { m_CRC = 0xFFFFFFFF; }

	// ======== Implementacja Stream ========
	virtual void Write(const void *Data, size_t Size);

	// Zwraca policzon� dotychczas sum�
	uint GetResult() { return ~m_CRC; }
	// Rozpoczyna liczenie nowej sumy
	void Reset() { m_CRC = 0xFFFFFFFF; }

	// ======== Statyczne ========
	// Po prostu oblicza sum� kontroln� z podanych danych
	static uint Calc(const void *Data, size_t DataLength);
};

// Suma ma 128 bit�w - 16 bajt�w. Jest typu uint1[16].
struct MD5_SUM
{
	uint1 Data[16];

	uint1 & operator [] (size_t i) { return Data[i]; }
	uint1 operator [] (size_t i) const { return Data[i]; }

	bool operator == (const MD5_SUM &s) const
	{
		return
			Data[ 0] == s.Data[ 0] && Data[ 1] == s.Data[ 1] && Data[ 2] == s.Data[ 2] && Data[ 3] == s.Data[ 3] &&
			Data[ 4] == s.Data[ 4] && Data[ 5] == s.Data[ 5] && Data[ 6] == s.Data[ 6] && Data[ 7] == s.Data[ 7] &&
			Data[ 8] == s.Data[ 8] && Data[ 9] == s.Data[ 9] && Data[10] == s.Data[10] && Data[11] == s.Data[11] &&
			Data[12] == s.Data[12] && Data[13] == s.Data[13] && Data[14] == s.Data[14] && Data[15] == s.Data[15];
	}
	bool operator != (const MD5_SUM &s) const
	{
		return
			Data[ 0] != s.Data[ 0] || Data[ 1] != s.Data[ 1] || Data[ 2] != s.Data[ 2] || Data[ 3] != s.Data[ 3] ||
			Data[ 4] != s.Data[ 4] || Data[ 5] != s.Data[ 5] || Data[ 6] != s.Data[ 6] || Data[ 7] != s.Data[ 7] ||
			Data[ 8] != s.Data[ 8] || Data[ 9] != s.Data[ 9] || Data[10] != s.Data[10] || Data[11] != s.Data[11] ||
			Data[12] != s.Data[12] || Data[13] != s.Data[13] || Data[14] != s.Data[14] || Data[15] != s.Data[15];
	}
};

void MD5ToStr(string *Out, const MD5_SUM MD5);
bool StrToMD5(MD5_SUM *Out, const string &s);

// Klasa obliczaj�ca sum� MD5 z kolejno podawanych blok�w danych
// [Strumie� nie seekable, tylko zapis]
// Prawid�owe u�ycie: Write, Write (lub inne funkcje zapisuj�ce), ..., Finish
// Po wywo�aniu Finish nie mo�na dalej zapisywa� danych!
// Reset - rozpoczyna liczenie sumy od nowa.
class MD5_Calc : public Stream
{
private:
	uint4 total[2];
	uint4 state[4];
	uint1 buffer[64];

	void Process(uint1 data[64]);

public:
	MD5_Calc();

	// ======== Implementacja Stream ========
	virtual void Write(const void *Data, size_t Size);

	// Ko�czy obliczenia i zwraca policzon� sum�
	void Finish(MD5_SUM *Out);
	// Rozpoczyna liczenie nowej sumy
	void Reset();

	// ======== Statyczne ========
	// Po prostu oblicza sum� kontroln� z podanych danych
	static void Calc(MD5_SUM *Out, const void *Buf, uint4 BufLen);
};

// Koduje lub dekoduje zapisywane/odczytywane bajty XOR podany bajt lub ci�g bajt�w
// Mapuje bezpo�rednio bajty na bajty strumienia do kt�rego jest pod��czony,
// nic nie buforuje, wi�c mo�na operowa� te� na strumieniu Stream.
class XorCoder : public OverlayStream
{
private:
	std::vector<char> m_Buf;
	std::vector<char> m_Key;
	uint m_EncodeKeyIndex;
	uint m_DecodeKeyIndex;

public:
	XorCoder(Stream *a_Stream, char KeyByte);
	XorCoder(Stream *a_Stream, const void *KeyData, size_t KeyLength);
	XorCoder(Stream *a_Stream, const string &Key);

	// ======== Implementacja Stream ========
	virtual void Write(const void *Data, size_t Size);
	virtual size_t Read(void *Data, size_t Size);

	// Resetuje liczniki tak �eby kodowanie i dekodowanie odbywa�o si� od pocz�tku klucza
	void Reset() { m_EncodeKeyIndex = m_DecodeKeyIndex = 0; }

	// ======== Statyczne ========
	// Po prostu przetwarza podane dane
	static void Code(void *Out, const void *Data, size_t DataLength, const void *Key, size_t KeyLength);
	static void Code(string *Out, const string &Data, const string &Key);
};

// Koduje strumie� binarnych danych wej�ciowych na ci�g z�o�ony ze znak�w '0' i '1' (po 8 na ka�dy bajt).
class BinEncoder : public OverlayStream
{
private:
	CharWriter m_CharWriter;

public:
	BinEncoder(Stream *a_Stream) : OverlayStream(a_Stream), m_CharWriter(a_Stream) { }

	// ======== Implementacja Stream ========
	virtual void Write(const void *Data, size_t Size);
	virtual void Flush() { m_CharWriter.Flush(); OverlayStream::Flush(); }

	// ======== Statyczne ========
	// Po prostu koduje podane dane
	// Bufor wyj�ciowy Out musi mie� zaalokowane przynajmniej DataLength * 8 bajt�w.
	static void Encode(char *Out, const void *Data, size_t DataLength);
	// �a�cuch wyj�ciowy Out nie musi mie� �adnego konkretnego rozmiaru - zostaje wyczyszczony i wype�niony od nowa.
	static void Encode(string *Out, const void *Data, size_t DataLength);
};

// Dekoduje ci�g znak�w wej�ciowych zapisanych binarnie (znakami '0' i '1') na dane binarne.
class BinDecoder : public OverlayStream
{
private:
	CharReader m_CharReader;
	DECODE_TOLERANCE m_Tolerance;

public:
	BinDecoder(Stream *a_Stream, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);

	DECODE_TOLERANCE GetTolerance() { return m_Tolerance; }

	// ======== Implementacja Stream ========
	virtual size_t Read(void *Out, size_t Size);
	virtual bool End() { return m_CharReader.End(); }

	// ======== Statyczne ========
	// Znajduje d�ugo�� zakodowanych danych i zwraca true.
	// Je�li nie uda�o si� ustali� d�ugo�ci, zwraca false.
	static bool DecodeLength(uint *OutLength, const string &s, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	static bool DecodeLength(uint *OutLength, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	// Dekoduje dane binarne. Zwraca liczb� zdekodowanych bajt�w.
	// Je�li nie uda�o si� zdekodowa� (jaki� b��d), zwraca MAXUINT4 (0xFFFFFFFF).
	// - OutData musi by� tablic� zaalokowan� przynajmniej na rozmiar ustalony wywo�aniem DecodeLength
	//   lub je�li nie chcesz wywo�ywa� DecodeLength, na d�ugo�� r�wn� d�ugo�ci danych wej�ciowych / 8.
	static size_t Decode(void *OutData, const string &s, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	static size_t Decode(void *OutData, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
};

// Koduje strumie� binarnych danych wej�ciowych na zapis szesnastkowy (po 2 znaki na ka�dy bajt).
class HexEncoder : public OverlayStream
{
private:
	CharWriter m_CharWriter;
	bool m_UpperCase;

public:
	HexEncoder(Stream *a_Stream, bool UpperCase = true) : OverlayStream(a_Stream), m_CharWriter(a_Stream), m_UpperCase(UpperCase) { }

	// ======== Implementacja Stream ========
	virtual void Write(const void *Data, size_t Size);
	virtual void Flush() { m_CharWriter.Flush(); OverlayStream::Flush(); }

	// ======== Statyczne ========
	// Po prostu koduje podane dane
	// Bufor wyj�ciowy Out musi mie� zaalokowane przynajmniej DataLength * 2 bajt�w.
	static void Encode(char *Out, const void *Data, size_t DataLength, bool UpperCase = true);
	// �a�cuch wyj�ciowy Out nie musi mie� �adnego konkretnego rozmiaru - zostaje wyczyszczony i wype�niony od nowa.
	static void Encode(string *Out, const void *Data, size_t DataLength, bool UpperCase = true);
};

// Dekoduje ci�g znak�w wej�ciowych zapisanych szesnastkowo na dane binarne.
// Akceptuje zar�wno ma�e, jak i du�e litery.
class HexDecoder : public OverlayStream
{
private:
	CharReader m_CharReader;
	DECODE_TOLERANCE m_Tolerance;

public:
	HexDecoder(Stream *a_Stream, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);

	DECODE_TOLERANCE GetTolerance() { return m_Tolerance; }

	// ======== Implementacja Stream ========
	virtual size_t Read(void *Out, size_t Size);
	virtual bool End() { return m_CharReader.End(); }

	// ======== Statyczne ========
	// Znajduje d�ugo�� zakodowanych danych i zwraca true.
	// Je�li nie uda�o si� ustali� d�ugo�ci, zwraca false.
	static bool DecodeLength(uint *OutLength, const string &s, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	static bool DecodeLength(uint *OutLength, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	// Dekoduje dane binarne. Zwraca liczb� zdekodowanych bajt�w.
	// Je�li nie uda�o si� zdekodowa� (jaki� b��d), zwraca MAXUINT4 (0xFFFFFFFF).
	// - OutData musi by� tablic� zaalokowan� przynajmniej na rozmiar ustalony wywo�aniem DecodeLength
	//   lub je�li nie chcesz wywo�ywa� DecodeLength, na d�ugo�� r�wn� d�ugo�ci danych wej�ciowych / 2.
	static size_t Decode(void *OutData, const string &s, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	static size_t Decode(void *OutData, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
};

// Koduje strumie� binarnych danych wej�ciowych na ci�g znak�w base64.
/*
- Zgodny z RFC 4648 "The Base16, Base32, and Base64 Data Encodings"
- Prawid�owe dane wyj�ciowe powstaj� dopiero po zako�czeniu zapisywania, kt�re nast�puje w destruktorze.
  Je�li chcesz zako�czy� wcze�niej, wywo�aj Finish, ale potem nie mo�na ju� nic dopisywa�.
*/
class Base64Encoder : public OverlayStream
{
private:
	CharWriter m_CharWriter;
	bool m_Finished; // Czy dane zako�czone
	uint1 m_Buf[2];
	uint m_BufIndex;

	void DoFinish();

public:
	Base64Encoder(Stream *a_Stream) : OverlayStream(a_Stream), m_CharWriter(a_Stream), m_Finished(false), m_BufIndex(0) { }
	~Base64Encoder();

	void Finish() { if (!m_Finished) { DoFinish(); m_Finished = true; } }

	// ======== Implementacja Stream ========
	virtual void Write(const void *Data, size_t Size);
	virtual void Flush() { m_CharWriter.Flush(); OverlayStream::Flush(); }

	// ======== Statyczne ========
	// Oblicza rozmiar bufora na zakodowane dane
	static size_t EncodeLength(size_t DataLength);
	// Po prostu koduje podane dane
	// Zwraca liczb� znak�w zapisanych do Out.
	// Bufor wyj�ciowy Out musi mie� zaalokowane przynajmniej tyle bajt�w ile wyliczy�o EncodeLength.
	static size_t Encode(char *Out, const void *Data, size_t DataLength);
	// �a�cuch wyj�ciowy Out nie musi mie� �adnego konkretnego rozmiaru - zostaje wyczyszczony i wype�niony od nowa.
	static size_t Encode(string *Out, const void *Data, size_t DataLength);
};

// Dekoduje ci�g znak�w wej�ciowych zapisanych w standardzie base64 na dane binarne.
/*
- Po napotkaniu ko�c�wki zawieraj�cej znaki '=' funkcje nie parsuj� ju� dalej,
  ale to tylko optymalizacja i nie nale�y na tym polega�.
- Spos�b dzia�ania strumienia - na raz wczytywane s� po 4 znaki i zamieniane na
  maksymalnie 3 bajty.
*/
class Base64Decoder : public OverlayStream
{
private:
	CharReader m_CharReader;
	DECODE_TOLERANCE m_Tolerance;
	// Bajty s� w kolejno�ci: [2][1][0]. Je�li jest mniej, to [1][0][-] albo [0][-][-].
	uint1 m_Buf[3];
	// Liczba zdekodowanych bajt�w w buforze
	uint m_BufLength;
	// True, je�li sparsowano ko�c�wk� z '='
	bool m_Finished;

	// Czyta 4 znaki ze strumienia pod��czonego (0 albo 4, inaczej b��d) i wype�nia m_Buf maksymalnie 3 zdekodowanymi bajtami.
	// Ustawia m_BufLength i je�li trzeba, to m_Finished.
	bool ReadNextBuf();

	bool GetNextByte(uint1 *OutByte) { if (m_BufLength == 0) { if (!ReadNextBuf()) return false; } *OutByte = m_Buf[--m_BufLength]; return true; }

public:
	Base64Decoder(Stream *a_Stream, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);

	DECODE_TOLERANCE GetTolerance() { return m_Tolerance; }

	// ======== Implementacja Stream ========
	virtual size_t Read(void *Out, size_t Size);
	virtual bool End();

	// ======== Statyczne ========
	// Znajduje d�ugo�� zakodowanych danych i zwraca true.
	// Je�li nie uda�o si� ustali� d�ugo�ci, zwraca false.
	static bool DecodeLength(uint *OutLength, const string &s, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	static bool DecodeLength(uint *OutLength, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	// Dekoduje dane binarne. Zwraca liczb� zdekodowanych bajt�w.
	// Je�li nie uda�o si� zdekodowa� (jaki� b��d), zwraca MAXUINT4 (0xFFFFFFFF).
	// OutData musi by� tablic� zaalokowan� przynajmniej na rozmiar ustalony wywo�aniem DecodeLength.
	static size_t Decode(void *OutData, const string &s, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	static size_t Decode(void *OutData, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
};

/*
Bufor ko�owy. Dzia�a szybko (na tyle na ile szybko mo�e dzia�a� strumie�, ze
swoimi metodami wirtualnymi itd.), ale ma z g�ry ograniczon� pojemno��. W
przypadku przepe�nienia przy zapisie rzuca wyj�tek.
*/
class RingBuffer : public Stream
{
public:
	RingBuffer(uint Capacity);
	virtual ~RingBuffer();

	// Zwraca liczb� bajt�w w buforze
	uint GetSize() { return m_Size; }
	// Zwraca pojemno�� bufora
	uint GetCapacity() { return m_Capacity; }
	// Zwraca true, je�li bufor jest pusty/pe�ny
	bool IsEmpty() { return GetSize() == 0; }
	bool IsFull() { return GetSize() == GetCapacity(); }

	// ======== Implementacja Stream ========
	virtual void Write(const void *Data, size_t Size);
	virtual size_t Read(void *Out, size_t MaxLength);
	virtual void MustRead(void *Out, size_t Length);
	virtual bool End();
	virtual size_t Skip(size_t MaxLength);

private:
	uint m_Capacity;
	uint m_Size;
	std::vector<char> m_Buf;
	uint m_BegIndex;
	uint m_EndIndex;
};


// Kopiuje dane z jednego strumienia do drugiego - co najwy�ej MaxLength bajt�w.
// Zwraca liczb� przekopiowanych danych.
inline size_t Copy(Stream *Dst, Stream *Src, size_t MaxLength) { return Dst->CopyFrom(Src, MaxLength); }
// Kopiuje dane z jednego strumienia do drugiego - dok�adnie Length bajt�w.
// Je�li si� nie uda przekopiowa� tylu bajt�w, rzuca b��d.
inline void MustCopy(Stream *Dst, Stream *Src, size_t Length) { return Dst->MustCopyFrom(Src, Length); }
// Kopiuje dane z jednego strumienia do drugiego a� do ko�ca tego �r�d�owego
inline void CopyToEnd(Stream *Dst, Stream *Src) { Dst->CopyFromToEnd(Src); }

} // namespace common

template <>
struct SthToStr_obj<common::MD5_SUM>
{
	void operator () (string *Str, const common::MD5_SUM &Sth)
	{
		common::MD5ToStr(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<common::MD5_SUM>
{
	bool operator () (common::MD5_SUM *Sth, const string &Str)
	{
		return common::StrToMD5(Sth, Str);
	}
	static inline bool IsSupported() { return true; }
};

#endif
