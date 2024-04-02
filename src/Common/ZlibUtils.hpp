/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * ZlibUtils - Modu³ wspmagaj¹cy biblioteki zlib [wymaga biblioteki zlib]
 * Dokumentacja: Patrz plik doc/ZlibUtils.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef COMMON_ZLIB_UTILS_H_
#define COMMON_ZLIB_UTILS_H_

#include "Stream.hpp"
#include "Error.hpp"

namespace common
{

class ZlibError : public Error
{
public:
	ZlibError(int Code, const string &Msg = "", const string &File = "", int Line = 0);
};


// Brak kompresji (co nie znaczy ¿e dane zostan¹ dos³ownie przepisane)
const int ZLIB_STORE_LEVEL   = 0;
// Najs³absza kompresja, ale najszybsza
const int ZLIB_FASTEST_LEVEL = 1;
// Najlepsza jakoœæ kompresji
const int ZLIB_BEST_LEVEL    = 9;
// Domyœlny poziom kompresji
extern const int ZLIB_DEFAULT_LEVEL;

class ZlibCompressionStream_pimpl;
class ZlibDecompressionStream_pimpl;

// Kompresuje strumieñ danych w formacie zlib.
class ZlibCompressionStream : public OverlayStream
{
private:
	scoped_ptr<ZlibCompressionStream_pimpl> pimpl;

public:
	ZlibCompressionStream(Stream *a_Stream, int Level = ZLIB_DEFAULT_LEVEL);
	virtual ~ZlibCompressionStream();

	// ======== Implementacja Stream ========
	
	virtual void Write(const void *Data, size_t Size);
	// Mo¿e spowodowaæ, ¿e skompresowane dane bêd¹ inaczej wyg³ada³y i obni¿yæ jakoœæ kompresji.
	virtual void Flush();
	
	// ======== Statyczne ========
	// Oblicza maksymalny rozmiar bufora na skompresowane dane
	static size_t CompressLength(size_t DataLength);
	// Po prostu kompresuje podane dane
	// Zwraca d³ugoœæ skompresowanych danych.
	// OutData musi byæ buforem o d³ugoœci conajmniej takiej, jak obliczona przez CompressLength.
	// BufLength to d³ugoœæ zaalokowanej pamiêci na OutData, podawana i sprawdzana na wszelki wypadek.
	static size_t Compress(void *OutData, size_t BufLength, const void *Data, size_t DataLength, int Level = ZLIB_DEFAULT_LEVEL);
};

// Wszystko jak wy¿ej, ale kompresuje w formacie gzip do³¹czaj¹c nag³ówek gzip z podanymi parametrami.
class GzipCompressionStream : public OverlayStream
{
private:
	scoped_ptr<ZlibCompressionStream_pimpl> pimpl;

public:
	// FileName, Comment - jeœli ma nie byæ, mo¿na podaæ NULL.
	GzipCompressionStream(Stream *a_Stream, const string *FileName, const string *Comment, int Level = ZLIB_DEFAULT_LEVEL);
	virtual ~GzipCompressionStream();

	// ======== Implementacja Stream ========
	
	virtual void Write(const void *Data, size_t Size);
	virtual void Flush();
};

class ZlibDecompressionStream : public OverlayStream
{
private:
	scoped_ptr<ZlibDecompressionStream_pimpl> pimpl;

public:
	ZlibDecompressionStream(Stream *a_Stream);
	virtual ~ZlibDecompressionStream();

	// ======== Implementacja Stream ========
	
	virtual size_t Read(void *Out, size_t MaxLength);
	virtual bool End();

	// ======== Statyczne ========
	// Po prostu dekompresuje podane dane
	// Zwraca d³ugoœæ rozkompresowanych danych.
	// Rozmiar danych po dekompresji musi byæ gdzieœ zapamiêtany i odpowiednio du¿y bufor musi byæ ju¿ zaalkowany.
	// BufLength to d³ugoœæ zaalokowanej pamiêci na OutData, podawana i sprawdzana na wszelki wypadek.
	static size_t Decompress(void *OutData, size_t BufLength, const void *Data, size_t DataLength);
};

// Wszystko jak wy¿ej, ale dekompresuje w formacie gzip odczytuj¹c te¿ nag³ówek.
class GzipDecompressionStream : public OverlayStream
{
private:
	scoped_ptr<ZlibDecompressionStream_pimpl> pimpl;

public:
	GzipDecompressionStream(Stream *a_Stream);
	virtual ~GzipDecompressionStream();

	// ======== Implementacja Stream ========
	
	virtual size_t Read(void *Out, size_t MaxLength);
	virtual bool End();

	// Jeœli plik posiada nag³ówek, ten nag³ówek przechowuje dan¹ informacjê i
	// zosta³ ju¿ w toku odczytywania odczytany, zwraca true i zwraca przez parametr
	// z t¹ informacjê.
	bool GetHeaderFileName(string *OutFileName);
	bool GetHeaderComment(string *OutComment);
};

enum GZIP_FILE_MODE
{
	GZFM_WRITE,
	GZFM_READ,
};

class GzipFileStream_pimpl;

// Obs³uga pliku w formacie gzip (zalecane rozszerzenie ".gz")
class GzipFileStream : public Stream
{
private:
	scoped_ptr<GzipFileStream_pimpl> pimpl;

public:
	// Level ma znaczenie tylko przy zapisie.
	GzipFileStream(const string &FileName, GZIP_FILE_MODE Mode, int Level = ZLIB_DEFAULT_LEVEL);
	virtual ~GzipFileStream();

	// ======== Implementacja Stream ========
	
	virtual void Write(const void *Data, size_t Size);
	virtual void Flush();
	
	virtual size_t Read(void *Out, size_t MaxLength);
	virtual bool End();
};

} // namespace common

#endif
