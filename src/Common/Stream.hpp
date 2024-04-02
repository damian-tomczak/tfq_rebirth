/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
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

// [Wewnêtrzne]
void _ThrowBufEndError(const char *File, int Line);

enum DECODE_TOLERANCE
{
	DECODE_TOLERANCE_NONE,       // ¯adne dodatkowe znaki nie bêd¹ tolerowane
	DECODE_TOLERANCE_WHITESPACE, // Bia³e znaki bêd¹ ignorowane i nie spowoduj¹ b³êdu
	DECODE_TOLERANCE_ALL,        // Wszelkie nieznane znaki bêd¹ ignorowane i nie spowoduj¹ b³¹du
};


// Abstrakcyjna klasa bazowa strumieni danych binarnych
class Stream
{
public:
	// wirtualny destruktor (klasa jest polimorficzna)
	virtual ~Stream() { }

	// ======== ZAPISYWANIE ========

	// Zapisuje dane
	// (W oryginale: zg³asza b³¹d)
	virtual void Write(const void *Data, size_t Size);
	// Domyœlnie nie robi nic.
	virtual void Flush() { }

	// Zapisuje dane, sama odczytuje rozmiar przekazanej zmiennej
	template <typename T>
	void WriteEx(const T &x) { return Write(&x, sizeof(x)); }
	// Zapisuje rozmiar na 1B i ³añcuch
	// Jeœli ³añcuch za d³ugi - b³¹d.
	void WriteString1(const string &s);
	// Zapisuje rozmiar na 2B i ³añcuch
	// Jeœli ³añcuch za d³ugi - b³¹d.
	void WriteString2(const string &s);
	// Zapisuje rozmiar na 4B i ³añcuch
	void WriteString4(const string &s);
	// Zapisuje ³añcuch bez rozmiaru
	void WriteStringF(const string &s);
	// Zapisuje wartoœæ logiczn¹ za pomoc¹ jednego bajtu
	void WriteBool(bool b);

	// ======== ODCZYTYWANIE ========

	// Odczytuje dane
	// Size to liczba bajtów do odczytania
	// Zwraca liczbê bajtów, jakie uda³o siê odczytaæ
	// Jeœli osi¹gniêto koniec, funkcja nie zwraca b³êdu, a liczba odczytanych bajtów to 0.
	// (W oryginale: zg³asza b³¹d)
	virtual size_t Read(void *Data, size_t MaxLength);
	// Odczytuje tyle bajtów, ile siê za¿¹da
	// Jeœli koniec pliku albo jeœli odczytano mniej, zg³asza b³¹d.
	// (Mo¿na j¹ prze³adowaæ, ale nie trzeba - ma swoj¹ wersjê oryginaln¹)
	virtual void MustRead(void *Data, size_t Length);
	// Tak samo jak MustRead, ale sama odczytuje rozmiar przekazanej zmiennej
	// Zwraca true, jeœli osi¹gniêto koniec strumienia
	// (W oryginale: zg³asza b³¹d)
	virtual bool End();
	// Pomija co najwy¿ej podan¹ liczbê bajtów (chyba ¿e wczeœniej osi¹gniêto koniec).
	// Zwraca liczbê pominiêtych bajtów.
	// (Mo¿na j¹ prze³adowaæ, ale nie trzeba - ma swoj¹ wersjê oryginaln¹)
	virtual size_t Skip(size_t MaxLength);

	template <typename T>
	void ReadEx(T *x) { MustRead(x, sizeof(*x)); }
	// Odczytuje ³añcuch poprzedzony rozmiarem 1B
	void ReadString1(string *s);
	// Odczytuje ³añcuch poprzedzony rozmiarem 2B
	void ReadString2(string *s);
	// Odczytuje ³añcuch poprzedzony rozmiarem 4B
	void ReadString4(string *s);
	// Odczytuje ³añcuch bez zapisanego rozmiaru
	void ReadStringF(string *s, size_t Length);
	// Odczytuje tyle znaków do ³añcucha, ile siê da do koñca strumienia
	void ReadStringToEnd(string *s);
	// Odczytuje wartoœæ logiczn¹ za pomoc¹ jednego bajtu
	void ReadBool(bool *b);
	// Pomija koniecznie podan¹ liczbê bajtów.
	// Jeœli wczeœniej osi¹gniêto koniec, zg³asza b³¹d.
	void MustSkip(size_t Length);

	// ======== KOPIOWANIE ========
	// Odczytuje podan¹ co najwy¿ej liczbê bajtów z podanego strumienia
	// Zwraca liczbê odczytanych bajtów
	size_t CopyFrom(Stream *s, size_t Size);
	// Odczytuje dok³adnie podan¹ liczbê bajtów z podanego strumienia
	// Jeœli odczyta mniej, zg³asza b³¹d.
	void MustCopyFrom(Stream *s, size_t Size);
	// Odczytuje dane do koñca z podanego strumienia
	void CopyFromToEnd(Stream *s);
};

// Abstrakcyjna klasa bazowa strumieni pozwalaj¹cych na odczytywanie rozmiaru i zmianê pozycji
class SeekableStream : public Stream
{
public:
	// ======== Implementacja Stream ========
	// (W Stream domyœlnie rzuca b³¹d, tu w SeekableStream dostaje ju¿ wersjê oryginaln¹. Mo¿na j¹ dalej nadpisaæ.)
	virtual bool End();
	// (W Stream ma wersjê oryginaln¹, tu w SeekableStream dostaje lepsz¹. Mo¿na j¹ dalej nadpisaæ.)
	virtual size_t Skip(size_t MaxLength);

	// Zwraca rozmiar
	// (W oryginale: zg³asza b³¹d)
	virtual size_t GetSize();
	// Zwraca pozycjê wzglêdem pocz¹tku
	// (W oryginale: zg³asza b³¹d)
	virtual int GetPos();
	// Ustawia pozycjê wzglêdem pocz¹tku
	// pos musi byæ dodatnie.
	// (W oryginale: zg³asza b³¹d)
	virtual void SetPos(int pos);
	// Ustawia pozycjê wzglêdem bie¿¹cej
	// pos mo¿e byæ dodatnie albo ujemne.
	// (Mo¿na j¹ nadpisaæ, ale ma wersjê oryginaln¹)
	virtual void SetPosFromCurrent(int pos);
	// Ustawia pozycjê wzglêdem koñcowej
	// Np. abu ustawiæ na ostatni znak, przesuñ do -1.
	// (Mo¿na j¹ nadpisaæ, ale ma wersjê oryginaln¹)
	virtual void SetPosFromEnd(int pos);
	// Ustawia pozycjê na pocz¹tku
	// (Mo¿na j¹ nadpisaæ, ale ma wersjê oryginaln¹)
	virtual void Rewind();
	// Ustawia rozmiar na podany
	// Po tym wywo³aniu pozycja kursora jest niezdefiniowana.
	// (W oryginale: zg³asza b³¹d)
	virtual void SetSize(size_t Size);
	// Obcina strumieñ od bie¿¹cej pozycji do koñca, czyli ustawia taki rozmiar jak bie¿¹ca pozycja.
	// (Mo¿na j¹ nadpisaæ, ale ma wersjê oryginaln¹)
	virtual void Truncate();
	// Opró¿nia strumieñ
	// (Mo¿na j¹ nadpisaæ, ale ma wersjê oryginaln¹)
	virtual void Clear();
};

// Nak³adka przyspieszaj¹ca zapisuj¹ca do strumienia pojedyncze znaki i inne rzeczy.
/*
- Nie jest strumieniem.
- Tylko zapis.
- Jest szybka, poniewa¿:
  1. Nie ma metod wirtualnych.
  2. Buforuje, wiêc nie u¿ywa za ka¿dym razem metod wirtualnych strumienia.
- Sama zapisuje dane pozosta³e w buforze w destruktorze.
  Jeœli chcesz to zrobiæ wczeœniej i z kontrol¹ b³êdów, wywo³aj Flush.
- W trybie Debug dzia³a nawet trochê wolniej ni¿ zapisywanie po bajcie prosto do strumienia.
  W trybie Release daje przyspieszenie prawie 10x wzglêdem zapisywania po bajcie prosto do strumienia.
- Celowo nie u¿ywam w tej klasie ERR_TRY .. ERR_CATCH_FUNC, ¿eby by³o szybciej.
*/
class CharWriter
{
private:
	Stream *m_Stream;
	std::vector<char> m_Buf;
	uint m_BufIndex;

	// Wykonuje Flush danych z bufora do strumienia nie sprawdzaj¹c ju¿, czy bufor nie jest pusty.
	// Oczywiœcie zeruje te¿ BufIndex.
	void DoFlush() { m_Stream->Write(&m_Buf[0], m_BufIndex); m_BufIndex = 0; }

public:
	CharWriter(Stream *a_Stream) : m_Stream(a_Stream), m_BufIndex(0) { m_Buf.resize(BUFFER_SIZE); }
	~CharWriter();

	// Pojedynczy znak
	void WriteChar(char Ch) { if (m_BufIndex == BUFFER_SIZE) DoFlush(); m_Buf[m_BufIndex++] = Ch; }
	// Zapisuje po prostu znaki ³añcucha, bez ¿adnej d³ugoœci
	void WriteString(const string &s);
	// £añcuch zakoñczony zerem
	void WriteString(const char *s) { WriteString(s, strlen(s)); }
	// £añcuch podanej d³ugoœci
	void WriteString(const char *s, size_t Length) { WriteData(s, Length); }
	// Surowe dane binarne
	void WriteData(const void *Data, size_t Length);
	// Zapisuje dowoln¹ informacjê przerobion¹ na postaæ tekstow¹, o ile da siê przerobiæ
	template <typename T> void WriteEx(const T &x) { string s; SthToStr<T>(&s, x); WriteString(s); }
	// Wymusza opró¿nienie bufora i wys³anie pozosta³ych w nim danych do strumienia
	void Flush() { if (m_BufIndex > 0) DoFlush(); }
};

// Nak³adka przyspieszaj¹ca odczytuj¹ca ze strumienia pojedyncze znaki i inne rzeczy.
/*
- Nie jest strumieniem.
- Tylko odczyt.
- Jest szybka, poniewa¿:
  1. Nie ma metod wirtualnych.
  2. Buforuje, wiêc nie u¿ywa za ka¿dym razem metod wirtualnych strumienia.
- Szybkoœæ wzglêdem wczytywania pojedynczych znaków prosto ze strumienia: Debug = 10 razy, Release = 450 razy!
- Celowo nie u¿ywam w tej klasie ERR_TRY .. ERR_CATCH_FUNC, ¿eby by³o szybciej.
*/
class CharReader
{
private:
	Stream *m_Stream;
	std::vector<char> m_Buf;
	// Miejsce, do którego doczyta³em z bufora
	uint m_BufBeg;
	// Miejsce, do którego bufor jest wype³niony danymi
	uint m_BufEnd;

	// Wczytuje now¹ porcjê danych do strumienia
	// Wywo³ywaæ tylko kiedy bufor siê skoñczy³, tzn. m_BufBeg == m_BufEnd.
	// Jeœli skoñczy³ siê strumieñ i nic nie uda³o siê wczytaæ, zwraca false.
	// Jeœli nie, zapewnia ¿e w buforze bêdzie co najmniej jeden znak.
	bool EnsureNewChars();

public:
	CharReader(Stream *a_Stream) : m_Stream(a_Stream), m_BufBeg(0), m_BufEnd(0) { m_Buf.resize(BUFFER_SIZE); }

	// Czy jesteœmy na koñcu danych?
	bool End() {
		return (m_BufBeg == m_BufEnd) && m_Stream->End(); // Nic nie zosta³o w buforze i nic nie zosta³o w strumieniu.
	}
	// Jeœli mo¿na odczytaæ nastêpny znak, wczytuje go i zwraca true.
	// Jeœli nie, nie zmienia Out i zwraca false. Oznacza to koniec strumienia.
	bool ReadChar(char *Out) { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) return false; } *Out = m_Buf[m_BufBeg++]; return true; }
	// Jeœli mo¿na odczytaæ nastêpny znak, wczytuje go.
	// Jeœli nie, rzuca wyj¹tek.
	char MustReadChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) _ThrowBufEndError(__FILE__, __LINE__); } return m_Buf[m_BufBeg++]; }
	// Jeœli mo¿na odczytaæ nastêpny znak, podgl¹da go zwracaj¹c przez Out i zwraca true. Nie przesuwa "kursora".
	// Jeœli nie, nie zmienia Out i zwraca false. Oznacza to koniec strumienia.
	bool PeekChar(char *Out) { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) return false; } *Out = m_Buf[m_BufBeg]; return true; }
	// Jeœli mo¿na odczytaæ nastêpny znak, podgl¹da go zwracaj¹c. Nie przesuwa "kursora".
	// Jeœli nie, rzuca wyj¹tek.
	char MustPeekChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) _ThrowBufEndError(__FILE__, __LINE__); } return m_Buf[m_BufBeg]; }
	// Wczytuje co najwy¿ej MaxLength znaków do podanego stringa.
	// StringStream jest czyszczony - nie musi byæ pusty ani zaalokowany.
	// Zwraca liczbê odczytanych znaków. Mniej ni¿ ¿¹dano oznacza koniec strumienia.
	size_t ReadString(string *Out, size_t MaxLength);
	// Wczytuje Length znaków do podanego stringa.
	// StringStream jest czyszczony - nie musi byæ pusty ani zaalokowany.
	// Jeœli nie uda siê odczytaæ wszystkich Length znaków, rzuca wyj¹tek.
	void MustReadString(string *Out, size_t Length);
	// Wczytuje co najwy¿ej MaxLength znaków pod podany wskaŸnik.
	// Out musi byæ tablic¹ zaalokowan¹ do d³ugoœci co najmniej MaxLength.
	// Zwraca liczbê odczytanych znaków. Mniej ni¿ ¿¹dano oznacza koniec strumienia.
	size_t ReadString(char *Out, size_t MaxLength);
	// Wczytuje Length znaków pod podany wskaŸnik.
	// Out musi byæ tablic¹ zaalokowan¹ do d³ugoœci co najmniej Length.
	// Jeœli nie uda siê odczytaæ wszystkich Length znaków, rzuca wyj¹tek.
	void MustReadString(char *Out, size_t Length);
	// Wczytuje co najwy¿ej MaxLength bajtów pod podany wskaŸnik.
	// Out musi byæ tablic¹ zaalokowan¹ do d³ugoœci co najmniej MaxLength bajtów.
	// Zwraca liczbê odczytanych bajtów. Mniej ni¿ ¿¹dano oznacza koniec strumienia.
	size_t ReadData(void *Out, size_t MaxLength);
	// Wczytuje Length bajtów pod podany wskaŸnik.
	// Out musi byæ tablic¹ zaalokowan¹ do d³ugoœci co najmniej Length bajtów.
	// Jeœli nie uda siê odczytaæ wszystkich Length bajtów, rzuca wyj¹tek.
	void MustReadData(void *Out, size_t Length);
	// Jeœli jest nastêpny znak do odczytania, pomija go i zwraca true.
	// Jeœli nie, zwraca false. Oznacza to koniec strumienia.
	bool SkipChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) return false; } m_BufBeg++; return true; }
	// Jeœli jest nastêpny znak do odczytania, pomija go.
	// Jeœli nie, rzuca wyj¹tek.
	void MustSkipChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) _ThrowBufEndError(__FILE__, __LINE__); } m_BufBeg++; }
	// Pomija co najwy¿ej MaxLength znaków.
	// Zwraca liczbê pominiêtych znaków. Mniej ni¿ ¿¹dano oznacza koniec strumienia.
	size_t Skip(size_t MaxLength);
	// Pomija Length znaków.
	// Jeœli nie uda siê pomin¹æ wszystkich Length znaków, rzuca wyj¹tek.
	void MustSkip(size_t Length);
	// Wczytuje linijkê tekstu i jeœli siê uda³o, zwraca true.
	// Jeœli nie uda³o siê wczytaæ ani jednego znaku, bo to ju¿ koniec strumienia, zwraca false.
	// Czyta znaki do napotkania koñca strumienia lub koñca wiersza, którym s¹ "\r", "\n" i "\r\n".
	// Koniec wiersza przeskakuje, ale nie do³¹cza go do Out.
	bool ReadLine(string *Out);
	// Wczytuje linijkê tekstu.
	// Jeœli nie uda³o siê wczytaæ ani jednego znaku, bo to ju¿ koniec strumienia, rzuca wyj¹tek.
	// Czyta znaki do napotkania koñca strumienia lub koñca wiersza, którym s¹ "\r", "\n" i "\r\n".
	// Koniec wiersza przeskakuje, ale nie do³¹cza go do Out.
	void MustReadLine(string *Out);
};

// Strumieñ pamiêci statycznej
// Strumieñ ma sta³¹ d³ugoœæ i nie mo¿e byæ rozszerzany - ani automatycznie, ani rêcznie.
class MemoryStream : public SeekableStream
{
private:
	char *m_Data;
	size_t m_Size;
	int m_Pos;
	bool m_InternalAlloc;

public:
	// Podaj wskaŸnik 0, aby strumieñ sam zaalkowa³, a w destruktorze zwolni³ pamiêæ.
	// Podaj wskaŸnik, jeœli ma korzystaæ z twojego obszaru pamiêci.
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

// Strumieñ pamiêci, ale dynamicznej.
// Strumieñ mo¿na rozszerzaæ i sam te¿ siê rozszerza.
class VectorStream : public SeekableStream
{
private:
	char *m_Data;
	size_t m_Size;
	size_t m_Capacity;
	int m_Pos;

	// Zmienia capacity, a tym samym przealokowuje pamiêæ.
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
	// Nie musi pozostaæ aktualny po wywo³aniach metod tej klasy!
	// (mo¿e zostaæ przealokowany)
	char *Data() { return m_Data; }
};

// Strumieñ oparty na ³añcuchu std::string
// Potrafi siê rozszerzaæ.
class StringStream : public SeekableStream
{
private:
	std::string *m_Data;
	int m_Pos;
	bool m_InternalAlloc;

public:
	// Podaj wskaŸnik 0, aby strumieñ sam zaalokowa³, a w destruktorze zwolni³ ³añcuch.
	// Podaj wskaŸnik, jeœli ma korzystaæ z twojego obszaru pamiêci.
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

// Abstrakcyjna klasa bazowa nak³adek na strumienie
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

// Nak³adka na strumieñ zliczaj¹ca iloœæ zapisanych i odczytanych bajtów
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

// Nak³adka na strumieñ ograniczaj¹ca liczbê odczytywanych i zapisywanych
// bajtów. Jesli przekroczy limit podczas zapisywania, zapisuje ile siê da i
// rzuca b³¹d zapisu. Jeœli przekroczy limit podczas odczytywania zachowuje siê
// tak, jakby strumieñ osi¹gn¹³ koniec.
class LimitOverlayStream : public OverlayStream
{
private:
	// Przechowuj¹ aktualny limit, jaki pozosta³.
	// S¹ zmniejszane przez funkcje odczytuj¹ce i zapisuj¹ce.
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

// Ten strumieñ zapisuje zapisywane dane do wielu pod³¹czonych do niego strumieni na raz.
class MultiWriterStream : public Stream
{
private:
	std::vector<Stream*> m_Streams;

public:
	MultiWriterStream() { }
	MultiWriterStream(Stream *a_Streams[], uint StreamCount);

	// Dodaje strumieñ do listy.
	void AddStream(Stream *a_Stream) { m_Streams.push_back(a_Stream); }
	// Usuwa strumieñ z listy. Jeœli takiego nie znajdzie, nic nie robi.
	void RemoveStream(Stream *a_Stream);
	// Usuwa wszystkie strumienie z listy.
	void ClearStreams() { m_Streams.clear(); }
	size_t GetStreamCount() { return m_Streams.size(); }

	// ======== Implementacja Stream ========
	virtual void Write(const void *Data, size_t Size);
	virtual void Flush();
};

// Klasa obliczaj¹ca hash 32-bitowy z kolejno podawanych bloków danych
// Algorytm: Jenkins One-at-a-time hash, http://www.burtleburtle.net/bob/hash/doobs.html
// [Strumieñ nie seekable, tylko zapis]
// Prawid³owe u¿ycie: Write, Write (lub inne funkcje zapisuj¹ce), ..., Finish
// Po wywo³aniu Finish nie mo¿na dalej zapisywaæ danych!
// Reset - rozpoczyna liczenie sumy od nowa.
class Hash_Calc : public Stream
{
private:
	uint4 m_Hash;

public:
	Hash_Calc() : m_Hash(0) { }

	// ======== Implementacja Stream ========
	virtual void Write(const void *Data, size_t Size);

	// Koñczy obliczenia i zwraca policzon¹ sumê
	uint4 Finish();
	// Rozpoczyna liczenie nowej sumy
	void Reset() { m_Hash = 0; }

	// ======== Statyczne ========
	// Po prostu oblicza sumê kontroln¹ z podanych danych
	static uint4 Calc(const void *Buf, uint4 BufLen);
	static uint4 Calc(const string &s);
};

// Klasa obliczaj¹ca sumê CRC32 z kolejno podawanych bloków danych
// [Strumieñ nie seekable, tylko zapis]
// GetResult - wyliczon¹ dotychczas sumê mo¿na otrzymywaæ w ka¿dej chwili, a potem dalej dodawaæ nowe dane.
// Reset - rozpoczyna liczenie nowej sumy kontrolnej.
class CRC32_Calc : public Stream
{
private:
	uint m_CRC;

public:
	CRC32_Calc() { m_CRC = 0xFFFFFFFF; }

	// ======== Implementacja Stream ========
	virtual void Write(const void *Data, size_t Size);

	// Zwraca policzon¹ dotychczas sumê
	uint GetResult() { return ~m_CRC; }
	// Rozpoczyna liczenie nowej sumy
	void Reset() { m_CRC = 0xFFFFFFFF; }

	// ======== Statyczne ========
	// Po prostu oblicza sumê kontroln¹ z podanych danych
	static uint Calc(const void *Data, size_t DataLength);
};

// Suma ma 128 bitów - 16 bajtów. Jest typu uint1[16].
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

// Klasa obliczaj¹ca sumê MD5 z kolejno podawanych bloków danych
// [Strumieñ nie seekable, tylko zapis]
// Prawid³owe u¿ycie: Write, Write (lub inne funkcje zapisuj¹ce), ..., Finish
// Po wywo³aniu Finish nie mo¿na dalej zapisywaæ danych!
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

	// Koñczy obliczenia i zwraca policzon¹ sumê
	void Finish(MD5_SUM *Out);
	// Rozpoczyna liczenie nowej sumy
	void Reset();

	// ======== Statyczne ========
	// Po prostu oblicza sumê kontroln¹ z podanych danych
	static void Calc(MD5_SUM *Out, const void *Buf, uint4 BufLen);
};

// Koduje lub dekoduje zapisywane/odczytywane bajty XOR podany bajt lub ci¹g bajtów
// Mapuje bezpoœrednio bajty na bajty strumienia do którego jest pod³¹czony,
// nic nie buforuje, wiêc mo¿na operowaæ te¿ na strumieniu Stream.
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

	// Resetuje liczniki tak ¿eby kodowanie i dekodowanie odbywa³o siê od pocz¹tku klucza
	void Reset() { m_EncodeKeyIndex = m_DecodeKeyIndex = 0; }

	// ======== Statyczne ========
	// Po prostu przetwarza podane dane
	static void Code(void *Out, const void *Data, size_t DataLength, const void *Key, size_t KeyLength);
	static void Code(string *Out, const string &Data, const string &Key);
};

// Koduje strumieñ binarnych danych wejœciowych na ci¹g z³o¿ony ze znaków '0' i '1' (po 8 na ka¿dy bajt).
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
	// Bufor wyjœciowy Out musi mieæ zaalokowane przynajmniej DataLength * 8 bajtów.
	static void Encode(char *Out, const void *Data, size_t DataLength);
	// £añcuch wyjœciowy Out nie musi mieæ ¿adnego konkretnego rozmiaru - zostaje wyczyszczony i wype³niony od nowa.
	static void Encode(string *Out, const void *Data, size_t DataLength);
};

// Dekoduje ci¹g znaków wejœciowych zapisanych binarnie (znakami '0' i '1') na dane binarne.
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
	// Znajduje d³ugoœæ zakodowanych danych i zwraca true.
	// Jeœli nie uda³o siê ustaliæ d³ugoœci, zwraca false.
	static bool DecodeLength(uint *OutLength, const string &s, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	static bool DecodeLength(uint *OutLength, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	// Dekoduje dane binarne. Zwraca liczbê zdekodowanych bajtów.
	// Jeœli nie uda³o siê zdekodowaæ (jakiœ b³¹d), zwraca MAXUINT4 (0xFFFFFFFF).
	// - OutData musi byæ tablic¹ zaalokowan¹ przynajmniej na rozmiar ustalony wywo³aniem DecodeLength
	//   lub jeœli nie chcesz wywo³ywaæ DecodeLength, na d³ugoœæ równ¹ d³ugoœci danych wejœciowych / 8.
	static size_t Decode(void *OutData, const string &s, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	static size_t Decode(void *OutData, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
};

// Koduje strumieñ binarnych danych wejœciowych na zapis szesnastkowy (po 2 znaki na ka¿dy bajt).
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
	// Bufor wyjœciowy Out musi mieæ zaalokowane przynajmniej DataLength * 2 bajtów.
	static void Encode(char *Out, const void *Data, size_t DataLength, bool UpperCase = true);
	// £añcuch wyjœciowy Out nie musi mieæ ¿adnego konkretnego rozmiaru - zostaje wyczyszczony i wype³niony od nowa.
	static void Encode(string *Out, const void *Data, size_t DataLength, bool UpperCase = true);
};

// Dekoduje ci¹g znaków wejœciowych zapisanych szesnastkowo na dane binarne.
// Akceptuje zarówno ma³e, jak i du¿e litery.
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
	// Znajduje d³ugoœæ zakodowanych danych i zwraca true.
	// Jeœli nie uda³o siê ustaliæ d³ugoœci, zwraca false.
	static bool DecodeLength(uint *OutLength, const string &s, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	static bool DecodeLength(uint *OutLength, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	// Dekoduje dane binarne. Zwraca liczbê zdekodowanych bajtów.
	// Jeœli nie uda³o siê zdekodowaæ (jakiœ b³¹d), zwraca MAXUINT4 (0xFFFFFFFF).
	// - OutData musi byæ tablic¹ zaalokowan¹ przynajmniej na rozmiar ustalony wywo³aniem DecodeLength
	//   lub jeœli nie chcesz wywo³ywaæ DecodeLength, na d³ugoœæ równ¹ d³ugoœci danych wejœciowych / 2.
	static size_t Decode(void *OutData, const string &s, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	static size_t Decode(void *OutData, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
};

// Koduje strumieñ binarnych danych wejœciowych na ci¹g znaków base64.
/*
- Zgodny z RFC 4648 "The Base16, Base32, and Base64 Data Encodings"
- Prawid³owe dane wyjœciowe powstaj¹ dopiero po zakoñczeniu zapisywania, które nastêpuje w destruktorze.
  Jeœli chcesz zakoñczyæ wczeœniej, wywo³aj Finish, ale potem nie mo¿na ju¿ nic dopisywaæ.
*/
class Base64Encoder : public OverlayStream
{
private:
	CharWriter m_CharWriter;
	bool m_Finished; // Czy dane zakoñczone
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
	// Zwraca liczbê znaków zapisanych do Out.
	// Bufor wyjœciowy Out musi mieæ zaalokowane przynajmniej tyle bajtów ile wyliczy³o EncodeLength.
	static size_t Encode(char *Out, const void *Data, size_t DataLength);
	// £añcuch wyjœciowy Out nie musi mieæ ¿adnego konkretnego rozmiaru - zostaje wyczyszczony i wype³niony od nowa.
	static size_t Encode(string *Out, const void *Data, size_t DataLength);
};

// Dekoduje ci¹g znaków wejœciowych zapisanych w standardzie base64 na dane binarne.
/*
- Po napotkaniu koñcówki zawieraj¹cej znaki '=' funkcje nie parsuj¹ ju¿ dalej,
  ale to tylko optymalizacja i nie nale¿y na tym polegaæ.
- Sposób dzia³ania strumienia - na raz wczytywane s¹ po 4 znaki i zamieniane na
  maksymalnie 3 bajty.
*/
class Base64Decoder : public OverlayStream
{
private:
	CharReader m_CharReader;
	DECODE_TOLERANCE m_Tolerance;
	// Bajty s¹ w kolejnoœci: [2][1][0]. Jeœli jest mniej, to [1][0][-] albo [0][-][-].
	uint1 m_Buf[3];
	// Liczba zdekodowanych bajtów w buforze
	uint m_BufLength;
	// True, jeœli sparsowano koñcówkê z '='
	bool m_Finished;

	// Czyta 4 znaki ze strumienia pod³¹czonego (0 albo 4, inaczej b³¹d) i wype³nia m_Buf maksymalnie 3 zdekodowanymi bajtami.
	// Ustawia m_BufLength i jeœli trzeba, to m_Finished.
	bool ReadNextBuf();

	bool GetNextByte(uint1 *OutByte) { if (m_BufLength == 0) { if (!ReadNextBuf()) return false; } *OutByte = m_Buf[--m_BufLength]; return true; }

public:
	Base64Decoder(Stream *a_Stream, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);

	DECODE_TOLERANCE GetTolerance() { return m_Tolerance; }

	// ======== Implementacja Stream ========
	virtual size_t Read(void *Out, size_t Size);
	virtual bool End();

	// ======== Statyczne ========
	// Znajduje d³ugoœæ zakodowanych danych i zwraca true.
	// Jeœli nie uda³o siê ustaliæ d³ugoœci, zwraca false.
	static bool DecodeLength(uint *OutLength, const string &s, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	static bool DecodeLength(uint *OutLength, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	// Dekoduje dane binarne. Zwraca liczbê zdekodowanych bajtów.
	// Jeœli nie uda³o siê zdekodowaæ (jakiœ b³¹d), zwraca MAXUINT4 (0xFFFFFFFF).
	// OutData musi byæ tablic¹ zaalokowan¹ przynajmniej na rozmiar ustalony wywo³aniem DecodeLength.
	static size_t Decode(void *OutData, const string &s, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
	static size_t Decode(void *OutData, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance = DECODE_TOLERANCE_NONE);
};

/*
Bufor ko³owy. Dzia³a szybko (na tyle na ile szybko mo¿e dzia³aæ strumieñ, ze
swoimi metodami wirtualnymi itd.), ale ma z góry ograniczon¹ pojemnoœæ. W
przypadku przepe³nienia przy zapisie rzuca wyj¹tek.
*/
class RingBuffer : public Stream
{
public:
	RingBuffer(uint Capacity);
	virtual ~RingBuffer();

	// Zwraca liczbê bajtów w buforze
	uint GetSize() { return m_Size; }
	// Zwraca pojemnoœæ bufora
	uint GetCapacity() { return m_Capacity; }
	// Zwraca true, jeœli bufor jest pusty/pe³ny
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


// Kopiuje dane z jednego strumienia do drugiego - co najwy¿ej MaxLength bajtów.
// Zwraca liczbê przekopiowanych danych.
inline size_t Copy(Stream *Dst, Stream *Src, size_t MaxLength) { return Dst->CopyFrom(Src, MaxLength); }
// Kopiuje dane z jednego strumienia do drugiego - dok³adnie Length bajtów.
// Jeœli siê nie uda przekopiowaæ tylu bajtów, rzuca b³¹d.
inline void MustCopy(Stream *Dst, Stream *Src, size_t Length) { return Dst->MustCopyFrom(Src, Length); }
// Kopiuje dane z jednego strumienia do drugiego a¿ do koñca tego Ÿród³owego
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
